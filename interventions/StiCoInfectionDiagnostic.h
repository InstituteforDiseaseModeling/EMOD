/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

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
        virtual bool Configure( const Configuration* pConfig );
        virtual ~StiCoInfectionDiagnostic();

    protected:
        virtual bool positiveTestResult();

    private:
#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
        // Serialization
        friend class ::boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive &ar, StiCoInfectionDiagnostic &obj, const unsigned int v);
#endif
    };
}


#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
BOOST_CLASS_EXPORT(Kernel::StiCoInfectionDiagnostic)

namespace Kernel {
    template<class Archive>
    void serialize(Archive &ar, StiCoInfectionDiagnostic& obj, const unsigned int v)
    {

        boost::serialization::void_cast_register<StiCoInfectionDiagnostic, IDistributableIntervention>();

        ar & boost::serialization::base_object<Kernel::SimpleDiagnostic>(obj);
    }
}
#endif
