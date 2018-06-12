/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "InterpolatedValueMap.h"
#include "HIVSimpleDiagnostic.h"

namespace Kernel
{
    class HIVPiecewiseByYearAndSexDiagnostic : public HIVSimpleDiagnostic
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_FACTORY_REGISTERED(InterventionFactory, HIVPiecewiseByYearAndSexDiagnostic, IDistributableIntervention)

    public: 
        HIVPiecewiseByYearAndSexDiagnostic();
        HIVPiecewiseByYearAndSexDiagnostic( const HIVPiecewiseByYearAndSexDiagnostic& );
        virtual bool Configure( const Configuration* pConfig ) override;
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;
        virtual bool positiveTestResult() override;

    protected:
        int interpolation_order;
        float female_multiplier;
        float default_value;
        InterpolatedValueMap year2ValueMap;
        float period_between_trials;
        float value_multiplier;

        DECLARE_SERIALIZABLE(HIVPiecewiseByYearAndSexDiagnostic);
    };
}
