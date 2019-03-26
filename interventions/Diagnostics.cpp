/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "Diagnostics.h"

#include "InterventionEnums.h"
#include "InterventionFactory.h"
#include "NodeEventContext.h"  // for INodeEventContext (ICampaignCostObserver)
#include "IIndividualHumanContext.h"
#include "ISimulationContext.h"
#include "RANDOM.h"

SETUP_LOGGING( "SimpleDiagnostic" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(SimpleDiagnostic)
        HANDLE_INTERFACE(IConfigurable)
        HANDLE_INTERFACE(IDistributableIntervention)
        HANDLE_INTERFACE(IBaseIntervention)
        HANDLE_ISUPPORTS_VIA(IDistributableIntervention)
    END_QUERY_INTERFACE_BODY(SimpleDiagnostic)

    IMPLEMENT_FACTORY_REGISTERED(SimpleDiagnostic)

    EventOrConfig::Enum
    SimpleDiagnostic::getEventOrConfig(
        const Configuration * inputJson
    )
    {
        EventOrConfig::Enum event_or_config;
        initConfig( "Event_Or_Config", event_or_config, inputJson, MetadataDescriptor::Enum("EventOrConfig", Event_Or_Config_DESC_TEXT, MDD_ENUM_ARGS( EventOrConfig ) ) );
        return event_or_config;
    }

    // This is deliberately broken out into separate function so that derived classes can invoke
    // without calling SD::Configure -- if they want Positive_Dx_Config/Event but not Treatment_Fraction.
    void SimpleDiagnostic::ConfigurePositiveEventOrConfig(
        const Configuration * inputJson
    )
    {
        use_event_or_config = getEventOrConfig( inputJson );

        if( use_event_or_config == EventOrConfig::Event || JsonConfigurable::_dryrun )
        {
            initConfigTypeMap("Positive_Diagnosis_Event", &positive_diagnosis_event, SD_Positive_Diagnosis_Config_Event_DESC_TEXT );
        }

        if( use_event_or_config == EventOrConfig::Config || JsonConfigurable::_dryrun )
        {
            initConfigComplexType("Positive_Diagnosis_Config", &positive_diagnosis_config, SD_Positive_Diagnosis_Config_DESC_TEXT, "Event_Or_Config", "Config" );
        }

    }
    
    bool SimpleDiagnostic::Configure(
        const Configuration * inputJson
    )
    {
        ConfigurePositiveEventOrConfig( inputJson );
       
        initConfigTypeMap("Treatment_Fraction", &treatment_fraction, SD_Treatment_Fraction_DESC_TEXT, 0, 1);

        bool ret = BaseIntervention::Configure( inputJson );
        LOG_DEBUG_F( "Base_Sensitivity = %f, Base_Specificity = %f\n", (float) base_sensitivity, (float) base_specificity );
        LOG_DEBUG_F( "Days_To_Diagnosis = %f\n", (float) days_to_diagnosis );
        use_event_or_config = getEventOrConfig( inputJson );
        
        if( ret && !JsonConfigurable::_dryrun )
        {
            if( use_event_or_config == EventOrConfig::Config )
            {
                InterventionValidator::ValidateIntervention( GetTypeName(),
                                                             InterventionTypeValidation::INDIVIDUAL,
                                                             positive_diagnosis_config._json,
                                                             inputJson->GetDataLocation() );
            }
            else if( use_event_or_config == EventOrConfig::Event )
            {
                CheckConfigTriggers( inputJson );
            }
            else
            {
                CheckPostiveEventConfig();
            }
        }
        return ret;
    }

    void SimpleDiagnostic::CheckConfigTriggers( const Configuration * inputJson )
    {
        if( positive_diagnosis_event.IsUninitialized() )
        {
            std::stringstream ss;
            ss << "Positive_Diagnosis_Event is not defined." << std::endl;
            throw InitializationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
    }

    void SimpleDiagnostic::CheckPostiveEventConfig()
    {
        if( !JsonConfigurable::_dryrun && 
            positive_diagnosis_event.IsUninitialized() &&
            (positive_diagnosis_config._json.Type() == ElementType::NULL_ELEMENT) )
        {
            const char* msg = "You must define either Positive_Diagnosis_Event or Positive_Diagnosis_Config";
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg );
        }
    }


    SimpleDiagnostic::SimpleDiagnostic()
    : BaseIntervention()
    , diagnostic_type(0)
    , base_specificity(0)
    , base_sensitivity(0)
    , treatment_fraction(1.0f)
    , days_to_diagnosis(0)
    , enable_isSymptomatic(false)
    , use_event_or_config( EventOrConfig::Event )
    , positive_diagnosis_config()
    , positive_diagnosis_event()
    {
        initConfigTypeMap("Base_Specificity",   &base_specificity, SD_Base_Specificity_DESC_TEXT,     1.0f );
        initConfigTypeMap("Base_Sensitivity",   &base_sensitivity, SD_Base_Sensitivity_DESC_TEXT,     1.0f );
        initConfigTypeMap("Treatment_Fraction", &treatment_fraction, SD_Treatment_Fraction_DESC_TEXT, 1.0f );
        initConfigTypeMap("Days_To_Diagnosis",  &days_to_diagnosis, SD_Days_To_Diagnosis_DESC_TEXT,   FLT_MAX, 0  );
        initConfigTypeMap("Cost_To_Consumer",   &cost_per_unit, SD_Cost_To_Consumer_DESC_TEXT,        0);
        initConfigTypeMap("Enable_IsSymptomatic", &enable_isSymptomatic, SD_Enable_IsSymptomatic_DESC_TEXT, 0 );

        days_to_diagnosis.handle = std::bind( &SimpleDiagnostic::Callback, this, std::placeholders::_1 );
    }

    SimpleDiagnostic::SimpleDiagnostic( const SimpleDiagnostic& master )
        : BaseIntervention( master )
        , diagnostic_type(master.diagnostic_type)
        , base_specificity(master.base_specificity)
        , base_sensitivity(master.base_sensitivity)
        , treatment_fraction(master.treatment_fraction)
        , days_to_diagnosis(master.days_to_diagnosis)
        , use_event_or_config(master.use_event_or_config)
        , positive_diagnosis_config(master.positive_diagnosis_config)
        , enable_isSymptomatic(master.enable_isSymptomatic )
        , positive_diagnosis_event(master.positive_diagnosis_event)
    {
        days_to_diagnosis.handle = std::bind( &SimpleDiagnostic::Callback, this, std::placeholders::_1 );
    }

    bool SimpleDiagnostic::Distribute(
        IIndividualHumanInterventionsContext *context,
        ICampaignCostObserver * const pICCO
    )
    {
        // ----------------------------------------------------------------------------------
        // --- Putting this here because we don't want anything to happen if we are aborting
        // ----------------------------------------------------------------------------------
        if( AbortDueToDisqualifyingInterventionStatus( context->GetParent() ) )
        {
            return false;
        }

        parent = context->GetParent();
        LOG_DEBUG_F( "Individual %d is getting tested and positive_diagnosis_event = %s.\n", parent->GetSuid().data, positive_diagnosis_event.c_str() );

        // Positive test result and distribute immediately if days_to_diagnosis <=0
        if ( positiveTestResult() )
        {
            LOG_DEBUG_F( "Individual %d tested positive: treatment fraction = %f.\n", parent->GetSuid().data, (float) treatment_fraction );

            if( parent->GetRng()->SmartDraw(treatment_fraction) )
            {
                // Don't act at distribute; wait until update so everything is consistent
                if ( days_to_diagnosis == 0 ) // use exactly 0. If configured to negative value, assume that's an ignore
                {
                    LOG_DEBUG_F( "Individual %d getting a diagnostic right now during Distribute, instead of waiting till Update.\n", parent->GetSuid().data );
                    //positiveTestDistribute(); // since there is no waiting time, distribute intervention right now
                    Callback( 0 ); // since there is no waiting time, distribute intervention right now
                }
            }
            else
            {
                onPatientDefault();
                expired = true;         // this person doesn't get the intervention despite the positive test
            }
            //else (regular case) we have to wait for days_to_diagnosis to count down, person does get drugs
        }
        // Negative test result
        else
        {
            LOG_DEBUG_F( "Individual %d tested negative.\n", parent->GetSuid().data );
            onNegativeTestResult();
        }

        return BaseIntervention::Distribute( context, pICCO );
    }

    void SimpleDiagnostic::Update( float dt )
    {
        //bool wasDistributed = false;
        if ( expired )
        {
            return; // don't give expired intervention.  should be cleaned up elsewhere anyways, though.
        }

        if( !BaseIntervention::UpdateIndividualsInterventionStatus() ) return;

        // Count down the time until a positive test result comes back
        days_to_diagnosis.Decrement( dt );
    }

    bool SimpleDiagnostic::applySensitivityAndSpecificity(bool infected) const
    {
        bool positiveTestReported = ( ( infected  && parent->GetRng()->SmartDraw( base_sensitivity   ) ) ||
                                      ( !infected && parent->GetRng()->SmartDraw( 1-base_specificity ) )
                                    ) ;
        LOG_DEBUG_F( "%s is returning %d\n", __FUNCTION__, positiveTestReported );
        return positiveTestReported;
    }

    bool SimpleDiagnostic::positiveTestResult()
    {
        // Apply diagnostic test with given specificity/sensitivity
        bool test_result;
        if( enable_isSymptomatic )
        {
            test_result = parent->GetEventContext()->IsInfected() && parent->GetEventContext()->IsSymptomatic();
        }
        else
        {
            test_result = parent->GetEventContext()->IsInfected();
        }

        // True positive (sensitivity), or False positive (1-specificity)
        return applySensitivityAndSpecificity( test_result );
    }

    void
    SimpleDiagnostic::onPatientDefault()
    {
    }


    void
    SimpleDiagnostic::onNegativeTestResult()
    {
        expired = true;
    }

    void
    SimpleDiagnostic::broadcastEvent( const EventTrigger& event )
    {
        if( !event.IsUninitialized() )
        {
            IIndividualEventBroadcaster* broadcaster = parent->GetEventContext()->GetNodeEventContext()->GetIndividualEventBroadcaster();
            LOG_DEBUG_F( "SimpleDiagnostic broadcasting event = %s.\n", event.c_str() );
            broadcaster->TriggerObservers( parent->GetEventContext(), event );
        }
    }

    void SimpleDiagnostic::Callback( float dt )
    {
        positiveTestDistribute();
    }

    void SimpleDiagnostic::positiveTestDistribute()
    {
        release_assert( parent );
        LOG_DEBUG_F( "Individual %d tested 'positive' in SimpleDiagnostic, receiving actual intervention: event = %s.\n", parent->GetSuid().data, positive_diagnosis_event.c_str() );

        // Next alternative is that we were configured to broadcast a raw event string. In which case the value will not
        // the "uninitialized" value.
        if( use_event_or_config == EventOrConfig::Event )
        {
            broadcastEvent( positive_diagnosis_event );
        }
        // third alternative is that we were configured to use an actual config, not broadcast an event.
        else if( positive_diagnosis_config._json.Type() != ElementType::NULL_ELEMENT )
        {
            // Important: Use the instance method to obtain the intervention factory obj instead of static method to cross the DLL boundary
            IGlobalContext *pGC = nullptr;
            const IInterventionFactory* ifobj = nullptr;
            if (s_OK == parent->QueryInterface(GET_IID(IGlobalContext), (void**)&pGC))
            {
                ifobj = pGC->GetInterventionFactory();
            }
            if (!ifobj)
            {
                throw NullPointerException( __FILE__, __LINE__, __FUNCTION__, "parent->GetInterventionFactoryObj()", "IInterventionFactory" );
            }

            // Distribute the test-positive intervention
            auto config = Configuration::CopyFromElement( positive_diagnosis_config._json, "campaign" );
            IDistributableIntervention *di = const_cast<IInterventionFactory*>(ifobj)->CreateIntervention( config );

            ICampaignCostObserver* pICCO;
            // Now make sure cost of the test-positive intervention is reported back to node
            if (s_OK == parent->GetEventContext()->GetNodeEventContext()->QueryInterface(GET_IID(ICampaignCostObserver), (void**)&pICCO) )
            {
                di->Distribute( parent->GetInterventionsContext(), pICCO );
                pICCO->notifyCampaignEventOccurred( (IBaseIntervention*)di, (IBaseIntervention*)this, parent );
            }
            else
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent->GetEventContext()->GetNodeEventContext()", "ICampaignCostObserver", "INodeEventContext" );
            }
            delete config;
            config = nullptr;
        }
        // this is the right thing to do but we need to deal with HIVRandomChoice
        //else
        //{
        //    throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, "neither event or config defined" );
        //}
        expired = true;
    }

    REGISTER_SERIALIZABLE(SimpleDiagnostic);

    void SimpleDiagnostic::serialize(IArchive& ar, SimpleDiagnostic* obj)
    {
        BaseIntervention::serialize( ar, obj );
        SimpleDiagnostic& diagnostic = *obj;
        ar.labelElement("diagnostic_type") & diagnostic.diagnostic_type;
        ar.labelElement("base_specificity"); diagnostic.base_specificity.serialize(ar);
        ar.labelElement("base_sensitivity"); diagnostic.base_sensitivity.serialize(ar);
        ar.labelElement("treatment_fraction"); diagnostic.treatment_fraction.serialize(ar);
        ar.labelElement("days_to_diagnosis") & diagnostic.days_to_diagnosis;
        ar.labelElement("use_event_or_config") & (uint32_t&)diagnostic.use_event_or_config;
        ar.labelElement("positive_diagnosis_config") & diagnostic.positive_diagnosis_config;
        ar.labelElement("positive_diagnosis_event") & diagnostic.positive_diagnosis_event;
        ar.labelElement("enable_isSymptomatic") & diagnostic.enable_isSymptomatic;
    }
}
