/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"
#include "Diagnostics.h"

#include "InterventionEnums.h"
#include "InterventionFactory.h"
#include "NodeEventContext.h"  // for INodeEventContext (ICampaignCostObserver)
#include "SimulationConfig.h"  // for event strings 

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
            positive_diagnosis_event.constraints = "<configuration>:Listed_Events.*";
            positive_diagnosis_event.constraint_param = &GET_CONFIGURABLE(SimulationConfig)->listed_events;
            initConfigTypeMap("Positive_Diagnosis_Event", &positive_diagnosis_event, SD_Positive_Diagnosis_Config_Event_DESC_TEXT, NO_TRIGGER_STR );
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
        EventOrConfig::Enum use_event_or_config = getEventOrConfig( inputJson );
        if( ret && (use_event_or_config == EventOrConfig::Config || JsonConfigurable::_dryrun) )
        {
            InterventionValidator::ValidateIntervention( positive_diagnosis_config._json );
        }
		return ret;
    }

    SimpleDiagnostic::SimpleDiagnostic()
    : parent(NULL)
    , diagnostic_type(0)
    , base_specificity(0)
    , base_sensitivity(0)
    , treatment_fraction(1.0f)
    , days_to_diagnosis(0)
    , positive_diagnosis_event("UNINITIALIZED")
    {
        initConfigTypeMap("Base_Specificity",  &base_specificity,  SD_Base_Specificity_DESC_TEXT,  0, 1);
        initConfigTypeMap("Base_Sensitivity",  &base_sensitivity,  SD_Base_Sensitivity_DESC_TEXT,  0, 1);
        initConfigTypeMap("Days_To_Diagnosis", &days_to_diagnosis, SD_Days_To_Diagnosis_DESC_TEXT, 0 ); 
        initConfigTypeMap("Cost_To_Consumer",  &cost_per_unit,     SD_Cost_To_Consumer_DESC_TEXT,  0 );
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
            auto draw = parent->GetRng()->e();
            LOG_DEBUG_F( "Individual %d tested positive: treatment fraction = %f, random draw = %f.\n", parent->GetSuid().data, treatment_fraction, draw );

            if ( draw > treatment_fraction ) 
            { 
                onPatientDefault();
                expired = true;         // this person doesn't get the intervention despite the positive test
            }
            else if ( days_to_diagnosis <= 0 ) 
            { 
                positiveTestDistribute(); // since there is no waiting time, distribute intervention right now
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

    bool SimpleDiagnostic::positiveTestResult()
    {
        // Apply diagnostic test with given specificity/sensitivity
        float rand     = parent->GetRng()->e();
        bool  infected = parent->GetEventContext()->IsInfected();
        
        // True positive (sensitivity), or False positive (1-specificity)
        bool positiveTest = ( infected && (rand < base_sensitivity) ) || ( !infected && (rand > base_specificity) );
        
        return positiveTest;
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
    SimpleDiagnostic::broadcastEvent( const std::string& event )
    {
        if( event != NO_TRIGGER_STR )
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
        if( positive_diagnosis_event != "UNINITIALIZED" )
        {
            broadcastEvent(positive_diagnosis_event);
        }
        // third alternative is that we were configured to use an actual config, not broadcast an event.
        else
        {
            // Important: Use the instance method to obtain the intervention factory obj instead of static method to cross the DLL boundary
            IGlobalContext *pGC = NULL;
            const IInterventionFactory* ifobj = NULL;
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
        }
        expired = true;
    }
}

#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
BOOST_CLASS_EXPORT(Kernel::SimpleDiagnostic)

namespace Kernel {
    template<class Archive>
    void serialize(Archive &ar, SimpleDiagnostic& obj, const unsigned int v)
    {
        static const char * _module = "SimpleDiagnostic";
        LOG_DEBUG("(De)serializing SimpleDiagnostic\n");

        boost::serialization::void_cast_register<SimpleDiagnostic, IDistributableIntervention>();
        ar & obj.positive_diagnosis_config;
        ar & (std::string) obj.positive_diagnosis_event;
        ar & obj.diagnostic_type;
        ar & obj.base_specificity;
        ar & obj.base_sensitivity;
        ar & obj.treatment_fraction;
        ar & obj.days_to_diagnosis;
        ar & boost::serialization::base_object<Kernel::BaseIntervention>(obj);
        //ar & boost::serialization::base_object<Kernel::SimpleHealthSeekingBehavior>(obj);
    }
    template void serialize( boost::mpi::packed_skeleton_iarchive&, Kernel::SimpleDiagnostic&, unsigned int);
}
#endif
