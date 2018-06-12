/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "VectorCohort.h"
#include "Exceptions.h"
#include "INodeContext.h"
#include "StrainIdentity.h"
#include "NodeVector.h"  // need this so we can cast from INodeContext to INodeVector
#include "SimulationConfig.h"
#include "VectorParameters.h"

SETUP_LOGGING( "VectorCohort" )

namespace Kernel
{
    std::string VectorCohortAbstract::_gambiae( "gambiae" );

    // QI stuff
    BEGIN_QUERY_INTERFACE_BODY(VectorCohortAbstract)
        HANDLE_INTERFACE(IVectorCohort)
        HANDLE_INTERFACE(IMigrate)
        HANDLE_ISUPPORTS_VIA(IVectorCohort)
    END_QUERY_INTERFACE_BODY(VectorCohortAbstract)

    VectorCohortAbstract::VectorCohortAbstract() 
        : vector_genetics( VectorMatingStructure() )
        , state( VectorStateEnum::STATE_ADULT )
        , progress(0.0)
        , population(DEFAULT_VECTOR_COHORT_SIZE)
        , age( 0.0 )
        , migration_type( MigrationType::NO_MIGRATION )
        , migration_destination( suids::nil_suid() )
        , pSpecies( &_gambiae )
    {
    }

    VectorCohortAbstract::VectorCohortAbstract( const VectorCohortAbstract& rThat )
        : vector_genetics( rThat.vector_genetics )
        , state( rThat.state )
        , progress( rThat.progress )
        , population( rThat.population )
        , age( rThat.age )
        , migration_type( rThat.migration_type )
        , migration_destination( rThat.migration_destination )
        , pSpecies( rThat.pSpecies )
    {
    }

    VectorCohortAbstract::VectorCohortAbstract( VectorStateEnum::Enum _state,
                                float _age,
                                float _progress,
                                uint32_t _population,
                                const VectorMatingStructure& _vector_genetics,
                                const std::string* vector_species_name )
        : vector_genetics(_vector_genetics)
        , state( _state )
        , progress(_progress)
        , population(_population)
        , age( _age )
        , migration_type( MigrationType::NO_MIGRATION )
        , migration_destination( suids::nil_suid() )
        , pSpecies( vector_species_name )
    {
    }

    VectorCohortAbstract::~VectorCohortAbstract()
    {
    }

    void VectorCohortAbstract::Initialize()
    {
    }

    static StrainIdentity static_strain;

    const IStrainIdentity& VectorCohortAbstract::GetStrainIdentity() const
    {
        // dummy strain identity for cohort model
        // derived VectorCohortIndividual will actually keep track of strains
        return static_strain;
    }

    void VectorCohortAbstract::ImmigrateTo( INodeContext* node )
    {
        // This is used often with vector migration. If LOG_DEBUG_F _isn't_ #defined away,
        // consider changing this to LOG_VALID_F.
        LOG_DEBUG_F( "Vector immigrating to node #%d\n", (node->GetSuid()).data );

        // static_cast is _much_ faster than QI and we're required to have a NodeVector here.
        INodeVector* pNV = static_cast<NodeVector*>(static_cast<Node*>(node));
        pNV->processImmigratingVector( this );
    }

    void VectorCohortAbstract::SetMigrating( suids::suid destination,
                                     MigrationType::Enum type,
                                     float timeUntilTrip,
                                     float timeAtDestination,
                                     bool isDestinationNewHome )
    {
        // This is used often with vector migration. If LOG_DEBUG_F _isn't_ #defined away,
        // consider changing this to LOG_VALID_F.
        LOG_DEBUG_F( "Setting vector migration destination to node ID #%d\n", destination.data );
        migration_destination = destination;
        migration_type = type;
    }


    const suids::suid& VectorCohortAbstract::GetMigrationDestination()
    {
        // This is used often with vector migration. If LOG_DEBUG_F _isn't_ #defined away,
        // consider changing this to LOG_VALID_F.
        LOG_DEBUG_F( "Got vector migration destination node ID #%d\n", migration_destination.data );
        return migration_destination;
    }

    MigrationType::Enum VectorCohortAbstract::GetMigrationType() const
    {
        return migration_type;
    }

    VectorStateEnum::Enum VectorCohortAbstract::GetState() const
    {
        return state;
    }

    void VectorCohortAbstract::SetState( VectorStateEnum::Enum newState )
    {
        state = newState;
        if( state == VectorStateEnum::STATE_INFECTED )
        {
            ClearProgress();
        }
    }

    const std::string & VectorCohortAbstract::GetSpecies()
    {
        return *pSpecies;
    }

    uint32_t VectorCohortAbstract::GetPopulation() const
    {
        return population;
    }

    void VectorCohortAbstract::SetPopulation(
        uint32_t new_pop
    )
    {
        population = new_pop;
    }

    float 
    VectorCohortAbstract::GetProgress() const
    {
        return progress;
    }

    void
    VectorCohortAbstract::ClearProgress()
    {
        progress = 0;
    }

    void
    VectorCohortAbstract::IncreaseProgress( float delta )
    {
        progress += delta;
        if( progress > 1.0 )
        {
            progress = 1.0;
        }
    }

    VectorMatingStructure& 
    VectorCohortAbstract::GetVectorGenetics()
    {
        return vector_genetics;
    }

    void
    VectorCohortAbstract::SetVectorGenetics( const VectorMatingStructure& new_value )
    {
        vector_genetics = new_value;
    }

    IMigrate* VectorCohortAbstract::GetIMigrate()
    {
        return static_cast<IMigrate*>(this);
    }

    float VectorCohortAbstract::GetAge() const
    {
        return age;
    }

    void VectorCohortAbstract::SetAge( float ageDays )
    {
        age = ageDays;
    }

    void VectorCohortAbstract::IncreaseAge( float dt )
    {
        age += dt;
    }

    void VectorCohortAbstract::serialize(IArchive& ar, VectorCohortAbstract* obj)
    {
        VectorCohortAbstract& cohort = *obj;
        ar.labelElement("state"                ) & (uint32_t&)cohort.state;
        ar.labelElement("progress"             ) & cohort.progress;
        ar.labelElement("population"           ) & cohort.population;
        ar.labelElement("age"                  ) & cohort.age;
        ar.labelElement("migration_destination") & cohort.migration_destination;
        ar.labelElement("migration_type"       ) & (uint32_t&)cohort.migration_type;

        ar.labelElement( "vector_genetics" );
        VectorMatingStructure::serialize( ar, cohort.vector_genetics );

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
                ss << "Error de-serializing VectorCohort: did not find vector cohort species name, '"
                   << tmp_species << "', in simulation configuration. Add species to configuration or "
                   << "remove vector cohorts from serialization file.\n";
                throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }
        }
    }

    // ------------------------------------------------------------------------
    // --- VectorCohort
    // ------------------------------------------------------------------------
    BEGIN_QUERY_INTERFACE_DERIVED( VectorCohort, VectorCohortAbstract )
    END_QUERY_INTERFACE_DERIVED( VectorCohort, VectorCohortAbstract )

    VectorCohort::VectorCohort()
        : VectorCohortAbstract()
        , neweggs()
    {
    }

    VectorCohort::VectorCohort( const VectorCohort& rThat )
        : VectorCohortAbstract( rThat )
        , neweggs( rThat.neweggs )
    {
    }

    VectorCohort::VectorCohort( VectorStateEnum::Enum _state,
                                float _age,
                                float _progress,
                                uint32_t _population,
                                const VectorMatingStructure& _vector_genetics,
                                const std::string* _vector_species_name )
        : VectorCohortAbstract( _state, _age, _progress, _population, _vector_genetics, _vector_species_name )
        , neweggs()
    {
    }

    VectorCohort::~VectorCohort()
    {
    }

    VectorCohort *VectorCohort::CreateCohort( VectorStateEnum::Enum _state,
                                              float _age,
                                              float _progress,
                                              uint32_t _population,
                                              const VectorMatingStructure& _vector_genetics,
                                              const std::string* _vector_species_name )
    {
        VectorCohort *newqueue = _new_ VectorCohort( _state, _age, _progress, _population, _vector_genetics, _vector_species_name );
        newqueue->Initialize();
        return newqueue;
    }

    void VectorCohort::AddNewEggs( uint32_t daysToGestate, uint32_t new_eggs )
    {
        while( neweggs.size() < daysToGestate )
        {
            neweggs.push_back( 0 );
        }
        neweggs[ daysToGestate - 1 ] += new_eggs;
    }

    uint32_t VectorCohort::GetGestatedEggs()
    {
        if( neweggs.size() == 0 ) return 0;

        uint32_t gestated_eggs = neweggs[ 0 ];

        // -----------------------------------------------------------------------------------
        // --- Shift eggs one day
        // --- NOTE: You cannot use memcpy() here because the memory overlaps.
        // --- I tried memmove().  It worked on Windows but I got different results on Linux.
        // -----------------------------------------------------------------------------------
        for( int i = 1; i < neweggs.size(); ++i )
        {
            neweggs[ i - 1 ] = neweggs[ i ];
        }
        neweggs.back() = 0; // clear last value

        return gestated_eggs;
    }

    void VectorCohort::AdjustEggsForDeath( uint32_t numDied )
    {
        if( (numDied == 0) || (neweggs.size() == 0) ) return;

        float percent_died = float( numDied ) / float( GetPopulation() );
        float percent_lived = 1.0 - percent_died;

        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // !!! Don't adjust the current eggs for death
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        for( int i = 1; i < neweggs.size(); ++i )
        {
            neweggs[ i ] = uint32_t( float( neweggs[ i ] ) * percent_lived );
        }
    }

    void VectorCohort::Merge( IVectorCohort* pCohortToAdd )
    {
        SetPopulation( GetPopulation() + pCohortToAdd->GetPopulation() );

        const std::vector<uint32_t>& r_vec_to_add = pCohortToAdd->GetNewEggs();
        for( int i = 0; i < r_vec_to_add.size(); ++i )
        {
            if( i < neweggs.size() )
            {
                neweggs[ i ] += r_vec_to_add[ i ];
            }
            else
            {
                neweggs.push_back( r_vec_to_add[ i ] );
            }
        }
    }

    IVectorCohort* VectorCohort::Split( uint32_t numLeaving )
    {
        VectorCohort* leaving = new VectorCohort( *this );

        float percent_leaving = float( numLeaving ) / float( GetPopulation() );

        for( int i = 0; i < leaving->neweggs.size(); ++i )
        {
            leaving->neweggs[ i ] = uint32_t( float( this->neweggs[ i ] ) * percent_leaving );
            this->neweggs[ i ] = this->neweggs[ i ] - leaving->neweggs[ i ];
        }

        uint32_t num_remaining = this->GetPopulation() - numLeaving;

        this->SetPopulation( num_remaining );
        leaving->SetPopulation( numLeaving );

        return leaving;
    }

    const std::vector<uint32_t>& VectorCohort::GetNewEggs() const
    {
        return neweggs;
    }

    REGISTER_SERIALIZABLE( VectorCohort );

    void VectorCohort::serialize( IArchive& ar, VectorCohort* obj )
    {
        VectorCohortAbstract::serialize( ar, obj );

        VectorCohort& cohort = *obj;
        ar.labelElement( "neweggs" ) & cohort.neweggs;
    }
}
