/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "VectorCohortIndividual.h"
#include "Exceptions.h"
#include "Log.h"
#include "NodeVector.h"
#include "SimulationConfig.h"
#include "VectorParameters.h"

SETUP_LOGGING( "VectorCohortIndividual" )

namespace Kernel
{
    static uint64_t VCI_COUNTER = 1 ; //TODO - need to make multi-core
    std::string VectorCohortIndividual::_gambiae("gambiae");

    // QI stuff
    BEGIN_QUERY_INTERFACE_DERIVED(VectorCohortIndividual, VectorCohortAging)
        HANDLE_INTERFACE( IVectorCohortIndividual )
    END_QUERY_INTERFACE_DERIVED(VectorCohortIndividual, VectorCohortAging)

    VectorCohortIndividual::VectorCohortIndividual() 
    : VectorCohortAging()
    , m_ID( VCI_COUNTER++ )
    , state(VectorStateEnum::STATE_ADULT)
    , additional_mortality(0.0f)
    , oviposition_timer(-0.1f)
    , parity(0)
    , neweggs(0)
    , migration_type(MigrationType::NO_MIGRATION)
    , pSpecies(&_gambiae)
    , migration_destination(suids::nil_suid())
    , m_strain(0, 0)
    {
    }

    VectorCohortIndividual::VectorCohortIndividual( VectorStateEnum::Enum _state, 
                                                    float age,
                                                    float progress,
                                                    int32_t initial_population,
                                                    const VectorMatingStructure& _vector_genetics,
                                                    const std::string* vector_species_name )
    : VectorCohortAging(age, progress, initial_population, _vector_genetics)
    , m_ID( VCI_COUNTER++ )
    , state(_state)
    , additional_mortality(0.0f)
    , oviposition_timer(-0.1f) // newly-mated mosquitoes feed on first cycle
    , parity(0)
    , neweggs(0)
    , migration_type(MigrationType::NO_MIGRATION)
    , pSpecies(vector_species_name)
    , migration_destination(suids::nil_suid())
    , m_strain(0, 0)
    {
    }

    void VectorCohortIndividual::Initialize()
    {
        VectorCohortAging::Initialize();
    }

    VectorCohortIndividual *VectorCohortIndividual::CreateCohort( VectorStateEnum::Enum state, 
                                                                  float age,
                                                                  float progress,
                                                                  int32_t initial_population,
                                                                  const VectorMatingStructure& _vector_genetics,
                                                                  const std::string* vector_species_name )
    {
        VectorCohortIndividual *newqueue;
        
        if (!_supply || (_supply->size() == 0))
        {
            newqueue = _new_ VectorCohortIndividual(state, age, progress, initial_population, _vector_genetics, vector_species_name);
        }
        else
        {
            VectorCohortIndividual* ptr = _supply->back();
            _supply->pop_back();
            newqueue = new(ptr) VectorCohortIndividual(state, age, progress, initial_population, _vector_genetics, vector_species_name);
        }
        newqueue->Initialize();

        return newqueue;
    }

    void VectorCohortIndividual::AcquireNewInfection( const IStrainIdentity *infstrain, int incubation_period_override )
    {
        if ( state != VectorStateEnum::STATE_ADULT )
        {
            // removing this as a mosquito can be exposed to multiple contagion populations (one for each antigen each time step) throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "Error trying to infect an already infected mosquito." );
        }
        else
        {
            progress = 0;
            state    = VectorStateEnum::STATE_INFECTED;
            m_strain = StrainIdentity( infstrain );
        }
    }

    const IStrainIdentity& VectorCohortIndividual::GetStrainIdentity() const
    {
        return m_strain;
    }

    void VectorCohortIndividual::ImmigrateTo( INodeContext* node )
    {
        // This is used often with vector migration. If LOG_DEBUG_F _isn't_ #defined away,
        // consider changing this to LOG_VALID_F.
        LOG_DEBUG_F( "Vector immigrating to node #%d\n", (node->GetSuid()).data );

        // static_cast is _much_ faster than QI and we're required to have a NodeVector here.
        INodeVector* pNV = static_cast<NodeVector*>(static_cast<Node*>(node));
        pNV->processImmigratingVector(this);
    }

    void VectorCohortIndividual::SetMigrating( suids::suid destination, 
                                               MigrationType::Enum type, 
                                               float timeUntilTrip, 
                                               float timeAtDestination,
                                               bool isDestinationNewHome )
    {
        // This is used often with vector migration. If LOG_DEBUG_F _isn't_ #defined away,
        // consider changing this to LOG_VALID_F.
        LOG_DEBUG_F( "Setting vector migration destination to node ID #%d\n", destination.data );
        migration_destination = destination;
        migration_type        = type;
    }

    const suids::suid& VectorCohortIndividual::GetMigrationDestination()
    {
        // This is used often with vector migration. If LOG_DEBUG_F _isn't_ #defined away,
        // consider changing this to LOG_VALID_F.
        LOG_DEBUG_F( "Got vector migration destination node ID #%d\n", migration_destination.data );
        return migration_destination;
    }

    VectorCohortIndividual::~VectorCohortIndividual()
    {
    }

    std::vector<VectorCohortIndividual*>* VectorCohortIndividual::_supply = nullptr;

    void VectorCohortIndividual::reclaim(IVectorCohortIndividual* ivci)
    {
        if (!_supply)
        {
            _supply = new std::vector<VectorCohortIndividual*>();
            _supply->reserve(65535);
        }
        VectorCohortIndividual* vci = static_cast<VectorCohortIndividual*>(ivci);
        vci->~VectorCohortIndividual();
        _supply->push_back(vci);
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
        VectorCohortAging::IncreaseAge( dt );
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
        
        if (GET_CONFIGURABLE(SimulationConfig)->vector_params->vector_aging)
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
        return *pSpecies;
    }

    REGISTER_SERIALIZABLE(VectorCohortIndividual);

    void VectorCohortIndividual::serialize(IArchive& ar, VectorCohortIndividual* obj)
    {
        VectorCohortAging::serialize(ar, obj);
        VectorCohortIndividual& cohort = *obj;
        //ar.labelElement("m_ID"                 ) & cohort.m_ID;
        ar.labelElement("state"                ) & (uint32_t&)cohort.state;
        ar.labelElement("additional_mortality" ) & cohort.additional_mortality;
        ar.labelElement("oviposition_timer"    ) & cohort.oviposition_timer;
        ar.labelElement("parity"               ) & cohort.parity;
        ar.labelElement("neweggs"              ) & cohort.neweggs;
        ar.labelElement("migration_destination") & cohort.migration_destination;
        ar.labelElement("migration_type"       ) & (uint32_t&)cohort.migration_type;

        // -----------------------------------------------------------------------------------------------------
        // --- When not using serialization, pSpecies usually points to VectorPopulationIndividual::species_ID.
        // --- We should probably get it to point at one of the vector_species_names versus having its own copy.
        // --- NOTE: We have pSpecies as a pointer to save RAM.
        // -----------------------------------------------------------------------------------------------------
        std::string tmp_species = *(cohort.pSpecies);
        ar.labelElement( "species" ) & tmp_species;
        if( ar.IsReader() )
        {
            bool found = false;
            for( auto& name : GET_CONFIGURABLE( SimulationConfig )->vector_params->vector_species_names )
            {
                if( name == tmp_species )
                {
                    cohort.pSpecies = &name;
                    found = true;
                    break;
                }
            }
            if( !found )
            {
                std::stringstream ss;
                ss << "Did not find species name=" << tmp_species;
                throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }
        }

        /* TODO - this 'has_strain' stuff is left over from when m_strain could be a nullptr. */
        bool has_strain = true;
        ar.labelElement("__has_strain__");
        ar & has_strain;
        // clorton TODO - perhaps the cohort should always have a non-null m_strain, just use a dummy StrainIdentity when there's no infection.
        if (has_strain)
        {
            ar.labelElement("m_strain");
            StrainIdentity::serialize(ar, cohort.m_strain);
        }
    }
}
