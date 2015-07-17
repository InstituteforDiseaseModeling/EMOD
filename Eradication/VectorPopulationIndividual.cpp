/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"
#include "SimulationConfig.h"
#include "VectorPopulationIndividual.h"
#include "VectorCohortIndividual.h"
#include "Vector.h"
#include "IContagionPopulation.h"
#include "TransmissionGroupMembership.h"
#include "StrainIdentity.h"
#include "NodeVector.h"
#include "Exceptions.h"
#include "Log.h"
#include "Debug.h"

#ifdef randgen
#undef randgen
#endif
#include "RANDOM.h"
#define randgen (m_context->GetRng())

static const char * _module = "VectorPopulationIndividual";

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
    , current_vci(NULL)
    { 
    }

    void VectorPopulationIndividual::InitializeVectorQueues(unsigned int adults, unsigned int _infectious)
    { 
        adult      = adults;
        infectious = _infectious;

        uint32_t adjusted_population = 0;

        if (adult > 0)
        { 
            adjusted_population = adults / m_mosquito_weight;
            for (uint32_t i = 0; i < adjusted_population; i++)
            { 
                // adult initialized at age 0
                this->AdultQueues.push_front( VectorCohortIndividual::CreateCohort( VectorStateEnum::STATE_ADULT, 0, 0, m_mosquito_weight, VectorMatingStructure( VectorGender::VECTOR_FEMALE ), species_ID ) );
            }

            // and a population of males as well
            MaleQueues.push_front( VectorCohortAging::CreateCohort( 0.0f, 0.0f, adult, VectorMatingStructure( VectorGender::VECTOR_MALE ) ) );
            males = (int32_t)adult;
        }

        if (infectious > 0)
        { 
            adjusted_population = _infectious / m_mosquito_weight;
            for (uint32_t i = 0; i < adjusted_population; i++)
            { 
                // infectious initialized at age 20
                AdultQueues.push_front( VectorCohortIndividual::CreateCohort( VectorStateEnum::STATE_INFECTIOUS, 20, 0, m_mosquito_weight, VectorMatingStructure( VectorGender::VECTOR_FEMALE ), species_ID ) );
            }
        }
    }

    VectorPopulationIndividual *VectorPopulationIndividual::CreatePopulation(INodeContext *context, std::string species_name, int32_t adult, int32_t infectious, uint32_t mosquito_weight)
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
        double temperature = m_context->GetLocalWeather()->airtemperature();

        // Get habitat-weighted average oviposition-trap killing fraction to be used in ProcessFeedingCycle
        // TODO: it might be more consistent to put this in with the other lifecycle probabilities somehow
        m_average_oviposition_killing = 0;
        float total_larval_capacity = 0;
        for (auto habitat : m_larval_habitats)
        {
            float capacity = habitat->GetCurrentLarvalCapacity();
            m_average_oviposition_killing  += capacity * habitat->GetOvipositionTrapKilling();
            total_larval_capacity += capacity;
        }
        m_average_oviposition_killing /= total_larval_capacity;

        // Use the verbose "foreach" construct here because empty queues (i.e. dead individuals) will be removed
        for ( VectorCohortList_t::iterator iList = AdultQueues.begin(); iList != AdultQueues.end(); )
        { 
            IVectorCohortIndividual *tempentry2 = NULL;
            if( (*iList)->QueryInterface( GET_IID( IVectorCohortIndividual ), (void**)&tempentry2 ) != s_OK )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "(*iList)", "IVectorCohortIndividual", "VectorCohort" );
            }
            current_vci = tempentry2;
            VectorCohortList_t::iterator iCurrent = iList++;

            // Increment age of individual mosquitoes
            tempentry2->IncreaseAge( dt );

            // Process feeding cycle
            ProcessFeedingCycle(dt, (*iCurrent), tempentry2->GetState());

            // Progress with sporogony in infected mosquitoes
            if(tempentry2->GetState() == VectorStateEnum::STATE_INFECTED )
            {
                (*iCurrent)->IncreaseProgress( (species()->infectedarrhenius1 * exp(-species()->infectedarrhenius2 / (temperature + (float)CELSIUS_TO_KELVIN))) * dt );

                if( (*iCurrent)->GetProgress() >=1 && (*iCurrent)->GetPopulation() > 0 )
                {
                    // change state to INFECTIOUS
                    tempentry2->SetState( VectorStateEnum::STATE_INFECTIOUS );

                    // update INFECTIOUS counters
                    queueIncrementTotalPopulation((*iCurrent), VectorStateEnum::STATE_INFECTIOUS);
                    continue;
                }
            }

            // Remove empty cohorts (i.e. dead mosquitoes)
            if ((*iCurrent)->GetPopulation() <= 0)
            { 
                AdultQueues.erase(iCurrent);
                delete tempentry2;
            }
            else
            {
                // Increment counters
                int32_t pop = (*iCurrent)->GetPopulation();
                queueIncrementTotalPopulation((*iCurrent), tempentry2->GetState());
            }
        }

        // Acquire infections with strain tracking for exposed queues

        LOG_DEBUG("Exposure to contagion: human to vector.\n");
        m_transmissionGroups->ExposeToContagion((IInfectable*)this, &NodeVector::human_to_vector_all, dt);
        IndoorExposedQueues.clear();
        OutdoorExposedQueues.clear();
    }

    uint32_t VectorPopulationIndividual::ProcessFeedingCycle(float dt, VectorCohort *queue, VectorStateEnum::Enum state)
    {
        IVectorCohortIndividual *tempentry2 = current_vci;

        // Uniform random number between 0 and 1 for feeding-cycle outcome calculations
        float outcome = randgen->e();

        // Reset counters
        float cumulative_probability = 0;
        uint32_t newinfected = 0;

        if (queue->GetPopulation() <= 0) 
            return newinfected;

        // Adjust human-feeding mortality for longer-probing infectious vectors
        // Wekesa, J. W., R. S. Copeland, et al. (1992). "Effect of Plasmodium Falciparum on Blood Feeding Behavior of Naturally Infected Anopheles Mosquitoes in Western Kenya." Am J Trop Med Hyg 47(4): 484-488.
        // ANDERSON, R. A., B. G. J. KNOLS, et al. (2000). "Plasmodium falciparum sporozoites increase feeding-associated mortality of their mosquito hosts Anopheles gambiae s.l." Parasitology 120(04): 329-333.
        float x_infectioushfmortmod  = (state == VectorStateEnum::STATE_INFECTIOUS) ? (float) species()->infectioushfmortmod  : 1.0f;
        float x_infectiouscorrection = (state == VectorStateEnum::STATE_INFECTIOUS) ? (float) infectiouscorrection : 1.0f;

        //Wolbachia-related impacts on mortality and infection susceptibility
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

        // Calculate local mortality (with or without age dependence) and convert to probability
        if (params()->vector_aging)
        { 
            localadultmortality = tempentry2->GetAdditionalMortality() + dryheatmortality + species()->adultmortality + mortalityFromAge(tempentry2->GetAge());
        }
        else
        { 
            localadultmortality = tempentry2->GetAdditionalMortality() + dryheatmortality + species()->adultmortality;
        }
        
        float p_local_mortality = 1.0f - exp(-dt * localadultmortality * x_mortalityWolbachia);

        // Determine whether it will feed?
        tempentry2->SetOvipositionTimer( tempentry2->GetOvipositionTimer() - dt );
        if(tempentry2->GetOvipositionTimer() > 0.0f)
        {
            //not feeding so just experiences mortality
            cumulative_probability = p_local_mortality + (1 - p_local_mortality) * probs()->diewithoutattemptingfeed;
            
            // possibly correct for sugar feeding
            if( params()->vector_sugar_feeding == VectorSugarFeeding::VECTOR_SUGAR_FEEDING_EVERY_DAY && probs()->sugarTrapKilling > 0 )
            {
                cumulative_probability += (1 - cumulative_probability) * probs()->sugarTrapKilling;  // add in sugarTrap to kill rate
            }
            
            if(outcome <= cumulative_probability)
            { 
                queue->SetPopulation(0);  //mosquito dies
            }

            return 0;
        }

        // Lays eggs and calculate oviposition killing before feeding
        if(tempentry2->GetNewEggs()>0)
        {
            if(m_average_oviposition_killing > 0)
            {
                if ( randgen->e() < m_average_oviposition_killing )
                {
                    queue->SetPopulation(0); // mosquito dies
                    tempentry2->SetNewEggs(0);    // and does not lay eggs

                    return 0;                 // exit feeding cycle
                }
            }

            neweggs += tempentry2->GetNewEggs();
            gender_mating_eggs[queue->GetVectorGenetics().GetIndex()] += tempentry2->GetNewEggs();

            tempentry2->IncrementParity(); // increment number of times vector has laid eggs
            tempentry2->SetNewEggs(0);                     // reset eggs for next time

            // Now if sugar feeding exists every day or after each feed, and mortality is associated, then check for killing
            if( (params()->vector_sugar_feeding == VectorSugarFeeding::VECTOR_SUGAR_FEEDING_EVERY_FEED || 
                params()->vector_sugar_feeding == VectorSugarFeeding::VECTOR_SUGAR_FEEDING_EVERY_DAY) && probs()->sugarTrapKilling > 0)
            {
                if(randgen->e() < probs()->sugarTrapKilling)
                {
                    queue->SetPopulation(0);

                    return 0;  // dead mosquito: no blood-feeding cycle
                }
            }
        }

        // Use initial random draw to assign individual mosquito feeding outcomes among the following:
        //     (1) diebeforeattempttohumanfeed
        //     (2) successfulfeed_animal
        //     (3) successfulfeed_AD
        //     (4) indoorattempttohumanfeed
        //     (5) outdoorattempttohumanfeed

        // Counters for feeding-cycle results (TODO: change these to accommodate multiple feeds and eggbatchmod)
        uint32_t successful_animal_feed = 0;
        uint32_t successful_AD_feed     = 0;
        uint32_t successful_human_feed  = 0;

        // (1) die before human feeding?
        cumulative_probability = p_local_mortality + (1 - p_local_mortality) * probs()->diebeforeattempttohumanfeed;
        if (outcome <= cumulative_probability)
        {
            //mosquito dies
            queue->SetPopulation(0);
            return 0;
        }

        do { // at least one feeding attempt

            // (2) feed on animal?
            cumulative_probability += (1 - p_local_mortality) * probs()->successfulfeed_animal;
            if (outcome <= cumulative_probability)
            { 
                successful_animal_feed += queue->GetPopulation();
                continue;
            }

            // (3) feed on artificial diet?
            cumulative_probability += (1 - p_local_mortality) * probs()->successfulfeed_AD;
            if (outcome <= cumulative_probability)
            { 
                successful_AD_feed += queue->GetPopulation();
                tempentry2->SetAdditionalMortality( probs()->ADbiocontrol_additional_mortality );
                continue;
            }

            // (4) attempt indoor feed?
            cumulative_probability += (1 - p_local_mortality) * probs()->indoorattempttohumanfeed;
            if (outcome <= cumulative_probability)
            { 
                // for the infectious queue, need to update infectious bites
                if ( state == VectorStateEnum::STATE_INFECTIOUS ) 
                { 
                    indoorinfectiousbites += queue->GetPopulation(); 

                    // deposit indoor contagion into vector-to-human pool
                    StrainIdentity *strain = const_cast<StrainIdentity *>(tempentry2->GetStrainIdentity());
                    m_transmissionGroups->DepositContagion(strain, queue->GetPopulation()  * species()->transmissionmod, &NodeVector::vector_to_human_indoor);
                }

                // update human biting rate as well
                indoorbites += queue->GetPopulation();

                // Reset cumulative probability for another random draw
                // to assign outdoor-feeding outcomes among the following:
                //     (a) die in attempt to feed
                //     (b) indoor_successfulfeed_AD
                //     (c) indoor_successfulfeed_human
                outcome = randgen->e();
                cumulative_probability = 0;

                // (a) die in attempt to indoor feed?
                cumulative_probability = (float)(cumulative_probability + probs()->indoor_diebeforefeeding + probs()->indoor_dieduringfeeding * x_infectioushfmortmod + probs()->indoor_diepostfeeding * x_infectiouscorrection);
                if (outcome <= cumulative_probability)
                {
                    queue->SetPopulation(0);
                    return 0;
                }

                // (b) artificial diet?
                cumulative_probability += probs()->indoor_successfulfeed_AD;
                if (outcome <= cumulative_probability)
                { 
                    successful_AD_feed += queue->GetPopulation();
                    tempentry2->SetAdditionalMortality( probs()->ADbiocontrol_additional_mortality );
                    continue;
                }

                // (c) successful human feed?
                cumulative_probability += probs()->indoor_successfulfeed_human * x_infectiouscorrection;
                if (outcome <= cumulative_probability) 
                { 
                    successful_human_feed += queue->GetPopulation(); 
                    if (state == VectorStateEnum::STATE_ADULT && probs()->indoor_successfulfeed_human > 0)
                    {
                        // push back to exposed queue (to be checked for new infection)
                        IndoorExposedQueues.push_back(queue);
                    }
                }
                else
                {
                    // no host found
                }
                continue;
            }

            // (5) attempt outdoor feed?
            cumulative_probability += (1 - p_local_mortality) * probs()->outdoorattempttohumanfeed;
            if (outcome <= cumulative_probability)
            { 
                // for the infectious queue, need to update infectious bites
                if (state == VectorStateEnum::STATE_INFECTIOUS) 
                { 
                    outdoorinfectiousbites += queue->GetPopulation(); 

                    // deposit outdoor contagion into vector-to-human pool
                    StrainIdentity *strain = const_cast<StrainIdentity *>(tempentry2->GetStrainIdentity());
                    m_transmissionGroups->DepositContagion(strain, queue->GetPopulation() * species()->transmissionmod, &NodeVector::vector_to_human_outdoor);
                }

                // update human biting rate as well
                outdoorbites += queue->GetPopulation();

                // Reset cumulative probability for another random draw
                // to assign outdoor-feeding outcomes among the following:
                //     (a) die in attempt to feed
                //     (b) outdoor_successfulfeed_human
                outcome = randgen->e();
                cumulative_probability = 0;

                // (a) die in attempt to outdoor feed?
                cumulative_probability = (float)(probs()->outdoor_diebeforefeeding + probs()->outdoor_dieduringfeeding * x_infectioushfmortmod + probs()->outdoor_diepostfeeding* x_infectiouscorrection + probs()->outdoor_successfulfeed_human * probs()->outdoor_returningmortality * x_infectiouscorrection);
                if (outcome <= cumulative_probability)
                { 
                    queue->SetPopulation(0);
                    return 0;
                }

                // (b) successful human feed?
                cumulative_probability += (1.0f - probs()->outdoor_returningmortality) * probs()->outdoor_successfulfeed_human * x_infectiouscorrection;
                if (outcome <= cumulative_probability) 
                { 
                    successful_human_feed += queue->GetPopulation();
                    if (state == VectorStateEnum::STATE_ADULT && probs()->outdoor_successfulfeed_human > 0)
                    {
                        // push back to exposed queue (to be checked for new infection)
                        OutdoorExposedQueues.push_back(queue);
                    }
                }
                else
                { 
                    //survived to next day
                }
                continue;
            }

        }
        while(0); // just one feed for now, but here is where we will decide whether to continue feeding

        // now adjust egg batch size
        uint32_t tempeggs = (uint32_t)(species()->eggbatchsize * x_infectedeggbatchmod * (successful_human_feed + successful_AD_feed * ADfeed_eggbatchmod + successful_animal_feed * animalfeed_eggbatchmod));
        tempentry2->SetNewEggs( tempeggs );

        // reset oviposition timer for next cycle
        ResetOvipositionTimer(tempentry2);

        return newinfected;
    }

    void VectorPopulationIndividual::ResetOvipositionTimer( IVectorCohortIndividual* mosquito )
    {
        if( !params()->temperature_dependent_feeding_cycle )
        {
            // Simple behavior with constant configurable number of days between feeds:
            mosquito->SetOvipositionTimer( species()->daysbetweenfeeds );
        }
        else
        {
            // Temperature-dependent gonotrophic cycle duration:
            float mean_cycle_duration = GetFeedingCycleDurationByTemperature();

            // Allocate timers randomly to upper and lower bounds of fractional duration
            // If mean is 2.8 days: 80% will have 3-day cycles, and 20% will have 2-day cycles
            if ( randgen->e() < ( mean_cycle_duration - (int)mean_cycle_duration ) )
            {
                mosquito->SetOvipositionTimer ( (int)mean_cycle_duration + 1.0f );
                LOG_DEBUG_F("Reset oviposition timer by %d days.\n", (int)mean_cycle_duration + 1);
            }
            else
            {
                mosquito->SetOvipositionTimer( (int)mean_cycle_duration );
                LOG_DEBUG_F("Reset oviposition timer by %d days.\n", (int)mean_cycle_duration);
            }
        }

        // Make sure that floating-point precision doesn't delay things by an extra day!
        mosquito->SetOvipositionTimer( mosquito->GetOvipositionTimer() - 0.01f );
    }

    void VectorPopulationIndividual::Update_Immature_Queue( float dt )
    { 
        uint32_t temppop;
        float    p_local_mortality   = 0;
        VectorCohortIndividual* tempentrynew;
        float currentProbability = 0.0;
        uint32_t matedPop = 0;


        // calculate local mortality, includes outdoor area killing, converting rates to probabilities
        localadultmortality = dryheatmortality + species()->adultmortality;
        p_local_mortality   = (float)(1.0f - exp(-dt * localadultmortality));
        p_local_mortality   = p_local_mortality + (1.0f - p_local_mortality) * probs()->outdoorareakilling;

        // Use the verbose "for" construct here because we may be modifying the list and need to protect the iterator.
        for (VectorCohortList_t::iterator iList = ImmatureQueues.begin(); iList != ImmatureQueues.end(); /* iList++ */)
        { 
            VectorCohort *tempentry1 = (*iList);
            release_assert( tempentry1 );

            VectorCohortList_t::iterator iCurrent = iList++;

            tempentry1->IncreaseProgress( dt * species()->immaturerate ); // introduce climate dependence here if we can figure it out
            tempentry1->SetPopulation( (int32_t)(tempentry1->GetPopulation() - randgen->binomial_approx(tempentry1->GetPopulation(), p_local_mortality)) );

            if (tempentry1->GetProgress() >= 1 || tempentry1->GetPopulation() <= 0)
            { 
                if (tempentry1->GetPopulation() > 0)
                { 
                    // female or male?
                    if (tempentry1->GetVectorGenetics().GetGender() == VectorGender::VECTOR_FEMALE) //female
                    {
                        // new mating calculations
                        if(gender_mating_males.empty()|| males == 0)// no males listed, so stay immature 
                        {
                            continue;
                        }
                        else if(gender_mating_males.size() == 1)// just one type of males, so all females mate with that type
                        {
                            temppop = (tempentry1->GetPopulation()) / m_mosquito_weight;
                            for (uint32_t i = 0; i < temppop; i++)
                            { 
                                // now if sugar feeding exists every day or after each feed, and mortality is associated, then check for killing
                                if((params()->vector_sugar_feeding == VectorSugarFeeding::VECTOR_SUGAR_FEEDING_ON_EMERGENCE_ONLY || 
                                    params()->vector_sugar_feeding == VectorSugarFeeding::VECTOR_SUGAR_FEEDING_EVERY_FEED || 
                                    params()->vector_sugar_feeding == VectorSugarFeeding::VECTOR_SUGAR_FEEDING_EVERY_DAY) && probs()->sugarTrapKilling >0)
                                {
                                    if(randgen->e() < probs()->sugarTrapKilling)
                                    {
                                        // it dies and no adult created
                                    
                                    }
                                    else
                                    {
                                        tempentrynew = VectorCohortIndividual::CreateCohort(VectorStateEnum::STATE_ADULT, 0, 0, m_mosquito_weight, tempentry1->GetVectorGenetics(), species_ID);
                                        ApplyMatingGenetics(tempentrynew, VectorMatingStructure(gender_mating_males.begin()->first));
                                        AdultQueues.push_front(tempentrynew);
                                        queueIncrementTotalPopulation(tempentrynew, VectorStateEnum::STATE_ADULT);//to keep accounting consistent with non-aging version
                                    }
                                }
                                else
                                {
                                    tempentrynew = VectorCohortIndividual::CreateCohort(VectorStateEnum::STATE_ADULT, 0, 0, m_mosquito_weight, tempentry1->GetVectorGenetics(), species_ID);
                                    ApplyMatingGenetics(tempentrynew, VectorMatingStructure(gender_mating_males.begin()->first));
                                    AdultQueues.push_front(tempentrynew);
                                    queueIncrementTotalPopulation(tempentrynew, VectorStateEnum::STATE_ADULT);//to keep accounting consistent with non-aging version
                                }
                            }
                        }
                        else
                        {
                            // now iterate over all males, there will be a slight rounding error
                            for (auto& maletypes : gender_mating_males)
                            {
                                currentProbability = float(maletypes.second)/males;
                                matedPop = currentProbability * tempentry1->GetPopulation();
                                temppop = matedPop / m_mosquito_weight;
                                for (uint32_t i = 0; i < temppop; i++)
                                { 
                                    // now if sugar feeding exists every day or after each feed, and mortality is associated, then check for killing
                                    if((params()->vector_sugar_feeding == VectorSugarFeeding::VECTOR_SUGAR_FEEDING_ON_EMERGENCE_ONLY || 
                                        params()->vector_sugar_feeding == VectorSugarFeeding::VECTOR_SUGAR_FEEDING_EVERY_FEED || 
                                        params()->vector_sugar_feeding == VectorSugarFeeding::VECTOR_SUGAR_FEEDING_EVERY_DAY) && probs()->sugarTrapKilling >0)
                                    {
                                        if(randgen->e() < probs()->sugarTrapKilling)
                                        {
                                            // it dies and no adult created
                                        }
                                        else
                                        {
                                            tempentrynew = VectorCohortIndividual::CreateCohort(VectorStateEnum::STATE_ADULT, 0, 0, m_mosquito_weight, tempentry1->GetVectorGenetics(), species_ID);
                                            ApplyMatingGenetics(tempentrynew, VectorMatingStructure(maletypes.first));
                                            AdultQueues.push_front(tempentrynew);
                                            queueIncrementTotalPopulation(tempentrynew, VectorStateEnum::STATE_ADULT);//to keep accounting consistent with non-aging version
                                        }
                                    }
                                    else
                                    {
                                        tempentrynew = VectorCohortIndividual::CreateCohort(VectorStateEnum::STATE_ADULT, 0, 0, m_mosquito_weight, tempentry1->GetVectorGenetics(), species_ID);
                                        ApplyMatingGenetics(tempentrynew, VectorMatingStructure(maletypes.first));
                                        AdultQueues.push_front(tempentrynew);
                                        queueIncrementTotalPopulation(tempentrynew, VectorStateEnum::STATE_ADULT);//to keep accounting consistent with non-aging version
                                    }
                                }
                            }
                        }
                    }
                    else // male
                    {
                        queueIncrementTotalPopulation(tempentry1);//update counter
                        // Create a new cohort of males with age=0
                        MaleQueues.push_front(VectorCohortAging::CreateCohort(0, 0, tempentry1->GetPopulation(), tempentry1->GetVectorGenetics()));
                    }
                }// new adults of age 0

                ImmatureQueues.erase(iCurrent);
                delete tempentry1;
            }
        }
    }

    // Same as VectorPopulationAging (but this class inherits directly from VectorPopulation?)
    void VectorPopulationIndividual::Update_Male_Queue( float dt )
    {
        males = 0;

        // Use the verbose "foreach" construct here because empty male cohorts (e.g. old vectors) will be removed
        for ( VectorCohortList_t::iterator iList = MaleQueues.begin(); iList != MaleQueues.end(); )
        { 
            IVectorCohortAging *tempentry = NULL;
            if( (*iList)->QueryInterface( GET_IID( IVectorCohortAging ), (void**)&tempentry ) != s_OK )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "(*iList)", "IVectorCohortAging", "VectorCohort" );
            }
            VectorCohortList_t::iterator iCurrent = iList++;

            // increment age and calculate age-dependent mortality
            tempentry->IncreaseAge( dt );
            localadultmortality = dryheatmortality + species()->adultmortality + mortalityFromAge(tempentry->GetAge());

            // Convert mortality rates to mortality probability (can make age dependent)
            float p_local_male_mortality = (float)EXPCDF(-dt * localadultmortality);
            p_local_male_mortality = p_local_male_mortality + (1.0f - p_local_male_mortality) * probs()->outdoorareakilling_male;

            // adults die
            if ((*iCurrent)->GetPopulation() > 0)
            {
                (*iCurrent)->SetPopulation(  (int32_t)((*iCurrent)->GetPopulation() - randgen->binomial_approx((*iCurrent)->GetPopulation(), p_local_male_mortality)) );
            }

            if ((*iCurrent)->GetPopulation() <= 0)
            {
                MaleQueues.erase(iCurrent);
                tempentry->Release();
            }
            else
            {
                queueIncrementTotalPopulation((*iCurrent));
            }
        }
    }

    void VectorPopulationIndividual::AddVectors(VectorMatingStructure _vector_genetics, unsigned long int releasedNumber)
    {
        VectorCohortIndividual* tempentry;
        VectorCohortAging* tempentrym;
        unsigned long int temppop = 0;
        temppop = releasedNumber / m_mosquito_weight;
        // insert into correct Male or Female list
        if (_vector_genetics.GetGender() == VectorGender::VECTOR_FEMALE) //female
        {
            // If unmated, put in Immature with progress 1, so that the females can mate with the local male population.  
            // This will throw an exception if only one of the extra fields is mated.
            if( !_vector_genetics.IsMated() )
            {
                ImmatureQueues.push_front(VectorCohort::CreateCohort(1, releasedNumber, _vector_genetics));
            }
            else
            { 
                // already mated, so go in AdultQueues
                for (uint32_t i = 0; i < temppop; i++)
                {
                    tempentry = VectorCohortIndividual::CreateCohort(VectorStateEnum::STATE_ADULT, 0, 0, m_mosquito_weight, _vector_genetics, species_ID);
                    AdultQueues.push_front(tempentry);
                    queueIncrementTotalPopulation(tempentry, VectorStateEnum::STATE_ADULT);
                }
            }
        }
        else
        {
            tempentrym = VectorCohortAging::CreateCohort(0, 0, releasedNumber, _vector_genetics);
            MaleQueues.push_front(tempentrym);
            queueIncrementTotalPopulation(tempentrym);
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

    void VectorPopulationIndividual::Expose( const IContagionPopulation* cp, float dt, TransmissionRoute::Enum transmission_route )
    {
        VectorCohortList_t list;
        float success_prob = 0;

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

    void VectorPopulationIndividual::ExposeCohortList( const IContagionPopulation* cp, VectorCohortList_t& list, float success_prob, float infection_prob )
    {
        StrainIdentity strainID;
        strainID.SetAntigenID(cp->GetAntigenId());
        float x_infectionWolbachia = 1.0;
        for (auto exposed : list)
        {
            if( exposed->GetVectorGenetics().GetWolbachia() != VectorWolbachia::WOLBACHIA_FREE )
            {
                x_infectionWolbachia = params()->WolbachiaInfectionModification;
            }
            // Determine if there is a new infection
            if ( randgen->e() < species()->acquiremod * x_infectionWolbachia * infection_prob / success_prob )
            {
                // Draw weighted random strain from ContagionPopulation
                cp->ResolveInfectingStrain(&strainID);

                // Set state to STATE_INFECTED and store strainID
                IVectorCohortIndividual *vci = NULL;
                if( (exposed)->QueryInterface( GET_IID( IVectorCohortIndividual ), (void**)&vci ) != s_OK )
                {
                    throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "exposed", "IVectorCohortIndividual", "VectorCohort" );
                }
                vci->AcquireNewInfection(&strainID);

                // Adjust population-level counters
                adult    -= exposed->GetPopulation();
                infected += exposed->GetPopulation();
            }
        }
    }

    // receives a rate, and sends that fraction of mosquitoes to other communities
    unsigned long int VectorPopulationIndividual::Vector_Migration(float migrate, VectorCohortList_t *Migration_Queue)
    { 
        unsigned long int migrating_vectors = 0;

        // only process Adult_Queue

        // check for valid Migration_Queue
        if (Migration_Queue)
        { 

            // Use the verbose "for" construct here because we may be modifying the list and need to protect the iterator.
            for (VectorCohortList_t::iterator iList = AdultQueues.begin(); iList != AdultQueues.end(); /* iList++ */)
            { 
                VectorCohort *tempentry = *iList;
                VectorCohortList_t::iterator iCurrent = iList++;

                // test if each vector will migrate this time step
                if (randgen->e() < migrate)
                { 
                    AdultQueues.erase(iCurrent);
                    Migration_Queue->push_front(tempentry);
                    migrating_vectors++;//increment the number in Migration_Queue
                }
            }
        }

        return migrating_vectors;
    }


    //template void VectorPopulationIndividual::serialize(boost::archive::binary_iarchive & ar, const unsigned int file_version);
    //template void VectorPopulationIndividual::serialize(boost::archive::binary_oarchive & ar, const unsigned int file_version);
}

#if USE_BOOST_SERIALIZATION
BOOST_CLASS_EXPORT(Kernel::VectorPopulationIndividual)
namespace Kernel {
    template<typename Archive>
    void serialize( Archive & ar, VectorPopulationIndividual& vpi, unsigned int version )
    {
        ar & vpi.m_mosquito_weight;
        ar & boost::serialization::base_object<Kernel::VectorPopulation>(vpi);
    }
}
#endif
