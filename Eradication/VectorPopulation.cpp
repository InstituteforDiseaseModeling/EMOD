/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"
#include "VectorPopulation.h"

#include "Debug.h"
#include "Exceptions.h"
#include "Log.h"
#include "IndividualVector.h"
#include "Interventions.h"
#include "NodeVector.h"
#include "NodeVectorEventContext.h"
#include "TransmissionGroupMembership.h"
#include "SimulationConfig.h"
#include "Vector.h"
#include "VectorCohortAging.h"
#include "VectorCohortIndividual.h"
#include "VectorCohortWithHabitat.h"
#include "SimulationConfig.h"

#ifdef randgen
#undef randgen
#endif
#include "RANDOM.h"
#define randgen (m_context->GetRng())

static const char * _module = "VectorPopulation";

namespace Kernel
{
#ifndef WIN32
    #define min(a,b) ( (a<b) ? a : b )
#endif
    // QI stuff
    BEGIN_QUERY_INTERFACE_BODY(VectorPopulation)
        HANDLE_INTERFACE(IVectorPopulation)
        HANDLE_INTERFACE(IInfectable)
        HANDLE_INTERFACE(IVectorReportingOperations)
        HANDLE_ISUPPORTS_VIA(IVectorPopulation)
    END_QUERY_INTERFACE_BODY(VectorPopulation)

    VectorPopulation::VectorPopulation() :
        species_ID("gambiae"),
        animalfeed_eggbatchmod(1.0f),
        ADfeed_eggbatchmod(1.0f),
        neweggs(0),
        adult(0),
        infected(0),
        infectious(0),
        males(0),
        dryheatmortality(0.0f),
        localadultmortality(0.0f),
        infectiouscorrection(0.0f),
        indoorinfectiousbites(0.0f),
        outdoorinfectiousbites(0.0f),
        indoorbites(0.0f),
        outdoorbites(0.0f),
        infectivity(0.0),
        infectivity_indoor(0.0f),
        infectivity_outdoor(0.0f),
        m_context(NULL),
        m_species_params(NULL),
        m_probabilities(NULL)
    {
    }

    void VectorPopulation::Initialize(INodeContext *context, std::string species_name, unsigned int adults, unsigned int _infectious)
    {
        SetContextTo(context);

        species_ID = species_name;
        LOG_DEBUG_F( "Creating VectorSpeciesParameters for %s and suid=%d\n", species_name.c_str(), context->GetSuid().data);
        m_species_params = VectorSpeciesParameters::_vspMap[ species_name ];

        // Query for vector node context
        IVectorNodeContext* ivnc = NULL;
        if (s_OK !=  context->QueryInterface(GET_IID(IVectorNodeContext), (void**)&ivnc))
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "IVectorNodeContext", "INodeContext" );
        }

        // Set pointer to shared vector lifecycle probabilities container.
        // The species-independent probabilities, e.g. dependent on individual-human vector-control interventions, are updated once by the node
        // The species-specific probabilities are overwritten when FinalizeTransitionProbabilites is called in Update_Lifecycle_Probabilities
        m_probabilities  = ivnc->GetVectorLifecycleProbabilities();

        // Correct diepostfeeding and successfulfeed for infectious, which have infectioushfmortmod
        // adjusted for increased feeding mortality due to longer probing
        // Wekesa, J. W., R. S. Copeland, et al. (1992). "Effect of Plasmodium Falciparum on Blood Feeding Behavior of Naturally Infected Anopheles Mosquitoes in Western Kenya." Am J Trop Med Hyg 47(4): 484-488.
        // ANDERSON, R. A., B. G. J. KNOLS, et al. (2000). "Plasmodium falciparum sporozoites increase feeding-associated mortality of their mosquito hosts Anopheles gambiae s.l." Parasitology 120(04): 329-333.
        infectiouscorrection = 0;
        if (params()->human_feeding_mortality * species()->infectioushfmortmod < 1.0)
        {
            infectiouscorrection = (float)( (1.0 - params()->human_feeding_mortality * species()->infectioushfmortmod) / (1.0 - params()->human_feeding_mortality) );
        }

        // Set up initial populations of adult/infectious/male mosquitoes
        InitializeVectorQueues(adults, _infectious);
    }

    void VectorPopulation::InitializeVectorQueues(unsigned int adults, unsigned int _infectious)
    {
        // Check initial settings against legacy version.
        adult      = adults;
        infectious = _infectious;
        males      = adults;

        AdultQueues.push_front( VectorCohort::CreateCohort( 0, adults, VectorMatingStructure( VectorGender::VECTOR_FEMALE ) ) );
        InfectiousQueues.push_front( VectorCohort::CreateCohort( 0, infectious, VectorMatingStructure( VectorGender::VECTOR_FEMALE ) ) );
        MaleQueues.push_front( VectorCohort::CreateCohort( 0, males, VectorMatingStructure( VectorGender::VECTOR_MALE ) ) );
    }

    VectorPopulation *VectorPopulation::CreatePopulation(INodeContext *context, std::string species_name, unsigned int adults, unsigned int infectious)
    {
        VectorPopulation *newpopulation = _new_ VectorPopulation();
        newpopulation->Initialize(context, species_name, adults, infectious);
        return newpopulation;
    }

    void VectorPopulation::SetupIntranodeTransmission(ITransmissionGroups *transmissionGroups)
    {
        m_transmissionGroups = transmissionGroups;
    }

    void VectorPopulation::SetupLarvalHabitat( INodeContext *context )
    {
        // Query for vector node context
        IVectorNodeContext* ivnc = NULL;
        if (s_OK !=  context->QueryInterface(GET_IID(IVectorNodeContext), (void**)&ivnc))
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "IVectorNodeContext", "INodeContext" );
        }

        // Create a larval habitat for each type specified in configuration file for this species
        for( int ihabitat = 0; ihabitat < species()->habitat_type.size(); ihabitat++ )
        {
            VectorHabitatType::Enum type = species()->habitat_type[ihabitat];
            float max_larval_capacity = species()->habitat_param[ihabitat] * params()->x_templarvalhabitat * ivnc->GetLarvalHabitatMultiplier(type);
            VectorHabitat* habitat = NULL;

#if 0
            if( params()->enable_vector_species_habitat_competition )
            {
                habitat = ivnc->GetVectorHabitatByType(type);

                // While the VectorHabitats are hooked up properly for this option,
                // the inter-species competition is not, so let's throw an exception:
                throw NotYetImplementedException(__FILE__, __LINE__, __FUNCTION__, "Shared-habitat inter-species competition not completely hooked up yet.");
            }
#endif

            if(habitat == NULL)
            {
                // If this larval habitat type doesn't exist yet,
                // or if the different species will not be competing for larval habitat,
                // create a new habitat pointer and push it back to the node
                habitat = VectorHabitat::CreateHabitat(type, max_larval_capacity);
                ivnc->AddVectorHabitat(habitat);
            }
            else
            {
                habitat->IncrementMaxLarvalCapacity(max_larval_capacity);
            }

            // Keep a local pointer, too
            m_larval_habitats.push_back(habitat);

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

        // memory allocated to VectorHabitat pointers in m_larval_habitats
        // is cleared in the NodeVector destructor.  TODO: shared_ptr?
    }

    void VectorPopulation::UpdateVectorPopulation( float dt )
    {
        // Reset EIR/HBR reporting
        m_EIR_by_pool = std::make_pair(0.0f, 0.0f);
        m_HBR_by_pool = std::make_pair(0.0f, 0.0f);

        // Reset counters
        neweggs                = 0;
        indoorinfectiousbites  = 0;
        outdoorinfectiousbites = 0;
        indoorbites            = 0;
        outdoorbites           = 0;

        gender_mating_eggs.clear();
        gender_mating_males.clear();
        vector_genetics_adults.clear();
        vector_genetics_infected.clear();
        vector_genetics_infectious.clear();

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
            m_EIR_by_pool.first  = (float)(indoorinfectiousbites / eff_pop);
            m_EIR_by_pool.second = (float)(outdoorinfectiousbites / eff_pop);
            // (2) total human bites (HBR)
            m_HBR_by_pool.first  = (float)(indoorbites / eff_pop);
            m_HBR_by_pool.second = (float)(outdoorbites / eff_pop);
        }
        else
        {
            LOG_WARN_F("The effective human population at node %lu is zero, so EIR and HBR are not being normalized in VectorPopulation::UpdateVectorPopulation.\n", m_context->GetSuid().data );
        }
        // (3) vector-to-human infectivity
        infectivity = GetEIRByPool(VectorPoolIdEnum::BOTH_VECTOR_POOLS) * species()->transmissionmod;
    }

    void VectorPopulation::Update_Lifecycle_Probabilities( float dt )
    {
        // Update adult transition probabilities:
        // initial decision tree starting after localadultmortality
        // calculated for each queue entry up to human indoor and outdoor feeding attempts
        probs()->FinalizeTransitionProbabilites( species()->anthropophily, species()->indoor_feeding); // TODO: rename this function now??

        // Update local adult mortality rate
        float temperature   = m_context->GetLocalWeather()->airtemperature();
        dryheatmortality    = dryHeatMortality(temperature);
        localadultmortality = species()->adultmortality + dryheatmortality;
    }

    void VectorPopulation::Update_Infectious_Queue( float dt )
    {
        infectious = 0;
        for (auto cohort : InfectiousQueues)
        {
            ProcessFeedingCycle(dt, cohort, VectorStateEnum::STATE_INFECTIOUS);

            if (cohort->GetPopulation() <= 0)
            {
                cohort->SetPopulation(  0 );
                // don't delete reusable queue
            }

            queueIncrementTotalPopulation(cohort, VectorStateEnum::STATE_INFECTIOUS);
        }
    }

    void VectorPopulation::Update_Infected_Queue( float dt )
    {
        infected = 0;
        float temperature = m_context->GetLocalWeather()->airtemperature();

        // Use the verbose "foreach" construct here because empty or completely-progressed queues will be removed
        for ( VectorCohortList_t::iterator iInfected = InfectedQueues.begin(); iInfected != InfectedQueues.end(); )
        {
            VectorCohort *cohort = (*iInfected);
            release_assert( cohort );

            // progress with sporogony
            cohort->IncreaseProgress( (species()->infectedarrhenius1 * exp(-species()->infectedarrhenius2 / (temperature + CELSIUS_TO_KELVIN))) * dt );

            ProcessFeedingCycle(dt, cohort, VectorStateEnum::STATE_INFECTED);

            // done with this queue if it is fully progressed or is empty
            if (cohort->GetProgress() >= 1 || cohort->GetPopulation() <= 0)
            {
                // infected queue completion, moving to infectious
                if (cohort->GetPopulation() > 0)
                {
                    queueIncrementTotalPopulation(cohort, VectorStateEnum::STATE_INFECTIOUS); // update INFECTIOUS counters

                    // Seek a compatible (same gender mating type) infectious queue and increase its population.
                    MergeProgressedCohortIntoCompatibleQueue(InfectiousQueues, cohort->GetPopulation(), cohort->GetVectorGenetics());
                }

                iInfected = InfectedQueues.erase(iInfected);
                delete cohort;
            }
            else
            {
                queueIncrementTotalPopulation(cohort, VectorStateEnum::STATE_INFECTED); // update INFECTED counters
                iInfected++;
            }
        }
    }

    void VectorPopulation::Update_Adult_Queue( float dt )
    {
        VectorCohort* tempentrynew;
        adult = 0;
        for (auto cohort : AdultQueues)
        {
            LOG_DEBUG_F("In adult queue: %d \t %s \t %s -- %s", cohort->GetPopulation(), VectorWolbachia::pairs::lookup_key(cohort->GetVectorGenetics().GetWolbachia()), VectorAllele::pairs::lookup_key(cohort->GetVectorGenetics().GetHEG().first), VectorAllele::pairs::lookup_key(cohort->GetVectorGenetics().GetHEG().second));
            uint32_t newinfected = ProcessFeedingCycle(dt, cohort, VectorStateEnum::STATE_ADULT);
            // correct for too high
            if (newinfected > (uint32_t)cohort->GetPopulation())
            {
                newinfected = cohort->GetPopulation();
            }

            if (newinfected > 0)
            {
                cohort->SetPopulation( cohort->GetPopulation() - newinfected );
                tempentrynew = VectorCohort::CreateCohort(0, newinfected, cohort->GetVectorGenetics());
                InfectedQueues.push_back(tempentrynew);
                queueIncrementTotalPopulation(tempentrynew, VectorStateEnum::STATE_INFECTED); // update INFECTED counters
            }

            if (cohort->GetPopulation() < 0)
            {
                cohort->SetPopulation(  0 );
            }

            queueIncrementTotalPopulation(cohort, VectorStateEnum::STATE_ADULT); // update ADULT counters
        }
    }

    uint32_t VectorPopulation::ProcessFeedingCycle(float dt, VectorCohort *queue, VectorStateEnum::Enum state)
    {
        // start of outcome calculation
        unsigned long int initPop = queue->GetPopulation();
        unsigned long int dead_mosquitoes        = 0;
        unsigned long int survived_wo_feeding    = 0;
        unsigned long int successful_animal_feed = 0;
        unsigned long int successful_AD_feed     = 0;
        unsigned long int attempt_indoor_feed    = 0;
        unsigned long int human_outdoor_feed     = 0;
        unsigned long int successful_human_feed  = 0;

        // reset counters
        float cumulative_probability = 0;
        uint32_t newinfected = 0;

        // pesticide resistance


        if (initPop <= 0) 
            return newinfected;

        // Adjust human-feeding mortality for longer-probing infectious vectors
        // Wekesa, J. W., R. S. Copeland, et al. (1992). "Effect of Plasmodium Falciparum on Blood Feeding Behavior of Naturally Infected Anopheles Mosquitoes in Western Kenya." Am J Trop Med Hyg 47(4): 484-488.
        // ANDERSON, R. A., B. G. J. KNOLS, et al. (2000). "Plasmodium falciparum sporozoites increase feeding-associated mortality of their mosquito hosts Anopheles gambiae s.l." Parasitology 120(04): 329-333.
        float x_infectioushfmortmod  = (state == VectorStateEnum::STATE_INFECTIOUS) ? (float) species()->infectioushfmortmod  : 1.0f;
        float x_infectiouscorrection = (state == VectorStateEnum::STATE_INFECTIOUS) ? (float) infectiouscorrection : 1.0f;

        //Wolbachia related impacts on mortality and infection susceptibility
        float x_mortalityWolbachia = 1.0;
        float x_infectionWolbachia = 1.0; 
        if( queue->GetVectorGenetics().GetWolbachia() != VectorWolbachia::WOLBACHIA_FREE )
        {
            x_mortalityWolbachia = params()->WolbachiaMortalityModification;
            x_infectionWolbachia = params()->WolbachiaInfectionModification;
        }

        // Oocysts, not sporozoites affect egg batch size:
        // Hogg, J. C. and H. Hurd (1997). "The effects of natural Plasmodium falciparum infection on the fecundity and mortality of Anopheles gambiae s. l. in north east Tanzania." Parasitology 114(04): 325-331.
        float x_infectedeggbatchmod  = (state == VectorStateEnum::STATE_INFECTED)   ? (float) species()->infectedeggbatchmod  : 1.0f;

        // calculate local mortality, convert rate to probability
        float p_local_mortality = (float)EXPCDF(-dt * localadultmortality * x_mortalityWolbachia);

        // calculate feeding rate, either by temperature or by species parameter
        float feedingrate = (params()->temperature_dependent_feeding_cycle) ? 1.0f / GetFeedingCycleDurationByTemperature() : species()->feedingrate;

        // die before human feeding
        unsigned long int remainingPop = initPop;
        float temp_probability = p_local_mortality + (1.0f - p_local_mortality) * feedingrate * dt * probs()->diebeforeattempttohumanfeed + (1.0f - p_local_mortality) * (1.0f - species()->feedingrate * dt) * probs()->diewithoutattemptingfeed;
        unsigned long int temp_number = (uint32_t)randgen->binomial_approx(remainingPop, temp_probability);
        dead_mosquitoes += temp_number;
        remainingPop -= temp_number;
        cumulative_probability += temp_probability;

        // successfully feed on animals
        if (probs()->successfulfeed_animal > 0 && remainingPop > 0)
        { 
            temp_probability = (1 - p_local_mortality) * feedingrate * dt * probs()->successfulfeed_animal;
            if (cumulative_probability < 1) 
            { 
                //adjust for conditional probability
                temp_probability /= (1.0f - cumulative_probability); 
            }
            else 
            { 
                // TODO: update error reporting throughout this section
                throw CalculatedValueOutOfRangeException(__FILE__, __LINE__, __FUNCTION__, "cumulative_probability", cumulative_probability, 1.0);
            }
            temp_number = (uint32_t)randgen->binomial_approx(remainingPop, temp_probability);
            successful_animal_feed += temp_number;
            remainingPop -= temp_number;
            cumulative_probability += (1 - p_local_mortality) * feedingrate * dt * probs()->successfulfeed_animal;
        }

        // successful feed on artificial diet
        if (probs()->successfulfeed_AD > 0 && remainingPop > 0)
        { 
            temp_probability = (1 - p_local_mortality) * feedingrate * dt * probs()->successfulfeed_AD;
            if (cumulative_probability < 1) { temp_probability /= (1.0f - cumulative_probability); } //adjust for conditional probability
            else
            {
                throw CalculatedValueOutOfRangeException( __FILE__, __LINE__, __FUNCTION__, "cumulative_probability", cumulative_probability, 1.0 );
            }
            temp_number = (uint32_t)randgen->binomial_approx(remainingPop, temp_probability);
            successful_AD_feed += temp_number;
            remainingPop -= temp_number;
            cumulative_probability += (1 - p_local_mortality) * feedingrate * dt * probs()->successfulfeed_AD;
        }

        // attempt to human indoor feed
        if (probs()->indoorattempttohumanfeed > 0 && remainingPop > 0)
        { 
            temp_probability = (1 - p_local_mortality) * feedingrate * dt * probs()->indoorattempttohumanfeed;
            if (cumulative_probability < 1) { temp_probability /= (1.0f - cumulative_probability); } //adjust for conditional probability
            else
            {
                throw CalculatedValueOutOfRangeException( __FILE__, __LINE__, __FUNCTION__, "cumulative_probability", cumulative_probability, 1.0 );
            }
            temp_number = (uint32_t)randgen->binomial_approx(remainingPop, temp_probability);
            attempt_indoor_feed += temp_number;
            remainingPop -= temp_number;
            cumulative_probability += (1 - p_local_mortality) * feedingrate * dt * probs()->indoorattempttohumanfeed;
        }

        // attempt to human outdoor feed
        if (probs()->outdoorattempttohumanfeed > 0 && remainingPop > 0)
        { 
            temp_probability = (1 - p_local_mortality) * feedingrate * dt * probs()->outdoorattempttohumanfeed;
            if (cumulative_probability < 1) { temp_probability /= (1.0f - cumulative_probability); } //adjust for conditional probability
            else
            {
                throw CalculatedValueOutOfRangeException( __FILE__, __LINE__, __FUNCTION__, "cumulative_probability", cumulative_probability, 1.0 );
            }
            temp_number = (uint32_t)randgen->binomial_approx(remainingPop, temp_probability);
            human_outdoor_feed += temp_number;
            remainingPop -= temp_number;
            cumulative_probability += (1 - p_local_mortality) * feedingrate * dt * probs()->outdoorattempttohumanfeed;
        }

        // survived without attempting feed
        if (remainingPop > 0)
        {
            survived_wo_feeding = remainingPop;
        }

        // for the infectious queue, need to update infectious bites
        if ( state == VectorStateEnum::STATE_INFECTIOUS )
        {
            indoorinfectiousbites += attempt_indoor_feed;
            outdoorinfectiousbites += human_outdoor_feed;

            // deposit indoor and outdoor contagion into vector-to-human group
            StrainIdentity *strain = const_cast<StrainIdentity *>(queue->GetStrainIdentity());

            m_context->DepositFromIndividual( strain, attempt_indoor_feed  * species()->transmissionmod, &NodeVector::vector_to_human_indoor );
            m_context->DepositFromIndividual( strain, human_outdoor_feed * species()->transmissionmod, &NodeVector::vector_to_human_outdoor );
            delete strain;
        }

        // update human biting rate
        indoorbites += attempt_indoor_feed;
        outdoorbites += human_outdoor_feed;

        // Now process human indoor and outdoor feeds.
        // Break up indoor feeds into die, survive w/o feeding, successful AD feed, and successful human feed.
        // Separated out 'indoorinfectiousbites' already, because they are not distributed uniformly, but depend on individual interventions,
        // the effect of which has been calculated below to determined effects on vectors
        // Remember infectious correction!
        if (attempt_indoor_feed > 0)
        {
            // start with those who die
            remainingPop = attempt_indoor_feed;
            cumulative_probability = 0;
            temp_probability = probs()->indoor_diebeforefeeding + probs()->indoor_dieduringfeeding * x_infectioushfmortmod + probs()->indoor_diepostfeeding * x_infectiouscorrection;
            temp_number = (uint32_t)randgen->binomial_approx(remainingPop, temp_probability);
            dead_mosquitoes += temp_number;
            remainingPop -= temp_number;
            cumulative_probability += temp_probability;

            // successful feed on artificial diet
            if (probs()->indoor_successfulfeed_AD > 0 && remainingPop > 0)
            { 
                temp_probability = probs()->indoor_successfulfeed_AD;
                if (cumulative_probability < 1) { temp_probability /= (1.0f - cumulative_probability); } //adjust for conditional probability
                else
                {
                    throw CalculatedValueOutOfRangeException( __FILE__, __LINE__, __FUNCTION__, "cumulative_probability", cumulative_probability, 1.0 );
                }
                temp_number = (uint32_t)randgen->binomial_approx(remainingPop, temp_probability);
                successful_AD_feed += temp_number;
                remainingPop -= temp_number;
                cumulative_probability += probs()->indoor_successfulfeed_AD;
            }

            // successful feed on human
            if (probs()->indoor_successfulfeed_human > 0 && remainingPop > 0)
            { 
                temp_probability = probs()->indoor_successfulfeed_human * x_infectiouscorrection;
                if (cumulative_probability < 1) { temp_probability /= (1.0f - cumulative_probability); } //adjust for conditional probability
                else
                {
                    throw CalculatedValueOutOfRangeException( __FILE__, __LINE__, __FUNCTION__, "cumulative_probability", cumulative_probability, 1.0 );
                }
                temp_number = (uint32_t)randgen->binomial_approx(remainingPop, temp_probability);
                successful_human_feed += temp_number;
                remainingPop -= temp_number;
                cumulative_probability += probs()->indoor_successfulfeed_human * x_infectiouscorrection;
                
                // some successful feeds result in infected adults
                if ( state == VectorStateEnum::STATE_ADULT && temp_number > 0 )
                {
                    float host_infectivity_indoor  = m_transmissionGroups->GetTotalContagion(&NodeVector::human_to_vector_indoor);
                    newinfected += (uint32_t)(randgen->binomial_approx(temp_number, species()->acquiremod * x_infectionWolbachia * host_infectivity_indoor / probs()->indoor_successfulfeed_human));
                }
            }

            // survived without feeding
            if (remainingPop > 0) { survived_wo_feeding += remainingPop; }
        }

        // now outdoor feeds
        if (human_outdoor_feed > 0)
        {
            // start with those who die
            remainingPop = human_outdoor_feed;
            temp_probability = probs()->outdoor_diebeforefeeding + probs()->outdoor_dieduringfeeding * x_infectioushfmortmod + probs()->outdoor_diepostfeeding* x_infectiouscorrection + probs()->outdoor_successfulfeed_human* probs()->outdoor_returningmortality * x_infectiouscorrection;
            temp_number = (uint32_t)randgen->binomial_approx(remainingPop, temp_probability);
            dead_mosquitoes += temp_number;
            remainingPop -= temp_number;
            cumulative_probability = temp_probability;

            // successful feed on human
            if (probs()->outdoor_successfulfeed_human > 0 && remainingPop > 0)
            { 
                temp_probability = (1.0f - probs()->outdoor_returningmortality) * probs()->outdoor_successfulfeed_human * x_infectiouscorrection;
                if (cumulative_probability < 1) { temp_probability /= (1.0f - cumulative_probability); } //adjust for conditional probability
                else
                {
                    throw CalculatedValueOutOfRangeException( __FILE__, __LINE__, __FUNCTION__, "cumulative_probability", cumulative_probability, 1.0 );
                }
                if( temp_probability >= 1.0)
                {
                    temp_number = (uint32_t)remainingPop;
                    successful_human_feed+=temp_number;
                    remainingPop = 0;
                }
                else
                {
                    temp_number = (uint32_t)randgen->binomial_approx(remainingPop, temp_probability);
                    successful_human_feed += temp_number;
                    remainingPop -= temp_number;
                }
                cumulative_probability += (1.0f - probs()->outdoor_returningmortality) * probs()->outdoor_successfulfeed_human * x_infectiouscorrection;
                
                // some successful feeds result in infected adults
                if ( state == VectorStateEnum::STATE_ADULT && temp_number > 0 )
                {
                    float host_infectivity_outdoor  = m_transmissionGroups->GetTotalContagion(&NodeVector::human_to_vector_outdoor);
                    newinfected += (uint32_t)(randgen->binomial_approx(temp_number, species()->acquiremod * x_infectionWolbachia * host_infectivity_outdoor / ((1.0f - probs()->outdoor_returningmortality) * probs()->outdoor_successfulfeed_human * x_infectiouscorrection)));
                }
            }

            // survived without feeding
            if (remainingPop > 0) { survived_wo_feeding += remainingPop; }
        }

        // now adjust population and eggs
        if (dead_mosquitoes > 0) { queue->SetPopulation( queue->GetPopulation() - dead_mosquitoes); }
        unsigned long int tempeggs = (unsigned long)(species()->eggbatchsize * x_infectedeggbatchmod * (successful_human_feed + successful_AD_feed * ADfeed_eggbatchmod + successful_animal_feed * animalfeed_eggbatchmod));
        neweggs += tempeggs;
        gender_mating_eggs[queue->GetVectorGenetics().GetIndex()] += tempeggs;
        LOG_DEBUG_F("adding %d eggs to vector genetics index %d.  current total=%d\n", tempeggs, queue->GetVectorGenetics().GetIndex(), gender_mating_eggs[queue->GetVectorGenetics().GetIndex()]);

        // correction to ensure that new infections won't surpass the upper bound of successful human feeds
        // return min(newinfected, successful_human_feed); // EAW: this would be more concise
        if (newinfected > successful_human_feed) { newinfected = successful_human_feed; }
        return newinfected;
    }

    float VectorPopulation::GetFeedingCycleDurationByTemperature() const
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
        float mean_cycle_duration = (airtemp > 15) ? 1.0f + 37.0f * ( (species()->daysbetweenfeeds - 1.0f) / 2.0f ) / ( airtemp - 11.5f ) : 10.0f;
        LOG_DEBUG_F("Mean gonotrophic cycle duration = %0.2f days at %0.2f degrees C.\n", mean_cycle_duration, airtemp);

        return mean_cycle_duration;
    }

    void VectorPopulation::Update_Immature_Queue( float dt )
    {
        float currentProbability = 0.0;
        unsigned long int tempPop = 0;

        // calculate local mortality, includes outdoor area killling
        float p_local_mortality = (float)EXPCDF(-dt * localadultmortality);
        p_local_mortality = p_local_mortality + (1.0f - p_local_mortality) * probs()->outdoorareakilling;

        // Use the verbose "for" construct here because we may be modifying the list and need to protect the iterator.
        for (VectorCohortList_t::iterator iList = ImmatureQueues.begin(); iList != ImmatureQueues.end(); /* iList++ */)
        {
            VectorCohort *cohort = (*iList);
            release_assert( cohort );

            VectorCohortList_t::iterator iCurrent = iList++;

            cohort->IncreaseProgress( dt * species()->immaturerate ); // introduce climate dependence here if we can figure it out
            cohort->SetPopulation(  (int32_t)(cohort->GetPopulation() - randgen->binomial_approx(cohort->GetPopulation(), p_local_mortality)) );
            // 
            if (cohort->GetProgress() >= 1 || cohort->GetPopulation() <= 0)
            {
                if (cohort->GetPopulation() > 0) // corrected in case of too long a time step
                {
                    // female or male?
                    if (cohort->GetVectorGenetics().GetGender() == VectorGender::VECTOR_FEMALE) //female
                    {
                        // new mating calculations
                        if(gender_mating_males.empty() || males == 0)// no males listed, so stay immature 
                        {
                            continue;
                        }
                        else if(gender_mating_males.size() == 1)// just one type of males, so all females mate with that type
                        {
                            ApplyMatingGenetics(cohort, VectorMatingStructure(gender_mating_males.begin()->first));
                            queueIncrementTotalPopulation(cohort, VectorStateEnum::STATE_ADULT);
                            MergeProgressedCohortIntoCompatibleQueue(AdultQueues, cohort->GetPopulation(), cohort->GetVectorGenetics());
                        }
                        else
                        {
                            // now iterate over all males, there will be a slight rounding error
                            for (auto& maletypes : gender_mating_males)
                            {
                                currentProbability = float(maletypes.second)/males;
                                tempPop = currentProbability * cohort->GetPopulation();
                                VectorCohort* tempentrynew = VectorCohort::CreateCohort(0, tempPop, cohort->GetVectorGenetics());
                                release_assert( tempentrynew );
                                ApplyMatingGenetics(tempentrynew, VectorMatingStructure(maletypes.first));
                                queueIncrementTotalPopulation(tempentrynew, VectorStateEnum::STATE_ADULT);
                                MergeProgressedCohortIntoCompatibleQueue(AdultQueues, tempentrynew->GetPopulation(), tempentrynew->GetVectorGenetics());
                                delete tempentrynew;
                            }
                        }
                    }
                    else // male
                    {
                        queueIncrementTotalPopulation(cohort);//update counter
                        // Seek a compatible (same gender mating type) male queue and increase its population.
                        MergeProgressedCohortIntoCompatibleQueue(MaleQueues, cohort->GetPopulation(), cohort->GetVectorGenetics());
                    }
                }

                ImmatureQueues.erase(iCurrent);
                delete cohort;
            }
        }
    }

    void VectorPopulation::ApplyMatingGenetics(VectorCohort* vq, VectorMatingStructure male_vector_genetics)
    {
        // (1) Determine female fertility
        if( vq->GetVectorGenetics().GetSterility() == VectorSterility::VECTOR_STERILE )
        {
            // Already sterile
        }
        else if( male_vector_genetics.GetSterility() == VectorSterility::VECTOR_STERILE )
        {
            // Female mated with a sterile male
            vq->GetVectorGenetics().SetSterility(VectorSterility::VECTOR_STERILE);
        }
        else if( !VectorMatingStructure::WolbachiaCompatibleMating(vq->GetVectorGenetics().GetWolbachia(), male_vector_genetics.GetWolbachia()) )
        {
            // Cytoplasmic incompatibility due to differing Wolbachia infections
            vq->GetVectorGenetics().SetSterility(VectorSterility::VECTOR_STERILE);
        }

        // (2) Wolbachia retains female type
        
        // (3) Solve for pesticide resistance of mating
        vq->GetVectorGenetics().SetPesticideResistance( vq->GetVectorGenetics().GetPesticideResistance().first, male_vector_genetics.GetPesticideResistance().first );

        // (4) Finally define mated HEGs case
        vq->GetVectorGenetics().SetHEG( vq->GetVectorGenetics().GetHEG().first, male_vector_genetics.GetHEG().first );
    }

    // Seek a compatible (same gender-mating type) queue in specified list (e.g. AdultQueues, InfectiousQueues) and increase its population.
    // EAW: If we have to do this a lot, then we might consider a different type of container (e.g. map instead of list).
    void VectorPopulation::MergeProgressedCohortIntoCompatibleQueue(VectorCohortList_t &queues, int32_t population, VectorMatingStructure vector_genetics)
    {
        VectorCohortList_t::iterator it = std::find_if( queues.begin(), queues.end(), [vector_genetics](VectorCohort* cohort){ return cohort->GetVectorGenetics() == vector_genetics; } );
        if ( it != queues.end() ) 
        { 
            (*it)->SetPopulation( (*it)->GetPopulation() + population ); 
        }
        else
        {
            LOG_DEBUG_F( "Creating new '%s' VectorCohort with population %d for type: %s, %s, %s, pesticide-resistance: %s-%s, HEG: %s-%s. \n", species_ID.c_str(), population, 
                         VectorGender::pairs::lookup_key(vector_genetics.GetGender()), 
                         VectorSterility::pairs::lookup_key(vector_genetics.GetSterility()), 
                         VectorWolbachia::pairs::lookup_key(vector_genetics.GetWolbachia()), 
                         VectorAllele::pairs::lookup_key(vector_genetics.GetPesticideResistance().first), 
                         VectorAllele::pairs::lookup_key(vector_genetics.GetPesticideResistance().second), 
                         VectorAllele::pairs::lookup_key(vector_genetics.GetHEG().first),
                         VectorAllele::pairs::lookup_key(vector_genetics.GetHEG().second) );

            queues.push_front(VectorCohort::CreateCohort(0, population, vector_genetics));
        }
    }

    void VectorPopulation::Update_Larval_Queue( float dt )
    {
        // Use the verbose "for" construct here because we may be modifying the list and need to protect the iterator.
        for (VectorCohortList_t::iterator iList = LarvaQueues.begin(); iList != LarvaQueues.end();)
        {
            //VectorCohortWithHabitat *larvaentry = static_cast<VectorCohortWithHabitat *>(*iList);
            IVectorCohortWithHabitat *larvaentry = NULL;
            if( (*iList)->QueryInterface( GET_IID( IVectorCohortWithHabitat ), (void**)&larvaentry ) != s_OK )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "larva", "IVectorCohortWithHabitat", "IVectorCohort" );
            }

            VectorCohortList_t::iterator iCurrent = iList++;

            // Apply temperature and over-crowding dependent larval development
            (*iCurrent)->IncreaseProgress( GetLarvalDevelopmentProgress(dt, larvaentry) );

            // Apply larval mortality, the probability of which may depend on over-crowding and Notre Dame instar-specific dynamics
            float p_larval_mortality = GetLarvalMortalityProbability(dt, larvaentry);
            (*iCurrent)->SetPopulation(  (int32_t)((*iCurrent)->GetPopulation() - randgen->binomial_approx((*iCurrent)->GetPopulation(), p_larval_mortality)) );

            if ((*iCurrent)->GetProgress() >= 1 || (*iCurrent)->GetPopulation() <= 0)
            {
                if ((*iCurrent)->GetPopulation() > 0)
                {
                    // Emerged larva become immature adults
                    ImmatureQueues.push_back(VectorCohort::CreateCohort(0, (*iCurrent)->GetPopulation(), (*iCurrent)->GetVectorGenetics()));
                    LOG_DEBUG_F("Immature adults emerging from larva queue: population=%d, vector_genetics index=%d.\n", (*iCurrent)->GetPopulation(), (*iCurrent)->GetVectorGenetics().GetIndex());
                }
                auto goodIterator = *iCurrent;
                LarvaQueues.erase(iCurrent);
                delete goodIterator;
            }
            else
            {
                // Only counting female larva to keep egg-crowding and larval-competition calculations backward-consistent
                if((*iCurrent)->GetVectorGenetics().GetGender() == VectorGender::VECTOR_FEMALE)
                {
                    // Pass back larva in this cohort to total count in habitat
                    larvaentry->GetHabitat()->AddLarva((*iCurrent)->GetPopulation(), (*iCurrent)->GetProgress());
                }
            }
        }
    }

    float VectorPopulation::GetLarvalDevelopmentProgress(float dt, IVectorCohortWithHabitat* larva) const
    {
        // Get local larval growth modifier, which depends on larval crowding
        float locallarvalgrowthmod = 1.0; 
        
        // if density dependent delay, slow growth
        if(!(params()->larval_density_dependence == LarvalDensityDependence::NO_DENSITY_DEPENDENCE || params()->larval_density_dependence == LarvalDensityDependence::LARVAL_AGE_DENSITY_DEPENDENT_MORTALITY_ONLY))
        {
            locallarvalgrowthmod = larva->GetHabitat()->GetLocalLarvalGrowthModifier();
        }

        // Larval development is temperature dependent
        float temperature = m_context->GetLocalWeather()->airtemperature();

        // Craig, M. H., R. W. Snow, et al. (1999).
        // "A climate-based distribution model of malaria transmission in sub-Saharan Africa."
        // Parasitol Today 15(3): 105-111.
        float progress = ( species()->aquaticarrhenius1 * exp(-species()->aquaticarrhenius2 / (temperature + CELSIUS_TO_KELVIN)) ) * dt * locallarvalgrowthmod;

        return progress;
    }

    float VectorPopulation::GetLarvalMortalityProbability(float dt, IVectorCohortWithHabitat* larva) const
    {
        VectorHabitat* habitat = larva->GetHabitat();

        // (1) Local larval mortality from larval competition in habitat
        // float larval_survival_weight = GetRelativeSurvivalWeight(habitat);
        IVectorCohort * vc = NULL;
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
        float p_larval_mortality = (float)EXPCDF(-dt * (locallarvalmortality + rainfallmortality));
        p_larval_mortality = p_larval_mortality + (1.0f - p_larval_mortality) * artificialmortality;

        return p_larval_mortality;
    }

    void VectorPopulation::Update_Egg_Laying( float dt )
    {
        // Get total larval capacity across habitats for calculation of fractional allocation of eggs
        float total_capacity = 0;
        for (auto habitat : m_larval_habitats)
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
        for (auto habitat : m_larval_habitats)
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
                    CreateEggCohortAlleleSorting(habitat, eggs_to_lay, vms);
                }
            }
        }
    }

    void VectorPopulation::CreateEggCohortOfType(VectorHabitat* habitat, uint32_t eggs_to_lay, VectorMatingStructure vms_egg)
    {
        // Find if there is a compatible existing cohort of eggs
        VectorCohortList_t::iterator itEggs = std::find_if( EggQueues.begin(), EggQueues.end(), [vms_egg, habitat](IVectorCohort* cohort) -> bool 
        {
            // If this QI gets cumbersome, nothing really prevents the EggQueues (and LarvaQueues) from being lists of the derived type pointers
            IVectorCohortWithHabitat *eggentry = NULL;
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
            LOG_DEBUG_F( "Laying %d eggs into existing egg queue (index=%d, habitat=%s).\n", eggs_to_lay, vms_egg.GetIndex(), VectorHabitatType::pairs::lookup_key( habitat->GetVectorHabitatType() ) );
        }
        else
        {
            EggQueues.push_back( VectorCohortWithHabitat::CreateCohort(habitat, 0, eggs_to_lay, vms_egg) );
            LOG_DEBUG_F( "Laying %d eggs and pushing into new egg queue (index=%d, habitat=%s).\n", eggs_to_lay, vms_egg.GetIndex(), VectorHabitatType::pairs::lookup_key( habitat->GetVectorHabitatType() ) );
        }

        // Males larvae are produced in Update_Egg_Hatching in equal proportion to female hatching eggs
    }

    void VectorPopulation::CreateEggCohortAlleleSorting(VectorHabitat* habitat, uint32_t eggs_to_lay, VectorMatingStructure vms)
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
                    CreateEggCohortHEGSorting(habitat, uint32_t(fraction_by_type.second * eggs_to_lay), vms);
                }
            }
        }
    }

    void VectorPopulation::CreateEggCohortHEGSorting(VectorHabitat* habitat, uint32_t eggs_to_lay, VectorMatingStructure vms)
    {
        
        AlleleFractions_t fractions;
        AllelePair_t matedHEGs;
        // This is the lowest-level Egg Cohort sorting function, invoked if HEGs are active
        // check whether early or late homing, now set to early homing only
        if(params()->heg_model == HEGModel::OFF)
        {
            fractions = VectorMatingStructure::GetAlleleFractionsEarlyHoming(vms.GetHEG(), 0);// no homing
        }else if(params()->heg_model == HEGModel::EGG_HOMING) // late
        {
            fractions = VectorMatingStructure::GetAlleleFractions(vms.GetHEG());

            // Corrections for HEGs homing in heterozygous individuals
            float homingFraction = params()->HEGhomingRate * fractions[VectorAllele::HALF];
            fractions[VectorAllele::HALF] -= homingFraction;
            fractions[VectorAllele::FULL] += homingFraction;
        }else if(params()->heg_model == HEGModel::GERMLINE_HOMING) // early in females only
        {
            fractions = VectorMatingStructure::GetAlleleFractionsEarlyHoming(vms.GetHEG(), params()->HEGhomingRate);
        }else if(params()->heg_model == HEGModel::DUAL_GERMLINE_HOMING) // early in males only
        {
            fractions = VectorMatingStructure::GetAlleleFractionsDualEarlyHoming(vms.GetHEG(), params()->HEGhomingRate);
        }else if(params()->heg_model == HEGModel::DRIVING_Y) // homing is fraction of eggs that would have been female that are now male (e.g. 90% male offspring is 80% homing)
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
                fractions[VectorAllele::HALF] = params()->HEGhomingRate;
                fractions[VectorAllele::FULL] = 1.0f-params()->HEGhomingRate;
            }else
            {
                throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__, "DRIVING_Y allele", matedHEGs.second, VectorAllele::pairs::lookup_key(matedHEGs.second) );
            }
        }

        // Corrections for HEGs fecundity
        float fecundityFactor = 1.0f;
        if(params()->heg_model == HEGModel::OFF)
        {
            fecundityFactor = 1.0f;
        }else if(params()->heg_model == HEGModel::DRIVING_Y)
        {
            switch( vms.GetHEG().second) // fitness depends on male
            {
            case VectorAllele::WILD:
                fecundityFactor = 1.0f;
                break;
            case VectorAllele::HALF:
                fecundityFactor = 1.0 - params()->HEGfecundityLimiting;
                break;
            case VectorAllele::FULL:
                fecundityFactor = 1.0 - params()->HEGfecundityLimiting;
                break;
            }
        }else{
            switch( vms.GetHEG().first )
            {
            case VectorAllele::FULL:
                // fecundity reduction assumed to act on female (just an assumption)
                fecundityFactor = 1.0 - params()->HEGfecundityLimiting;
                break;

            case VectorAllele::HALF:
                fecundityFactor = 1.0;// in this current temporary version, heterozygous has no fitness cost
                break;
            }
        }

        // Now finally lay eggs
        for (auto& fraction_by_type : fractions)
        {
            vms.SetHEG( fraction_by_type.first, VectorAllele::NotMated );
            if( fraction_by_type.second > 0 )
            {
                // Check for Driving Y male biased case here, or just proceed as normal
                if(params()->heg_model == HEGModel::DRIVING_Y && fraction_by_type.first == VectorAllele::HALF)
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
        float eggHatchDelayFactor = 1.0;
        if ( params()->egg_hatch_delay_dist != EggHatchDelayDist::NO_DELAY && params()->meanEggHatchDelay > 0 )
        {
            eggHatchDelayFactor = dt / params()->meanEggHatchDelay;
            eggHatchDelayFactor = min(eggHatchDelayFactor, 1.0); // correct to avoid too many eggs
        }

        // Now process remaining eggs
        for (VectorCohortList_t::iterator iList = EggQueues.begin(); iList != EggQueues.end();)
        {
            //VectorCohortWithHabitat *eggentry = static_cast<VectorCohortWithHabitat *>(*iList);
            IVectorCohortWithHabitat *eggentry = static_cast<VectorCohortWithHabitat *>(*iList);
            if( (*iList)->QueryInterface( GET_IID( IVectorCohortWithHabitat ), (void**)&eggentry ) != s_OK )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "eggentry", "IVectorCohortWithHabitat", "IVectorCohort" );
            }
            VectorCohortList_t::iterator iCurrent = iList++;
            VectorHabitat* habitat = eggentry->GetHabitat();

            // Potential inter-species competitive weighting
            // float egg_survival_weight = GetRelativeSurvivalWeight(habitat);

            // Calculate egg-crowding correction for these eggs based on habitat and decrease population
            float egg_crowding_correction = habitat->GetEggCrowdingCorrection();
            (*iCurrent)->SetPopulation(  (int32_t)((*iCurrent)->GetPopulation() * egg_crowding_correction) );

            uint32_t hatched = eggHatchDelayFactor * (*iCurrent)->GetPopulation();

            if( hatched > 0 )
            {
                VectorMatingStructure vms_larva = (*iCurrent)->GetVectorGenetics();
                LarvaQueues.push_back( VectorCohortWithHabitat::CreateCohort(habitat, 0, hatched, vms_larva) );
                LOG_DEBUG_F("Hatching %d female eggs and pushing into larval queues (index=%d).\n", hatched, vms_larva.GetIndex());

                // Eggs calculations were only done for female eggs
                // So, here on hatching we will push back an equal number of male larva as well
                vms_larva.SetGender(VectorGender::VECTOR_MALE);
                LarvaQueues.push_back( VectorCohortWithHabitat::CreateCohort(habitat, 0, hatched, vms_larva) );
                LOG_DEBUG_F("Hatching %d male eggs and pushing into larval queues (index=%d).\n", hatched, vms_larva.GetIndex());
            }

            (*iCurrent)->SetPopulation( (*iCurrent)->GetPopulation() - hatched );

            if((*iCurrent)->GetPopulation() <= 0)
            {
                auto goodIterator = *iCurrent;
                EggQueues.erase(iCurrent);
                delete goodIterator;
            }
        }
    }

    void VectorPopulation::Update_Male_Queue( float dt )
    {
        // Convert mortality rates to mortality probability (can make age dependent)
        float p_local_male_mortality = (float)EXPCDF(-dt * localadultmortality);
        p_local_male_mortality = p_local_male_mortality + (1.0f - p_local_male_mortality) * probs()->outdoorareakilling_male;

        males = 0;
        for (auto cohort : MaleQueues)
        {
            // adults die
            if (cohort->GetPopulation() > 0)
            {
                cohort->SetPopulation(  (int32_t)(cohort->GetPopulation() - randgen->binomial_approx(cohort->GetPopulation(), p_local_male_mortality)) );
            }

            if (cohort->GetPopulation() < 0)
            {
                cohort->SetPopulation(  0 );
            }

            queueIncrementTotalPopulation(cohort);
        }
    }

    void VectorPopulation::queueIncrementTotalPopulation(VectorCohort* vq, VectorStateEnum::Enum state)
    {
        VectorGeneticIndex_t index = vq->GetVectorGenetics().GetIndex();
        if(vq->GetVectorGenetics().GetGender() == VectorGender::VECTOR_FEMALE)
        {
            if(state==VectorStateEnum::STATE_ADULT)
            {
                adult += vq->GetPopulation();
                vector_genetics_adults[index] += vq->GetPopulation();
            }
            else if(state==VectorStateEnum::STATE_INFECTED)
            {
                infected +=vq->GetPopulation();
                vector_genetics_infected[index] += vq->GetPopulation();
            }
            else if(state==VectorStateEnum::STATE_INFECTIOUS)
            {
                infectious += vq->GetPopulation();
                vector_genetics_infectious[index] += vq->GetPopulation();
            }
        }
        else
        {
            males +=vq->GetPopulation();
            gender_mating_males[index] += vq->GetPopulation();
        }
    }

    void VectorPopulation::AddVectors(VectorMatingStructure _vector_genetics, unsigned long int releasedNumber)
    {
        VectorCohort* tempentry;
        
        // Insert into correct Male or Female list
        if (_vector_genetics.GetGender() == VectorGender::VECTOR_FEMALE)
        {
            // If unmated, put in Immature with progress 1, so that the females can mate with the local male population.
            // This will throw an exception if only one of the extra fields is mated.
            if( !_vector_genetics.IsMated() )
            {
                tempentry = VectorCohort::CreateCohort(1, releasedNumber, _vector_genetics);
                ImmatureQueues.push_front(tempentry);
            }
            else
            { 
                // already mated, so go in AdultQueues
                tempentry = VectorCohort::CreateCohort(0, releasedNumber, _vector_genetics);
                AdultQueues.push_front(tempentry);
                queueIncrementTotalPopulation(tempentry, VectorStateEnum::STATE_ADULT);//update counter
            }
        }
        else
        {
            tempentry = VectorCohort::CreateCohort(0, releasedNumber, _vector_genetics);
            MaleQueues.push_front(VectorCohort::CreateCohort(0, releasedNumber, _vector_genetics));
            queueIncrementTotalPopulation(tempentry);//update counter
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

    unsigned long int VectorPopulation::Vector_Migration(float migrate, VectorCohortList_t *Migration_Queue)
    {
        throw NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "Vector migration only currently supported for individual (not cohort) model." );
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
            break;

        case VectorPoolIdEnum::OUTDOOR_VECTOR_POOL:
            return m_EIR_by_pool.second;
            break;

        case VectorPoolIdEnum::BOTH_VECTOR_POOLS:
            return m_EIR_by_pool.first + m_EIR_by_pool.second;
            break;

        default:
            throw OutOfRangeException( __FILE__, __LINE__, __FUNCTION__, "VectorPopulation::GetEIRByPool is only valid for indoor/outdoor/combined biting." );
        }
    }

    float VectorPopulation::GetHBRByPool(VectorPoolIdEnum::Enum pool_id) const
    {
        switch(pool_id)
        {
        case VectorPoolIdEnum::INDOOR_VECTOR_POOL:
            return m_HBR_by_pool.first;
            break;

        case VectorPoolIdEnum::OUTDOOR_VECTOR_POOL:
            return m_HBR_by_pool.second;
            break;

        case VectorPoolIdEnum::BOTH_VECTOR_POOLS:
            return m_HBR_by_pool.first + m_HBR_by_pool.second;
            break;

        default:
            throw OutOfRangeException( __FILE__, __LINE__, __FUNCTION__, "VectorPopulation::GetHBRByPool is only valid for indoor/outdoor/combined biting." );
        }
    }

    float VectorPopulation::GetRelativeSurvivalWeight(VectorHabitat* habitat) const
    {
        float survival_weight = 1.0f;

#if 0
        // Modified for inter-species competitive advantage for egg-crowding and larval mortality in case of shared habitat
        if( params()->enable_vector_species_habitat_competition )
        {
            float max_capacity = habitat->GetMaximumLarvalCapacity();
            survival_weight = (max_capacity > 0) ? ( m_larval_capacities.at(habitat->GetVectorHabitatType()) / max_capacity ) : 0;
        }
#endif

        return survival_weight;
    }

    int32_t VectorPopulation::getAdultCount()       const  { return adult; }
    int32_t VectorPopulation::getInfectedCount()    const  { return infected; }
    int32_t VectorPopulation::getInfectiousCount()  const  { return infectious; }
    double  VectorPopulation::getInfectivity()      const  { return infectivity; }
    std::string VectorPopulation::get_SpeciesID()   const  { return species_ID; }

    const VectorHabitatList_t& VectorPopulation::GetHabitats() const  { return m_larval_habitats; }
    
    void VectorPopulation::SetContextTo(INodeContext *context) { m_context = context; }

    const SimulationConfig* VectorPopulation::params()  const { return GET_CONFIGURABLE(SimulationConfig); }

    const infection_list_t& VectorPopulation::GetInfections() const
    {
        throw NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__ );
    }

    float VectorPopulation::GetInterventionReducedAcquire() const
    {
        throw NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__ );
    }
}

#if USE_BOOST_SERIALIZATION
namespace Kernel {

    template< typename Archive >
    void serialize( Archive& ar, VectorPopulation &obj, unsigned int file_version )
    {
        // Serialize fields
        ar & obj.EggQueues;
        ar & obj.LarvaQueues;
        ar & obj.ImmatureQueues;
        ar & obj.AdultQueues;
        ar & obj.InfectedQueues;
        ar & obj.InfectiousQueues;
        ar & obj.MaleQueues;
        ar & obj.species_ID;
        ar & obj.m_species_params;
        ar & obj.m_probabilities;

        ar & obj.animalfeed_eggbatchmod;
        ar & obj.ADfeed_eggbatchmod;

        ar & obj.neweggs;
        ar & obj.adult;        // female population
        ar & obj.infected;
        ar & obj.infectious;
        ar & obj.males;

        // local variations on base rates
        ar & obj.dryheatmortality;
        ar & obj.localadultmortality;
        ar & obj.infectiouscorrection;

        // intermediate counters
        ar & obj.indoorinfectiousbites;
        ar & obj.outdoorinfectiousbites;
        ar & obj.indoorbites;
        ar & obj.outdoorbites;

        // reporting
        ar & m_EIR_by_pool;
        ar & m_HBR_by_pool;

        // output 
        ar & obj.infectivity;
        ar & obj.infectivity_indoor;
        ar & obj.infectivity_outdoor;

        // counters to track population stats (Wolbachia type, sterility, etc...)
        ar & obj.gender_mating_eggs;   // depends on female type;
        ar & obj.gender_mating_males;
        ar & obj.vector_genetics_adults;
        ar & obj.vector_genetics_infected;
        ar & obj.vector_genetics_infectious;
    }

    template void serialize(boost::archive::binary_iarchive & ar, VectorPopulation&, const unsigned int file_version);
    template void serialize(boost::archive::binary_oarchive & ar, VectorPopulation&, const unsigned int file_version);
    template void serialize(boost::mpi::packed_skeleton_iarchive& ar, VectorPopulation&, const unsigned int file_version);
    template void serialize(boost::mpi::packed_skeleton_oarchive& ar, VectorPopulation&, const unsigned int file_version);
    template void serialize(boost::mpi::packed_oarchive& ar, VectorPopulation&, const unsigned int file_version);
    template void serialize(boost::mpi::packed_iarchive& ar, VectorPopulation&, const unsigned int file_version);
    template void serialize(boost::mpi::detail::content_oarchive& ar, VectorPopulation&, const unsigned int file_version);
    template void serialize(boost::mpi::detail::mpi_datatype_oarchive& ar, VectorPopulation&, const unsigned int file_version);

}
#endif
