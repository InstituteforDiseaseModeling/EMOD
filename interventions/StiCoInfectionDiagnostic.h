/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "Diagnostics.h"

namespace Kernel
{
    class StiCoInfectionDiagnostic : public SimpleDiagnostic 
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()
        DECLARE_FACTORY_REGISTERED(InterventionFactory, StiCoInfectionDiagnostic, IDistributableIntervention)

    public: 
        StiCoInfectionDiagnostic();
        StiCoInfectionDiagnostic( const StiCoInfectionDiagnostic& );
        virtual bool Configure( const Configuration* pConfig ) override;
        virtual ~StiCoInfectionDiagnostic();

    protected:
        virtual bool positiveTestResult() override;

        DECLARE_SERIALIZABLE(StiCoInfectionDiagnostic);
    };
}
