/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"
#include "HIVDrawBlood.h"

#include "InterventionEnums.h"
#include "InterventionFactory.h"
#include "NodeEventContext.h"  // for INodeEventContext (ICampaignCostObserver)
#include "SusceptibilityHIV.h" // for time-date util function and access into IHIVCascadeOfCare
#include "HIVInterventionsContainer.h" // for time-date util function and access into IHIVCascadeOfCare
#include "IIndividualHumanHIV.h"

static const char * _module = "HIVDrawBlood";

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED(HIVDrawBlood, HIVSimpleDiagnostic)
    END_QUERY_INTERFACE_DERIVED(HIVDrawBlood, HIVSimpleDiagnostic)

    IMPLEMENT_FACTORY_REGISTERED(HIVDrawBlood)

    HIVDrawBlood::HIVDrawBlood()
    : HIVSimpleDiagnostic()
    {
    }

    HIVDrawBlood::HIVDrawBlood( const HIVDrawBlood& master )
    : HIVSimpleDiagnostic( master )
    {
    }

    bool HIVDrawBlood::Configure(const Configuration* inputJson)
    {
        bool ret = HIVSimpleDiagnostic::Configure( inputJson );
        if( negative_diagnosis_event != NO_TRIGGER_STR )
        {
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, "HIVDrawBlood can't have a Negative_Diagnosis_Event." );
        }

        return ret;
    }

    bool HIVDrawBlood::positiveTestResult()
    {
        return true;
    }

    void HIVDrawBlood::positiveTestDistribute()
    {
        IIndividualHumanHIV * hiv_parent = nullptr;
        if (parent->QueryInterface(GET_IID(IIndividualHumanHIV), (void**)&hiv_parent) != s_OK)
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "IIndividualHumanHIV", "IIndividualHumanContext" );
        }
        IHIVMedicalHistory * med_parent = nullptr;
        if (parent->GetInterventionsContext()->QueryInterface(GET_IID(IHIVMedicalHistory), (void**)&med_parent) != s_OK)
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "IHIVMedicalHistory", "IIndividualHumanContext" );
        }

        float cd4count = hiv_parent->GetHIVSusceptibility()->GetCD4count();
        med_parent->OnTestCD4(cd4count);

        expired = true;

        HIVSimpleDiagnostic::positiveTestDistribute();
    }
}

#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
BOOST_CLASS_EXPORT(Kernel::HIVDrawBlood)

namespace Kernel {
    template<class Archive>
    void serialize(Archive &ar, HIVDrawBlood& obj, const unsigned int v)
    {
        static const char * _module = "HIVDrawBlood";
        LOG_DEBUG("(De)serializing HIVDrawBlood\n");

        boost::serialization::void_cast_register<HIVDrawBlood, IDistributableIntervention>();
        //ar & obj.event2ProbabilityMap;     // todo: serialize this!
        ar & boost::serialization::base_object<Kernel::HIVSimpleDiagnostic>(obj);
    }
    template void serialize( boost::mpi::packed_skeleton_iarchive&, Kernel::HIVDrawBlood&, unsigned int);
}
#endif
