/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"
#include "StiCoInfectionDiagnostic.h"
#include "InterventionEnums.h"
#include "InterventionFactory.h"
#include "IIndividualHumanSTI.h"

static const char * _module = "StiCoInfectionDiagnostic";

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED(StiCoInfectionDiagnostic, SimpleDiagnostic)
    END_QUERY_INTERFACE_DERIVED(StiCoInfectionDiagnostic, SimpleDiagnostic)

    IMPLEMENT_FACTORY_REGISTERED(StiCoInfectionDiagnostic)

    bool StiCoInfectionDiagnostic::Configure(
        const Configuration * inputJson
    )
    {
        ConfigurePositiveEventOrConfig( inputJson );
        return JsonConfigurable::Configure(inputJson); 
    }

    StiCoInfectionDiagnostic::StiCoInfectionDiagnostic() : SimpleDiagnostic()
    {
        initSimTypes( 2, "STI_SIM", "HIV_SIM" );
    }

    StiCoInfectionDiagnostic::StiCoInfectionDiagnostic( const StiCoInfectionDiagnostic& master )
    : SimpleDiagnostic( master )
    {
    }
        
    StiCoInfectionDiagnostic::~StiCoInfectionDiagnostic()
    { 
        LOG_DEBUG("Destructing Active Diagnostic \n");
    }

    bool StiCoInfectionDiagnostic::positiveTestResult()
    {
        LOG_DEBUG("Positive test Result function\n");

        // Apply diagnostic test with given specificity/sensitivity
        float rand = parent->GetRng()->e();

        IIndividualHumanSTI* sti_ind = NULL;
        if(parent->QueryInterface( GET_IID( IIndividualHumanSTI ), (void**)&sti_ind ) != s_OK)
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "IIndividualHumanSTI", "IIndividualHuman" );
        }
        bool activeinf = sti_ind->HasSTICoInfection();

        // always return negative if the person is not infected, intended to be used with GroupEventCoordinator
        // TODO: allow to distribute Smear diagnostic to non-infected individuals?

        bool positiveTest = false;
        // True positive (sensitivity), or False positive (1-specificity)
        positiveTest = ( activeinf && (rand < base_sensitivity) ) || ( !activeinf && (rand > base_specificity) );
        return positiveTest;

    }
}
