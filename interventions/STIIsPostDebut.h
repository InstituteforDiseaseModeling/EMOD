/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include "Diagnostics.h"

namespace Kernel
{
    class IDMAPI STIIsPostDebut : public SimpleDiagnostic
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_FACTORY_REGISTERED(InterventionFactory, STIIsPostDebut, IDistributableIntervention)

    public: 
        STIIsPostDebut();
        STIIsPostDebut( const STIIsPostDebut& ); // copy ctor

        // IDistributingDistributableIntervention
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject);

        // SimpleDiagnostic
        virtual bool positiveTestResult(); // Test if recipient "tests positive"
        virtual void onNegativeTestResult();  // What to do if recipient "tests negative"

    protected:
#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        ConstrainedString negative_diagnosis_event;
#pragma warning( pop )

    private:
#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
        friend class ::boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive &ar, STIIsPostDebut &obj, const unsigned int v);
#endif
    };
}
