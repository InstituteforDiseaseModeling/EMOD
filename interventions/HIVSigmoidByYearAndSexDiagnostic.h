/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include "HIVSimpleDiagnostic.h"
#include "IHealthSeekingBehavior.h"

namespace Kernel
{
    class HIVSigmoidByYearAndSexDiagnostic : public HIVSimpleDiagnostic//, public IHealthSeekingBehavior
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_FACTORY_REGISTERED(InterventionFactory, HIVSigmoidByYearAndSexDiagnostic, IDistributableIntervention)

    public: 
        HIVSigmoidByYearAndSexDiagnostic();
        HIVSigmoidByYearAndSexDiagnostic( const HIVSigmoidByYearAndSexDiagnostic& );
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject);
        virtual bool positiveTestResult();

    protected:

        float rampMin;
        float rampMax;
        float rampMidYear;
        float rampRate;
        float femaleMultiplier;

    private:
#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
        friend class ::boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive &ar, HIVSigmoidByYearAndSexDiagnostic &obj, const unsigned int v);
#endif
    };
}
