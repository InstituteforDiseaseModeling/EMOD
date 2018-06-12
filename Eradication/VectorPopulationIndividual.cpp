/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "SimulationConfig.h"
#include "VectorPopulationIndividual.h"
#include "VectorCohortIndividual.h"
#include "Vector.h"
#include "IContagionPopulation.h"
#include "StrainIdentity.h"
#include "NodeVector.h"
#include "Exceptions.h"
#include "Log.h"
#include "Debug.h"
#include "IMigrationInfoVector.h"
#include "VectorParameters.h"
#include "VectorSpeciesParameters.h"

#ifdef randgen
#undef randgen
#endif
#include "RANDOM.h"
#define randgen (m_context->GetRng())

SETUP_LOGGING( "VectorPopulationIndividual" )

// VectorPopulationIndividual handles treatment of Vector population as an ensemble
// of individual vectors, although it is possible to run the code with each vector
// entry representing multiple real world vectors

// All vectors are in AdultQueues, the others are not used

namespace Kernel
{
    // QI stuff
    BEGIN_QUERY_INTERFACE_DERIVED(VectorPopulationIndividual, VectorPopulation)
    END_QUERY_INTERFACE_DERIVED(VectorPopulationIndividual, VectorPopulation)

    VectorPopulationIndividual::VectorPopulationIndividual(uint32_t mosquito_weight) 
    : VectorPopulation()
    , m_mosquito_weight(mosquito_weight)
    , m_average_oviposition_killing(0.0f)
    , current_vci(nullptr)
    { 
    }

    void VectorPopulationIndividual::InitializeVectorQueues( uint32_t adults, uint32_t _infectious )
    {
        adult      = adults;
        infectious = _infectious;

        if (adult > 0)
        {
            uint32_t adjusted_population = adults / m_mosquito_weight;
            for (uint32_t i = 0; i < adjusted_population; i++)
            {
                VectorCohortIndividual* pvci = VectorCohortIndividual::CreateCohort( VectorStateEnum::STATE_ADULT,
                                                                                     0,
                                                                                     0,
                                                                                     m_mosquito_weight,
                                                                                     VectorMatingStructure( VectorGender::VECTOR_FEMALE ),
                                                                                     &species_ID );

                RandomlySetOvipositionTimer( pvci );

                AdultQueues.push_back( pvci );
            }

            // and a population of males as well
            // progress is 1 since males should be at 1 from progressing from Immature to Male
            MaleQueues.push_back( VectorCohort::CreateCohort( VectorStateEnum::STATE_MALE,
                                                              0,
                                                              1,
                                                              adult,
                                                              VectorMatingStructure( VectorGender::VECTOR_MALE ),
                                                              &species_ID ) );
            males = adult;
        }

        if (infectious > 0)
        {
            uint32_t adjusted_population = _infectious / m_mosquito_weight;
            for (uint32_t i = 0; i < adjusted_population; i++)
            { 
                // infectious initialized at age 20
                AdultQueues.push_back( VectorCohortIndividual::CreateCohort( VectorStateEnum::STATE_INFECTIOUS,
                                                                            20,
                                                                            0,
                                                                            m_mosquito_weight,
                                                                            VectorMatingStructure(VectorGender::VECTOR_FEMALE),
                                                                            &species_ID ) );
            }
        }
    }

    VectorPopulationIndividual *VectorPopulationIndividual::CreatePopulation( INodeContext *context,
                                                                              const std::string& species_name,
                                                                              uint32_t adult,
                                                                              uint32_t infectious,
                                                                              uint32_t mosquito_weight )
    {
        VectorPopulationIndividual *newpopulation = _new_ VectorPopulationIndividual(mosquito_weight);
        newpopulation->Initialize(context, species_name, adult, infectious);

        return newpopulation;
    }

    VectorPopulationIndividual::~VectorPopulationIndividual()
    {
    }

    void VectorPopulationIndividual::Update_Infectious_Queue( float dt )
    {
        // just reset counter, all entries are in Adult_Queue for Vector_pop_individual
        infectious = 0;
    }

    void VectorPopulationIndividual::Update_Infected_Queue( float dt )
    {
        // no queue entries, since all entries are in the Adult_Queue
        // only need to reset counter
        infected = 0;
    }

    void VectorPopulationIndividual::Update_Adult_Queue( float dt )
    {
        adult = 0; // other counters were reset in other functions

        // Get habitat-weighted average oviposition-trap killing fraction to be used in ProcessFeedingCycle
        // TODO: it might be more consistent to put this in with the other lifecycle probabilities somehow
        m_average_oviposition_killing = 0;
        float total_larval_capacity = 0;
        for (auto habitat : (*m_larval_habitats))
        {
            float capacity = habitat->GetCurrentLarvalCapacity();
            m_average_oviposition_killing  += capacity * habitat->GetOvipositionTrapKilling();
            total_larval_capacity += capacity;
        }
        m_average_oviposition_killing /= total_larval_capacity;

        // Use the verbose "foreach" construct here because empty queues (i.e. dead individuals) will be removed
        for (size_t iCohort = 0; iCohort < AdultQueues.size(); /* increment in loop */)
        {
            // static cast is _much_ faster than QI and we _must_ have an IVectorCohortIndividual here.
            IVectorCohort* cohort = AdultQueues[iCohort];
            IVectorCohortIndividual* tempentry2 = current_vci = static_cast<IVectorCohortIndividual*>(static_cast<VectorCohortIndividual*>(static_cast<VectorCohortAbstract*>(cohort)));

            // Increment age of individual mosquitoes
            UpdateAge( cohort, dt );

            ProcessFeedingCycle( dt, cohort ) ;

            // Progress with sporogony in infected mosquitoes
            if( cohort->GetState() == VectorStateEnum::STATE_INFECTED )
            {
                cohort->IncreaseProgress( infected_progress_this_timestep );

                if ( (cohort->GetProgress() >= 1) && (cohort->GetPopulation() > 0) )
                {
                    // change state to INFECTIOUS
                    cohort->SetState( VectorStateEnum::STATE_INFECTIOUS );

                    // update INFECTIOUS counters
                    queueIncrementTotalPopulation( cohort );
                    ++iCohort;
                    continue;
                }
            }

            // Remove empty cohorts (i.e. dead mosquitoes)
            if ( cohort->GetPopulation() <= 0 )
            {
                AdultQueues[iCohort] = AdultQueues.back();
                AdultQueues.pop_back();
                VectorCohortIndividual::reclaim(tempentry2);

                // !! Don't increment iCohort. !!
            }
            else
            {
                // Increment counters
                queueIncrementTotalPopulation( cohort );
                ++iCohort;
            }
        }

        // Acquire infections with strain tracking for exposed queues

        LOG_DEBUG("Exposure to contagion: human to vector.\n");
        m_transmissionGroups->ExposeToContagion((IInfectable*)this, &NodeVector::human_to_vector_all, dt);
        IndoorExposedQueues.clear();
        OutdoorExposedQueues.clear();
    }

    void VectorPopulationIndividual::AdjustForFeedingRate( float dt, float p_local_mortality, VectorPopulation::FeedingProbabilities& rFeedProbs )
    {
        rFeedProbs.die_without_attempting_to_feed += p_local_mortality;
        rFeedProbs.die_before_human_feeding       += p_local_mortality;
    }

    void VectorPopulationIndividual::AdjustForCumulativeProbability( VectorPopulation::FeedingProbabilities& rFeedProbs )
    {
        // not cumulative
        //rFeedProbs.die_without_attempting_to_feed = rFeedProbs.die_without_attempting_to_feed;
        //rFeedProbs.die_before_human_feeding       = rFeedProbs.die_before_human_feeding;

        rFeedProbs.successful_feed_animal          += rFeedProbs.die_before_human_feeding;
        rFeedProbs.successful_feed_artifical_diet  += rFeedProbs.successful_feed_animal;
        rFeedProbs.successful_feed_attempt_indoor  += rFeedProbs.successful_feed_artifical_diet;
        rFeedProbs.successful_feed_attempt_outdoor += rFeedProbs.successful_feed_attempt_indoor;

        // start with die_indoor
        //rFeedProbs.die_indoor                            = rFeedProbs.die_indoor;
        rFeedProbs.successful_feed_artifical_diet_indoor += rFeedProbs.die_indoor;
        rFeedProbs.successful_feed_human_indoor          += rFeedProbs.successful_feed_artifical_diet_indoor;

        // start with die_outdoor
        //rFeedProbs.die_outdoor                   = rFeedProbs.die_outdoor;
        rFeedProbs.successful_feed_human_outdoor += rFeedProbs.die_outdoor;
    }

    void VectorPopulationIndividual::VectorToHumanDeposit( const IStrainIdentity& strain,
                                                           uint32_t attemptFeed,
                                                           const TransmissionGroupMembership_t* pTransmissionVectorToHuman )
    {
        m_transmissionGroups->DepositContagion( strain, float( attemptFeed ) * species()->transmissionmod, pTransmissionVectorToHuman );
    }

    bool VectorPopulationIndividual::DidDie( IVectorCohort* cohort,
                                             float probDies,
                                             float outcome, 
                                             uint32_t& rCounter )
    {
        bool vector_dies = (outcome <= probDies);
        if( vector_dies )
        {
            if( m_VectorMortality )
            {
                //mosquito dies
                cohort->SetPopulation( 0 );
                ++rCounter;
            }
        }
        return vector_dies;
    }

    bool VectorPopulationIndividual::DidFeed( IVectorCohort* cohort,
                                              float probFeed,
                                              float outcome,
                                              uint32_t& rCounter )
    {
        bool fed = (outcome <= probFeed);
        if( fed )
        {
            rCounter += cohort->GetPopulation();
        }
        return fed;
    }

    bool VectorPopulationIndividual::DiedLayingEggs( IVectorCohort* cohort )
    {
        uint32_t num_eggs = cohort->GetGestatedEggs();
        if( num_eggs > 0 )
        {
            if( m_average_oviposition_killing > 0 ) // avoid drawing random number if zero
            {
                if( DidDie( cohort, m_average_oviposition_killing, randgen->e(), dead_mosquitoes_before ) ) return true;
            }

            AddEggsToLayingQueue( cohort, num_eggs );
            current_vci->IncrementParity(); // increment number of times vector has laid eggs

                                           // Now if sugar feeding exists every day or after each feed, and mortality is associated, then check for killing
            if( ( (params()->vector_params->vector_sugar_feeding == VectorSugarFeeding::VECTOR_SUGAR_FEEDING_EVERY_FEED) ||
                  (params()->vector_params->vector_sugar_feeding == VectorSugarFeeding::VECTOR_SUGAR_FEEDING_EVERY_DAY ) )
                && (probs()->sugarTrapKilling > 0) )  // avoid drawing random number if zero
            {
                if( DidDie( cohort, probs()->sugarTrapKilling, randgen->e(), dead_mosquitoes_before ) ) return true;
            }
        }
        return false;
    }

    uint32_t VectorPopulationIndividual::ProcessFeedingCycle( float dt, IVectorCohort* cohort )
    {
        if( cohort->GetPopulation() <= 0 )
            return 0;

        IVectorCohortIndividual *tempentry2 = current_vci;

        FeedingProbabilities feed_probs = CalculateFeedingProbabilities( dt, cohort );

        // Uniform random number between 0 and 1 for feeding-cycle outcome calculations
        float outcome = randgen->e();

        // Determine whether it will feed? (i.e. die without attempting to feed)
        tempentry2->SetOvipositionTimer( tempentry2->GetOvipositionTimer() - dt );
        if(tempentry2->GetOvipositionTimer() > 0.0f)
        {
            //not feeding so just experiences mortality
            float cumulative_probability = feed_probs.die_without_attempting_to_feed;

            // possibly correct for sugar feeding
            if( (params()->vector_params->vector_sugar_feeding == VectorSugarFeeding::VECTOR_SUGAR_FEEDING_EVERY_DAY) &&
                (probs()->sugarTrapKilling > 0) )
            {
                cumulative_probability += (1.0 - cumulative_probability) * probs()->sugarTrapKilling;  // add in sugarTrap to kill rate
            }

            DidDie( cohort, cumulative_probability, outcome, dead_mosquitoes_before );

            return 0;
        }

        // Lays eggs and calculate oviposition killing before feeding
        if( DiedLayingEggs( cohort ) ) return 0;

        // Use initial random draw to assign individual mosquito feeding outcomes among the following:
        //     (1) diebeforeattempttohumanfeed
        //     (2) successfulfeed_animal
        //     (3) successfulfeed_AD
        //     (4) indoorattempttohumanfeed
        //     (5) outdoorattempttohumanfeed

        // Counters for feeding-cycle results
        uint32_t successful_animal_feed = 0;
        uint32_t successful_AD_feed     = 0;
        uint32_t successful_human_feed  = 0;

        // (1) die before human feeding?
        if( DidDie( cohort, feed_probs.die_before_human_feeding, outcome, dead_mosquitoes_before ) ) return 0;

        do { // at least one feeding attempt

            // (2) feed on animal?
            if( DidFeed( cohort, feed_probs.successful_feed_animal, outcome, successful_animal_feed ) ) continue;

            // (3) feed on artificial diet?
            if( DidFeed( cohort, feed_probs.successful_feed_artifical_diet, outcome, successful_AD_feed ) ) continue;

            // (4) attempt indoor feed?
            uint32_t indoor_bites = 0;
            if( DidFeed( cohort, feed_probs.successful_feed_attempt_indoor, outcome, indoor_bites ) )
            {
                // update human biting rate as well
                indoorbites += float( indoor_bites );

                // for the infectious cohort, need to update infectious bites
                indoorinfectiousbites += VectorToHumanTransmission( INDOOR_STR, &NodeVector::vector_to_human_indoor, cohort, cohort->GetPopulation() );

                // Reset cumulative probability for another random draw
                // to assign outdoor-feeding outcomes among the following:
                outcome = randgen->e();

                // (a) die in attempt to indoor feed?
                if( DidDie( cohort, feed_probs.die_indoor, outcome, dead_mosquitoes_indoor ) ) return 0;

                // (b) artificial diet?
                if( DidFeed( cohort, feed_probs.successful_feed_artifical_diet_indoor, outcome, successful_AD_feed ) ) continue;

                // (c) successful human feed?
                if( DidFeed( cohort, feed_probs.successful_feed_human_indoor, outcome, successful_human_feed ) )
                { 
                    if( (cohort->GetState() == VectorStateEnum::STATE_ADULT) && (probs()->indoor_successfulfeed_human > 0) )
                    {
                        // push back to exposed cohort (to be checked for new infection)
                        IndoorExposedQueues.push_back(cohort);
                    }
                }
                // else no host found
                continue;
            }

            // (5) attempt outdoor feed?
            uint32_t outdoor_bites = 0;
            if( DidFeed( cohort, feed_probs.successful_feed_attempt_outdoor, outcome, outdoor_bites ) )
            { 
                // update human biting rate
                outdoorbites += float( outdoor_bites );

                // for the infectious cohort, need to update infectious bites
                outdoorinfectiousbites += VectorToHumanTransmission( OUTDOOR_STR, &NodeVector::vector_to_human_outdoor, cohort, cohort->GetPopulation() );

                // Reset cumulative probability for another random draw
                // to assign outdoor-feeding outcomes among the following:
                outcome = randgen->e();

                // (a) die in attempt to outdoor feed?
                if( DidDie( cohort, feed_probs.die_outdoor, outcome, dead_mosquitoes_outdoor ) ) return 0;

                // (b) successful human feed?
                if( DidFeed( cohort, feed_probs.successful_feed_human_outdoor, outcome, successful_human_feed ) )
                { 
                    if( (cohort->GetState() == VectorStateEnum::STATE_ADULT) && (probs()->outdoor_successfulfeed_human > 0) )
                    {
                        // push back to exposed cohort (to be checked for new infection)
                        OutdoorExposedQueues.push_back(cohort);
                    }
                }
                //else survived to next day
                continue;
            }
        }
        while(0); // just one feed for now, but here is where we will decide whether to continue feeding

        // now adjust egg batch size and reset oviposition timer for next cycle
        GenerateEggs( successful_human_feed, successful_AD_feed , successful_animal_feed, cohort );

        return 0; // not used
    }

    void VectorPopulationIndividual::GenerateEggs( uint32_t numFeedHuman,
                                                   uint32_t numFeedAD,
                                                   uint32_t numFeedAnimal,
                                                   IVectorCohort* cohort )
    {
        float egg_batch_size = CalculateEggBatchSize( cohort );

        uint32_t num_feed = numFeedHuman + numFeedAD + numFeedAnimal;
        uint32_t num_eggs = uint32_t( egg_batch_size * float( num_feed ) );
        cohort->AddNewEggs( CalculateOvipositionTime(), num_eggs ); // also sets oviposition timer
    }

    uint32_t VectorPopulationIndividual::CalculateOvipositionTime()
    {
        // Get duration that can be a temperature-dependent gonotrophic cycle duration:
        float mean_cycle_duration = GetFeedingCycleDuration();

        // Allocate timers randomly to upper and lower bounds of fractional duration
        // If mean is 2.8 days: 80% will have 3-day cycles, and 20% will have 2-day cycles
        uint32_t timer = randgen->randomRound( mean_cycle_duration );

        return timer;
    }

    void VectorPopulationIndividual::RandomlySetOvipositionTimer( VectorCohortIndividual* pvci )
    {
        uint32_t days_between_feeds = CalculateOvipositionTime();
        uint32_t timer = randgen->uniformZeroToN( days_between_feeds );

        // --------------------------------------------------------------------------------------
        // --- We shift these values by one day so that 1/days_between_feeds will feed each day.
        // --- The group with timer=1, will feed the first day, timer=2 will feed the second day
        // --- and so on.  If we allow timer=0, then that group AND the timer=1 group will feed
        // --- on the first day.  Also, there will be NO one feeding on timer=days_between_feeds.
        // --------------------------------------------------------------------------------------
        timer += 1;

        if( params()->vector_params->vector_aging )
        {
            float age = float( (days_between_feeds+1) - timer ); // +1 to offset +1 to timer above
            pvci->SetAge( age );
        }
        pvci->AddNewEggs( timer, uint32_t(species()->eggbatchsize) ); // also sets timer
    }

    void VectorPopulationIndividual::AddAdultsFromMating( const VectorGeneticIndex_t& rVgiMale,
                                                          const VectorGeneticIndex_t& rVgiFemle,
                                                          uint32_t pop )
    {
        VectorSugarFeeding::Enum vsf = params()->vector_params->vector_sugar_feeding;
        bool is_sugar_trap_killing_enabled = ( (vsf == VectorSugarFeeding::VECTOR_SUGAR_FEEDING_ON_EMERGENCE_ONLY) ||
                                               (vsf == VectorSugarFeeding::VECTOR_SUGAR_FEEDING_EVERY_FEED       ) ||
                                               (vsf == VectorSugarFeeding::VECTOR_SUGAR_FEEDING_EVERY_DAY        ) )
                                             && (probs()->sugarTrapKilling > 0.0);

        uint32_t temppop = pop / m_mosquito_weight;
        for( uint32_t i = 0; i < temppop; i++ )
        {
            // now if sugar feeding exists every day or after each feed, and mortality is associated, then check for killing
            if( !is_sugar_trap_killing_enabled || (randgen->e() >= probs()->sugarTrapKilling) )
            {
                VectorCohortIndividual* tempentrynew = VectorCohortIndividual::CreateCohort( VectorStateEnum::STATE_ADULT, 0, 0, m_mosquito_weight, rVgiFemle, &species_ID );
                ApplyMatingGenetics( tempentrynew, VectorMatingStructure( rVgiMale ) );
                AdultQueues.push_back( tempentrynew );
                queueIncrementTotalPopulation( tempentrynew );//to keep accounting consistent with non-aging version
                new_adults += 1;
            }
        }
    }

    void VectorPopulationIndividual::AddVectors_Adults( const VectorMatingStructure& _vector_genetics, uint32_t releasedNumber )
    {
        uint32_t temppop = releasedNumber / m_mosquito_weight;
        // already mated, so go in AdultQueues
        for (uint32_t i = 0; i < temppop; i++)
        {
            IVectorCohort* tempentry = VectorCohortIndividual::CreateCohort(VectorStateEnum::STATE_ADULT, 0, 0, m_mosquito_weight, _vector_genetics, &species_ID);
            AdultQueues.push_back(tempentry);
            queueIncrementTotalPopulation( tempentry );
            new_adults += 1;
        }
    }

    void VectorPopulationIndividual::Expose( const IContagionPopulation* cp, float dt, TransmissionRoute::Enum transmission_route )
    {
        // Get the infectiouness from the contagion population
        float infection_prob = cp->GetTotalContagion();

        // Nothing to do if there isn't any
        if ( infection_prob == 0 ) return;

        // Determine whether we are acting on an indoor or outdoor exposed population
        if ( transmission_route == TransmissionRoute::TRANSMISSIONROUTE_HUMAN_TO_VECTOR_INDOOR )
        {
            ExposeCohortList( cp, IndoorExposedQueues, probs()->indoor_successfulfeed_human, infection_prob );
        }
        else if ( transmission_route == TransmissionRoute::TRANSMISSIONROUTE_HUMAN_TO_VECTOR_OUTDOOR )
        {
            ExposeCohortList( cp, OutdoorExposedQueues, probs()->outdoor_successfulfeed_human, infection_prob );
        }
        else
        {
            std::ostringstream oss;
            oss << "Error in " << __FUNCTION__ << " : don't know what to do with transmission_route = " << transmission_route << std::endl;
            throw IllegalOperationException(__FILE__, __LINE__, __FUNCTION__, oss.str().c_str() );
        }
    }

    void VectorPopulationIndividual::ExposeCohortList( const IContagionPopulation* cp, VectorCohortVector_t& list, float success_prob, float infection_prob )
    {
        StrainIdentity strainID;
        strainID.SetAntigenID(cp->GetAntigenID());

        float x_infectionWolbachia = 1.0;
        for (auto exposed : list)
        {
            if( exposed->GetVectorGenetics().GetWolbachia() != VectorWolbachia::WOLBACHIA_FREE )
            {
                x_infectionWolbachia = params()->vector_params->WolbachiaInfectionModification;
            }
            // Determine if there is a new infection
            if( randgen->e() < species()->acquiremod * x_infectionWolbachia * infection_prob / success_prob )
            {
                LOG_DEBUG_F( "(individual) VECTOR (%d) acquired infection from HUMAN.\n", exposed->GetPopulation() );
                // Draw weighted random strain from ContagionPopulation
                cp->ResolveInfectingStrain( &strainID );

                // Set state to STATE_INFECTED and store strainID
                IVectorCohortIndividual *vci = nullptr;
                if( (exposed)->QueryInterface( GET_IID( IVectorCohortIndividual ), (void**)&vci ) != s_OK )
                {
                    throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "exposed", "IVectorCohortIndividual", "VectorCohort" );
                }
                vci->AcquireNewInfection( &strainID );

                // Adjust population-level counters
                adult    -= exposed->GetPopulation();
                infected += exposed->GetPopulation();
            }
        }
    }

    void VectorPopulationIndividual::Vector_Migration( float dt, IMigrationInfo* pMigInfo, VectorCohortVector_t* pMigratingQueue)
    {
        release_assert( pMigInfo );
        release_assert( pMigratingQueue );

        // Use the verbose "for" construct here because we may be modifying the list and need to protect the iterator.
        for (size_t iCohort = 0; iCohort < AdultQueues.size(); /* increment in loop */)
        {
            IVectorCohort* tempentry = AdultQueues[iCohort];

            suids::suid destination = suids::nil_suid();
            MigrationType::Enum mig_type = MigrationType::NO_MIGRATION;
            float time = 0.0;
            pMigInfo->PickMigrationStep( nullptr, 1.0, destination, mig_type, time );

            // test if each vector will migrate this time step
            if( !destination.is_nil() && (time <= dt) )
            {
                AdultQueues[iCohort] = AdultQueues.back();
                AdultQueues.pop_back();

                // Used to use dynamic_cast here which is _very_ slow.
                IMigrate* emigre = tempentry->GetIMigrate();
                emigre->SetMigrating( destination, mig_type, 0.0, 0.0, false );
                pMigratingQueue->push_back(tempentry);
            }
            else
            {
                ++iCohort;
            }
        }
    }

    void VectorPopulationIndividual::AddImmigratingVector( IVectorCohort* pvc )
    {
        AdultQueues.push_back( pvc );
    }

    uint32_t VectorPopulationIndividual::getInfectedCount( IStrainIdentity* pStrain ) const
    {
        uint32_t num_infected = infected;
        if( pStrain != nullptr )
        {
            num_infected = 0;
            for( auto p_cohort : AdultQueues )
            {
                if( (p_cohort->GetState() == VectorStateEnum::STATE_INFECTED) &&
                    (p_cohort->GetStrainIdentity().GetGeneticID() == pStrain->GetGeneticID()) )
                {
                    num_infected += p_cohort->GetPopulation();
                }
            }
        }
        return num_infected;
    }

    uint32_t VectorPopulationIndividual::getInfectiousCount( IStrainIdentity* pStrain ) const
    {
        uint32_t num_infectious = infectious;
        if( pStrain != nullptr )
        {
            num_infectious = 0;
            for( auto p_cohort : AdultQueues )
            {
                if( (p_cohort->GetState() == VectorStateEnum::STATE_INFECTIOUS) &&
                    (p_cohort->GetStrainIdentity().GetGeneticID() == pStrain->GetGeneticID()) )
                {
                    num_infectious += p_cohort->GetPopulation();
                }
            }
        }
        return num_infectious;
    }


    std::vector<uint64_t> VectorPopulationIndividual::GetNewlyInfectedVectorIds() const
    {
        std::vector<uint64_t> suids;
        for (auto cohort : AdultQueues)
        {
            if ( cohort->GetState() != VectorStateEnum::STATE_INFECTED)
                continue;

            if (cohort->GetProgress() == 0)
            {
                IVectorCohortIndividual* ivci = NULL;
                if( s_OK != cohort->QueryInterface( GET_IID( IVectorCohortIndividual ), (void**)&ivci ) )
                {
                    throw QueryInterfaceException(
                        __FILE__, __LINE__, __FUNCTION__,
                        "cohort", "IVectorCohortIndividual", "VectorCohort" );
                }

                suids.push_back(ivci->GetID());
            }
        }

        return suids;
    }

    std::vector<uint64_t> VectorPopulationIndividual::GetInfectiousVectorIds() const
    {
        std::vector<uint64_t> suids;
        for (auto cohort : AdultQueues)
        {
            if (cohort->GetState() != VectorStateEnum::STATE_INFECTIOUS)
                continue;

            IVectorCohortIndividual* ivci = NULL;
            if( s_OK != cohort->QueryInterface( GET_IID( IVectorCohortIndividual ), (void**)&ivci ) )
            {
                throw QueryInterfaceException(
                    __FILE__, __LINE__, __FUNCTION__,
                    "cohort", "IVectorCohortIndividual", "VectorCohort" );
            }

            suids.push_back(ivci->GetID());
        }

        return suids;
    }

    REGISTER_SERIALIZABLE(VectorPopulationIndividual);

    void VectorPopulationIndividual::serialize(IArchive& ar, VectorPopulationIndividual* obj)
    {
        VectorPopulation::serialize(ar, obj);

        VectorPopulationIndividual& population = *obj;
        ar.labelElement("m_mosquito_weight") & population.m_mosquito_weight;

        // ------------------------------------------------------------------
        // --- The following are only use temporarily during an update cycle.
        // ---  They are cleared and populated each update.
        // ------------------------------------------------------------------
        //m_average_oviposition_killing
        //IndoorExposedQueues
        //OutdoorExposedQueues
        //current_vci
    }
}
