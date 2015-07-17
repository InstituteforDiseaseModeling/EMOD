/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"

#include "VectorCohortIndividual.h"
#include "Exceptions.h"
#include "Log.h"
#include "NodeVector.h"
#include "SimulationConfig.h"

static const char * _module = "VectorCohortIndividual";

namespace Kernel
{
    // QI stuff
    BEGIN_QUERY_INTERFACE_DERIVED(VectorCohortIndividual, VectorCohortAging)
        HANDLE_INTERFACE( IVectorCohortIndividual )
    END_QUERY_INTERFACE_DERIVED(VectorCohortIndividual, VectorCohortAging)

    VectorCohortIndividual::VectorCohortIndividual() 
    : state(VectorStateEnum::STATE_ADULT)
    , additional_mortality(0.0f)
    , oviposition_timer(-0.1f)
    , parity(0)
    , neweggs(0)
    , m_strain(NULL)
    , migration_destination(suids::nil_suid())
    , species("gambiae")
    {
    }

    VectorCohortIndividual::VectorCohortIndividual(VectorStateEnum::Enum _state, float age, float progress, int32_t initial_population, VectorMatingStructure _vector_genetics, std::string vector_species_name)
    : VectorCohortAging(age, progress, initial_population, _vector_genetics)
    , state(_state)
    , additional_mortality(0.0f)
    , oviposition_timer(-0.1f) // newly-mated mosquitoes feed on first cycle
    , parity(0)
    , neweggs(0)
    , m_strain(NULL)
    , migration_destination(suids::nil_suid())
    , species(vector_species_name)
    {
    }

    void VectorCohortIndividual::Initialize()
    {
        VectorCohortAging::Initialize();
    }

    VectorCohortIndividual *VectorCohortIndividual::CreateCohort(VectorStateEnum::Enum state, float age, float progress, int32_t initial_population, VectorMatingStructure _vector_genetics, std::string vector_species_name)
    {
        VectorCohortIndividual *newqueue = _new_ VectorCohortIndividual(state, age, progress, initial_population, _vector_genetics, vector_species_name);
        newqueue->Initialize();

        return newqueue;
    }

    void VectorCohortIndividual::AcquireNewInfection( StrainIdentity *infstrain, int incubation_period_override )
    {
        if ( state != VectorStateEnum::STATE_ADULT )
        {
            // removing this as a mosquito can be exposed to multiple contagion populations (one for each antigen each time step) throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "Error trying to infect an already infected mosquito." );
        }
        else
        {

            progress = 0;
            state    = VectorStateEnum::STATE_INFECTED;
            m_strain = _new_ StrainIdentity( infstrain->GetAntigenID(), infstrain->GetGeneticID() );
        }
    }

    const StrainIdentity* VectorCohortIndividual::GetStrainIdentity() const
    {
        return m_strain;
    }

    void VectorCohortIndividual::ImmigrateTo(Node* node)
    {
        LOG_DEBUG_F( "Vector immigrating to node #%d\n", (node->GetSuid()).data );
        INodeVector* pNV = NULL;
        if( node->QueryInterface( GET_IID( INodeVector ), (void**)&pNV ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "node", "INodeVector", "Node" );
        }
        pNV->processImmigratingVector(this);
    }

    void VectorCohortIndividual::SetMigrationDestination(suids::suid destination)
    {
        LOG_DEBUG_F( "Setting vector migration destination to node ID #%d\n", destination.data );
        migration_destination = destination;
    }

    const suids::suid& VectorCohortIndividual::GetMigrationDestination()
    {
        LOG_DEBUG_F( "Got vector migration destination node ID #%d\n", migration_destination.data );
        return migration_destination;
    }

    VectorCohortIndividual::~VectorCohortIndividual()
    {
        if ( m_strain != NULL ) delete m_strain;
    }

    void
    VectorCohortIndividual::IncrementParity()
    {
        parity++;
    }

    void
    VectorCohortIndividual::ReduceOvipositionTimer(
        float delta
    )
    {
        oviposition_timer -= delta;
    }

    void
    VectorCohortIndividual::SetOvipositionTimer(
        float new_opt
    )
    {
        oviposition_timer = new_opt;
    }

    float
    VectorCohortIndividual::GetAdditionalMortality()
    const
    {
        return additional_mortality;
    }

    void
    VectorCohortIndividual::SetAdditionalMortality(
        float new_mortality
    )
    {
        additional_mortality = new_mortality;
    }

    void
    VectorCohortIndividual::SetNewEggs(
        int new_neweggs
    )
    {
        neweggs = new_neweggs;
    }

    bool
    VectorCohortIndividual::IsProgressedOrEmpty()
    const
    {
        if( progress >= 1 || population <= 0 )
        {
            return true;
        }
        return false;
    }

    float
    VectorCohortIndividual::GetAge() const
    {
        return VectorCohortAging::GetAge();
    }

    void
    VectorCohortIndividual::IncreaseAge( float dt )
    {
        return VectorCohortAging::IncreaseAge( dt );
    }

    VectorStateEnum::Enum &
    VectorCohortIndividual::GetState()
    {
        return state;
    }

    void 
    VectorCohortIndividual::SetState(
        const VectorStateEnum::Enum & newState
    )
    {
        state = newState;
        if( state == VectorStateEnum::STATE_INFECTED )
        {
            ClearProgress();
        }
    }

    float
    VectorCohortIndividual::GetMortality(
        uint32_t addition
    )
    const
    {
        float ret = 0.0f;
        
        if (GET_CONFIGURABLE(SimulationConfig)->vector_aging)
        {
            ret = additional_mortality + addition + mortalityFromAge( age );
        }
        else
        {
            // does this really ever get called?
            ret = additional_mortality + addition;
        }
        return ret;
    }

    float VectorCohortIndividual::GetOvipositionTimer()
    {
        return oviposition_timer;

    }

    int VectorCohortIndividual::GetParity()
    {
        return parity;

    }

    int VectorCohortIndividual::GetNewEggs()
    {
        return neweggs;

    }

    const std::string & VectorCohortIndividual::GetSpecies()
    {
        return species;
    }

#if 0
    template<class Archive>
    void VectorCohortIndividual::serialize_inner( Archive & ar, const unsigned int file_version )
    {
        // Register derived types - N/A

        // Serialize fields
        typemap.serialize(this, ar, file_version);
        ar & m_strain;

        // Serialize base class
        ar & boost::serialization::base_object<VectorCohortAging>(*this);
    }

    template void VectorCohortIndividual::serialize_inner( boost::archive::binary_iarchive & ar, const unsigned int file_version );
    template void VectorCohortIndividual::serialize_inner( boost::archive::binary_oarchive & ar, const unsigned int file_version );
#endif
}

#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
BOOST_CLASS_EXPORT(Kernel::VectorCohortIndividual)
namespace Kernel {
    template< typename Archive >
    void serialize( Archive& ar, VectorCohortIndividual &obj, unsigned int file_version )
    {
        ar & obj.state;
        ar & obj.additional_mortality;
        ar & obj.oviposition_timer;
        ar & obj.parity;
        ar & obj.neweggs;
        ar & obj.migration_destination;
        ar & obj.species;
        ar & obj.m_strain;
        ar & boost::serialization::base_object<Kernel::VectorCohortAging>(obj);
    }
    template void serialize( boost::archive::binary_iarchive&, VectorCohortIndividual &obj, unsigned int file_version );
    template void serialize( boost::mpi::packed_iarchive&, VectorCohortIndividual &obj, unsigned int file_version );
    template void serialize( boost::mpi::packed_skeleton_oarchive&, VectorCohortIndividual &obj, unsigned int file_version );
    template void serialize( boost::archive::binary_oarchive&, VectorCohortIndividual &obj, unsigned int file_version );
    template void serialize( boost::mpi::packed_oarchive&, VectorCohortIndividual &obj, unsigned int file_version );
    template void serialize( boost::mpi::detail::content_oarchive&, VectorCohortIndividual &obj, unsigned int file_version );
    template void serialize( boost::mpi::detail::mpi_datatype_oarchive&, VectorCohortIndividual &obj, unsigned int file_version );
}
#endif
