/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "HIVRapidHIVDiagnostic.h"

#include "InterventionEnums.h"
#include "InterventionFactory.h"
#include "NodeEventContext.h"  // for INodeEventContext (ICampaignCostObserver)
#include "IHIVInterventionsContainer.h" // for time-date util function

SETUP_LOGGING( "HIVRapidHIVDiagnostic" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED(HIVRapidHIVDiagnostic, HIVSimpleDiagnostic)
    END_QUERY_INTERFACE_DERIVED(HIVRapidHIVDiagnostic, HIVSimpleDiagnostic)

    IMPLEMENT_FACTORY_REGISTERED(HIVRapidHIVDiagnostic)

    HIVRapidHIVDiagnostic::HIVRapidHIVDiagnostic()
    : HIVSimpleDiagnostic()
    , m_ProbReceivedResults(1.0)
    {
        initConfigTypeMap("Probability_Received_Result", &m_ProbReceivedResults, HIV_RHD_Probability_Received_Result_DESC_TEXT, 0, 1.0, 1.0);
    }

    HIVRapidHIVDiagnostic::HIVRapidHIVDiagnostic( const HIVRapidHIVDiagnostic& master )
        : HIVSimpleDiagnostic( master )
        , m_ProbReceivedResults( master.m_ProbReceivedResults )
    {
    }

    // runs on a positive test when in positive treatment fraction
    void HIVRapidHIVDiagnostic::positiveTestDistribute()
    {
        IHIVMedicalHistory * hiv_parent = nullptr;
        if( parent->GetInterventionsContext()->QueryInterface( GET_IID(IHIVMedicalHistory), (void**)&hiv_parent ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "IHIVMedicalHistory", "IHIVInterventionsContainer" );
        }

        // update the patient's medical chart with testing history
        hiv_parent->OnTestForHIV(true);

        onReceivedResult( hiv_parent, true );

        // distribute the intervention
        HIVSimpleDiagnostic::positiveTestDistribute();
    }

    // runs on a negative test when in negative treatment fraction
    void HIVRapidHIVDiagnostic::onNegativeTestResult()
    {
        IHIVMedicalHistory * hiv_parent = nullptr;
        if( parent->GetInterventionsContext()->QueryInterface( GET_IID(IHIVMedicalHistory), (void**)&hiv_parent ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "IHIVMedicalHistory", "IHIVInterventionsContainer" );
        }

        // update the patent's medical chart with testing history
        hiv_parent->OnTestForHIV(false);

        onReceivedResult( hiv_parent, false );

        // distribute the intervention
        HIVSimpleDiagnostic::onNegativeTestResult();
    }

    void HIVRapidHIVDiagnostic::onReceivedResult( IHIVMedicalHistory* pMedHistory, bool resultIsHivPositive )
    {
        if( SMART_DRAW( m_ProbReceivedResults ) )
        {
            pMedHistory->OnReceivedTestResultForHIV( resultIsHivPositive );
        }

        INodeTriggeredInterventionConsumer* broadcaster = nullptr;
        if (s_OK != parent->GetEventContext()->GetNodeEventContext()->QueryInterface(GET_IID(INodeTriggeredInterventionConsumer), (void**)&broadcaster))
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent->GetEventContext()->GetNodeEventContext()", "INodeTriggeredInterventionConsumer", "INodeEventContext" );
        }

        if( resultIsHivPositive )
        {
            broadcaster->TriggerNodeEventObservers( parent->GetEventContext(), EventTrigger::HIVTestedPositive );
        }
        else
        {
            broadcaster->TriggerNodeEventObservers( parent->GetEventContext(), EventTrigger::HIVTestedNegative );
        }
    }

    REGISTER_SERIALIZABLE(HIVRapidHIVDiagnostic);

    void HIVRapidHIVDiagnostic::serialize(IArchive& ar, HIVRapidHIVDiagnostic* obj)
    {
        HIVSimpleDiagnostic::serialize( ar, obj );
        HIVRapidHIVDiagnostic& diag = *obj;

        ar.labelElement("m_ProbReceivedResults") & diag.m_ProbReceivedResults;
    }
}

