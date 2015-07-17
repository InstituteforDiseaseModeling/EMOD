/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

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
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject);
        virtual bool Configure(const Configuration* inputJson);

        // HIVSimpleDiagnostic
        virtual bool positiveTestResult();
        virtual void positiveTestDistribute();

    protected:

    private:
#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
        friend class ::boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive &ar, HIVDrawBlood &obj, const unsigned int v);
#endif
    };
}
