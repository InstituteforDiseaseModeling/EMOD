/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include <string>
#include <list>
#include <vector>

#include "Diagnostics.h"

namespace Kernel
{
    class ActiveDiagnostic : public SimpleDiagnostic 
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()
        DECLARE_FACTORY_REGISTERED(InterventionFactory, ActiveDiagnostic, IDistributableIntervention)

    public: 
        ActiveDiagnostic();
        virtual bool Configure( const Configuration* pConfig );
        virtual ~ActiveDiagnostic();

    protected:
        virtual bool positiveTestResult();

    private:
#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
        // Serialization
        friend class ::boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive &ar, ActiveDiagnostic &obj, const unsigned int v);
#endif
    };
}


#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
BOOST_CLASS_EXPORT(Kernel::ActiveDiagnostic)

namespace Kernel {
    template<class Archive>
    void serialize(Archive &ar, ActiveDiagnostic& obj, const unsigned int v)
    {

        boost::serialization::void_cast_register<ActiveDiagnostic, IDistributableIntervention>();

        ar & boost::serialization::base_object<Kernel::SimpleDiagnostic>(obj);
    }
}
#endif
