/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "DiagnosticsTreatNeg.h"
#include "InterventionEnums.h"
#include "InterventionFactory.h"
#include "NodeEventContext.h"  // for INodeEventContext (ICampaignCostObserver)
#include "TBContexts.h"

SETUP_LOGGING( "DiagnosticTreatNeg" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED(DiagnosticTreatNeg, SimpleDiagnostic)
    END_QUERY_INTERFACE_DERIVED(DiagnosticTreatNeg, SimpleDiagnostic)

    IMPLEMENT_FACTORY_REGISTERED(DiagnosticTreatNeg)

    bool DiagnosticTreatNeg::Configure(
        const Configuration * inputJson
    )
    {
        EventOrConfig::Enum use_event_or_config;
        initConfig( "Event_Or_Config", use_event_or_config, inputJson, MetadataDescriptor::Enum("EventOrConfig", Event_Or_Config_DESC_TEXT, MDD_ENUM_ARGS( EventOrConfig ) ) );
        if( use_event_or_config == EventOrConfig::Event || JsonConfigurable::_dryrun )
        {
            initConfigTypeMap( "Negative_Diagnosis_Event", &negative_diagnosis_event, DTN_Negative_Diagnosis_Config_Event_DESC_TEXT );
            initConfigTypeMap( "Defaulters_Event", &defaulters_event, DTN_Defaulters_Diagnosis_Config_Event_DESC_TEXT );
        }

        if( use_event_or_config == EventOrConfig::Config || JsonConfigurable::_dryrun )
        {
            initConfigComplexType("Negative_Diagnosis_Config", &negative_diagnosis_config, DTN_Negative_Diagnosis_Config_DESC_TEXT, "Event_Or_Config", "Config" );
            initConfigComplexType("Defaulters_Config", &defaulters_config, DTN_Defaulters_Diagnosis_Config_DESC_TEXT, "Event_Or_Config", "Config" );
        }

        bool ret = SimpleDiagnostic::Configure( inputJson );
        if( ret  )
        {
            if( use_event_or_config == EventOrConfig::Config || JsonConfigurable::_dryrun )
            {
                InterventionValidator::ValidateIntervention( GetTypeName(),
                                                             InterventionTypeValidation::INDIVIDUAL,
                                                             negative_diagnosis_config._json,
                                                             inputJson->GetDataLocation() );
                InterventionValidator::ValidateIntervention( GetTypeName(),
                                                             InterventionTypeValidation::INDIVIDUAL,
                                                             defaulters_config._json,
                                                             inputJson->GetDataLocation() );
            }

            if( !JsonConfigurable::_dryrun && 
                negative_diagnosis_event.IsUninitialized() &&
                (negative_diagnosis_config._json.Type() == ElementType::NULL_ELEMENT) )
            {
                const char* msg = "You must define either Negative_Diagnosis_Event or Negative_Diagnosis_Config";
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg );
            }

            if( !JsonConfigurable::_dryrun && 
                defaulters_event.IsUninitialized() &&
                (defaulters_config._json.Type() == ElementType::NULL_ELEMENT) )
            {
                const char* msg = "You must define either Defaulters_Event or Defaulters_Config";
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg );
            }
        }
        return ret ;
    }

    DiagnosticTreatNeg::DiagnosticTreatNeg()
    : SimpleDiagnostic()
    , negative_diagnosis_config()
    , negative_diagnosis_event()
    , defaulters_config()
    , defaulters_event()
    , m_gets_positive_test_intervention(false)
    {
        initSimTypes( 1, "TBHIV_SIM" );
        days_to_diagnosis.handle = std::bind( &DiagnosticTreatNeg::onDiagnosisComplete, this, 0.0f );
    }

    DiagnosticTreatNeg::DiagnosticTreatNeg( const DiagnosticTreatNeg& master )
    : SimpleDiagnostic( master )
    , negative_diagnosis_config(master.negative_diagnosis_config)
    , negative_diagnosis_event(master.negative_diagnosis_event)
    //, defaulters_config(master.defaulters_config)
    , defaulters_event(master.defaulters_event)
    , m_gets_positive_test_intervention(master.m_gets_positive_test_intervention)
    {
        initSimTypes( 1, "TBHIV_SIM" );
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // !!! IndividualInterventionConfig - the copy constructor and assignment operator are different.
        // !!! I needed to use the assignment operator to get this to work correctly.
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        defaulters_config = master.defaulters_config;
        days_to_diagnosis.handle = std::bind( &DiagnosticTreatNeg::onDiagnosisComplete, this, 0.0f );
    }

    DiagnosticTreatNeg::~DiagnosticTreatNeg()
    { 
        LOG_DEBUG("Destructing DiagnosticTreatNeg \n");
    }

    
    bool DiagnosticTreatNeg::Distribute(
        IIndividualHumanInterventionsContext *context,
        ICampaignCostObserver * const pICCO
    )
    {
        //flag for pos/neg test result so that if you are counting down days_to_diagnosis you know what to give when the time comes
        m_gets_positive_test_intervention = true;
        bool ret = SimpleDiagnostic::Distribute( context, pICCO );
        return ret;
    }

    void DiagnosticTreatNeg::Update( float dt )
    {
        if( !BaseIntervention::UpdateIndividualsInterventionStatus() ) return;

        //bool wasDistributed = false;
        if ( expired )
        {
            return; // don't give expired intervention.  should be cleaned up elsewhere anyways, though.
        }

        // You have already been chosen to not default, count down the time until your intervention 
        days_to_diagnosis.Decrement( dt );
        LOG_DEBUG_F( "Individual %d will not default and has a diagnosis but has %f more days until the intervention is distributed.\n", parent->GetSuid().data, float(days_to_diagnosis) );
    }

    void DiagnosticTreatNeg::onDiagnosisComplete( float dt )
    {
        // Give the intervention if the test has come back
        LOG_DEBUG_F("Individual %d finished counting down days_to_diagnosis, my treatment outcome flag is %d \n", parent->GetSuid().data, m_gets_positive_test_intervention);

        if (m_gets_positive_test_intervention)
        {
            positiveTestDistribute();
        }
        else
        {
            negativeTestDistribute();
        }
    }
    
    //if negative test result, either distribute the negative test intervention now, or note you got a negative test and count your days_to_diagnosis down
    void
    DiagnosticTreatNeg::onNegativeTestResult()
    {
        LOG_DEBUG("Negative test Result function\n");

        m_gets_positive_test_intervention = false;
        LOG_DEBUG_F("Reset test result flag to %d \n", m_gets_positive_test_intervention);

        if( !SMART_DRAW( getTreatmentFractionNegative() ) )
        {
            onPatientDefault();
            expired = true;         // this person doesn't get the negative intervention despite the negative test
        }
        else if ( days_to_diagnosis <= 0 ) 
        { 
            negativeTestDistribute(); // since there is no waiting time, distribute intervention right now
        }
        //else we have to wait for days_to_diagnosis to count down until person gets negative intervention
    }


    bool DiagnosticTreatNeg::positiveTestResult()
    {
        LOG_DEBUG_F("Individual %d is taking the test \n", parent->GetSuid().data);
        
        //This test is the same as smear, but if you are smear neg you can get a different intervention

        // Apply diagnostic test with given specificity/sensitivity
        IIndividualHumanTB* tb_ind = nullptr;
        if(parent->QueryInterface( GET_IID( IIndividualHumanTB ), (void**)&tb_ind ) != s_OK)
        {
            LOG_WARN("DiagnosticTreatNeg works with TB sims ONLY");
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "IIndividualHumanTB", "IIndividualHuman" );
        }

        bool activeinf = tb_ind->HasActiveInfection() && !tb_ind->HasActivePresymptomaticInfection();

        if (activeinf)
        {
            // True positive (sensitivity), or False positive (1-specificity)
            bool smearpos = tb_ind->IsSmearPositive();
            bool positiveTest = applySensitivityAndSpecificity( smearpos );
            LOG_DEBUG_F("Individual %d is active %d, smearpos %d, Test sensitivity is %f, result is %d \n", parent->GetSuid().data, activeinf, smearpos, (float) base_sensitivity, positiveTest);
            return positiveTest;
        }
        else
        { 
            LOG_DEBUG("Got a negative result \n");
            return false;
        }
    }

    void DiagnosticTreatNeg::negativeTestDistribute()
    {

        LOG_DEBUG_F( "Individual %d tested 'negative', receiving negative intervention.\n", parent->GetSuid().data );
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

        if( use_event_or_config == EventOrConfig::Event )
        {
            INodeTriggeredInterventionConsumer* broadcaster = nullptr;

            if (s_OK != parent->GetEventContext()->GetNodeEventContext()->QueryInterface(GET_IID(INodeTriggeredInterventionConsumer), (void**)&broadcaster))

            {

                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent->GetEventContext()->GetNodeEventContext()", "INodeTriggeredInterventionConsumer", "INodeEventContext" );
            }
            broadcaster->TriggerNodeEventObservers( parent->GetEventContext(), negative_diagnosis_event );
        }
        else if( negative_diagnosis_config._json.Type() != ElementType::NULL_ELEMENT )
        {
            auto tmp_config = Configuration::CopyFromElement( negative_diagnosis_config._json, "campaign" );

            // Distribute the test-negative intervention
            IDistributableIntervention *di = const_cast<IInterventionFactory*>(ifobj)->CreateIntervention( tmp_config );

            delete tmp_config;
            tmp_config = nullptr;

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
        }
        else
        {
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, "neither event or config defined" );
        }
        expired = true;
    }
    
    void
    DiagnosticTreatNeg::onPatientDefault()
    {
        LOG_DEBUG_F( "Individual %d got the test but defaulted, receiving Defaulters intervention without waiting for days_to_diagnosis (actually means days_to_intervention) \n", parent->GetSuid().data );

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


        if( use_event_or_config == EventOrConfig::Event )
        {
            INodeTriggeredInterventionConsumer* broadcaster = nullptr;

            if (s_OK != parent->GetEventContext()->GetNodeEventContext()->QueryInterface(GET_IID(INodeTriggeredInterventionConsumer), (void**)&broadcaster))

            {

                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent->GetEventContext()->GetNodeEventContext()", "INodeTriggeredInterventionConsumer", "INodeEventContext" );
            }
            broadcaster->TriggerNodeEventObservers( parent->GetEventContext(), defaulters_event );
        }
        else if( defaulters_config._json.Type() != ElementType::NULL_ELEMENT )
        {
            auto tmp_config = Configuration::CopyFromElement( defaulters_config._json, "campaign" );

            // Distribute the defaulters intervention, right away (do not use the days_to_diagnosis
            IDistributableIntervention *di = const_cast<IInterventionFactory*>(ifobj)->CreateIntervention( tmp_config );

            delete tmp_config;
            tmp_config = nullptr;

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
        }
        else
        {
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, "neither event or config defined" );
        }
    }

    //this getter works differently in Resistance Diagnostics
    float DiagnosticTreatNeg::getTreatmentFractionNegative() const
    {
        return treatment_fraction;
    }

    REGISTER_SERIALIZABLE(DiagnosticTreatNeg);

    void DiagnosticTreatNeg::serialize(IArchive& ar, DiagnosticTreatNeg* obj)
    {
        SimpleDiagnostic::serialize(ar, obj);
        DiagnosticTreatNeg& diagnostic = *obj;
        ar.labelElement("negative_diagnosis_config") & diagnostic.negative_diagnosis_config;
        ar.labelElement("negative_diagnosis_event") & diagnostic.negative_diagnosis_event;
        ar.labelElement("defaulters_config") & diagnostic.defaulters_config;
        ar.labelElement("defaulters_event") & diagnostic.defaulters_event;
        ar.labelElement("m_gets_positive_test_intervention") & diagnostic.m_gets_positive_test_intervention;
    }
}


#if 0
namespace Kernel {
    template<class Archive>
    void serialize(Archive &ar, DiagnosticTreatNeg& obj, const unsigned int v)
    {
        ar & obj.defaulters_config;
        ar & (std::string) obj.defaulters_event;
        ar & obj.negative_diagnosis_config;
        ar & (std::string) obj.negative_diagnosis_event;
        ar & obj.m_gets_positive_test_intervention;
        ar & boost::serialization::base_object<Kernel::SimpleDiagnostic>(obj);
    }
}
#endif
