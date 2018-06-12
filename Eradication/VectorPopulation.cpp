/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "VectorPopulation.h"

#include "Debug.h"
#include "Exceptions.h"
#include "Log.h"
#include "NodeVector.h"
#include "SimulationConfig.h"
#include "Vector.h"
#include "VectorParameters.h"
#include "VectorSpeciesParameters.h"
#include "VectorCohortWithHabitat.h"
#include "StrainIdentity.h"
#include "IMigrationInfoVector.h"

#ifdef randgen
#undef randgen
#endif
#include "RANDOM.h"
#define randgen (m_context->GetRng())

SETUP_LOGGING( "VectorPopulation" )

namespace Kernel
{
#ifndef WIN32
    #define min(a,b) ( (a<b) ? a : b )
#endif
    // QI stuff
    BEGIN_QUERY_INTERFACE_BODY(VectorPopulation)
        HANDLE_INTERFACE(IVectorPopulation)
        HANDLE_INTERFACE(IInfectable)
        HANDLE_INTERFACE(IVectorPopulationReporting)
        HANDLE_ISUPPORTS_VIA(IVectorPopulation)
    END_QUERY_INTERFACE_BODY(VectorPopulation)

    // ---------------------------------------------------------
    // --- integer form of the maximum age of a vector in days
    // ---------------------------------------------------------
    uint32_t I_MAX_AGE = 150;

    // ------------------------------------------------------------------------------
    // --- Table for storing mortailityFromAge() calculations for each possible age.
    // --- Age should be a factor of dt and we are assuming that dt is an integer day
    // ------------------------------------------------------------------------------
    std::vector<float> VectorPopulation::m_MortalityTable;

    VectorPopulation::VectorPopulation()
        : m_larval_habitats( nullptr )
        , neweggs(0)
        , adult(0)
        , infected(0)
        , infectious(0)
        , males(0)
        , new_adults(0)
        , dead_mosquitoes_before(0)
        , dead_mosquitoes_indoor(0)
        , dead_mosquitoes_outdoor(0)
        , dryheatmortality(0.0f)
        , infectiouscorrection(0.0f)
        , infected_progress_this_timestep(0.0f)
        , indoorinfectiousbites(0.0f)
        , outdoorinfectiousbites(0.0f)
        , indoorbites(0.0f)
        , outdoorbites(0.0f)
        , infectivity(0.0)
        , infectivity_indoor(0.0f)
        , infectivity_outdoor(0.0f)
        , species_ID("gambiae")
        , m_context(nullptr)
        , m_species_params(nullptr)
        , m_probabilities(nullptr)
        , m_VectorMortality(true)
        , m_LocalMortalityProbabilityTable()
        , m_DefaultLocalMortalityProbability( 1.0 )
    {
        if( m_MortalityTable.size() == 0 )
        {
            for( uint32_t i = 0; i <= I_MAX_AGE; ++i )
            {
                float t_age = float( i );
                float mortality = mortalityFromAge( t_age );
                m_MortalityTable.push_back( mortality );
            }
        }
        m_LocalMortalityProbabilityTable.push_back( std::vector<float>() ); // WOLBACHIA_FREE
        m_LocalMortalityProbabilityTable.push_back( std::vector<float>() ); // not WOLBACHIA_FREE
    }

    void VectorPopulation::Initialize( INodeContext *context, const std::string& species_name, uint32_t adults, uint32_t _infectious )
    {
        species_ID = species_name;

        SetContextTo(context);

        // Correct diepostfeeding and successfulfeed for infectious, which have infectioushfmortmod
        // adjusted for increased feeding mortality due to longer probing
        // Wekesa, J. W., R. S. Copeland, et al. (1992). "Effect of Plasmodium Falciparum on Blood Feeding Behavior of Naturally Infected Anopheles Mosquitoes in Western Kenya." Am J Trop Med Hyg 47(4): 484-488.
        // ANDERSON, R. A., B. G. J. KNOLS, et al. (2000). "Plasmodium falciparum sporozoites increase feeding-associated mortality of their mosquito hosts Anopheles gambiae s.l." Parasitology 120(04): 329-333.
        infectiouscorrection = 0;
        if (params()->vector_params->human_feeding_mortality * species()->infectioushfmortmod < 1.0)
        {
            infectiouscorrection = float((1.0 - params()->vector_params->human_feeding_mortality * species()->infectioushfmortmod) / (1.0 - params()->vector_params->human_feeding_mortality));
        }

        // Set up initial populations of adult/infectious/male mosquitoes
        InitializeVectorQueues(adults, _infectious);
    }

    void VectorPopulation::InitializeVectorQueues( uint32_t adults, uint32_t _infectious )
    {
        adult = adults;
        males = adult;
        infectious = _infectious;

        if( adult > 0 )
        {
            // adult initialized at age 0
            VectorCohort* pvc = VectorCohort::CreateCohort( VectorStateEnum::STATE_ADULT, 0, 0, adult, VectorMatingStructure( VectorGender::VECTOR_FEMALE ), &species_ID );
            AdultQueues.push_back( pvc );

            // Initialize the cohort's egg queue so that they start off laying eggs
            // This should be similar to how VectorPopulationIndividual has individual vectors starting off with eggs.
            if( species()->eggbatchsize > 0 )
            {
                float days_between_feeds = randgen->randomRound( GetFeedingCycleDuration() );
                float percent_feed_per_day = 1.0f / days_between_feeds ;
                uint32_t num_eggs = uint32_t( percent_feed_per_day * pvc->GetPopulation() * species()->eggbatchsize );

                // ------------------------------------------------------------------------------------------------
                // --- We assign days [1, days_between_feeds] (vs [0,days_between_feeds-1] because
                // --- we want to simulate that the eggs were added at the end of feeding.
                // --- For example, if day=1, then the cohort will birth eggs on the first update.
                // --- If day = days_between_feeds, the cohort will birth eggs on the "days_between_feeds'th" day.
                // ------------------------------------------------------------------------------------------------
                for( float day = 1.0f ; day <= days_between_feeds ; day += 1.0f )
                {
                    pvc->AddNewEggs( uint32_t(day), num_eggs );
                }
            }

            // progress is 1 since males should be at 1 from progressing from Immature to Male
            VectorCohort* pvc_male = VectorCohort::CreateCohort( VectorStateEnum::STATE_MALE, 0, 1, males, VectorMatingStructure( VectorGender::VECTOR_MALE ), &species_ID );
            MaleQueues.push_back( pvc_male );
        }

        if( infectious > 0 )
        {
            // infectious initialized at age 20
            // progress is 1 since Infectious should be at 1 from progressing from Infected to Infectious
            InfectiousQueues.push_back( VectorCohort::CreateCohort( VectorStateEnum::STATE_INFECTIOUS, 20, 1, infectious, VectorMatingStructure( VectorGender::VECTOR_FEMALE ), &species_ID ) );
        }
    }

    VectorPopulation *VectorPopulation::CreatePopulation( INodeContext *context, const std::string& species_name, uint32_t adults, uint32_t infectious )
    {
        VectorPopulation *newpopulation = _new_ VectorPopulation();
        release_assert( newpopulation );
        newpopulation->Initialize( context, species_name, adults, infectious );
        return newpopulation;
    }

    void VectorPopulation::SetupIntranodeTransmission(ITransmissionGroups *transmissionGroups)
    {
        m_transmissionGroups = transmissionGroups;
    }

    void VectorPopulation::SetupLarvalHabitat( INodeContext *context )
    {
        // Query for vector node context
        IVectorNodeContext* ivnc = nullptr;
        if (s_OK !=  context->QueryInterface(GET_IID(IVectorNodeContext), (void**)&ivnc))
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "IVectorNodeContext", "INodeContext" );
        }

        // Create a larval habitat for each type specified in configuration file for this species
        for( auto habitat_param : species()->habitat_params.habitat_map )
        {
            VectorHabitatType::Enum type = habitat_param.first;
            IVectorHabitat* habitat = ivnc->GetVectorHabitatBySpeciesAndType( species_ID, type, habitat_param.second );
            float max_larval_capacity = habitat->GetMaximumLarvalCapacity() * params()->vector_params->x_tempLarvalHabitat * ivnc->GetLarvalHabitatMultiplier(type,species_ID);

            habitat->SetMaximumLarvalCapacity( max_larval_capacity );

            // TODO: probably it would be preferable to store the "Required_Habitat_Parameter" values
            //       as a map-by-Habitat_Type already in VectorSpeciesParameters.
            m_larval_capacities[type] = max_larval_capacity; 
        }
    }

    VectorPopulation::~VectorPopulation()
    {
        for (auto queue : EggQueues)
        {
            delete queue;
        }

        for (auto queue : LarvaQueues)
        {
            delete queue;
        }

        for (auto queue : ImmatureQueues)
        {
            delete queue;
        }

        for (auto queue : AdultQueues)
        {
            delete queue;
        }

        for (auto queue : InfectedQueues)
        {
            delete queue;
        }

        for (auto queue : InfectiousQueues)
        {
            delete queue;
        }

        for (auto queue : MaleQueues)
        {
            delete queue;
        }
    }

    void VectorPopulation::UpdateLocalAdultMortalityProbability( float dt )
    {
        m_LocalMortalityProbabilityTable[ 0 ].clear();
        m_LocalMortalityProbabilityTable[ 1 ].clear();

        float wolb_free = 1.0;
        float wolb_mod = params()->vector_params->WolbachiaMortalityModification;

        // if we are not aging vectors, then "from_age" below is always zero.
        uint32_t num_ages = params()->vector_params->vector_aging ? m_MortalityTable.size() : 1;

        // calculate probability for immature vectors
        float local_adult_mortality = species()->adultmortality + dryheatmortality;
        m_DefaultLocalMortalityProbability = float( EXPCDF( -dt * local_adult_mortality * wolb_free ) );

        for( uint32_t i_age = 0 ; i_age < num_ages; ++i_age )
        {
            float from_age = 0.0;
            if( params()->vector_params->vector_aging )
            {
                from_age = m_MortalityTable[ i_age ];
            }

            float p_local_mortality_free = float( EXPCDF( -dt * (local_adult_mortality + from_age) * wolb_free ) );
            float p_local_mortality_mod  = float( EXPCDF( -dt * (local_adult_mortality + from_age) * wolb_mod  ) );

            m_LocalMortalityProbabilityTable[ 0 ].push_back( p_local_mortality_free );
            m_LocalMortalityProbabilityTable[ 1 ].push_back( p_local_mortality_mod  );
        }
    }


    float VectorPopulation::GetLocalAdultMortalityProbability( float dt, IVectorCohort* pvc, VectorWolbachia::Enum wolb ) const
    {
        float p_local_mortality = m_DefaultLocalMortalityProbability;
        if( pvc != nullptr )
        {
            // --------------------------------------------------------------------------------
            // --- Use the enum passed in and not the value in the cohort.
            // --- This allows the calling methods to override the enum.
            // --- We don't use the WolbachiaMortalityModification on Males and Immature vectors 
            // --------------------------------------------------------------------------------
            uint32_t i_wolb = (wolb == VectorWolbachia::WOLBACHIA_FREE) ? 0 : 1;
            uint32_t i_age  = (params()->vector_params->vector_aging  ) ? uint32_t( pvc->GetAge() ) : 0;

            if( i_age > I_MAX_AGE )
            {
                // When I_MAX_AGE is large (i.e. > 100 ), then mortalityFromAge() approaches zero
                // so the last value should not be different from a value calculated at a larger age.
                return i_age = I_MAX_AGE;
            }

            p_local_mortality = m_LocalMortalityProbabilityTable[ i_wolb ][ i_age ];
        }
        return p_local_mortality;
    }

    void VectorPopulation::UpdateAge( IVectorCohort* pvc, float dt )
    {
        if( (pvc != nullptr) && params()->vector_params->vector_aging )
        {
            pvc->IncreaseAge( dt );
        }
    }

    void VectorPopulation::UpdateVectorPopulation( float dt )
    {
        // Reset EIR/HBR reporting
        m_EIR_by_pool = std::make_pair(0.0f, 0.0f);
        m_HBR_by_pool = std::make_pair(0.0f, 0.0f);

        // Reset counters
        neweggs                 = 0;
        indoorinfectiousbites   = 0;
        outdoorinfectiousbites  = 0;
        indoorbites             = 0;
        outdoorbites            = 0;
        new_adults              = 0;
        dead_mosquitoes_before  = 0;
        dead_mosquitoes_indoor  = 0;
        dead_mosquitoes_outdoor = 0;

        gender_mating_eggs.clear();
        gender_mating_males.clear();
        vector_genetics_adults.clear();
        vector_genetics_infected.clear();
        vector_genetics_infectious.clear();

        float temperature = m_context->GetLocalWeather()->airtemperature();
        infected_progress_this_timestep = (species()->infectedarrhenius1 * exp( -species()->infectedarrhenius2 / (temperature + float( CELSIUS_TO_KELVIN )) )) * dt;

        // Over-ride some lifecycle probabilities with species-specific values
        Update_Lifecycle_Probabilities(dt);

        // Update vector-cohort lists
        Update_Egg_Hatching(dt);
        Update_Infectious_Queue(dt);
        Update_Infected_Queue(dt);
        Update_Adult_Queue(dt);
        Update_Male_Queue(dt);
        Update_Immature_Queue(dt);
        Update_Larval_Queue(dt);
        Update_Egg_Laying(dt);

        // Now calculate quantities for reporting:
        float eff_pop = m_context->GetStatPop(); //probs()->effective_host_population; // actual bites divided by actual pop (not risk-weighted pop for feeding probs)
        if( eff_pop > 0 )
        {
            // (1) infectious bites (EIR)
            m_EIR_by_pool.first  = float(indoorinfectiousbites / eff_pop);
            m_EIR_by_pool.second = float(outdoorinfectiousbites / eff_pop);
            // (2) total human bites (HBR)
            m_HBR_by_pool.first  = float(indoorbites / eff_pop);
            m_HBR_by_pool.second = float(outdoorbites / eff_pop);
        }
        else
        {
            LOG_DEBUG_F("The effective human population at node %lu is zero, so EIR and HBR are not being normalized in VectorPopulation::UpdateVectorPopulation.\n", m_context->GetSuid().data );
        }
        // (3) vector-to-human infectivity
        infectivity = GetEIRByPool(VectorPoolIdEnum::BOTH_VECTOR_POOLS) * species()->transmissionmod;
    }

    void VectorPopulation::Update_Lifecycle_Probabilities( float dt )
    {
        // Update adult transition probabilities:
        // calculated for each queue entry up to human indoor and outdoor feeding attempts
        probs()->FinalizeTransitionProbabilites( species()->anthropophily, species()->indoor_feeding); // TODO: rename this function now??

        // Update mortality from heat
        dryheatmortality = 0.0;
        if( m_VectorMortality )
        {
            float temperature = m_context->GetLocalWeather()->airtemperature();
            dryheatmortality  = dryHeatMortality(temperature);
        }
        UpdateLocalAdultMortalityProbability( dt );
    }

    void VectorPopulation::Update_Infectious_Queue( float dt )
    {
        infectious = 0;
        // Use the verbose "foreach" construct here because empty infectious cohorts (e.g. old vectors) will be removed
        for( size_t iCohort = 0; iCohort < InfectiousQueues.size(); /* increment in loop */ )
        {
            IVectorCohort* cohort = InfectiousQueues[ iCohort ];

            UpdateAge( cohort, dt );

            ProcessFeedingCycle( dt, cohort );

            if( cohort->GetPopulation() <= 0 )
            {
                InfectiousQueues[ iCohort ] = InfectiousQueues.back();
                InfectiousQueues.pop_back();
                delete cohort;

                // !! Don't increment iCohort. !!
            }
            else
            {
                queueIncrementTotalPopulation( cohort );// update INFECTIOUS counters

                ++iCohort;
            }
        }
    }

    void VectorPopulation::Update_Infected_Queue( float dt )
    {
        infected = 0;

        // Use the verbose "foreach" construct here because empty or completely-progressed queues will be removed
        for( size_t iCohort = 0; iCohort < InfectedQueues.size(); /* increment in loop */ )
        {
            IVectorCohort* cohort = InfectedQueues[ iCohort ];

            UpdateAge( cohort, dt );

            // progress with sporogony
            cohort->IncreaseProgress( infected_progress_this_timestep );

            ProcessFeedingCycle( dt, cohort );

            // done with this queue if it is fully progressed or is empty
            if (cohort->GetProgress() >= 1 || cohort->GetPopulation() <= 0)
            {
                bool delete_cohort = true;

                // infected queue completion, moving to infectious
                if (cohort->GetPopulation() > 0)
                {
                    cohort->SetState( VectorStateEnum::STATE_INFECTIOUS );
                    queueIncrementTotalPopulation( cohort ); // update INFECTIOUS counters
                    MergeProgressedCohortIntoCompatibleQueue( InfectiousQueues, cohort, 0.0 );
                    delete_cohort = false;
                }

                InfectedQueues[ iCohort ] = InfectedQueues.back();
                InfectedQueues.pop_back();
                if( delete_cohort )
                {
                    delete cohort;
                }
                // !! Don't increment iCohort. !!
            }
            else
            {
                queueIncrementTotalPopulation( cohort ); // update INFECTED counters

                ++iCohort;
            }
        }
    }

    void VectorPopulation::Update_Adult_Queue( float dt )
    {
        adult = 0;
        // Use the verbose "foreach" construct here because empty adult cohorts (e.g. old vectors) will be removed
        for (size_t iCohort = 0; iCohort < AdultQueues.size(); /* increment in loop */)
        {
            IVectorCohort* cohort = AdultQueues[iCohort];

            UpdateAge( cohort, dt );

            uint32_t newinfected = ProcessFeedingCycle( dt, cohort );
            // correct for too high
            if( newinfected > cohort->GetPopulation() )
            {
                newinfected = cohort->GetPopulation();
            }

            if (newinfected > 0)
            {
                // Reduce this cohort
                cohort->SetPopulation( cohort->GetPopulation() - newinfected );

                // Move newly infected vectors into another cohort in the infected queue
                VectorCohort* tempentrynew = VectorCohort::CreateCohort( VectorStateEnum::STATE_INFECTED, cohort->GetAge(), 0, newinfected, cohort->GetVectorGenetics(), &species_ID );
                queueIncrementTotalPopulation( tempentrynew ); // update INFECTED counters
                MergeProgressedCohortIntoCompatibleQueue( InfectedQueues, tempentrynew, infected_progress_this_timestep );
            }

            if( cohort->GetPopulation() <= 0 )
            {
                AdultQueues[ iCohort ] = AdultQueues.back();
                AdultQueues.pop_back();
                delete cohort;

                // !! Don't increment iCohort. !!
            }
            else
            {
                queueIncrementTotalPopulation( cohort ); // update ADULT counters

                ++iCohort;
            }
        }
    }

    float VectorPopulation::AdjustForConditionalProbability( float& rCumulative, float probability )
    {
        if( rCumulative >= 1.0 )
        {
            return 0.0;
        }

        //adjust for conditional probability
        float adjusted = probability / (1.0f - rCumulative);

        rCumulative += probability;
        if( rCumulative > 1.0 )
        {
            rCumulative = 1.0;
        }

        return adjusted;
    }

    void VectorPopulation::AdjustForFeedingRate( float dt, float p_local_mortality, VectorPopulation::FeedingProbabilities& rFeedProbs )
    {
        float feedingrate = 1.0f / GetFeedingCycleDuration();

        rFeedProbs.die_without_attempting_to_feed  *= (1.0f - feedingrate * dt);
        rFeedProbs.die_before_human_feeding        *= feedingrate * dt;
        rFeedProbs.successful_feed_animal          *= feedingrate * dt;
        rFeedProbs.successful_feed_artifical_diet  *= feedingrate * dt;
        rFeedProbs.successful_feed_attempt_indoor  *= feedingrate * dt;
        rFeedProbs.successful_feed_attempt_outdoor *= feedingrate * dt;

        rFeedProbs.die_without_attempting_to_feed += p_local_mortality;
    }

    void VectorPopulation::AdjustForCumulativeProbability( VectorPopulation::FeedingProbabilities& rFeedProbs )
    {
        float cum_prob = 0.0;
        cum_prob += rFeedProbs.die_without_attempting_to_feed;
        cum_prob += rFeedProbs.die_before_human_feeding;

        rFeedProbs.successful_feed_animal          = AdjustForConditionalProbability( cum_prob, rFeedProbs.successful_feed_animal          );
        rFeedProbs.successful_feed_artifical_diet  = AdjustForConditionalProbability( cum_prob, rFeedProbs.successful_feed_artifical_diet  );
        rFeedProbs.successful_feed_attempt_indoor  = AdjustForConditionalProbability( cum_prob, rFeedProbs.successful_feed_attempt_indoor  );
        rFeedProbs.successful_feed_attempt_outdoor = AdjustForConditionalProbability( cum_prob, rFeedProbs.successful_feed_attempt_outdoor );

        cum_prob = rFeedProbs.die_indoor;
        rFeedProbs.successful_feed_artifical_diet_indoor = AdjustForConditionalProbability( cum_prob, rFeedProbs.successful_feed_artifical_diet_indoor );
        rFeedProbs.successful_feed_human_indoor          = AdjustForConditionalProbability( cum_prob, rFeedProbs.successful_feed_human_indoor          );

        cum_prob = rFeedProbs.die_outdoor;
        rFeedProbs.successful_feed_human_outdoor = AdjustForConditionalProbability( cum_prob, rFeedProbs.successful_feed_human_outdoor );
    }

    VectorPopulation::FeedingProbabilities VectorPopulation::CalculateFeedingProbabilities( float dt, IVectorCohort* cohort )
    {
        // Adjust human-feeding mortality for longer-probing infectious vectors
        // Wekesa, J. W., R. S. Copeland, et al. (1992). 
        //     "Effect of Plasmodium Falciparum on Blood Feeding Behavior of Naturally Infected Anopheles Mosquitoes in Western Kenya."
        //     Am J Trop Med Hyg 47(4): 484-488.
        // ANDERSON, R. A., B. G. J. KNOLS, et al. (2000).
        //     "Plasmodium falciparum sporozoites increase feeding-associated mortality of their mosquito hosts Anopheles gambiae s.l."
        //     Parasitology 120(04): 329-333.
        float x_infectioushfmortmod  = 1.0f;
        float x_infectiouscorrection = 1.0f;
        if( cohort->GetState() == VectorStateEnum::STATE_INFECTIOUS )
        {
            x_infectioushfmortmod  = species()->infectioushfmortmod;
            x_infectiouscorrection = infectiouscorrection;
        }

        // calculate local mortality / survivability
        float p_local_mortality = GetLocalAdultMortalityProbability( dt, cohort, cohort->GetVectorGenetics().GetWolbachia() );
        float p_local_survivability = 1.0f - p_local_mortality;

        FeedingProbabilities feed_probs;

        feed_probs.die_without_attempting_to_feed  = p_local_survivability * probs()->diewithoutattemptingfeed;
        feed_probs.die_before_human_feeding        = p_local_survivability * probs()->diebeforeattempttohumanfeed;
        feed_probs.successful_feed_animal          = p_local_survivability * probs()->successfulfeed_animal;
        feed_probs.successful_feed_artifical_diet  = p_local_survivability * probs()->successfulfeed_AD;
        feed_probs.successful_feed_attempt_indoor  = p_local_survivability * probs()->indoorattempttohumanfeed;
        feed_probs.successful_feed_attempt_outdoor = p_local_survivability * probs()->outdoorattempttohumanfeed;

        feed_probs.die_indoor                      = probs()->indoor_diebeforefeeding 
                                                   + probs()->indoor_dieduringfeeding * x_infectioushfmortmod
                                                   + probs()->indoor_diepostfeeding   * x_infectiouscorrection;

        feed_probs.successful_feed_artifical_diet_indoor = probs()->indoor_successfulfeed_AD;

        feed_probs.successful_feed_human_indoor   = probs()->indoor_successfulfeed_human * x_infectiouscorrection;

        feed_probs.die_outdoor                    = probs()->outdoor_diebeforefeeding
                                                  + probs()->outdoor_dieduringfeeding * x_infectioushfmortmod
                                                  + probs()->outdoor_diepostfeeding   * x_infectiouscorrection
                                                  + probs()->outdoor_successfulfeed_human * probs()->outdoor_returningmortality * x_infectiouscorrection;

        feed_probs.successful_feed_human_outdoor  = (1.0f - probs()->outdoor_returningmortality) * probs()->outdoor_successfulfeed_human * x_infectiouscorrection;

        AdjustForFeedingRate( dt, p_local_mortality, feed_probs );

        AdjustForCumulativeProbability( feed_probs );

        LOG_DEBUG_F( "die_without_attempting_to_feed       =%14.11f\n", feed_probs.die_without_attempting_to_feed );
        LOG_DEBUG_F( "die_before_human_feeding             =%14.11f\n", feed_probs.die_before_human_feeding );
        LOG_DEBUG_F( "successful_feed_animal               =%14.11f\n", feed_probs.successful_feed_animal );
        LOG_DEBUG_F( "successful_feed_artifical_diet       =%14.11f\n", feed_probs.successful_feed_artifical_diet );
        LOG_DEBUG_F( "successful_feed_attempt_indoor       =%14.11f\n", feed_probs.successful_feed_attempt_indoor );
        LOG_DEBUG_F( "successful_feed_attempt_outdoor      =%14.11f\n", feed_probs.successful_feed_attempt_outdoor );
        LOG_DEBUG_F( "die_indoor                           =%14.11f\n", feed_probs.die_indoor );
        LOG_DEBUG_F( "successful_feed_artifical_diet_indoor=%14.11f\n", feed_probs.successful_feed_artifical_diet_indoor );
        LOG_DEBUG_F( "successful_feed_human_indoor         =%14.11f\n", feed_probs.successful_feed_human_indoor );
        LOG_DEBUG_F( "die_outdoor                          =%14.11f\n", feed_probs.die_outdoor );
        LOG_DEBUG_F( "successful_feed_human_outdoor        =%14.11f\n", feed_probs.successful_feed_human_outdoor );

        return feed_probs;
    }

    uint32_t VectorPopulation::CalculatePortionInProbability( bool isForDeath, uint32_t& rRemainingPop, float prob )
    {
        if( (isForDeath && !m_VectorMortality) || (rRemainingPop <= 0) || (prob <= 0.0) )
        {
            return 0;
        }
        else if( prob >= 1.0 )
        {
            // avoid drawing random number
            uint32_t num_in_prob = rRemainingPop;
            rRemainingPop = 0;
            return num_in_prob;
        }
        else
        {
            uint32_t num_in_prob = uint32_t( randgen->binomial_approx( rRemainingPop, prob ) );
            rRemainingPop -= num_in_prob;

            return num_in_prob;
        }
    }

    void VectorPopulation::VectorToHumanDeposit( const IStrainIdentity& strain,
                                                 uint32_t attemptFeed,
                                                 const TransmissionGroupMembership_t* pTransmissionVectorToHuman )
    {
        m_context->DepositFromIndividual( strain, float( attemptFeed ) * species()->transmissionmod, pTransmissionVectorToHuman );
    }

    uint32_t VectorPopulation::VectorToHumanTransmission( const char* indoor_or_outdoor_str,
                                                          const TransmissionGroupMembership_t* pTransmissionVectorToHuman,
                                                          IVectorCohort* cohort,
                                                          uint32_t attemptFeed )
    {
        uint32_t infected_bites = 0;
        if( cohort->GetState() == VectorStateEnum::STATE_INFECTIOUS )
        {
            infected_bites = attemptFeed;

            // deposit indoor and outdoor contagion into vector-to-human group
            const IStrainIdentity& strain = cohort->GetStrainIdentity();

            LOG_DEBUG_F( "Vector->Human [%s] infectiousness (aka bite) of strain %d, 'population' %d, xmod %f.\n",
                         indoor_or_outdoor_str,
                         strain.GetAntigenID(),
                         attemptFeed,
                         species()->transmissionmod );

            VectorToHumanDeposit( strain, attemptFeed, pTransmissionVectorToHuman );
        }
        return infected_bites;
    }

    uint32_t VectorPopulation::CalculateHumanToVectorInfection( const TransmissionGroupMembership_t* transmissionHumanToVector,
                                                                IVectorCohort* cohort,
                                                                float probSuccessfulFeed,
                                                                uint32_t numHumanFeed )
    {
        uint32_t num_infected = 0;
        if( (cohort->GetState() == VectorStateEnum::STATE_ADULT) && (numHumanFeed > 0) )
        {
            float host_infectivity = m_transmissionGroups->GetTotalContagion( transmissionHumanToVector );

            //Wolbachia related impacts on infection susceptibility
            float x_infectionWolbachia = 1.0;
            if( cohort->GetVectorGenetics().GetWolbachia() != VectorWolbachia::WOLBACHIA_FREE )
            {
                x_infectionWolbachia = params()->vector_params->WolbachiaInfectionModification;
            }
            float prob_infected = species()->acquiremod * x_infectionWolbachia * host_infectivity / probSuccessfulFeed;

            num_infected = uint32_t( randgen->binomial_approx( numHumanFeed, prob_infected ) );
        }
        return num_infected;
    }

    float VectorPopulation::CalculateEggBatchSize( IVectorCohort* cohort )
    {
        // Oocysts, not sporozoites affect egg batch size:
        // Hogg, J. C. and H. Hurd (1997). 
        //    "The effects of natural Plasmodium falciparum infection on the fecundity
        //    and mortality of Anopheles gambiae s. l. in north east Tanzania."
        // Parasitology 114(04): 325-331.
        float x_infectedeggbatchmod = (cohort->GetState() == VectorStateEnum::STATE_INFECTED) ? float( species()->infectedeggbatchmod ) : 1.0f;

        float egg_batch_size = species()->eggbatchsize * x_infectedeggbatchmod;

        return egg_batch_size;
    }

    void VectorPopulation::GenerateEggs( uint32_t numFeedHuman,
                                         uint32_t numFeedAD,
                                         uint32_t numFeedAnimal,
                                         IVectorCohort* cohort )
    {
        float egg_batch_size = CalculateEggBatchSize( cohort );

        uint32_t num_feed = numFeedHuman + numFeedAD + numFeedAnimal;

        // -----------------------------------------------------------------------------------------------------
        // --- Since the duration is likely to be a non-integer, we want to have some portion of the
        // --- vectors lay their eggs on day1 (the truncated day) and the rest on day 2 (truncated day plus 1).
        // --- This should simulate how the individual vectors will randomly make this same decision.
        // -----------------------------------------------------------------------------------------------------
        float feeding_duration = GetFeedingCycleDuration();

        uint32_t day1 = uint32_t( feeding_duration );
        uint32_t day2 = day1 + 1;

        float feeding_day2_percent = feeding_duration - uint32_t( feeding_duration );

        uint32_t num_feed_day2 = randgen->binomial_approx( num_feed, feeding_day2_percent );
        uint32_t num_feed_day1 = num_feed - num_feed_day2;

        uint32_t num_eggs_day1 = uint32_t( egg_batch_size * float( num_feed_day1 ) );
        uint32_t num_eggs_day2 = uint32_t( egg_batch_size * float( num_feed_day2 ) );

        cohort->AddNewEggs( day1, num_eggs_day1 );
        cohort->AddNewEggs( day2, num_eggs_day2 );
    }

    void VectorPopulation::AdjustEggsForDeath( IVectorCohort* cohort, uint32_t numDied )
    {
        cohort->AdjustEggsForDeath( numDied );
    }

    void VectorPopulation::AddEggsToLayingQueue( IVectorCohort* cohort, uint32_t num_eggs )
    {
        neweggs += num_eggs;
        gender_mating_eggs[ cohort->GetVectorGenetics().GetIndex() ] += num_eggs;

        LOG_DEBUG_F( "adding %d eggs to vector genetics index %d.  current total=%d\n",
                     num_eggs,
                     cohort->GetVectorGenetics().GetIndex(),
                     gender_mating_eggs[ cohort->GetVectorGenetics().GetIndex() ] );
    }

    uint32_t VectorPopulation::ProcessFeedingCycle( float dt, IVectorCohort* cohort )
    {
        if( cohort->GetPopulation() <= 0 )
            return 0;

        FeedingProbabilities feed_probs = CalculateFeedingProbabilities( dt, cohort );

        uint32_t remaining_pop = cohort->GetPopulation();

        uint32_t died_without_feeding   = CalculatePortionInProbability( true,  remaining_pop, feed_probs.die_without_attempting_to_feed  );

        AdjustEggsForDeath( cohort, died_without_feeding );
        AddEggsToLayingQueue( cohort, cohort->GetGestatedEggs() );

        uint32_t died_before_feeding    = CalculatePortionInProbability( true,  remaining_pop, feed_probs.die_before_human_feeding        );
        uint32_t successful_animal_feed = CalculatePortionInProbability( false, remaining_pop, feed_probs.successful_feed_animal          );
        uint32_t successful_AD_feed     = CalculatePortionInProbability( false, remaining_pop, feed_probs.successful_feed_artifical_diet  );
        uint32_t attempt_feed_indoor    = CalculatePortionInProbability( false, remaining_pop, feed_probs.successful_feed_attempt_indoor  );
        uint32_t attempt_feed_outdoor   = CalculatePortionInProbability( false, remaining_pop, feed_probs.successful_feed_attempt_outdoor );

        // update human biting rate
        indoorbites  += attempt_feed_indoor;
        outdoorbites += attempt_feed_outdoor;

        // for the infectious cohort, need to update infectious bites
        indoorinfectiousbites  += VectorToHumanTransmission( INDOOR_STR,  &NodeVector::vector_to_human_indoor,  cohort, attempt_feed_indoor  );
        outdoorinfectiousbites += VectorToHumanTransmission( OUTDOOR_STR, &NodeVector::vector_to_human_outdoor, cohort, attempt_feed_outdoor );

        // indoor feeds
        uint32_t died_indoor = 0;
        uint32_t feed_indoor = 0;
        uint32_t infected_indoor = 0;
        if( attempt_feed_indoor > 0 )
        {
            died_indoor         = CalculatePortionInProbability( true,  attempt_feed_indoor, feed_probs.die_indoor                            );
            successful_AD_feed += CalculatePortionInProbability( false, attempt_feed_indoor, feed_probs.successful_feed_artifical_diet_indoor );
            feed_indoor         = CalculatePortionInProbability( false, attempt_feed_indoor, feed_probs.successful_feed_human_indoor          );

            // some successful feeds result in infected adults
            // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            // !!! NOTE: no infectious correction as in outdoor !!!
            // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            float prob_feed = probs()->indoor_successfulfeed_human;

            infected_indoor = CalculateHumanToVectorInfection( &NodeVector::human_to_vector_indoor,
                                                               cohort,
                                                               prob_feed,
                                                               feed_indoor );
        }

        // outdoor feeds
        uint32_t died_outdoor = 0;
        uint32_t feed_outdoor = 0;
        uint32_t infected_outdoor = 0;
        if( attempt_feed_outdoor > 0 )
        {
            died_outdoor = CalculatePortionInProbability( true,  attempt_feed_outdoor, feed_probs.die_outdoor                   );
            feed_outdoor = CalculatePortionInProbability( false, attempt_feed_outdoor, feed_probs.successful_feed_human_outdoor );

            // some successful feeds result in infected adults
            // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            // !!! NOTE: infectious correction in outdoor but not indoor !!!
            // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            float x_infectiouscorrection = (cohort->GetState() == VectorStateEnum::STATE_INFECTIOUS) ? float( infectiouscorrection ) : 1.0f;

            float prob_feed = (1.0f - probs()->outdoor_returningmortality) * probs()->outdoor_successfulfeed_human * x_infectiouscorrection;

            infected_outdoor = CalculateHumanToVectorInfection( &NodeVector::human_to_vector_outdoor,
                                                                cohort,
                                                                prob_feed,
                                                                feed_outdoor );
        }

        // adjust population
        dead_mosquitoes_before  += died_without_feeding + died_before_feeding;
        dead_mosquitoes_indoor  += died_indoor;
        dead_mosquitoes_outdoor += died_outdoor;

        uint32_t num_dead = died_without_feeding + died_before_feeding + died_indoor + died_outdoor;
        cohort->SetPopulation( cohort->GetPopulation() - num_dead );

        // Generate eggs for laying
        uint32_t successful_human_feed = feed_indoor + feed_outdoor;
        GenerateEggs( successful_human_feed, successful_AD_feed, successful_animal_feed, cohort );

        return (infected_indoor + infected_outdoor);
    }

    float VectorPopulation::GetFeedingCycleDuration() const
    {
        float mean_cycle_duration = species()->daysbetweenfeeds;
        if( params()->vector_params->temperature_dependent_feeding_cycle != TemperatureDependentFeedingCycle::NO_TEMPERATURE_DEPENDENCE )
        {
            // Temperature-dependent gonotrophic cycle duration:
            // (1) Hoshen MB and Morse AP.
            //     "A weather-driven model of malaria transmission".
            //     Malaria Journal 2004, 3:32.
            // (2) Rua GL, Quinones ML, Velez ID, Zuluaga JS, Rojas W, Poveda G, Ruiz D.
            //     "Laboratory estimation of the effects of increasing temperatures on the duration of gonotrophic cycle of Anopheles albimanus (Diptera: Culicidae)"
            //     Mem Inst Oswaldo Cruz 2005, 100(5):515-20.

            float airtemp = m_context->GetLocalWeather()->airtemperature();

            // For VectorSpeciesParameters::daysbetweenfeeds = 3 days:
            //   Three-day feeding cycle at normal tropical temperatures (30 C)
            //   Colder -> longer cycles  (4 days at 24 C)
            //   Hotter -> shorter cycles (increasing fraction of 2-day cycles from 30 C to 48 C)

            // Whole temperature-dependent distribution can be shifted by changing
            // "Days_Between_Feeds" parameter, which is always the value at 30 degrees C.
            if (params()->vector_params->temperature_dependent_feeding_cycle == TemperatureDependentFeedingCycle::BOUNDED_DEPENDENCE)
            {
                mean_cycle_duration = (airtemp > 15) ? 1.0f + 37.0f * ( (species()->daysbetweenfeeds - 1.0f) / 2.0f ) / ( airtemp - 11.5f ) : 10.0f;
            }
            else if (params()->vector_params->temperature_dependent_feeding_cycle == TemperatureDependentFeedingCycle::ARRHENIUS_DEPENDENCE)
            {
                mean_cycle_duration = 1/( species()->cyclearrhenius1 * exp(-species()->cyclearrhenius2 / (airtemp + CELSIUS_TO_KELVIN)) );// * dt;  ( 4.090579e+10 * exp(-7.740230e+03 / (airtemp + CELSIUS_TO_KELVIN)) );
            }
            else
            {
                throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__,
                    "Unknown Temperature_Dependent_Feeding_Cycle in GetFeedingCycleDuration()",
                    params()->vector_params->temperature_dependent_feeding_cycle, 
                    TemperatureDependentFeedingCycle::pairs::lookup_key( params()->vector_params->temperature_dependent_feeding_cycle ) );
            }

            LOG_VALID_F("Mean gonotrophic cycle duration = %0.5f days at %0.2f degrees C.\n", mean_cycle_duration, airtemp);
        }
        return mean_cycle_duration;
    }

    void VectorPopulation::Update_Immature_Queue( float dt )
    {
        // calculate local mortality, includes outdoor area killling
        float p_local_mortality = GetLocalAdultMortalityProbability( dt, nullptr );
        p_local_mortality = p_local_mortality + (1.0f - p_local_mortality) * probs()->outdoorareakilling;

        // Use the verbose "for" construct here because we may be modifying the list
        for( size_t iCohort = 0; iCohort < ImmatureQueues.size(); /* increment in loop */ )
        {
            IVectorCohort* cohort = ImmatureQueues[ iCohort ];

            // introduce climate dependence here if we can figure it out
            cohort->IncreaseProgress( dt * species()->immaturerate );

            if( m_VectorMortality )
            {
                uint32_t die = uint32_t( randgen->binomial_approx( cohort->GetPopulation(), p_local_mortality ) );
                cohort->SetPopulation( cohort->GetPopulation() - die );
            }

            if (cohort->GetProgress() >= 1 || cohort->GetPopulation() <= 0)
            {
                bool delete_cohort = true;
                if (cohort->GetPopulation() > 0) // corrected in case of too long a time step
                {
                    // female or male?
                    if (cohort->GetVectorGenetics().GetGender() == VectorGender::VECTOR_FEMALE) //female
                    {
                        // new mating calculations
                        if(gender_mating_males.empty() || males == 0)// no males listed, so stay immature 
                        {
                            ++iCohort;
                            continue;
                        }
                        else if(gender_mating_males.size() == 1)// just one type of males, so all females mate with that type
                        {
                            AddAdultsFromMating( gender_mating_males.begin()->first,
                                                 cohort->GetVectorGenetics().GetIndex(),
                                                 cohort->GetPopulation() );
                        }
                        else
                        {
                            // now iterate over all males, there will be a slight rounding error
                            for (auto& maletypes : gender_mating_males)
                            {
                                float currentProbability = float(maletypes.second)/males;
                                uint32_t matedPop = uint32_t( currentProbability * float(cohort->GetPopulation()) );

                                AddAdultsFromMating( maletypes.first, 
                                                     cohort->GetVectorGenetics().GetIndex(),
                                                     matedPop );
                            }
                        }
                    }
                    else // male
                    {
                        cohort->SetState( VectorStateEnum::STATE_MALE );
                        queueIncrementTotalPopulation( cohort );//update counter
                        MergeProgressedCohortIntoCompatibleQueue( MaleQueues, cohort, 0.0 );
                        delete_cohort = false;
                    }
                }// new adults of age 0

                ImmatureQueues[ iCohort ] = ImmatureQueues.back();
                ImmatureQueues.pop_back();
                if( delete_cohort )
                {
                    delete cohort;
                }
                // !! Don't increment iCohort. !!
            }
            else
            {
                ++iCohort;
            }
        }
    }

    void VectorPopulation::AddAdultsFromMating( const VectorGeneticIndex_t& rVgiMale,
                                                const VectorGeneticIndex_t& rVgiFemle,
                                                uint32_t pop )
    {
        IVectorCohort* temp_cohort = VectorCohort::CreateCohort( VectorStateEnum::STATE_ADULT, 0.0, 0, pop, rVgiFemle, &species_ID );
        ApplyMatingGenetics( temp_cohort, VectorMatingStructure( rVgiMale ) );
        queueIncrementTotalPopulation( temp_cohort );
        new_adults += temp_cohort->GetPopulation();
        MergeProgressedCohortIntoCompatibleQueue( AdultQueues, temp_cohort, 0.0 );
    }


    void VectorPopulation::ApplyMatingGenetics( IVectorCohort* cohort, const VectorMatingStructure& male_vector_genetics )
    {
        // (1) Determine female fertility
        if( cohort->GetVectorGenetics().GetSterility() == VectorSterility::VECTOR_STERILE )
        {
            // Already sterile
        }
        else if( male_vector_genetics.GetSterility() == VectorSterility::VECTOR_STERILE )
        {
            // Female mated with a sterile male
            cohort->GetVectorGenetics().SetSterility(VectorSterility::VECTOR_STERILE);
        }
        else if( !VectorMatingStructure::WolbachiaCompatibleMating(cohort->GetVectorGenetics().GetWolbachia(), male_vector_genetics.GetWolbachia()) )
        {
            // Cytoplasmic incompatibility due to differing Wolbachia infections
            cohort->GetVectorGenetics().SetSterility(VectorSterility::VECTOR_STERILE);
        }

        // (2) Wolbachia retains female type
        
        // (3) Solve for pesticide resistance of mating
        cohort->GetVectorGenetics().SetPesticideResistance( cohort->GetVectorGenetics().GetPesticideResistance().first, male_vector_genetics.GetPesticideResistance().first );

        // (4) Finally define mated HEGs case
        cohort->GetVectorGenetics().SetHEG( cohort->GetVectorGenetics().GetHEG().first, male_vector_genetics.GetHEG().first );
    }

    // Seek a compatible (same gender-mating type) queue in specified list (e.g. AdultQueues, InfectiousQueues) and increase its population.
    // EAW: If we have to do this a lot, then we might consider a different type of container (e.g. map instead of vector).
#if 1
    void VectorPopulation::MergeProgressedCohortIntoCompatibleQueue( VectorCohortVector_t &queues, IVectorCohort* pvc, float progressThisTimestep )
    {
        // -----------------------------------------------------------------------
        // --- Select the cohort that is within a time step of an existing cohort.
        // --- i.e. +/- half a day
        // -----------------------------------------------------------------------
        float delta_progress = progressThisTimestep * 0.5 ;

        VectorCohortVector_t::iterator it = std::find_if( queues.begin(),
                                                          queues.end(),
                                                          [pvc, delta_progress ](IVectorCohort* cohort)
                                                          {
                                                              return (cohort->GetState()          == pvc->GetState()         ) &&
                                                                     (cohort->GetVectorGenetics() == pvc->GetVectorGenetics()) &&
                                                                     (cohort->GetAge()            == pvc->GetAge()           ) &&
                                                                     ((cohort->GetProgress() - delta_progress) <= pvc->GetProgress()) &&
                                                                     (pvc->GetProgress() <= (cohort->GetProgress() + delta_progress));
                                                          } );
        if (it != queues.end())
        {
            (*it)->Merge( pvc );
            delete pvc;
        }
        else
        {
            LOG_DEBUG_F("Creating new '%s' VectorCohort with population %d for type: %s, %s, %s, pesticide-resistance: %s-%s, HEG: %s-%s. \n", species_ID.c_str(), pvc->GetPopulation(),
                VectorGender::pairs::lookup_key(    pvc->GetVectorGenetics().GetGender() ),
                VectorSterility::pairs::lookup_key( pvc->GetVectorGenetics().GetSterility() ),
                VectorWolbachia::pairs::lookup_key( pvc->GetVectorGenetics().GetWolbachia() ),
                VectorAllele::pairs::lookup_key(    pvc->GetVectorGenetics().GetPesticideResistance().first ),
                VectorAllele::pairs::lookup_key(    pvc->GetVectorGenetics().GetPesticideResistance().second ),
                VectorAllele::pairs::lookup_key(    pvc->GetVectorGenetics().GetHEG().first ),
                VectorAllele::pairs::lookup_key(    pvc->GetVectorGenetics().GetHEG().second ) );

            queues.push_back( pvc );
        }
    }
#else
    void VectorPopulation::MergeProgressedCohortIntoCompatibleQueue( VectorCohortVector_t &queues, IVectorCohort* pvc, float progressThisTimestep )
    {
        IVectorCohort* p_found_cohort = nullptr;
        float min_delta_progress = progressThisTimestep;
        for( auto cohort : queues )
        {
            float delta_progress = fabs( cohort->GetProgress() - pvc->GetProgress() );

            if( (cohort->GetState() == pvc->GetState()) &&
                (cohort->GetVectorGenetics() == pvc->GetVectorGenetics()) &&
                (cohort->GetAge() == pvc->GetAge()) &&
                (delta_progress <= min_delta_progress) )
            {
                p_found_cohort = cohort;
                min_delta_progress = delta_progress;

                // saves about 5% overall on queues that have no progress
                if( min_delta_progress == 0.0 ) break;
            }
        }

        if( p_found_cohort != nullptr )
        {
            p_found_cohort->Merge( pvc );
            delete pvc;
        }
        else
        {
            LOG_DEBUG_F( "Creating new '%s' VectorCohort with population %d for type: %s, %s, %s, pesticide-resistance: %s-%s, HEG: %s-%s. \n", species_ID.c_str(), pvc->GetPopulation(),
                         VectorGender::pairs::lookup_key( pvc->GetVectorGenetics().GetGender() ),
                         VectorSterility::pairs::lookup_key( pvc->GetVectorGenetics().GetSterility() ),
                         VectorWolbachia::pairs::lookup_key( pvc->GetVectorGenetics().GetWolbachia() ),
                         VectorAllele::pairs::lookup_key( pvc->GetVectorGenetics().GetPesticideResistance().first ),
                         VectorAllele::pairs::lookup_key( pvc->GetVectorGenetics().GetPesticideResistance().second ),
                         VectorAllele::pairs::lookup_key( pvc->GetVectorGenetics().GetHEG().first ),
                         VectorAllele::pairs::lookup_key( pvc->GetVectorGenetics().GetHEG().second ) );

            queues.push_back( pvc );
        }
    }
#endif

    void VectorPopulation::Update_Larval_Queue( float dt )
    {
        // Use the verbose "for" construct here because we may be modifying the list
        for( size_t iCohort = 0; iCohort < LarvaQueues.size(); /* increment in loop */ )
        {
            IVectorCohort* cohort = LarvaQueues[ iCohort ];

            IVectorCohortWithHabitat *larvaentry = nullptr;
            if( cohort->QueryInterface( GET_IID( IVectorCohortWithHabitat ), (void**)&larvaentry ) != s_OK )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "larva", "IVectorCohortWithHabitat", "IVectorCohort" );
            }

            // Apply temperature and over-crowding dependent larval development
            cohort->IncreaseProgress( GetLarvalDevelopmentProgress(dt, larvaentry) );

            // Apply larval mortality, the probability of which may depend on over-crowding and Notre Dame instar-specific dynamics
            float p_larval_mortality = GetLarvalMortalityProbability(dt, larvaentry);
            uint32_t nowPop = cohort->GetPopulation();
            uint32_t newPop = nowPop - uint32_t( randgen->binomial_approx( nowPop, p_larval_mortality ) );
            LOG_VALID_F( "Adjusting larval population from %d to %d based on overcrowding considerations.\n", nowPop, newPop );
            cohort->SetPopulation( newPop );

            if ( cohort->GetProgress() >= 1 || cohort->GetPopulation() <= 0)
            {
                if ( cohort->GetPopulation() > 0)
                {
                    // Emerged larva become immature adults
                    ImmatureQueues.push_back(VectorCohort::CreateCohort( VectorStateEnum::STATE_IMMATURE, 0.0, 0, cohort->GetPopulation(), cohort->GetVectorGenetics(), &species_ID ));
                    LOG_DEBUG_F("Immature adults emerging from larva queue: population=%d, vector_genetics index=%d.\n", cohort->GetPopulation(), cohort->GetVectorGenetics().GetIndex());
                }

                LarvaQueues[ iCohort ] = LarvaQueues.back();
                LarvaQueues.pop_back();
                delete cohort;

                // !! Don't increment iCohort. !!
            }
            else
            {
                // Only counting female larva to keep egg-crowding and larval-competition calculations backward-consistent
                if(cohort->GetVectorGenetics().GetGender() == VectorGender::VECTOR_FEMALE)
                {
                    // Pass back larva in this cohort to total count in habitat
                    larvaentry->GetHabitat()->AddLarva(cohort->GetPopulation(), cohort->GetProgress());
                }

                ++iCohort;
            }
        }
    }

    float VectorPopulation::GetLarvalDevelopmentProgress(float dt, IVectorCohortWithHabitat* larva) const
    {
        // Get local larval growth modifier, which depends on larval crowding
        float locallarvalgrowthmod = 1.0; 
        
        // if density dependent delay, slow growth
        if(!(params()->vector_params->larval_density_dependence == LarvalDensityDependence::NO_DENSITY_DEPENDENCE ||
             params()->vector_params->larval_density_dependence == LarvalDensityDependence::LARVAL_AGE_DENSITY_DEPENDENT_MORTALITY_ONLY))
        {
            locallarvalgrowthmod = larva->GetHabitat()->GetLocalLarvalGrowthModifier();
        }

        // Larval development is temperature dependent
        float temperature = m_context->GetLocalWeather()->airtemperature();

        // Craig, M. H., R. W. Snow, et al. (1999).
        // "A climate-based distribution model of malaria transmission in sub-Saharan Africa."
        // Parasitol Today 15(3): 105-111.
        float progress = ( species()->aquaticarrhenius1 * exp(-species()->aquaticarrhenius2 / (temperature + CELSIUS_TO_KELVIN)) ) * dt * locallarvalgrowthmod;
        LOG_VALID_F( "%s returning %f based on temperature (%f), and growth modifier (%f).\n", __FUNCTION__, progress, temperature, locallarvalgrowthmod );

        return progress;
    }

    float VectorPopulation::GetLarvalMortalityProbability(float dt, IVectorCohortWithHabitat* larva) const
    {
        IVectorHabitat* habitat = larva->GetHabitat();

        // (1) Local larval mortality from larval competition in habitat
        // float larval_survival_weight = GetRelativeSurvivalWeight(habitat);
        IVectorCohort * vc = nullptr;
        if( larva->QueryInterface( GET_IID( IVectorCohort ), (void**)&vc ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "IVectorCohortWithHabitat", "IVectorCohort", "larva" );
        }
        float locallarvalmortality = habitat->GetLocalLarvalMortality(species()->aquaticmortalityrate, vc->GetProgress());

        // (2) Rainfall mortality
        float rainfallmortality = habitat->GetRainfallMortality();

        // (3) Artificial mortality (e.g. larvicides)
        // N.B. Larvicide mortality is modeled as instant and complete in treated habitat.
        //      May later add in survival of pupal stages, which are resistant to some larvicides.
        float artificialmortality = habitat->GetArtificialLarvalMortality();

        // Calculate total mortality probability from mortality rates by cause
        float p_larval_mortality = float(EXPCDF(-dt * (locallarvalmortality + rainfallmortality)));
        p_larval_mortality = p_larval_mortality + (1.0f - p_larval_mortality) * artificialmortality;

        LOG_VALID_F( "%s returning %f based on local larval mortality (%f), rainfall mortality (%f), and artifical mortality (%f).\n",
                     __FUNCTION__, p_larval_mortality, locallarvalmortality, rainfallmortality, artificialmortality );
        return p_larval_mortality;
    }

    void VectorPopulation::Update_Egg_Laying( float dt )
    {
        // Get total larval capacity across habitats for calculation of fractional allocation of eggs
        float total_capacity = 0;
        for (auto habitat : (*m_larval_habitats))
        {
            total_capacity += habitat->GetCurrentLarvalCapacity();
        }

        // WARNING: How do we want to deal with the allocation of integer eggs to various cohorts (by genetic ID) and habitats
        //          when the numbers get small?  Rounding to the nearest integer or just rounding down are simple, but have the
        //          potential to add up to numbers less than the total number of new eggs, if fragmented into enough cohorts.
        //          Schemes that keep count of remaining eggs to give out, and adjust allocation accordingly, will always
        //          distribute the right number of eggs, but will have systematic biases in which genetic indices get the few eggs.
        //          At present, we will just round and live with the potential bias away from completely full larval habitat.

        // Loop over larval habitats
        for (auto habitat : (*m_larval_habitats))
        {
            float fractional_allocation = habitat->GetCurrentLarvalCapacity() / total_capacity;
            habitat->AddEggs(neweggs * fractional_allocation);

            // Now lay each type of egg, laying male and female eggs, except for sterile ones
            for (auto& eggs : gender_mating_eggs)
            {
                // Use the index of the egg map to construct the vector-genetics structure
                VectorMatingStructure vms = VectorMatingStructure(eggs.first);

                // If the female was sterile, her eggs are not viable
                if( vms.GetSterility() == VectorSterility::VECTOR_STERILE )
                    continue;

                // Eggs laid in this habitat are scaled by fraction of available habitat here
                // TODO: revisit rounding issue described in warning above.
                int eggs_to_lay = eggs.second * fractional_allocation;
                if( eggs_to_lay < 1 )
                    continue;

                // Otherwise, add a female egg cohort in this larval habitat
                // If delayed hatching is enabled, these will be merged with existing egg cohorts by habitat and genetics

                // We now have the number of eggs laid in a given habitat for a given mated female genetic structure
                // We now need to separate out the eggs for each specific unmated egg type
                
                // We start with the simple case of pesticide resistance being sensitive-sensitive and HEGs being WT-WT.  This is most simulations, and it allows them to remain as fast as possible
                // This simple case also still handles all Wolbachia simulations
                // TODO: this and the pesticide-only case may not really be a noticable time advantage to single out
                if( vms.GetPesticideResistance().first == VectorAllele::WILD &&
                    vms.GetPesticideResistance().second == VectorAllele::WILD &&
                    vms.GetHEG().first == VectorAllele::WILD &&
                    vms.GetHEG().second == VectorAllele::WILD )
                {
                    VectorMatingStructure vms_egg = vms;
                    vms_egg.SetUnmated();
                    CreateEggCohortOfType(habitat, eggs_to_lay, vms_egg);
                }
                else
                {
                    // Now for the more interesting cases involving HEGs or pesticide Resistance
                    // Moved into a separate function for clarity
                    CreateEggCohortAlleleSorting( habitat, eggs_to_lay, vms );
                }
            }
        }
    }

    void VectorPopulation::CreateEggCohortOfType(IVectorHabitat* habitat, uint32_t eggs_to_lay, const VectorMatingStructure& vms_egg)
    {
        // Find if there is a compatible existing cohort of eggs
        VectorCohortVector_t::iterator itEggs = std::find_if( EggQueues.begin(), EggQueues.end(), [vms_egg, habitat](IVectorCohort* cohort) -> bool 
        {
            // If this QI gets cumbersome, nothing really prevents the EggQueues (and LarvaQueues) from being lists of the derived type pointers
            IVectorCohortWithHabitat *eggentry = nullptr;
            if( cohort->QueryInterface( GET_IID( IVectorCohortWithHabitat ), (void**)&eggentry ) != s_OK )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "cohort", "IVectorCohortWithHabitat", "IVectorCohort" );
            }

            // Compatible means with matching habitat and genetics
            return ( cohort->GetVectorGenetics() == vms_egg ) && ( eggentry->GetHabitat() == habitat );
        } );

        if ( itEggs != EggQueues.end() ) 
        { 
            (*itEggs)->SetPopulation( (*itEggs)->GetPopulation() + eggs_to_lay ); 
            LOG_VALID_F( "Laying %d eggs into existing egg queue (index=%d, habitat=%s).\n", eggs_to_lay, vms_egg.GetIndex(), VectorHabitatType::pairs::lookup_key( habitat->GetVectorHabitatType() ) );
        }
        else
        {
            EggQueues.push_back( VectorCohortWithHabitat::CreateCohort( habitat, VectorStateEnum::STATE_EGG, 0, eggs_to_lay, vms_egg, &species_ID ) );
            LOG_VALID_F( "Laying %d eggs and pushing into new egg queue (index=%d, habitat=%s).\n", eggs_to_lay, vms_egg.GetIndex(), VectorHabitatType::pairs::lookup_key( habitat->GetVectorHabitatType() ) );
        }

        // Males larvae are produced in Update_Egg_Hatching in equal proportion to female hatching eggs
    }

    void VectorPopulation::CreateEggCohortAlleleSorting( IVectorHabitat* habitat, uint32_t eggs_to_lay, VectorMatingStructure vms )
    {
        // This first Allele Sorting function focuses on pesticide resistance alleles and then calls a second function for HEG sorting, if necessary
        AlleleFractions_t fractions = VectorMatingStructure::GetAlleleFractions(vms.GetPesticideResistance());

        // now check to see if HEGs relevant
        if( vms.GetHEG().first == VectorAllele::WILD && vms.GetHEG().second == VectorAllele::WILD )
        {
            // WILD HEGs only here
            vms.SetHEG( VectorAllele::WILD, VectorAllele::NotMated );

            for (auto& fraction_by_type : fractions)
            {
                vms.SetPesticideResistance( fraction_by_type.first, VectorAllele::NotMated );
                if( fraction_by_type.second > 0 )
                {
                    // Jump straight to creating eggs
                    CreateEggCohortOfType(habitat, uint32_t(fraction_by_type.second * eggs_to_lay), vms);
                }
            }
        }
        else
        {
            // Now have to process HEGs
            for (auto& fraction_by_type : fractions)
            {
                vms.SetPesticideResistance( fraction_by_type.first, VectorAllele::NotMated );
                if( fraction_by_type.second > 0 )
                {
                    // Do the additional sorting by HEG for each pesticide-resistance type
                    CreateEggCohortHEGSorting( habitat, uint32_t(fraction_by_type.second * eggs_to_lay), vms );
                }
            }
        }
    }

    void VectorPopulation::CreateEggCohortHEGSorting( IVectorHabitat* habitat, uint32_t eggs_to_lay, VectorMatingStructure vms )
    {
        
        AlleleFractions_t fractions;
        AllelePair_t matedHEGs;
        // This is the lowest-level Egg Cohort sorting function, invoked if HEGs are active
        // check whether early or late homing, now set to early homing only
        if(params()->vector_params->heg_model == HEGModel::OFF)
        {
            fractions = VectorMatingStructure::GetAlleleFractionsEarlyHoming(vms.GetHEG(), 0);// no homing
        }else if(params()->vector_params->heg_model == HEGModel::EGG_HOMING) // late
        {
            fractions = VectorMatingStructure::GetAlleleFractions(vms.GetHEG());

            // Corrections for HEGs homing in heterozygous individuals
            float homingFraction = params()->vector_params->HEGhomingRate * fractions[VectorAllele::HALF];
            fractions[VectorAllele::HALF] -= homingFraction;
            fractions[VectorAllele::FULL] += homingFraction;
        }else if(params()->vector_params->heg_model == HEGModel::GERMLINE_HOMING) // early in females only
        {
            fractions = VectorMatingStructure::GetAlleleFractionsEarlyHoming(vms.GetHEG(), params()->vector_params->HEGhomingRate);
        }else if(params()->vector_params->heg_model == HEGModel::DUAL_GERMLINE_HOMING) // early in males only
        {
            fractions = VectorMatingStructure::GetAlleleFractionsDualEarlyHoming(vms.GetHEG(), params()->vector_params->HEGhomingRate);
        }else if(params()->vector_params->heg_model == HEGModel::DRIVING_Y) // homing is fraction of eggs that would have been female that are now male (e.g. 90% male offspring is 80% homing)
        {
            // this HEG works differently than the others
            // all males inherit the driving-Y, females born are wildtype
            // this HEG breaks the symmetry of male-female hatching in equal numbers, so this is the only time a male cohort is laid
            // when it hatches, it hatches an equivalent male cohort
            // when the residual female fraction hatches, it hatches a male cohort that caries driving-Y
            // as such, the fractions are used as follows: FULL is for females laid (they act as wild type, since the HEG is inactive in females, but the male matched hatching has the HEG
            // HALF is used to indicate below that a cohort is to be laid as males
            // if the male is WILD, then all progeny are normal proceed normally, regardless of the female
            matedHEGs = vms.GetHEG();
            if(matedHEGs.second == VectorAllele::WILD)
            {
                fractions[VectorAllele::WILD] = 1.0f;
                fractions[VectorAllele::HALF] = 0;
                fractions[VectorAllele::FULL] = 0;
            }else if(matedHEGs.second == VectorAllele::FULL)
            {
                fractions[VectorAllele::WILD] = 0;
                fractions[VectorAllele::HALF] = params()->vector_params->HEGhomingRate;
                fractions[VectorAllele::FULL] = 1.0f-params()->vector_params->HEGhomingRate;
            }else
            {
                throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__, "DRIVING_Y allele", matedHEGs.second, VectorAllele::pairs::lookup_key(matedHEGs.second) );
            }
        }

        // Corrections for HEGs fecundity
        float fecundityFactor = 1.0f;
        if(params()->vector_params->heg_model == HEGModel::OFF)
        {
            fecundityFactor = 1.0f;
        }else if(params()->vector_params->heg_model == HEGModel::DRIVING_Y)
        {
            switch( vms.GetHEG().second ) // fitness depends on male
            {
            case VectorAllele::WILD:
                fecundityFactor = 1.0f;
                break;
            case VectorAllele::HALF:
                fecundityFactor = 1.0 - params()->vector_params->HEGfecundityLimiting;
                break;
            case VectorAllele::FULL:
                fecundityFactor = 1.0 - params()->vector_params->HEGfecundityLimiting;
                break;
            case VectorAllele::NotMated: break;
            default: break;
            }
        }else{
            switch( vms.GetHEG().first )
            {
            case VectorAllele::FULL:
                // fecundity reduction assumed to act on female (just an assumption)
                fecundityFactor = 1.0 - params()->vector_params->HEGfecundityLimiting;
                break;

            case VectorAllele::HALF:
                fecundityFactor = 1.0;// in this current temporary version, heterozygous has no fitness cost
                break;
            case VectorAllele::WILD: break;
            case VectorAllele::NotMated: break;
            default: break;
            }
        }

        // Now finally lay eggs
        for (auto& fraction_by_type : fractions)
        {
            vms.SetHEG( fraction_by_type.first, VectorAllele::NotMated );
            if( fraction_by_type.second > 0 )
            {
                // Check for Driving Y male biased case here, or just proceed as normal
                if(params()->vector_params->heg_model == HEGModel::DRIVING_Y && fraction_by_type.first == VectorAllele::HALF)
                {
                    vms.SetHEG(VectorAllele::FULL, VectorAllele::NotMated);
                    vms.SetGender(VectorGender::VECTOR_MALE);
                    CreateEggCohortOfType(habitat, uint32_t(fraction_by_type.second * fecundityFactor * eggs_to_lay), vms);
                    vms.SetGender(VectorGender::VECTOR_FEMALE);
                }else
                {
                    // Apply the fecundity factor here
                    CreateEggCohortOfType(habitat, uint32_t(fraction_by_type.second * fecundityFactor * eggs_to_lay), vms);
                }
            }
        }
    }

    void VectorPopulation::Update_Egg_Hatching( float dt )
    {
        // Calculate egg-hatch delay factor
        Fraction eggHatchDelayFactor = dt;
        Fraction localdensdephatchmod = 1.0;
        NonNegativeFloat temperature = m_context->GetLocalWeather()->airtemperature();

        // These options are mutually exclusive
        if(params()->vector_params->temperature_dependent_egg_hatching)
        {
            Fraction tempdephatch = ( params()->vector_params->eggarrhenius1 * exp(-params()->vector_params->eggarrhenius2 / (temperature + CELSIUS_TO_KELVIN)) );
            eggHatchDelayFactor *= tempdephatch;
        }
        else if( params()->vector_params->egg_hatch_delay_dist != EggHatchDelayDist::NO_DELAY && params()->vector_params->meanEggHatchDelay > 0 )
        {
            eggHatchDelayFactor /= params()->vector_params->meanEggHatchDelay;
            eggHatchDelayFactor = min(float(eggHatchDelayFactor), 1.0f); // correct to avoid too many eggs
        }

        // Now process remaining eggs
        for( size_t iCohort = 0; iCohort < EggQueues.size(); /* increment in loop */ )
        {
            IVectorCohort* cohort = EggQueues[ iCohort ];

            IVectorCohortWithHabitat *eggentry = nullptr;
            if( cohort->QueryInterface( GET_IID( IVectorCohortWithHabitat ), (void**)&eggentry ) != s_OK )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "cohort", "IVectorCohortWithHabitat", "IVectorCohort" );
            }
            IVectorHabitat* habitat = eggentry->GetHabitat();

            // Potential inter-species competitive weighting
            // float egg_survival_weight = GetRelativeSurvivalWeight(habitat);

            // Calculate egg-crowding correction for these eggs based on habitat and decrease population
            if (params()->vector_params->delayed_hatching_when_habitat_dries_up)  // if delayed hatching is given, we need them to survive upon drought, thus only adjust population when there is water
            {
                if( habitat->GetCurrentLarvalCapacity() >= 1 ) // no drought
                {
                    NonNegativeFloat egg_crowding_correction = habitat->GetEggCrowdingCorrection( true );
                    uint32_t nowPop = cohort->GetPopulation();
                    uint32_t newPop = uint32_t( nowPop * egg_crowding_correction );
                    LOG_VALID_F( "Updating egg population from %d to %d with egg_crowding_correction of %f\n",
                         nowPop, newPop, float( egg_crowding_correction ) );
                    cohort->SetPopulation( newPop );
                }
                // else do nothing (in case of drought, with the dhwhdr param set, don't reduce eggs with egg_crowding_correction)

            }
            else   // otherwise, use original 'anopheles implementation'
            {
                NonNegativeFloat egg_crowding_correction = habitat->GetEggCrowdingCorrection();
                uint32_t nowPop = cohort->GetPopulation();
                uint32_t newPop = uint32_t( nowPop * egg_crowding_correction );
                LOG_VALID_F( "Updating egg population from %d to %d with egg_crowding_correction of %f\n",
                        nowPop, newPop, float( egg_crowding_correction ) );
                cohort->SetPopulation( newPop );
            }

            // Include a daily egg mortality to prevent perfect hybernation
            if (params()->vector_params->egg_mortality)
            {
                uint32_t currPop = cohort->GetPopulation();
                uint32_t newerPop = uint32_t( currPop * species()->eggsurvivalrate );
                cohort->SetPopulation( newerPop ); // (default 0.99 is based on Focks 1993)
                LOG_VALID_F( "Updating egg population due to egg mortality: old_pop = %d, new_pop = %d.\n", currPop, newerPop );
            }

            if (params()->vector_params->delayed_hatching_when_habitat_dries_up)
            {
                if (habitat->GetCurrentLarvalCapacity() < 1)
                {
                    LOG_VALID_F( "Multiplying eggHatchDelayFactor (%f) by drought delay factor from config (%f)\n", float( eggHatchDelayFactor ), params()->vector_params->droughtEggHatchDelay );
                    eggHatchDelayFactor *= params()->vector_params->droughtEggHatchDelay; //  (should be , default 1/3 is based on Focks 1993)
                }
            }

            // Get local density dependence hatching modifier, which depends on larval crowding
            // if density dependent delay, slow growth
            if (params()->vector_params->egg_hatch_density_dependence == EggHatchDensityDependence::DENSITY_DEPENDENCE)
            {
                if (habitat->GetCurrentLarvalCapacity() < 1)
                {
                    localdensdephatchmod = habitat->GetLocalLarvalGrowthModifier();
                    if (params()->vector_params->delayed_hatching_when_habitat_dries_up)
                    {
                        localdensdephatchmod = max(params()->vector_params->droughtEggHatchDelay,float(localdensdephatchmod));
                    }
                    LOG_VALID_F( "localdensdephatchmod set to %f due to egg_hatch_density_dependence = %s, larval CC = %f, larval growth mod = %f, and configurable dought egg hatch delay factor = %f.\n",
                                 float( localdensdephatchmod ),
                                 EggHatchDensityDependence::pairs::lookup_key( params()->vector_params->egg_hatch_density_dependence ),
                                 float( habitat->GetCurrentLarvalCapacity() ),
                                 float( habitat->GetLocalLarvalGrowthModifier() ),
                                 params()->vector_params->droughtEggHatchDelay
                               );
                }
            }

            //}  NOTE: For now without if statement. Need to include a seperate parameter to address this.  
            uint32_t hatched = eggHatchDelayFactor * localdensdephatchmod * cohort->GetPopulation();
            LOG_VALID_F( "temperature = %f, local density dependence modifier is %f, egg hatch delay factor is %f, current population is %d, hatched is %d.\n",
                         float(temperature),
                         float(localdensdephatchmod),
                         float(eggHatchDelayFactor),
                         cohort->GetPopulation(),
                         hatched
                     );

            if( hatched > 0 )
            {
                VectorMatingStructure vms_larva = cohort->GetVectorGenetics();
                LarvaQueues.push_back( VectorCohortWithHabitat::CreateCohort( habitat, VectorStateEnum::STATE_LARVA, 0, hatched, vms_larva, &species_ID ) );
                LOG_DEBUG_F("Hatching %d female eggs and pushing into larval queues (index=%d).\n", hatched, vms_larva.GetIndex());

                // Eggs calculations were only done for female eggs
                // So, here on hatching we will push back an equal number of male larva as well
                vms_larva.SetGender(VectorGender::VECTOR_MALE);
                LarvaQueues.push_back( VectorCohortWithHabitat::CreateCohort( habitat, VectorStateEnum::STATE_LARVA, 0, hatched, vms_larva, &species_ID ) );
                LOG_DEBUG_F("Hatching %d male eggs and pushing into larval queues (index=%d).\n", hatched, vms_larva.GetIndex());
            }

            auto nowPop = cohort->GetPopulation();
            auto newPop = nowPop - hatched;
            LOG_VALID_F( "Updating egg population from %d to %d based on hatching of %d\n", nowPop, newPop, hatched );
            cohort->SetPopulation( newPop );

            if(cohort->GetPopulation() <= 0)
            {
                EggQueues[ iCohort ] = EggQueues.back();
                EggQueues.pop_back();
                delete cohort;

                // !! Don't increment iCohort. !!
            }
            else
            {
                ++iCohort;
            }
        }
    }

    void VectorPopulation::Update_Male_Queue( float dt )
    {
        males = 0;
        // Use the verbose "foreach" construct here because empty male cohorts (e.g. old vectors) will be removed
        for( size_t iCohort = 0; iCohort < MaleQueues.size(); /* increment in loop */ )
        {
            IVectorCohort* cohort = MaleQueues[ iCohort ];

            UpdateAge( cohort, dt );

            // Convert mortality rates to mortality probability (can make age dependent)
            float p_local_male_mortality = GetLocalAdultMortalityProbability( dt, cohort );
            p_local_male_mortality = p_local_male_mortality + (1.0f - p_local_male_mortality) * probs()->outdoorareakilling_male;

            // adults die
            uint32_t new_pop = cohort->GetPopulation();
            if( (new_pop > 0) && m_VectorMortality )
            {
                uint32_t dead_mosquitos = uint32_t(randgen->binomial_approx( new_pop, p_local_male_mortality ) );
                new_pop -= dead_mosquitos;
            }
            cohort->SetPopulation( new_pop );

            if( cohort->GetPopulation() <= 0 )
            {
                MaleQueues[ iCohort ] = MaleQueues.back();
                MaleQueues.pop_back();
                cohort->Release();
                //delete cohort;

                // !! Don't increment iCohort. !!
            }
            else
            {
                queueIncrementTotalPopulation( cohort );

                ++iCohort;
            }
        }
    }

    void VectorPopulation::queueIncrementTotalPopulation( IVectorCohort* cohort )
    {
        VectorGeneticIndex_t index = cohort->GetVectorGenetics().GetIndex();
        if(cohort->GetVectorGenetics().GetGender() == VectorGender::VECTOR_FEMALE)
        {
            if( cohort->GetState() == VectorStateEnum::STATE_ADULT )
            {
                adult += cohort->GetPopulation();
                vector_genetics_adults[index] += cohort->GetPopulation();
            }
            else if( cohort->GetState() == VectorStateEnum::STATE_INFECTED )
            {
                infected +=cohort->GetPopulation();
                vector_genetics_infected[index] += cohort->GetPopulation();
            }
            else if( cohort->GetState() == VectorStateEnum::STATE_INFECTIOUS )
            {
                infectious += cohort->GetPopulation();
                vector_genetics_infectious[index] += cohort->GetPopulation();
            }
        }
        else
        {
            males +=cohort->GetPopulation();
            gender_mating_males[index] += cohort->GetPopulation();
        }
    }

    void VectorPopulation::AddVectors( const VectorMatingStructure& _vector_genetics, uint32_t releasedNumber )
    {
        // Insert into correct Male or Female list
        if (_vector_genetics.GetGender() == VectorGender::VECTOR_FEMALE)
        {
            // If unmated, put in Immature with progress 1, so that the females can mate with the local male population.
            // This will throw an exception if only one of the extra fields is mated.
            if( !_vector_genetics.IsMated() )
            {
                VectorCohort* tempentry = VectorCohort::CreateCohort( VectorStateEnum::STATE_IMMATURE, 0.0, 1, releasedNumber, _vector_genetics, &species_ID );
                ImmatureQueues.push_back(tempentry);
            }
            else
            { 
                AddVectors_Adults( _vector_genetics, releasedNumber );
            }
        }
        else
        {
            // progress is 1 since males should be at 1 from progressing from Immature to Male
            VectorCohort* tempentry = VectorCohort::CreateCohort( VectorStateEnum::STATE_MALE, 0.0, 1.0, releasedNumber, _vector_genetics, &species_ID );
            MaleQueues.push_back( tempentry );
            queueIncrementTotalPopulation( tempentry );//update counter
        }

        LOG_INFO_F( "We added %lu '%s' mosquitoes of type: %s, %s, %s, pesticide-resistance: %s-%s, HEG: %s-%s. \n", releasedNumber, species_ID.c_str(), 
                    VectorGender::pairs::lookup_key(_vector_genetics.GetGender()), 
                    VectorSterility::pairs::lookup_key(_vector_genetics.GetSterility()), 
                    VectorWolbachia::pairs::lookup_key(_vector_genetics.GetWolbachia()), 
                    VectorAllele::pairs::lookup_key(_vector_genetics.GetPesticideResistance().first), 
                    VectorAllele::pairs::lookup_key(_vector_genetics.GetPesticideResistance().second), 
                    VectorAllele::pairs::lookup_key(_vector_genetics.GetHEG().first), 
                    VectorAllele::pairs::lookup_key(_vector_genetics.GetHEG().second) );
    }

    void VectorPopulation::AddVectors_Adults( const VectorMatingStructure& _vector_genetics, uint32_t releasedNumber )
    {
        // already mated, so go in AdultQueues
        VectorCohort* tempentry = VectorCohort::CreateCohort( VectorStateEnum::STATE_ADULT, 0.0, 0, releasedNumber, _vector_genetics, &species_ID );
        AdultQueues.push_back( tempentry );
        queueIncrementTotalPopulation( tempentry );//update counter
        new_adults += tempentry->GetPopulation();
    }

    uint32_t PrngForShuffle( uint32_t N )
    {
        uint32_t r = EnvPtr->RNG->uniformZeroToN( N );
        return r;
    }

    // Return a vector that contains a randomly order set of indexes that are from 0 to (N-1)
    std::vector<uint32_t> VectorPopulation::GetRandomIndexes( uint32_t N )
    {
        // Fill vector with indexes
        std::vector<uint32_t> selected_indexes;
        for( uint32_t index = 0 ; index < N ; ++index )
        {
            selected_indexes.push_back( index );
        }

        // Randomly shuffle the entries of the vector
        std::random_shuffle( selected_indexes.begin(), selected_indexes.end(), PrngForShuffle );

        return selected_indexes;
    }

    void VectorPopulation::Vector_Migration( float dt, IMigrationInfo* pMigInfo, VectorCohortVector_t* pMigratingQueue)
    {
        release_assert( pMigInfo );
        release_assert( pMigratingQueue );

        // -------------------------------------------------------------------
        // --- NOTE: r_cdf is a probability cumulative distribution function.
        // --- This means it is an array in ascending order such that
        // --- the first value is >= zero and the last value is equal to one.
        // --- The rates are converted to probabilities when calcualting the CDF.
        // --- Here we convert them back to rates.
        // -------------------------------------------------------------------
        float                                   total_rate        = pMigInfo->GetTotalRate();
        const std::vector<float              >& r_cdf             = pMigInfo->GetCumulativeDistributionFunction();
        const std::vector<suids::suid        >& r_reachable_nodes = pMigInfo->GetReachableNodes();
        const std::vector<MigrationType::Enum>& r_migration_types = pMigInfo->GetMigrationTypes();

        if( (r_cdf.size() == 0) || (total_rate == 0.0) )
        {
            return;
        }

        float total_fraction_traveling = 1.0 - exp( -1.0 * total_rate );  // preserve absolute fraction travelling
        std::vector<float> fraction_traveling;
        fraction_traveling.push_back( r_cdf[ 0 ] * total_fraction_traveling );  // apportion fraction to destinations
        for( int i = 1; i < r_cdf.size(); ++i )
        {
            float prob = r_cdf[ i ] - r_cdf[ i - 1 ];
            fraction_traveling.push_back( prob * total_fraction_traveling );
        }
        release_assert( fraction_traveling.size() == r_reachable_nodes.size() );

        std::vector<uint32_t> random_indexes = GetRandomIndexes( r_reachable_nodes.size() );

        Vector_Migration_Queue( random_indexes, r_reachable_nodes, r_migration_types, fraction_traveling, pMigratingQueue, AdultQueues      );
        Vector_Migration_Queue( random_indexes, r_reachable_nodes, r_migration_types, fraction_traveling, pMigratingQueue, InfectedQueues   );
        Vector_Migration_Queue( random_indexes, r_reachable_nodes, r_migration_types, fraction_traveling, pMigratingQueue, InfectiousQueues );
    }

    void VectorPopulation::Vector_Migration_Queue( const std::vector<uint32_t>& rRandomIndexes,
                                                   const std::vector<suids::suid>& rReachableNodes,
                                                   const std::vector<MigrationType::Enum>& rMigrationTypes,
                                                   const std::vector<float>& rFractionTraveling,
                                                   VectorCohortVector_t* pMigratingQueue,
                                                   VectorCohortVector_t& rQueue )
    {
        for( size_t iCohort = 0; iCohort < rQueue.size(); ++iCohort )
        {
            IVectorCohort* p_vc = rQueue[ iCohort ];

            std::vector<uint64_t> travelers = EnvPtr->RNG->multinomial_approx( p_vc->GetPopulation(), rFractionTraveling );

            for( uint32_t index : rRandomIndexes )
            {
                suids::suid         to_node       = rReachableNodes[ index ];
                MigrationType::Enum mig_type      = rMigrationTypes[ index ];
                uint32_t            num_travelers = travelers[ index ];
                
                if( num_travelers > 0 )
                {
                    if( num_travelers > p_vc->GetPopulation() )
                    {
                        num_travelers = p_vc->GetPopulation();
                    }

                    IVectorCohort* p_traveling_vc = p_vc->Split( num_travelers );

                    IMigrate* emigre = p_traveling_vc->GetIMigrate();
                    emigre->SetMigrating( to_node, mig_type, 0.0, 0.0, false );
                    pMigratingQueue->push_back( p_traveling_vc );
                }
            }
        }

        for( size_t iCohort = 0; iCohort < rQueue.size(); /* index loop */ )
        {
            if( rQueue[ iCohort ]->GetPopulation() <= 0 )
            {
                IVectorCohort* pvc = rQueue[ iCohort ];
                delete pvc;

                rQueue[ iCohort ] = rQueue.back();
                rQueue.pop_back();
            }
            else
            {
                ++iCohort;
            }
        }
    }

    void VectorPopulation::AddImmigratingVector( IVectorCohort* pvc )
    {
        switch( pvc->GetState() )
        {
            case VectorStateEnum::STATE_ADULT:
                MergeProgressedCohortIntoCompatibleQueue( AdultQueues, pvc, 0.0 );
                break;

            case VectorStateEnum::STATE_INFECTED:
                MergeProgressedCohortIntoCompatibleQueue( InfectedQueues, pvc, infected_progress_this_timestep );
                break;

            case VectorStateEnum::STATE_INFECTIOUS:
                MergeProgressedCohortIntoCompatibleQueue( InfectiousQueues, pvc, 0.0 );
                break;

            default:
                throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__, "IVectorCohort::GetState()", pvc->GetState(), VectorStateEnum::pairs::lookup_key( pvc->GetState() ) );
        }
    }

    void VectorPopulation::Expose( const IContagionPopulation* cp, float dt, TransmissionRoute::Enum transmission_route )
    {
        /* Strain-tracking exposure pattern only used in VectorPopulationIndividual at the moment */
    }

    float VectorPopulation::GetEIRByPool(VectorPoolIdEnum::Enum pool_id) const
    {
        switch(pool_id)
        {
        case VectorPoolIdEnum::INDOOR_VECTOR_POOL:
            return m_EIR_by_pool.first;

        case VectorPoolIdEnum::OUTDOOR_VECTOR_POOL:
            return m_EIR_by_pool.second;

        case VectorPoolIdEnum::BOTH_VECTOR_POOLS:
            return m_EIR_by_pool.first + m_EIR_by_pool.second;

        default:
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "VectorPopulation::GetEIRByPool is only valid for indoor/outdoor/combined biting." );
        }
    }

    float VectorPopulation::GetHBRByPool(VectorPoolIdEnum::Enum pool_id) const
    {
        switch(pool_id)
        {
        case VectorPoolIdEnum::INDOOR_VECTOR_POOL:
            return m_HBR_by_pool.first;

        case VectorPoolIdEnum::OUTDOOR_VECTOR_POOL:
            return m_HBR_by_pool.second;

        case VectorPoolIdEnum::BOTH_VECTOR_POOLS:
            return m_HBR_by_pool.first + m_HBR_by_pool.second;

        default:
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "VectorPopulation::GetHBRByPool is only valid for indoor/outdoor/combined biting." );
        }
    }

    float VectorPopulation::GetRelativeSurvivalWeight(VectorHabitat* habitat) const
    {
        float survival_weight = 1.0f;

#if 0
        // Modified for inter-species competitive advantage for egg-crowding and larval mortality in case of shared habitat
        if( params()->vector_params->enable_vector_species_habitat_competition )
        {
            float max_capacity = habitat->GetMaximumLarvalCapacity();
            survival_weight = (max_capacity > 0) ? ( m_larval_capacities.at(habitat->GetVectorHabitatType()) / max_capacity ) : 0;
        }
#endif

        return survival_weight;
    }

    uint32_t VectorPopulation::getInfectedCount( IStrainIdentity* pStrain ) const
    {
        uint32_t num_infected = infected;
        if( pStrain != nullptr )
        {
            num_infected = 0;
            for( auto p_cohort : InfectedQueues )
            {
                if( p_cohort->GetStrainIdentity().GetGeneticID() == pStrain->GetGeneticID() )
                {
                    num_infected += p_cohort->GetPopulation();
                }
            }
        }
        return num_infected;
    }

    uint32_t VectorPopulation::getInfectiousCount( IStrainIdentity* pStrain ) const
    {
        uint32_t num_infectious = infectious;
        if( pStrain != nullptr )
        {
            num_infectious = 0;
            for( auto p_cohort : InfectiousQueues )
            {
                if( p_cohort->GetStrainIdentity().GetGeneticID() == pStrain->GetGeneticID() )
                {
                    num_infectious += p_cohort->GetPopulation();
                }
            }
        }
        return num_infectious;
    }

    uint32_t VectorPopulation::getAdultCount()       const { return adult;      }
    uint32_t VectorPopulation::getMaleCount()        const { return males;      }
    uint32_t VectorPopulation::getNewEggsCount()     const { return neweggs;    }

    uint32_t VectorPopulation::getNewAdults()                   const { return new_adults;              }
    uint32_t VectorPopulation::getNumDiedBeforeFeeding()        const { return dead_mosquitoes_before;  }
    uint32_t VectorPopulation::getNumDiedDuringFeedingIndoor()  const { return dead_mosquitoes_indoor;  }
    uint32_t VectorPopulation::getNumDiedDuringFeedingOutdoor() const { return dead_mosquitoes_outdoor; }

    double  VectorPopulation::getInfectivity()      const  { return infectivity; }

    const std::string& VectorPopulation::get_SpeciesID() const { return species_ID; }

    const VectorHabitatList_t& VectorPopulation::GetHabitats() const  { return (*m_larval_habitats); }
    
    const VectorSpeciesParameters* GetVectorSpeciesParameters( const std::string& species_name )
    {
        try
        {
            return GET_CONFIGURABLE(SimulationConfig)->vector_params->vspMap.at( species_name );
        }
        catch( const std::out_of_range& )
        {
            std::stringstream msg;
            msg << "VectorPopulation of species '" << species_name.c_str() << "' not found in configuration. ";
            msg << "Add '" << species_name.c_str() << "' to Vector_Species_Params in configuration or, ";
            msg << "if using serialized populations, remove vector populations of this species.\n";
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }
    }

    void VectorPopulation::SetContextTo(INodeContext *context)
    {
        m_context = context;
        
        LOG_DEBUG_F( "Creating VectorSpeciesParameters for %s and suid=%d\n", species_ID.c_str(), context->GetSuid().data);
        m_species_params = GetVectorSpeciesParameters( species_ID );

        // Query for vector node context
        IVectorNodeContext* ivnc = nullptr;
        if (s_OK !=  context->QueryInterface(GET_IID(IVectorNodeContext), (void**)&ivnc))
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "IVectorNodeContext", "INodeContext" );
        }

        // Set pointer to shared vector lifecycle probabilities container.
        // The species-independent probabilities, e.g. dependent on individual-human vector-control interventions, are updated once by the node
        // The species-specific probabilities are overwritten when FinalizeTransitionProbabilites is called in Update_Lifecycle_Probabilities
        m_probabilities  = ivnc->GetVectorLifecycleProbabilities();

        m_larval_habitats = ivnc->GetVectorHabitatsBySpecies( species_ID );

        // For each cohort in EggQueues, look at it's habitat type and (re)wire it to the correct habitat instance.
        for ( auto cohort : EggQueues )
        {
            auto with_habitat = dynamic_cast<IVectorCohortWithHabitat*>(cohort);
            with_habitat->SetHabitat( ivnc->GetVectorHabitatBySpeciesAndType( species_ID, with_habitat->GetHabitatType(), nullptr ) );
        }

        // For each cohort in LarvaQueues, look at it's habitat type and (re)wire it to the correct habitat instance.
        for ( auto cohort : LarvaQueues )
        {
            auto with_habitat = dynamic_cast<IVectorCohortWithHabitat*>(cohort);
            with_habitat->SetHabitat( ivnc->GetVectorHabitatBySpeciesAndType( species_ID, with_habitat->GetHabitatType(), nullptr ) );
        }
    }

    std::vector<uint64_t> VectorPopulation::GetNewlyInfectedVectorIds() const
    {
        throw NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "Only supported in individual vector model." );
    }

    std::vector<uint64_t> VectorPopulation::GetInfectiousVectorIds() const
    {
        throw NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "Only supported in individual vector model." );
    }

    const SimulationConfig* VectorPopulation::params()  const { return GET_CONFIGURABLE(SimulationConfig); }

    const infection_list_t& VectorPopulation::GetInfections() const
    {
        throw NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "Only supported in individual vector model." );
    }

    float VectorPopulation::GetInterventionReducedAcquire() const
    {
        throw NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "Only supported in individual vector model." );
    }

    REGISTER_SERIALIZABLE(VectorPopulation);

    void VectorPopulation::serialize(IArchive& ar, VectorPopulation* obj)
    {
        VectorPopulation& population = *obj;
//        ar.labelElement("m_larval_habitats") & population.m_larval_habitats;  // NodeVector owns this information.
        ar.labelElement("m_larval_capacities"); Kernel::serialize(ar, population.m_larval_capacities);
        ar.labelElement("neweggs") & population.neweggs;
        ar.labelElement("adult") & population.adult;
        ar.labelElement("infected") & population.infected;
        ar.labelElement("infectious") & population.infectious;
        ar.labelElement("males") & population.males;
        ar.labelElement("dryheatmortality") & population.dryheatmortality;
        ar.labelElement("infectiouscorrection") & population.infectiouscorrection;
        ar.labelElement("indoorinfectiousbites") & population.indoorinfectiousbites;
        ar.labelElement("outdoorinfectiousbites") & population.outdoorinfectiousbites;
        ar.labelElement("indoorbites") & population.indoorbites;
        ar.labelElement("outdoorbites") & population.outdoorbites;
        ar.labelElement("infectivity") & population.infectivity;
        ar.labelElement("infectivity_indoor") & population.infectivity_indoor;
        ar.labelElement("infectivity_outdoor") & population.infectivity_outdoor;
        ar.labelElement("m_EIR_by_pool"); Kernel::serialize(ar, population.m_EIR_by_pool);
        ar.labelElement("m_HBR_by_pool"); Kernel::serialize(ar, population.m_HBR_by_pool);
        ar.labelElement("gender_mating_eggs"); Kernel::serialize(ar, population.gender_mating_eggs);
        ar.labelElement("gender_mating_males"); Kernel::serialize(ar, population.gender_mating_males);
        ar.labelElement("vector_genetics_adults"); Kernel::serialize(ar, population.vector_genetics_adults);
        ar.labelElement("vector_genetics_infected"); Kernel::serialize(ar, population.vector_genetics_infected);
        ar.labelElement("vector_genetics_infectious"); Kernel::serialize(ar, population.vector_genetics_infectious);
        ar.labelElement("species_ID") & population.species_ID;
        ar.labelElement("EggQueues") & population.EggQueues;
        ar.labelElement("LarvaQueues") & population.LarvaQueues;
        ar.labelElement("ImmatureQueues") & population.ImmatureQueues;
        ar.labelElement("AdultQueues") & population.AdultQueues;
        ar.labelElement("InfectedQueues") & population.InfectedQueues;
        ar.labelElement("InfectiousQueues") & population.InfectiousQueues;
        ar.labelElement("MaleQueues") & population.MaleQueues;
        // Species parameters and probabilities will be set in SetContextTo()
        // ar.labelElement("m_species_params"); VectorSpeciesParameters::serialize(ar, const_cast<VectorSpeciesParameters*&>(population.m_species_params));
        // ar.labelElement("m_probabilities"); VectorProbabilities::serialize(ar, population.m_probabilities);
        ar.labelElement("m_VectorMortality") & population.m_VectorMortality;

        // do not need to serialize these since they are updated each time step before they are used
        //m_LocalMortalityProbabilityTable()
        //m_DefaultLocalMortalityProbability( 1.0 )
    }

    void serialize(IArchive& ar, std::pair<float, float>& pair)
    {
        ar.startObject();
            ar.labelElement("first") & pair.first;
            ar.labelElement("second") & pair.second;
        ar.endObject();
    }

    void serialize(IArchive& ar, std::map<uint32_t, int>& mapping)
    {
        size_t count = ar.IsWriter() ? mapping.size() : -1;
        ar.startArray(count);
        if (ar.IsWriter())
        {
            for (auto entry : mapping)
            {
                ar.startObject();
                    ar.labelElement("key") & (uint32_t&)entry.first;
                    ar.labelElement("value") & entry.second;
                ar.endObject();
            }
        }
        else
        {
            for (size_t i = 0; i < count; ++i)
            {
                uint32_t key;
                int value;
                ar.startObject();
                    ar.labelElement("key") & key;
                    ar.labelElement("value") & value;
                ar.endObject();
                mapping[key] = value;
            }
        }
        ar.endArray();
    }
}
