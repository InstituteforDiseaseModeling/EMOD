/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

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
    static uint64_t VCI_COUNTER = 1 ; //TODO - need to make multi-core

    // QI stuff
    BEGIN_QUERY_INTERFACE_DERIVED(VectorCohortIndividual, VectorCohortAging)
        HANDLE_INTERFACE( IVectorCohortIndividual )
    END_QUERY_INTERFACE_DERIVED(VectorCohortIndividual, VectorCohortAging)

    VectorCohortIndividual::VectorCohortIndividual() 
    : m_ID( VCI_COUNTER++ )
    , state(VectorStateEnum::STATE_ADULT)
    , additional_mortality(0.0f)
    , oviposition_timer(-0.1f)
    , parity(0)
    , neweggs(0)
    , migration_destination(suids::nil_suid())
    , migration_type(MigrationType::NO_MIGRATION)
    , species("gambiae")
    , m_strain(nullptr)
    {
    }

    VectorCohortIndividual::VectorCohortIndividual(VectorStateEnum::Enum _state, float age, float progress, int32_t initial_population, VectorMatingStructure _vector_genetics, std::string vector_species_name)
    : VectorCohortAging(age, progress, initial_population, _vector_genetics)
    , m_ID( VCI_COUNTER++ )
    , state(_state)
    , additional_mortality(0.0f)
    , oviposition_timer(-0.1f) // newly-mated mosquitoes feed on first cycle
    , parity(0)
    , neweggs(0)
    , migration_destination(suids::nil_suid())
    , migration_type(MigrationType::NO_MIGRATION)
    , species(vector_species_name)
    , m_strain(nullptr)
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

    void VectorCohortIndividual::ImmigrateTo( INodeContext* node )
    {
        LOG_DEBUG_F( "Vector immigrating to node #%d\n", (node->GetSuid()).data );
        INodeVector* pNV = nullptr;
        if( node->QueryInterface( GET_IID( INodeVector ), (void**)&pNV ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "node", "INodeVector", "Node" );
        }
        pNV->processImmigratingVector(this);
    }

    void VectorCohortIndividual::SetMigrating( suids::suid destination, 
                                               MigrationType::Enum type, 
                                               float timeUntilTrip, 
                                               float timeAtDestination,
                                               bool isDestinationNewHome )
    {
        LOG_DEBUG_F( "Setting vector migration destination to node ID #%d\n", destination.data );
        migration_destination = destination;
        migration_type        = type;
    }

    const suids::suid& VectorCohortIndividual::GetMigrationDestination()
    {
        LOG_DEBUG_F( "Got vector migration destination node ID #%d\n", migration_destination.data );
        return migration_destination;
    }

    VectorCohortIndividual::~VectorCohortIndividual()
    {
        if ( m_strain != nullptr ) delete m_strain;
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

    REGISTER_SERIALIZABLE(VectorCohortIndividual);

    void VectorCohortIndividual::serialize(IArchive& ar, VectorCohortIndividual* obj)
    {
        VectorCohortAging::serialize(ar, obj);
        VectorCohortIndividual& cohort = *obj;
        ar.labelElement("m_ID"                 ) & cohort.m_ID;
        ar.labelElement("state"                ) & (uint32_t&)cohort.state;
        ar.labelElement("additional_mortality" ) & cohort.additional_mortality;
        ar.labelElement("oviposition_timer"    ) & cohort.oviposition_timer;
        ar.labelElement("parity"               ) & cohort.parity;
        ar.labelElement("neweggs"              ) & cohort.neweggs;
        ar.labelElement("migration_destination") & cohort.migration_destination.data;
        ar.labelElement("migration_type"       ) & (uint32_t&)cohort.migration_type;
        ar.labelElement("species"              ) & cohort.species;

        bool has_strain = ar.IsWriter() ? (cohort.m_strain != nullptr) : false; // We put false here, but this is just a placeholder since we're reading from the archive.
        ar.labelElement("__has_strain__");
        ar & has_strain;
        // clorton TODO - perhaps the cohort should always have a non-null m_strain, just use a dummy StrainIdentity when there's no infection.
        if (has_strain)
        {
            ar.labelElement("m_strain");
#if defined(WIN32)
            Kernel::serialize(ar, cohort.m_strain);
#endif
        }
    }
}
