/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "HIVSimpleDiagnostic.h"

namespace Kernel
{
    class IDMAPI HIVSetCascadeState : public HIVSimpleDiagnostic
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_FACTORY_REGISTERED(InterventionFactory, HIVSetCascadeState, IDistributableIntervention)

    public: 
        virtual bool positiveTestResult();

        HIVSetCascadeState();
        HIVSetCascadeState( const HIVSetCascadeState& );
        virtual bool Configure(const Configuration* inputJson);

        // IDistributingDistributableIntervention
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject);

    protected:

#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        DECLARE_SERIALIZABLE(HIVSetCascadeState);
#pragma warning( pop )
    };
}
