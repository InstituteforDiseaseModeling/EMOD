/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "HIVSimpleDiagnostic.h"

namespace Kernel
{
    class IDMAPI HIVDrawBlood : public HIVSimpleDiagnostic
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_FACTORY_REGISTERED(InterventionFactory, HIVDrawBlood, IDistributableIntervention)

    public: 
        HIVDrawBlood();
        HIVDrawBlood( const HIVDrawBlood& );

        // IDistributingDistributableIntervention
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;
        virtual bool Configure(const Configuration* inputJson) override;

        // HIVSimpleDiagnostic
        virtual bool positiveTestResult() override;
        virtual void positiveTestDistribute() override;

#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        DECLARE_SERIALIZABLE(HIVDrawBlood);
#pragma warning( pop )
    };
}
