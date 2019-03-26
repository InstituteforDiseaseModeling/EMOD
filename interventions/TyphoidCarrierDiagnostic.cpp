/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "TyphoidCarrierDiagnostic.h"
#include "IndividualTyphoid.h"
#include "InterventionFactory.h"
#include "NodeEventContext.h"  // for INodeEventContext (ICampaignCostObserver)
#include "TBContexts.h"

SETUP_LOGGING( "TyphoidCarrierDiagnostic" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED(TyphoidCarrierDiagnostic, SimpleDiagnostic)
    END_QUERY_INTERFACE_DERIVED(TyphoidCarrierDiagnostic, SimpleDiagnostic)

    IMPLEMENT_FACTORY_REGISTERED(TyphoidCarrierDiagnostic)

    bool TyphoidCarrierDiagnostic::Configure(
        const Configuration * inputJson
    )
    {
        bool ret = SimpleDiagnostic::Configure(inputJson);
        return ret;
    }

    TyphoidCarrierDiagnostic::TyphoidCarrierDiagnostic() : SimpleDiagnostic()
    {
        initSimTypes( 1, "TYPHOID_SIM" );
    }

    TyphoidCarrierDiagnostic::~TyphoidCarrierDiagnostic()
    { 
        LOG_DEBUG("Destructing TyphoidCarrier Diagnostic \n");
    }

    bool TyphoidCarrierDiagnostic::positiveTestResult()
    {
        LOG_DEBUG("Positive test Result function\n");

        bool infected = parent->GetEventContext()->IsInfected();
        if( !infected )
        {
            return false;
        }

        // Apply diagnostic test with given specificity/sensitivity
        IIndividualHumanTyphoid * typhoid_ind = nullptr;
        if(parent->QueryInterface( GET_IID( IIndividualHumanTyphoid ), (void**)&typhoid_ind ) != s_OK)
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "IIndividualHumanTyphoid", "IIndividualHuman" );
        }
        bool carrier = typhoid_ind->IsChronicCarrier( false );
        LOG_DEBUG_F( "Individual %d is %s from God-level.\n", parent->GetSuid().data, ( carrier ? "positive" : "negative" ) );

        // True positive (sensitivity), or False positive (1-specificity)
        bool positiveTest = applySensitivityAndSpecificity( carrier );
        LOG_DEBUG_F( "Individual %d is %s from human-level.\n", parent->GetSuid().data, ( positiveTest ? "positive" : "negative" ) );

        return positiveTest;
    }

    REGISTER_SERIALIZABLE(TyphoidCarrierDiagnostic);

    void TyphoidCarrierDiagnostic::serialize(IArchive& ar, TyphoidCarrierDiagnostic* obj)
    {
        SimpleDiagnostic::serialize(ar, obj);
    }
}

