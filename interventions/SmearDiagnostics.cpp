/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "SmearDiagnostics.h"
#include "InterventionEnums.h"
#include "InterventionFactory.h"
#include "NodeEventContext.h"  // for INodeEventContext (ICampaignCostObserver)
#include "TBContexts.h"

SETUP_LOGGING( "SmearDiagnostic" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED(SmearDiagnostic, SimpleDiagnostic)
    END_QUERY_INTERFACE_DERIVED(SmearDiagnostic, SimpleDiagnostic)

    IMPLEMENT_FACTORY_REGISTERED(SmearDiagnostic)

    bool SmearDiagnostic::Configure(
        const Configuration * inputJson
    )
    {
        return SimpleDiagnostic::Configure( inputJson );
    }

    SmearDiagnostic::SmearDiagnostic() : SimpleDiagnostic()
    {
        initSimTypes( 0 );
    }

    SmearDiagnostic::SmearDiagnostic( const SmearDiagnostic& master )
    : SimpleDiagnostic( master )
    {
    }

    SmearDiagnostic::~SmearDiagnostic()
    { 
        LOG_DEBUG("Destructing Smear Diagnostic \n");
    }

    bool SmearDiagnostic::positiveTestResult()
    {
        LOG_DEBUG("Positive test Result function\n");

        // Apply diagnostic test with given specificity/sensitivity

        IIndividualHumanTB* tb_ind = nullptr;
        if(parent->QueryInterface( GET_IID( IIndividualHumanTB ), (void**)&tb_ind ) != s_OK)
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "IIndividualHumanTB", "IIndividualHuman" );
        }
        bool activeinf = tb_ind->HasActiveInfection() && !tb_ind->HasActivePresymptomaticInfection();

        // always return negative if the person is not infected, intended to be used with GroupEventCoordinator
        // TODO: allow to distribute Smear diagnostic to non-infected individuals?

        if (activeinf)
        {
            // True positive (sensitivity), or False positive (1-specificity)
            bool smearpos = tb_ind->IsSmearPositive();
            bool positiveTest = applySensitivityAndSpecificity( smearpos );
            return positiveTest;
        }
        else
        { return false;}
    }

    REGISTER_SERIALIZABLE(SmearDiagnostic);

    void SmearDiagnostic::serialize(IArchive& ar, SmearDiagnostic* obj)
    {
        SimpleDiagnostic::serialize( ar, obj );
        // No SmearDiagnostic specific fields yet.
    }
}
