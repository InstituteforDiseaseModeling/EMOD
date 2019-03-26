/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "ImmunityBloodTest.h"
#include "IIndividualHumanContext.h"
#include "IIndividualHuman.h"

SETUP_LOGGING("ImmunityBloodTest")

const float ImmuneThreshold = 1.0f; //threshold to be considered immune

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED(ImmunityBloodTest, SimpleDiagnostic)
    END_QUERY_INTERFACE_DERIVED(ImmunityBloodTest, SimpleDiagnostic)
    IMPLEMENT_FACTORY_REGISTERED(ImmunityBloodTest)

    bool ImmunityBloodTest::Configure(const Configuration * inputJson)
    {
        initConfigTypeMap("Negative_Diagnosis_Event", &negative_diagnosis_event, IBT_Negative_Diagnosis_Config_Event_DESC_TEXT, "Event_Or_Config", "Event");
        initConfigTypeMap("Positive_Threshold_AcquisitionImmunity", &threshold_acquisitionImmunity, IBT_Positive_Threshold_AcquisitionImmunity_DESC_TEXT, 0.0, 1.0, ImmuneThreshold);
        return SimpleDiagnostic::Configure(inputJson);
    }

    EventOrConfig::Enum ImmunityBloodTest::getEventOrConfig( const Configuration * inputJson )
    {
        return EventOrConfig::Event;
    }

    void ImmunityBloodTest::CheckConfigTriggers( const Configuration * inputJson )
    {
        if( negative_diagnosis_event.IsUninitialized() && positive_diagnosis_event.IsUninitialized() )
        {
            std::stringstream ss;
            ss << "Neither Positive_Diagnosis_Event nor Negative_Diagnosis_Event is defined." << std::endl;
            throw InitializationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
        else if( positive_diagnosis_event.IsUninitialized() )
        {
            LOG_INFO( "Positive_Diagnosis_Event is not defined. No notification will be given for this event.\n" );
        }
        else if( negative_diagnosis_event.IsUninitialized() )
        {
            LOG_INFO( "Negative_Diagnosis_Event is not defined. No notification will be given for this event.\n" );
        }        
    }

    ImmunityBloodTest::ImmunityBloodTest() 
        : SimpleDiagnostic()
        , threshold_acquisitionImmunity(ImmuneThreshold)
    {
        initSimTypes(1, "*");
    }

    ImmunityBloodTest::ImmunityBloodTest(const ImmunityBloodTest& master) 
        : SimpleDiagnostic(master)
        , threshold_acquisitionImmunity(master.threshold_acquisitionImmunity)
        , negative_diagnosis_event(master.negative_diagnosis_event)
    {
    }

    ImmunityBloodTest::~ImmunityBloodTest()
    {
        LOG_DEBUG("Destructing ImmunityBloodTest \n");
    }

    bool ImmunityBloodTest::positiveTestResult()
    {
        IIndividualHuman* p_iih = nullptr;
        if( parent->QueryInterface( GET_IID( IIndividualHuman ), (void**)&p_iih ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "IIndividualHuman", "IIndividualHumanContext" );
        }

        // return true if individual has natural immunity or immunity aquired by intervention
        float acquisitionSusceptibility = p_iih->GetAcquisitionImmunity();   // actually susceptibility 
        bool has_attribute = (acquisitionSusceptibility <= (1.0 - threshold_acquisitionImmunity));
        LOG_DEBUG_F("acquisitionModifier = %f,  has_attribute = %d  threshold__AcquisitionImmunity = %f\n", acquisitionSusceptibility, has_attribute, threshold_acquisitionImmunity);

        return applySensitivityAndSpecificity(has_attribute);
    }

    void ImmunityBloodTest::onNegativeTestResult()
    {
        auto iid = parent->GetSuid().data;
        LOG_DEBUG_F( "Individual %d tested 'negative' in ImmunityBloodTest, broadcasting negative event.\n", iid );

        if (!negative_diagnosis_event.IsUninitialized())
        {
            LOG_DEBUG_F( "Broadcasting event %s as negative diagnosis event for individual %d.\n", negative_diagnosis_event.c_str(), iid );
            broadcastEvent(negative_diagnosis_event);
        }
        else
        {
            LOG_DEBUG_F( "Negative diagnosis event is not defined for individual %d.\n", iid );
        }
        expired = true;
    }

    REGISTER_SERIALIZABLE(ImmunityBloodTest);

    void ImmunityBloodTest::serialize(IArchive& ar, ImmunityBloodTest* obj)
    {
        SimpleDiagnostic::serialize(ar, obj);
        ImmunityBloodTest& ibt = *obj;
        ar.labelElement("threshold_acquisitionImmunity") & ibt.threshold_acquisitionImmunity;
        ar.labelElement("negative_diagnosis_event") & ibt.negative_diagnosis_event;
    }
}
