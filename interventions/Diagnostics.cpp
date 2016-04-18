/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "Diagnostics.h"

#include "InterventionEnums.h"
#include "InterventionFactory.h"
#include "NodeEventContext.h"  // for INodeEventContext (ICampaignCostObserver)

static const char * _module = "SimpleDiagnostic";

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
        EventOrConfig::Enum use_event_or_config;
        initConfig( "Event_Or_Config", use_event_or_config, inputJson, MetadataDescriptor::Enum("EventOrConfig", Event_Or_Config_DESC_TEXT, MDD_ENUM_ARGS( EventOrConfig ) ) );
        return use_event_or_config;
    }

    // This is deliberately broken out into separate function so that derived classes can invoke
    // without calling SD::Configure -- if they want Positive_Dx_Config/Event but not Treatment_Fraction.
    void SimpleDiagnostic::ConfigurePositiveEventOrConfig(
        const Configuration * inputJson
    )
    {
        EventOrConfig::Enum use_event_or_config = getEventOrConfig( inputJson );

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

        bool ret = JsonConfigurable::Configure( inputJson );
        LOG_DEBUG_F( "Base_Sensitivity = %f, Base_Specificity = %f\n", (float) base_sensitivity, (float) base_specificity );
        EventOrConfig::Enum use_event_or_config = getEventOrConfig( inputJson );
        if( ret )
        {
            if( use_event_or_config == EventOrConfig::Config || JsonConfigurable::_dryrun )
            {
                InterventionValidator::ValidateIntervention( positive_diagnosis_config._json );
            }

            CheckPostiveEventConfig();
        }
        return ret;
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
    : parent(nullptr)
    , diagnostic_type(0)
    , base_specificity(0)
    , base_sensitivity(0)
    , treatment_fraction(1.0f)
    , days_to_diagnosis(0)
    , positive_diagnosis_config()
    , positive_diagnosis_event()
    {
        initConfigTypeMap("Base_Specificity",   &base_specificity, SD_Base_Specificity_DESC_TEXT,     1.0f );
        initConfigTypeMap("Base_Sensitivity",   &base_sensitivity, SD_Base_Sensitivity_DESC_TEXT,     1.0f );
        initConfigTypeMap("Treatment_Fraction", &treatment_fraction, SD_Treatment_Fraction_DESC_TEXT, 1.0f );
        initConfigTypeMap("Days_To_Diagnosis",  &days_to_diagnosis, SD_Days_To_Diagnosis_DESC_TEXT,   0 );
        initConfigTypeMap("Cost_To_Consumer",   &cost_per_unit, SD_Cost_To_Consumer_DESC_TEXT,        0);
    }

    SimpleDiagnostic::SimpleDiagnostic( const SimpleDiagnostic& master )
        :BaseIntervention( master )
    {
        diagnostic_type = master.diagnostic_type;
        base_specificity = master.base_specificity;
        base_sensitivity = master.base_sensitivity;
        treatment_fraction = master.treatment_fraction;
        days_to_diagnosis = master.days_to_diagnosis;
        positive_diagnosis_event = master.positive_diagnosis_event;
        positive_diagnosis_config = master.positive_diagnosis_config;
    }

    bool SimpleDiagnostic::Distribute(
        IIndividualHumanInterventionsContext *context,
        ICampaignCostObserver * const pICCO
    )
    {
        parent = context->GetParent();
        LOG_DEBUG_F( "Individual %d is getting tested and positive_diagnosis_event = %s.\n", parent->GetSuid().data, positive_diagnosis_event.c_str() );

        // Positive test result and distribute immediately if days_to_diagnosis <=0
        if ( positiveTestResult() )
        {
            LOG_DEBUG_F( "Individual %d tested positive: treatment fraction = %f.\n", parent->GetSuid().data, (float) treatment_fraction );

            if( SMART_DRAW(treatment_fraction) )
            {
                if ( days_to_diagnosis <= 0 )
                {
                    positiveTestDistribute(); // since there is no waiting time, distribute intervention right now
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

        // Count down the time until a positive test result comes back
        days_to_diagnosis -= dt;

        // Give the intervention if the test has come back
        if( days_to_diagnosis <= 0 )
        {
            positiveTestDistribute();
        }
    }

    bool
    SimpleDiagnostic::applySensitivityAndSpecificity(
        bool infected
    )
    const
    {
        bool positiveTestReported = ( ( infected  && ( SMART_DRAW( base_sensitivity ) ) ) ||
                                      ( !infected && ( SMART_DRAW( 1-base_specificity ) ) )
                                    ) ;
        LOG_DEBUG_F( "%s is returning %d\n", __FUNCTION__, positiveTestReported );
        return positiveTestReported;
    }

    bool SimpleDiagnostic::positiveTestResult()
    {
        // Apply diagnostic test with given specificity/sensitivity
        bool  infected = parent->GetEventContext()->IsInfected();

        // True positive (sensitivity), or False positive (1-specificity)

        return applySensitivityAndSpecificity( infected );
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
        if( (event != NO_TRIGGER_STR) && !event.IsUninitialized() )
        {
            INodeTriggeredInterventionConsumer* broadcaster = nullptr;
            if (s_OK != parent->GetEventContext()->GetNodeEventContext()->QueryInterface(GET_IID(INodeTriggeredInterventionConsumer), (void**)&broadcaster))
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent->GetEventContext()->GetNodeEventContext()", "INodeTriggeredInterventionConsumer", "INodeEventContext" );
            }
            LOG_DEBUG_F( "SimpleDiagnostic broadcasting event = %s.\n", event.c_str() );
            broadcaster->TriggerNodeEventObserversByString( parent->GetEventContext(), event );
        }
    }

    void SimpleDiagnostic::positiveTestDistribute()
    {
        LOG_DEBUG_F( "Individual %d tested 'positive' in SimpleDiagnostic, receiving actual intervention: event = %s.\n", parent->GetSuid().data, positive_diagnosis_event.c_str() );

        // Next alternative is that we were configured to broadcast a raw event string. In which case the value will not
        // the "uninitialized" value.
        if( !positive_diagnosis_event.IsUninitialized() )
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
                throw NullPointerException( __FILE__, __LINE__, __FUNCTION__, "parent->GetInterventionFactoryObj()" );
            }

            // Distribute the test-positive intervention
            auto config = Configuration::CopyFromElement( positive_diagnosis_config._json );
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
        // this is the right thing to do but we need to deal with HIVRandomChoice and HIVSetCascadeState
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
        ar.labelElement("positive_diagnosis_config") & diagnostic.positive_diagnosis_config;
// Remove after testing (implemented above)
// clorton        if ( ar.IsWriter() )
// clorton        {
// clorton            std::ostringstream string_stream;
// clorton            json::Writer::Write( diagnostic.positive_diagnosis_config._json, string_stream );
// clorton            ar & string_stream.str();
// clorton        }
// clorton        else
// clorton        {
// clorton            std::string json;
// clorton            ar & json;
// clorton            std::istringstream string_stream( json );
// clorton            json::Reader::Read( diagnostic.positive_diagnosis_config._json, string_stream );
// clorton        }
        ar.labelElement("positive_diagnosis_event") & diagnostic.positive_diagnosis_event;
    }
}
