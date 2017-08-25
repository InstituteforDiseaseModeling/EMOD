/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "ResistanceDiagnostics.h"
#include "InterventionEnums.h"
#include "InterventionFactory.h"
#include "NodeEventContext.h"  // for INodeEventContext (ICampaignCostObserver)
#include "TBContexts.h"

SETUP_LOGGING( "MDRDiagnostic" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED(MDRDiagnostic, DiagnosticTreatNeg)
    END_QUERY_INTERFACE_DERIVED(MDRDiagnostic, DiagnosticTreatNeg)

    IMPLEMENT_FACTORY_REGISTERED(MDRDiagnostic)

    bool MDRDiagnostic::Configure(
        const Configuration * inputJson
    )
    {
        return DiagnosticTreatNeg::Configure( inputJson );
    }

    MDRDiagnostic::MDRDiagnostic() 
        : DiagnosticTreatNeg()
        , treatment_fraction_neg(0)
    {
        initConfigTypeMap("Treatment_Fraction_Negative_Diagnosis", &treatment_fraction_neg, Treatment_Fraction_Negative_Diagnosis_DESC_TEXT, 0.0f, 1.0f);
    }

    MDRDiagnostic::~MDRDiagnostic()
    { 
        LOG_DEBUG("Destructing MDR Diagnostic \n"); //GHH check if need to delete more
    }

    bool MDRDiagnostic::positiveTestResult()
    {
        LOG_DEBUG("MDR testing \n");

        // Apply diagnostic test with given specificity/sensitivity
        bool  infected = parent->GetEventContext()->IsInfected();

        IIndividualHumanTB* tb_ind = nullptr;
        if(parent->QueryInterface( GET_IID( IIndividualHumanTB ), (void**)&tb_ind ) != s_OK)
        {
            LOG_WARN("ResistanceDiagnostics works with TB sims ONLY");
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "IIndividualHumanTB", "IIndividualHuman" );
        }

        // always return negative if the person is not infected
        if (infected)
        {
            // True positive (sensitivity), or False positive (1-specificity)
            bool hasMDR = tb_ind->IsMDR();
            bool positiveTest = applySensitivityAndSpecificity( hasMDR );
            LOG_DEBUG_F("MDR test result is %d \n", positiveTest);
            return positiveTest;
        }
        else
        { 
            LOG_DEBUG("Got a negative MDR result \n");
            return false;
        }
    }

    //this getter works differently in Resistance Diagnostics
    float MDRDiagnostic::getTreatmentFractionNegative() const
    {
        LOG_DEBUG_F("The treatment fraction for negative test result people is %f \n", treatment_fraction_neg);
        return treatment_fraction_neg;
    }

/* clorton
    REGISTER_SERIALIZABLE(MDRDiagnostic);

    void MDRDiagnostic::serialize(IArchive& ar, MDRDiagnostic* obj)
    {
        DiagnosticTreatNeg::serialize(ar, obj);
        MDRDiagnostic& diagnostic = *obj;
        ar.labelElement("treatment_fraction_neg") & diagnostic.treatment_fraction_neg;
    }
*/
}
