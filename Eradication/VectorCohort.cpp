
#include "stdafx.h"

#include <numeric>

#include "VectorCohort.h"
#include "Exceptions.h"
#include "INodeContext.h"
#include "StrainIdentity.h"
#include "NodeVector.h"  // need this so we can cast from INodeContext to INodeVector
#include "SimulationConfig.h"
#include "VectorParameters.h"
#include "RANDOM.h"
#include "VectorEnums.h"

SETUP_LOGGING( "VectorCohort" )

namespace Kernel
{
    // ---------------------------------------------------------
    // --- integer form of the maximum age of a vector in days
    // --- I selected 60 because that is about where the mortalityFromAge()
    // --- really starts to Asymptote.  One could go as low as 50,
    // --- but 60 seems better.
    // ---------------------------------------------------------
    float MAX_AGE = 60.0;
    uint32_t VectorCohortAbstract::I_MAX_AGE = uint32_t(MAX_AGE + 1); // plus one so arrays have slot for MAX_AGE

    uint32_t LARGE_DAYS_BETWEEN_FEEDS = 8;  // 8 is supposed to represent a large number of days between feeds

    // QI stuff
    BEGIN_QUERY_INTERFACE_BODY(VectorCohortAbstract)
        HANDLE_INTERFACE(IVectorCohort)
        HANDLE_INTERFACE(IMigrate)
        HANDLE_ISUPPORTS_VIA(IVectorCohort)
    END_QUERY_INTERFACE_BODY(VectorCohortAbstract)

    VectorCohortAbstract::VectorCohortAbstract()
        : m_ID(0)
        , species_index(-1)
        , genome_self()
        , genome_mate()
        , state( VectorStateEnum::STATE_ADULT )
        , progress(0.0)
        , population(DEFAULT_VECTOR_COHORT_SIZE)
        , age( 0.0 )
        , migration_type( MigrationType::NO_MIGRATION )
        , migration_destination( suids::nil_suid() )
        , microsporidia_infection_duration( 0.0 )
    {
    }

    VectorCohortAbstract::VectorCohortAbstract( const VectorCohortAbstract& rThat )
        : m_ID(0) // dont' copy the ID
        , species_index( rThat.species_index )
        , genome_self( rThat.genome_self )
        , genome_mate( rThat.genome_mate )
        , state( rThat.state )
        , progress( rThat.progress )
        , population( rThat.population )
        , age( rThat.age )
        , migration_type( rThat.migration_type )
        , migration_destination( rThat.migration_destination )
        , microsporidia_infection_duration( rThat.microsporidia_infection_duration )
    {
    }

    VectorCohortAbstract::VectorCohortAbstract( uint32_t vectorID,
                                                VectorStateEnum::Enum _state,
                                                float _age,
                                                float _progress,
                                                float _microsporidiaDuration,
                                                uint32_t _population,
                                                const VectorGenome& rGenome,
                                                int speciesIndex )
        : m_ID(vectorID)
        , species_index( speciesIndex )
        , genome_self( rGenome )
        , genome_mate()
        , state( _state )
        , progress( 0.0f ) // setting to zero and letting the setter set the value
        , population(_population)
        , age( 0.0f ) // setting to zero and letting the setter set the value
        , migration_type( MigrationType::NO_MIGRATION )
        , migration_destination( suids::nil_suid() )
        , microsporidia_infection_duration( _microsporidiaDuration )
    {
        // the setters will ensure the member variables are within the right ranges
        SetAge( _age );
        SetProgress( _progress );
    }

    VectorCohortAbstract::~VectorCohortAbstract()
    {
    }

    void VectorCohortAbstract::Initialize()
    {
    }

    void VectorCohortAbstract::ImmigrateTo( INodeContext* node )
    {
        LOG_VALID_F("Vector ID=%d state=%s species_index=%d population=%d immigrating to node #%d\n", m_ID, VectorStateEnum::pairs::get_keys()[state], species_index, population, (node->GetSuid()).data);

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
       // LOG_VALID_F( "Setting vector ID=%d state=%s species_index=%d population=%d migration destination to node ID #%d\n", m_ID, VectorStateEnum::pairs::get_keys()[state], species_index, population, destination.data );
        migration_destination = destination;
        migration_type = type;
    }


    const suids::suid& VectorCohortAbstract::GetMigrationDestination()
    {
       // LOG_VALID_F( "Got vector ID=%d state=%s species_index=%d population=%d migration destination node ID #%d\n", m_ID, VectorStateEnum::pairs::get_keys()[state], species_index, population, migration_destination.data );
        return migration_destination;
    }

    MigrationType::Enum VectorCohortAbstract::GetMigrationType() const
    {
        return migration_type;
    }

    int VectorCohortAbstract::GetSpeciesIndex() const
    {
        return species_index;
    }

    const VectorGenome& VectorCohortAbstract::GetGenome() const
    {
        return genome_self;
    }

    void VectorCohortAbstract::SetMateGenome( const VectorGenome& rGenomeMate )
    {
        genome_mate = rGenomeMate;
    }

    const VectorGenome& VectorCohortAbstract::GetMateGenome() const
    {
        return genome_mate;
    }

    bool VectorCohortAbstract::HasMated() const
    {
        return (genome_mate.GetGender() != genome_self.GetGender());
    }

    VectorStateEnum::Enum VectorCohortAbstract::GetState() const
    {
        return state;
    }

    void VectorCohortAbstract::SetState( VectorStateEnum::Enum newState )
    {
        state = newState;
    }

    const std::string & VectorCohortAbstract::GetSpecies()
    {
        VectorParameters* p_vp = GET_CONFIGURABLE( SimulationConfig )->vector_params;
        return p_vp->vector_species[ species_index ]->name;
    }

    VectorWolbachia::Enum VectorCohortAbstract::GetWolbachia() const
    {
        return genome_self.GetWolbachia();
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

    void VectorCohortAbstract::SetProgress( float newProgress )
    {
        progress = newProgress;
        if( progress > 1.0 )
        {
            progress = 1.0;
        }
    }

    void
    VectorCohortAbstract::ClearProgress()
    {
        SetProgress( 0.0 );
    }

    void
    VectorCohortAbstract::IncreaseProgress( float delta )
    {
        float tmp_progress = progress + delta;
        SetProgress( tmp_progress );
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

        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // !!! We restrict the max age so that in VectorCohortCollectionStdMapWithAging
        // !!! we can use a std::vector that is indexed by age.
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        if( age > MAX_AGE )
        {
            age = MAX_AGE;
        }
    }

    void VectorCohortAbstract::IncreaseAge( float dt )
    {
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // !!! Assuming that dt = 1 so that we can assume that the age is integer days.
        // !!! This is so we can do faster look ups into arrays.
        // !!! The max age is so that these arrays can have a max size.
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        release_assert( dt == 1.0 );

        float tmp_age = age + dt;
        SetAge( tmp_age );
    }

    void VectorCohortAbstract::Update( RANDOMBASE* pRNG,
                                       float dt,
                                       const VectorTraitModifiers& rTraitModifiers,
                                       float progressThisTimestep,
                                       bool hadMicrosporidiaPreviously )
    {
        if( hadMicrosporidiaPreviously && HasMicrosporidia() )
        {
            microsporidia_infection_duration += dt;
        }

        switch( GetState() )
        {
            case VectorStateEnum::STATE_LARVA:
                IncreaseAge( dt );
                // fall through because we want the rest to happen
            case VectorStateEnum::STATE_IMMATURE:
            case VectorStateEnum::STATE_INFECTED:
                IncreaseProgress( progressThisTimestep );
                if( (GetProgress() >= 1) && (GetPopulation() > 0) )
                {
                    // NOTE: We don't have IMMATURE going to ADLUT/MALE because that is being handled
                    // in VectorPopulation and can involve the creation of a new object.
                    if( GetState() == VectorStateEnum::STATE_LARVA )
                    {
                        SetState( VectorStateEnum::STATE_IMMATURE );
                        // cannot clear progress or age here because we want the stats on when it changes state
                    }
                    else if( GetState() == VectorStateEnum::STATE_INFECTED )
                    {
                        SetState( VectorStateEnum::STATE_INFECTIOUS );
                        ClearProgress();
                    }
                }
                break;
            case VectorStateEnum::STATE_EGG:
            case VectorStateEnum::STATE_ADULT:
            case VectorStateEnum::STATE_MALE:
            case VectorStateEnum::STATE_INFECTIOUS:
                break;
            default:
                throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__, "state", GetState(), nullptr );
        }
    }

    bool VectorCohortAbstract::HasWolbachia() const
    {
        return (GetWolbachia() != VectorWolbachia::VECTOR_WOLBACHIA_FREE);
    }

    bool VectorCohortAbstract::HasMicrosporidia() const
    {
        return GetGenome().HasMicrosporidia();
    }

    void VectorCohortAbstract::InfectWithMicrosporidia( int strainIndex )
    {
        return genome_self.SetMicrosporidiaStrain( strainIndex );
    }

    float VectorCohortAbstract::GetDurationOfMicrosporidia() const
    {
        return microsporidia_infection_duration;
    }

    void VectorCohortAbstract::serialize(IArchive& ar, VectorCohortAbstract* obj)
    {
        VectorCohortAbstract& cohort = *obj;
        ar.labelElement("m_ID"                            ) & cohort.m_ID;
        ar.labelElement("species_index"                   ) & cohort.species_index;
        ar.labelElement("genome_self"                     ) & cohort.genome_self;
        ar.labelElement("genome_mate"                     ) & cohort.genome_mate;
        ar.labelElement("state"                           ) & (uint32_t&)cohort.state;
        ar.labelElement("progress"                        ) & cohort.progress;
        ar.labelElement("population"                      ) & cohort.population;
        ar.labelElement("age"                             ) & cohort.age;
        ar.labelElement("migration_destination"           ) & cohort.migration_destination;
        ar.labelElement("migration_type"                  ) & (uint32_t&)cohort.migration_type;
        ar.labelElement("microsporidia_infection_duration") & cohort.microsporidia_infection_duration;
    }

    // ------------------------------------------------------------------------
    // --- VectorCohort
    // ------------------------------------------------------------------------
    BEGIN_QUERY_INTERFACE_DERIVED( VectorCohort, VectorCohortAbstract )
    END_QUERY_INTERFACE_DERIVED( VectorCohort, VectorCohortAbstract )

    VectorCohort::VectorCohort()
        : VectorCohortAbstract()
        , gestating_queue()
        , total_gestating(0)
        , habitat_type( VectorHabitatType::NONE )
        , habitat(nullptr)
    {
        gestating_queue.reserve( LARGE_DAYS_BETWEEN_FEEDS );
    }

    VectorCohort::VectorCohort( const VectorCohort& rThat )
        : VectorCohortAbstract( rThat )
        , gestating_queue( rThat.gestating_queue )
        , total_gestating( rThat.total_gestating )
        , habitat_type( rThat.habitat_type)
        , habitat( rThat.habitat )
    {
    }

    VectorCohort::VectorCohort( IVectorHabitat* _habitat,
                                uint32_t vectorID,
                                VectorStateEnum::Enum _state,
                                float _age,
                                float _progress,
                                float microsporidiaDuration,
                                uint32_t _population,
                                const VectorGenome& rGenome,
                                int speciesIndex )
        : VectorCohortAbstract( vectorID,
                                _state,
                                _age,
                                _progress,
                                microsporidiaDuration,
                                _population,
                                rGenome,
                                speciesIndex )
        , gestating_queue()
        , total_gestating( 0 )
        , habitat_type( _habitat ? _habitat->GetVectorHabitatType() : VectorHabitatType::NONE )
        , habitat(_habitat)
    {
        gestating_queue.reserve( LARGE_DAYS_BETWEEN_FEEDS );
    }

    VectorCohort::~VectorCohort()
    {
    }

    VectorCohort *VectorCohort::CreateCohort( uint32_t vectorID,
                                              VectorStateEnum::Enum _state,
                                              float _age,
                                              float _progress,
                                              float microsporidiaDuration,
                                              uint32_t _population,
                                              const VectorGenome& rGenome,
                                              int speciesIndex )
    {
        VectorCohort *newqueue = _new_ VectorCohort( nullptr,
                                                     vectorID,
                                                     _state,
                                                     _age,
                                                     _progress,
                                                     microsporidiaDuration,
                                                     _population,
                                                     rGenome,
                                                     speciesIndex );
        newqueue->Initialize();
        return newqueue;
    }

    VectorCohort *VectorCohort::CreateCohort( IVectorHabitat* _habitat,
                                              uint32_t vectorID,
                                              VectorStateEnum::Enum _state,
                                              float _progress,
                                              float microsporidiaDuration,
                                              uint32_t _population,
                                              const VectorGenome& rGenome,
                                              int speciesIndex )
    {
        VectorCohort *newqueue = _new_ VectorCohort( _habitat,
                                                     vectorID,
                                                     _state,
                                                     0.0f,
                                                     _progress,
                                                     microsporidiaDuration,
                                                     _population,
                                                     rGenome,
                                                     speciesIndex );
        newqueue->Initialize();
        return newqueue;
    }

    void VectorCohort::SetPopulation( uint32_t newPop )
    {
        //release_assert( newPop >= total_gestating );
        VectorCohortAbstract::SetPopulation( newPop );
        if( newPop == 0 )
        {
            total_gestating = 0;
            for( auto& r_val : gestating_queue )
            {
                r_val = 0;
            }
        }
    }

    uint32_t VectorCohort::GetNumLookingToFeed() const
    {
        return (population - total_gestating);
    }

    void VectorCohort::AddNewGestating( uint32_t daysToGestate, uint32_t newGestating )
    {
        while( gestating_queue.size() < daysToGestate )
        {
            gestating_queue.push_back( 0 );
        }
        total_gestating += newGestating;
        release_assert( total_gestating <= population );
        gestating_queue[ daysToGestate - 1 ] += newGestating;
    }

    uint32_t VectorCohort::GetNumGestating() const
    {
        //uint32_t num = std::accumulate( gestating_queue.begin(), gestating_queue.end(), 0 );
        //release_assert( num == total_gestating );
        release_assert( total_gestating <= population );
        return total_gestating;
    }

    uint32_t VectorCohort::RemoveNumDoneGestating()
    {
        if( gestating_queue.size() == 0 ) return 0;

        uint32_t num_done_gestating = gestating_queue[ 0 ];

        // -----------------------------------------------------------------------------------
        // --- Shift those gestating one day
        // --- NOTE: You cannot use memcpy() here because the memory overlaps.
        // --- I tried memmove().  It worked on Windows but I got different results on Linux.
        // -----------------------------------------------------------------------------------
        for( int i = 1; i < gestating_queue.size(); ++i )
        {
            gestating_queue[ i - 1 ] = gestating_queue[ i ];
        }
        gestating_queue.back() = 0; // clear last value

        release_assert( total_gestating >= num_done_gestating );
        total_gestating -= num_done_gestating;

        return num_done_gestating;
    }

    // --------------------------------------------------------------------------------------
    // --- NOTE: Once upon a time, we had the cohorts keeping track of eggs but we switched
    // --- to number of vectors because when a vector dies we lose the entire batch of eggs,
    // --- not just a portion of it.
    // --------------------------------------------------------------------------------------
    uint32_t VectorCohort::AdjustGestatingForDeath( RANDOMBASE* pRNG, float percentDied, bool killGestatingOnly )
    {
        if( percentDied == 0.0f )
        {
            return 0;
        }

        release_assert( population >= total_gestating );

        uint32_t num_not_gestating = population - total_gestating;

        uint32_t num_died = 0;
        if( total_gestating > 0 )
        {
            if( percentDied == 1.0f )
            {
                num_died = total_gestating;
                population -= total_gestating;
                std::fill( gestating_queue.begin(), gestating_queue.end(), 0 );
                total_gestating = 0;
            }
            else
            {
                for( int index = 0; index < gestating_queue.size(); ++index )
                {
                    if( gestating_queue[ index ] > 0 )
                    {
                        uint32_t sub_died = pRNG->binomial_approx( gestating_queue[ index ], percentDied );

                        release_assert( total_gestating >= sub_died );
                        release_assert( gestating_queue[ index ] >= sub_died );

                        gestating_queue[ index ] -= sub_died;
                        total_gestating -= sub_died;
                        population -= sub_died;
                        num_died += sub_died;
                    }
                }
            }
        }

        if( !killGestatingOnly )
        {
            uint32_t sub_died = 0;
            if( percentDied == 1.0f )
            {
                sub_died = num_not_gestating;
            }
            else
            {
                sub_died = pRNG->binomial_approx( num_not_gestating, percentDied );
            }
            release_assert( population >= sub_died );

            population -= sub_died;
            num_died += sub_died;
        }
        release_assert( population >= total_gestating );

        return num_died;
    }

    void VectorCohort::Merge( IVectorCohort* pCohortToAdd )
    {
        SetPopulation( GetPopulation() + pCohortToAdd->GetPopulation() );

        const std::vector<uint32_t>& r_vec_to_add = pCohortToAdd->GetGestatingQueue();
        for( int i = 0; i < r_vec_to_add.size(); ++i )
        {
            if( i < gestating_queue.size() )
            {
                gestating_queue[ i ] += r_vec_to_add[ i ];
            }
            else
            {
                gestating_queue.push_back( r_vec_to_add[ i ] );
            }
        }
        total_gestating += pCohortToAdd->GetNumGestating();
    }

    IVectorCohort* VectorCohort::SplitPercent( RANDOMBASE* pRNG, uint32_t newVectorID, float percentLeaving )
    {
        static std::vector<uint32_t> orig_gestating_queue( LARGE_DAYS_BETWEEN_FEEDS, 0 );
        orig_gestating_queue = this->gestating_queue;

        uint32_t orig_pop = GetPopulation();

        uint32_t num_leaving = AdjustGestatingForDeath( pRNG, percentLeaving, false );
        if( num_leaving == 0 )
        {
            return nullptr;
        }

        VectorCohort* leaving = new VectorCohort( *this );
        leaving->m_ID = newVectorID;

        leaving->total_gestating = 0;
        for( int i = 0; i < leaving->gestating_queue.size(); ++i )
        {
            release_assert( orig_gestating_queue[ i ] >= this->gestating_queue[ i ] );

            leaving->gestating_queue[ i ] = orig_gestating_queue[ i ] - this->gestating_queue[ i ];
            leaving->total_gestating += leaving->gestating_queue[ i ];
        }

        leaving->SetPopulation( num_leaving );
        release_assert( leaving->GetPopulation() >= leaving->total_gestating );
        release_assert(    this->GetPopulation() >=    this->total_gestating );
        release_assert( orig_pop == (this->GetPopulation() + leaving->GetPopulation()) );

        return leaving;
    }

    IVectorCohort* VectorCohort::SplitNumber( RANDOMBASE* pRNG, uint32_t newVectorID, uint32_t numLeaving )
    {
        VectorCohort* leaving = nullptr;
        if( (population > numLeaving) && (total_gestating > 0) )
        {
            // -------------------------------------------------------------------------
            // --- The following finds the portion of the cohort in each gestating bin.
            // --- Also create a list of the number of vectors at each gestating period.
            // -------------------------------------------------------------------------
            std::vector<float> probabilities;
            std::vector<int64_t> max_queue;
            for( int i = 0; i < gestating_queue.size(); ++i )
            {
                float prob = float(gestating_queue[ i ]) / float(population);
                probabilities.push_back( prob );
                max_queue.push_back( gestating_queue[ i ] );
            }

            release_assert( population >= total_gestating );
            uint32_t num_not_gestating = population - total_gestating;
            float prob = float(num_not_gestating) / float(population);
            probabilities.push_back( prob );
            max_queue.push_back( num_not_gestating );

            // ------------------------------------------------------------------------
            // --- The following is a multinomial_approx() algorithm with bounds.
            // --- Since we are trying to get an exact number, we must get these from
            // --- the appropriate gestating bins.  We can't take more than there is
            // --- in any one gestating bin.  Hence, the algorithm applies a min/max
            // --- on what can be taking from a given bin.  This ensures that after the
            // --- split we have the same number of gestating vectors as before.
            // ------------------------------------------------------------------------
            std::vector<int64_t> num_leaving_per_gestating( gestating_queue.size()+1, 0 );
            int64_t total_subsets = 0;
            double total_fraction = 0.0;
            int64_t possible_remaining = population;

            for( int i = 0; i < probabilities.size(); i++ )
            {
                double expected_fraction_of_total = probabilities[ i ];

                int64_t subset = 0;
                if( total_fraction < 1.0 )
                {
                    int64_t total_remaining = numLeaving - total_subsets;
                    double expected_fraction_of_remaining = expected_fraction_of_total / (1.0 - total_fraction);
                    subset = int64_t( pRNG->binomial_approx( total_remaining, expected_fraction_of_remaining ) );
                    if( subset > max_queue[ i ] )
                    {
                        subset = max_queue[ i ];
                    }
                    else
                    {
                        // find the minimum that must be leaving for this gestating period
                        int64_t possible_after_i = possible_remaining - max_queue[ i ];
                        int64_t min = total_remaining - (possible_remaining - max_queue[ i ]);
                        min = (min < 0) ? 0 : min;
                        if( subset < min )
                        {
                            subset = min;
                        }
                    }
                    possible_remaining -= max_queue[ i ];
                }
                num_leaving_per_gestating[ i ] = subset;

                total_fraction += expected_fraction_of_total;
                total_subsets += subset;
            }

            // -----------------------------------------------------------------------------
            // --- Create a new cohort and update the gestating bins using the results from
            // --- our multinomial_approx().
            // -----------------------------------------------------------------------------
            leaving = new VectorCohort( *this );
            leaving->m_ID = newVectorID;
            leaving->total_gestating = 0;
            leaving->population = 0;

            for( int i = 0; i < this->gestating_queue.size(); ++i )
            {
                if( this->gestating_queue[ i ] > 0 )
                {
                    this->gestating_queue[ i ] -= num_leaving_per_gestating[ i ];
                    leaving->gestating_queue[ i ] = num_leaving_per_gestating[ i ];

                    this->total_gestating -= num_leaving_per_gestating[ i ];
                    leaving->total_gestating += num_leaving_per_gestating[ i ];

                    this->population -= num_leaving_per_gestating[ i ];
                    leaving->population += num_leaving_per_gestating[ i ];
                }
            }
            this->population -= num_leaving_per_gestating.back();
            leaving->population += num_leaving_per_gestating.back();

            release_assert( this->population >= this->total_gestating );
            release_assert( leaving->population >= leaving->total_gestating );
        }
        else
        {
            uint32_t orig_pop = GetPopulation();

            leaving = new VectorCohort( *this );
            leaving->m_ID = newVectorID;

            uint32_t num_remaining = this->GetPopulation() - numLeaving;
            this->SetPopulation( num_remaining );
            leaving->SetPopulation( numLeaving );
            release_assert( orig_pop == (this->GetPopulation() + leaving->GetPopulation()) );
        }

        return leaving;
    }

    void VectorCohort::ReportOnGestatingQueue( std::vector<uint32_t>& rNumGestatingQueue ) const
    {
        for( int i = 0; i < gestating_queue.size(); ++i )
        {
            if( i < rNumGestatingQueue.size() )
            {
                rNumGestatingQueue[ i ] += gestating_queue[ i ];
            }
            else
            {
                rNumGestatingQueue.push_back( gestating_queue[ i ] );
            }
        }
    }

    const std::vector<uint32_t>& VectorCohort::GetGestatingQueue() const
    {
        return gestating_queue;
    }

    VectorHabitatType::Enum VectorCohort::GetHabitatType()
    {
        return habitat_type;
    }

    IVectorHabitat* VectorCohort::GetHabitat()
    {
        return habitat;
    }

    void VectorCohort::SetHabitat( IVectorHabitat* new_habitat )
    {
        // ------------------------------------------------------------------------
        // --- A vector gets one habitat and it cannot change.  This method is used when
        // --- deserializing a vector.  It is part of the effort to reconnect the pointers
        // --- so that there is a base set of habitat objects with the vector objects having
        // --- pointers to them.
        // ------------------------------------------------------------------------
        release_assert( habitat == nullptr );
        habitat = new_habitat;
    }

    REGISTER_SERIALIZABLE( VectorCohort );

    void VectorCohort::serialize( IArchive& ar, VectorCohort* obj )
    {
        VectorCohortAbstract::serialize( ar, obj );

        VectorCohort& cohort = *obj;
        ar.labelElement( "gestating_queue" ) & cohort.gestating_queue;
        ar.labelElement( "total_gestating" ) & cohort.total_gestating;

        // ------------------------------------------------------------------------
        // --- We have a separate habitat_type so that we only serialize the type.
        // --- When we deserialize, we use this type to get the pointer to the
        // --- actual habitat during VectorPopulation::SetContextTo()
        // ------------------------------------------------------------------------
        ar.labelElement("habitat_type") & (uint32_t&)cohort.habitat_type;
    }


    // ------------------------------------------------------------------------
    // --- VectorCohortMale
    // ------------------------------------------------------------------------
    BEGIN_QUERY_INTERFACE_DERIVED(VectorCohortMale, VectorCohortAbstract)
    END_QUERY_INTERFACE_DERIVED(VectorCohortMale, VectorCohortAbstract)

    VectorCohortMale::VectorCohortMale()
        : VectorCohortAbstract()
        , unmated_count(0)
        , unmated_count_cdf(0)
    {
        unmated_count = population;
    }

    VectorCohortMale::VectorCohortMale( const VectorCohortMale& rThat )
        : VectorCohortAbstract(rThat)
        , unmated_count(0)
        , unmated_count_cdf(0)
    {
    }

    VectorCohortMale::VectorCohortMale( uint32_t vectorID,
                                        float _age,
                                        float _progress,
                                        float microsporidiaDuration,
                                        uint32_t _population,
                                        const VectorGenome& rGenome,
                                        int speciesIndex )
        : VectorCohortAbstract( vectorID,
                                VectorStateEnum::STATE_MALE,
                                _age,
                                _progress,
                                microsporidiaDuration,
                                _population,
                                rGenome,
                                speciesIndex )
        , unmated_count(0)
        , unmated_count_cdf(0)
    {
        unmated_count = _population;
    }

    VectorCohortMale::~VectorCohortMale()
    {
    }

    VectorCohortMale* VectorCohortMale::CreateCohort( uint32_t vectorID,
                                                      float _age,
                                                      float _progress,
                                                      float microsporidiaDuration,
                                                      uint32_t _population,
                                                      const VectorGenome& rGenome,
                                                      int speciesIndex )
    {
        VectorCohortMale* newqueue = _new_ VectorCohortMale( vectorID,
                                                             _age,
                                                             _progress,
                                                             microsporidiaDuration,
                                                             _population,
                                                             rGenome,
                                                             speciesIndex );
        newqueue->Initialize(); 
        return newqueue;
    }

    uint32_t VectorCohortMale::GetUnmatedCount() const
    {
        return unmated_count;
    }

    void VectorCohortMale::SetUnmatedCount( uint32_t new_unmated_count )
    {
        unmated_count = new_unmated_count;
    }

    uint32_t VectorCohortMale::GetUnmatedCountCDF() const
    {
        return unmated_count_cdf;
    }

    void VectorCohortMale::SetUnmatedCountCDF( uint32_t new_unmated_count_cdf )
    {
        unmated_count_cdf = new_unmated_count_cdf;
    }


    void VectorCohortMale::Merge( IVectorCohort* pCohortToAdd )
    {
        SetPopulation( GetPopulation() + pCohortToAdd->GetPopulation() );
        SetUnmatedCount( GetUnmatedCount() + ( static_cast<VectorCohortMale*>( static_cast<VectorCohortAbstract*>( pCohortToAdd ) ) )->GetUnmatedCount() );
        // keeping cdf as-is so can be user for new microsporidia-related merging where the pCohortToAdd is all mated
    }

    VectorCohortMale* VectorCohortMale::SplitPercent( RANDOMBASE* pRNG, uint32_t newVectorID, float percentLeaving )
    {
        uint32_t    orig_pop = GetPopulation();
        uint32_t num_leaving = pRNG->binomial_approx( orig_pop, percentLeaving );
        return SplitHelper( pRNG, newVectorID, num_leaving, percentLeaving );
    }

    VectorCohortMale* VectorCohortMale::SplitNumber( RANDOMBASE* pRNG, uint32_t newVectorID, uint32_t numLeaving )
    {
        return SplitHelper( pRNG, newVectorID, numLeaving, -1 );
    }

    VectorCohortMale* VectorCohortMale::SplitHelper( RANDOMBASE* pRNG, uint32_t newVectorID, uint32_t numLeaving, float percentLeaving )
    {
        if ( numLeaving == 0 )
        {
            return nullptr;
        }
        uint32_t           orig_pop = GetPopulation();
        uint32_t orig_unmated_count = GetUnmatedCount();
        uint32_t    unmated_leaving = numLeaving; // case: unmated == total population
        if ( orig_pop > orig_unmated_count ) // case some are mated in the population
        {
            if ( percentLeaving < 0 ) // means we only know numLeaving, calculate percentLeaving ourselves
            {
                percentLeaving = static_cast<float> ( numLeaving ) / orig_pop;
            }
            unmated_leaving = static_cast<uint32_t>( pRNG->binomial_approx( orig_unmated_count, percentLeaving ) );
        }
        if ( unmated_leaving > numLeaving )  // because we're drawing two binomials, sometimes it's not quite a subset
        {
            unmated_leaving = numLeaving;
        }
        uint32_t unmated_staying = orig_unmated_count - unmated_leaving;
        uint32_t     pop_staying = orig_pop - numLeaving;
        if ( unmated_staying > pop_staying ) // not enough of the migrating males were unmated
        {
            uint32_t these_unmated_should_leave = unmated_staying - pop_staying;
            unmated_staying -= these_unmated_should_leave;
            unmated_leaving += these_unmated_should_leave;
        }

        // creating leaving cohort
        VectorCohortMale* leaving = new VectorCohortMale(*this);
        leaving->m_ID = newVectorID;
        leaving->SetPopulation( numLeaving );
        leaving->SetUnmatedCount( unmated_leaving );
        // unmated_count_cdf is initialized to 0 and will be updated by BuildMaleMatingCDF at the beginning of next timestep

        // Updating staying cohort
        this->SetPopulation( pop_staying );
        this->SetUnmatedCount( unmated_staying );
        // unmated_count_cdf is untouched, because if this split is for VectorPopulationIndividual microsporidia infection, this cohort is still in play

        release_assert( orig_pop                 == ( this->GetPopulation() + leaving->GetPopulation() ) );
        release_assert( orig_unmated_count       == ( this->GetUnmatedCount() + leaving->GetUnmatedCount() ) );
        release_assert( leaving->GetPopulation() >= leaving->GetUnmatedCount() );
        release_assert( this->GetPopulation()    >= this->GetUnmatedCount() );

        return leaving;
    }

    REGISTER_SERIALIZABLE( VectorCohortMale );

    void VectorCohortMale::serialize( IArchive& ar, VectorCohortMale* obj )
    {
        VectorCohortAbstract::serialize( ar, obj );
        VectorCohortMale& cohort = *obj;
        // -------------------------------------------------------------------
        // --- In the serialization the '&' between labelElement() and the variable is not for reference.
        // --- We are using operator overloading and we are 'anding' the label and the variable together 
        // --- to basically create JSON.
        // -------------------------------------------------------------------
        ar.labelElement( "unmated_count" )    & cohort.unmated_count; 
        ar.labelElement( "unmated_count_cdf" )& cohort.unmated_count_cdf;

    }
}
