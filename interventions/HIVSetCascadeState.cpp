/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"
#include "HIVSetCascadeState.h"

static const char * _module = "HIVSetCascadeState";

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED(HIVSetCascadeState, HIVSimpleDiagnostic)
    END_QUERY_INTERFACE_DERIVED(HIVSetCascadeState, HIVSimpleDiagnostic)

    IMPLEMENT_FACTORY_REGISTERED(HIVSetCascadeState)

    HIVSetCascadeState::HIVSetCascadeState()
    : HIVSimpleDiagnostic(  )
    {
    }

    HIVSetCascadeState::HIVSetCascadeState( const HIVSetCascadeState& master )
    : HIVSimpleDiagnostic( master )
    {
    }

    bool
    HIVSetCascadeState::Configure(const Configuration* inputJson)
    {
        bool ret = HIVSimpleDiagnostic::Configure( inputJson );
        if( negative_diagnosis_event != NO_TRIGGER_STR )
        {
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, "HIVSetCascadeState can't have a Negative_Diagnosis_Event." );
        }

        return ret;
    }

    bool HIVSetCascadeState::positiveTestResult()
    {
        return true;
    }
}

#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
BOOST_CLASS_EXPORT(Kernel::HIVSetCascadeState)

namespace Kernel {
    template<class Archive>
    void serialize(Archive &ar, HIVSetCascadeState& obj, const unsigned int v)
    {
        static const char * _module = "HIVSetCascadeState";
        LOG_DEBUG("(De)serializing HIVSetCascadeState\n");

        boost::serialization::void_cast_register<HIVSetCascadeState, IDistributableIntervention>();
        ar & boost::serialization::base_object<Kernel::HIVSimpleDiagnostic>(obj);
    }
    template void serialize( boost::mpi::packed_skeleton_iarchive&, Kernel::HIVSetCascadeState&, unsigned int);
}
#endif
