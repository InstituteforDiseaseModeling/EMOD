/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <string>
#include <list>
#include <vector>

#include "DiagnosticsTreatNeg.h"

namespace Kernel
{
    class MDRDiagnostic : public DiagnosticTreatNeg 
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()
        DECLARE_FACTORY_REGISTERED(InterventionFactory, MDRDiagnostic, IDistributableIntervention)

    public: 
        MDRDiagnostic();
        virtual ~MDRDiagnostic();// { }
        virtual bool Configure( const Configuration* pConfig ) override;
        virtual bool positiveTestResult() override;
        virtual float getTreatmentFractionNegative() const override;

    protected:

        float treatment_fraction_neg;

// clorton        DECLARE_SERIALIZABLE(MDRDiagnostic);
    };
}
