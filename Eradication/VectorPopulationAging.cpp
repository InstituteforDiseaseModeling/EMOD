/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"
#include "Debug.h"
#include "VectorPopulationAging.h"

#include "Log.h"
#include "NodeVector.h"
#include "VectorCohortAging.h"
#include "VectorPopulation.h"
#include "Vector.h"

#ifdef randgen
#undef randgen
#endif
#include "RANDOM.h"
#define randgen (m_context->GetRng())

static const char * _module = "VectorPopulationAging";

namespace Kernel
{ 
    // QI stuff
    BEGIN_QUERY_INTERFACE_DERIVED(VectorPopulationAging, VectorPopulation)
    END_QUERY_INTERFACE_DERIVED(VectorPopulationAging, VectorPopulation)

    VectorPopulationAging::VectorPopulationAging() : VectorPopulation()
    { 
    }

    void VectorPopulationAging::InitializeVectorQueues(unsigned int adults, unsigned int _infectious)
    { 
        adult = adults;
        infectious = _infectious;

        if (adult > 0)
        { 
            // adult initialized at age 0
            AdultQueues.push_front( VectorCohortAging::CreateCohort( 0, 0, adult, VectorMatingStructure( VectorGender::VECTOR_FEMALE ) ) );
            MaleQueues.push_front( VectorCohortAging::CreateCohort( 0, 0, adult, VectorMatingStructure( VectorGender::VECTOR_MALE ) ) );
            males = (int32_t)adult;
        }

        if (infectious > 0)
        { 
            // infectious initialized at age 20
            InfectiousQueues.push_front( VectorCohortAging::CreateCohort( 20, 0, infectious, VectorMatingStructure( VectorGender::VECTOR_FEMALE ) ) );
        }
    }

    VectorPopulationAging *VectorPopulationAging::CreatePopulation(INodeContext *context, std::string species_name, int32_t initial_adults, int32_t initial_infectious)
    { 
        VectorPopulationAging *newpopulation = _new_ VectorPopulationAging();
        newpopulation->Initialize(context, species_name, initial_adults, initial_infectious);

        return newpopulation;
    }

    VectorPopulationAging::~VectorPopulationAging()
    { 
    }

    void VectorPopulationAging::Update_Infectious_Queue( float dt )
    { 
        infectious = 0;
        // Use the verbose "foreach" construct here because empty infectious cohorts (e.g. old vectors) will be removed
        for ( VectorCohortList_t::iterator iList = InfectiousQueues.begin(); iList != InfectiousQueues.end(); )
        { 
            VectorCohortAging *tempentry = static_cast<VectorCohortAging *>(*iList);
            release_assert( tempentry );

            VectorCohortList_t::iterator iCurrent = iList++;

            // increment age and calculate age-dependent mortality
            tempentry->IncreaseAge( dt );
            localadultmortality = dryheatmortality + species()->adultmortality + mortalityFromAge(tempentry->GetAge());

            ProcessFeedingCycle(dt, tempentry, VectorStateEnum::STATE_INFECTIOUS);

            if (tempentry->GetPopulation() <= 0)
            { 
                InfectiousQueues.erase(iCurrent);
                delete tempentry;
            }
            else
            { 
                queueIncrementTotalPopulation(tempentry, VectorStateEnum::STATE_INFECTIOUS);// update INFECTIOUS counters
            }
        }
    }

    void VectorPopulationAging::Update_Infected_Queue( float dt )
    { 
        infected = 0;
        float temperature = m_context->GetLocalWeather()->airtemperature();

        // Use the verbose "foreach" construct here because empty or completely-progressed queues will be removed
        for ( VectorCohortList_t::iterator iList = InfectedQueues.begin(); iList != InfectedQueues.end();  )
        { 
            VectorCohortAging *tempentry = static_cast<VectorCohortAging *>(*iList);
            release_assert( tempentry );

            VectorCohortList_t::iterator iCurrent = iList++;

            // increment age and progress of sporogeny; calculate age-dependent mortality
            tempentry->IncreaseAge( dt );
            tempentry->IncreaseProgress( ( species()->infectedarrhenius1 * exp( -species()->infectedarrhenius2 / ( temperature + (float)CELSIUS_TO_KELVIN ) ) ) * dt );
            localadultmortality = dryheatmortality + species()->adultmortality + mortalityFromAge(tempentry->GetAge());

            ProcessFeedingCycle(dt, tempentry, VectorStateEnum::STATE_INFECTED);

            // done with this queue if it is fully progressed or is empty
            if (tempentry->GetProgress() >= 1 || tempentry->GetPopulation() <= 0)
            { 
                if (tempentry->GetPopulation() > 0)
                { 
                    InfectiousQueues.push_front(VectorCohortAging::CreateCohort(tempentry->GetAge(), 0, tempentry->GetPopulation(), tempentry->GetVectorGenetics()));
                    queueIncrementTotalPopulation(tempentry, VectorStateEnum::STATE_INFECTIOUS); // update INFECTIOUS counters
                }

                InfectedQueues.erase(iCurrent);
                delete tempentry;
            }
            else
            { 
                queueIncrementTotalPopulation(tempentry, VectorStateEnum::STATE_INFECTED); // update INFECTED counters
            }
        }
    }

    void VectorPopulationAging::Update_Adult_Queue( float dt )
    { 
        adult = 0;
        // Use the verbose "foreach" construct here because empty adult cohorts (e.g. old vectors) will be removed
        for ( VectorCohortList_t::iterator iList = AdultQueues.begin(); iList != AdultQueues.end(); )
        { 
            VectorCohortAging *tempentry = static_cast<VectorCohortAging *>(*iList);
            release_assert( tempentry );
            VectorCohortList_t::iterator iCurrent = iList++;

            // increment age and calculate age-dependent mortality
            tempentry->IncreaseAge( dt );
            localadultmortality = dryheatmortality + species()->adultmortality + mortalityFromAge(tempentry->GetAge());

            //uint32_t newinfected = min( ProcessFeedingCycle(dt, tempentry, STATE_ADULT), (uint32_t) tempentry->GetPopulation() );
            uint32_t newinfected = ProcessFeedingCycle(dt, tempentry, VectorStateEnum::STATE_ADULT);
            if (newinfected > (uint32_t)tempentry->GetPopulation()) { newinfected = tempentry->GetPopulation(); } // correct for too high

            if (newinfected > 0)
            { 
                tempentry->SetPopulation( tempentry->GetPopulation() - newinfected );
                VectorCohortAging* tempentrynew = VectorCohortAging::CreateCohort(tempentry->GetAge(), 0, newinfected, tempentry->GetVectorGenetics());
                InfectedQueues.push_front(tempentrynew);
                queueIncrementTotalPopulation(tempentrynew, VectorStateEnum::STATE_INFECTED);// update INFECTED counters
            }
       
            if (tempentry->GetPopulation() <= 0)
            { 
                AdultQueues.erase(iCurrent);
                delete tempentry;
            }
            else
            { 
                queueIncrementTotalPopulation(tempentry, VectorStateEnum::STATE_ADULT); // update ADULT counters
            }
        }
    }

    void VectorPopulationAging::Update_Immature_Queue( float dt )
    { 
        unsigned long int tempPop = 0;
        float currentProbability = 0.0;

        float p_local_mortality = 0;

        localadultmortality = dryheatmortality + species()->adultmortality;

        // calculate local mortality, includes outdoor area killling, converting rates to probabilities
        p_local_mortality = (float)(1.0f - exp(-dt * localadultmortality));
        p_local_mortality = p_local_mortality + (1.0f - p_local_mortality) * probs()->outdoorareakilling;

        // Use the verbose "for" construct here because we may be modifying the list and need to protect the iterator.
        for (VectorCohortList_t::iterator iList = ImmatureQueues.begin(); iList != ImmatureQueues.end(); /* iList++ */)
        { 
            VectorCohort *tempentry1 = *iList;
            release_assert( tempentry1 );
            VectorCohortList_t::iterator iCurrent = iList++;

            tempentry1->IncreaseProgress( dt * species()->immaturerate ); // introduce climate dependence here if we can figure it out
            localadultmortality = dryheatmortality + species()->adultmortality;
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
                            ApplyMatingGenetics(tempentry1, VectorMatingStructure(gender_mating_males.begin()->first));
                            queueIncrementTotalPopulation(tempentry1, VectorStateEnum::STATE_ADULT);
                            AdultQueues.push_front(VectorCohortAging::CreateCohort(0, 0, tempentry1->GetPopulation(), tempentry1->GetVectorGenetics()));
                        }
                        else
                        {
                            // now iterate over all males, there will be a slight rounding error
                            for (auto& maletypes : gender_mating_males)
                            {
                                currentProbability = float(maletypes.second)/males;
                                tempPop = currentProbability * tempentry1->GetPopulation();
                                VectorCohortAging* tempentrynew = VectorCohortAging::CreateCohort(0, 0, tempPop, tempentry1->GetVectorGenetics());
                                ApplyMatingGenetics(tempentrynew, VectorMatingStructure(maletypes.first));
                                queueIncrementTotalPopulation(tempentrynew, VectorStateEnum::STATE_ADULT);
                                AdultQueues.push_front(tempentrynew);
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

    void VectorPopulationAging::Update_Male_Queue( float dt )
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

    void VectorPopulationAging::AddVectors(VectorMatingStructure _vector_genetics, unsigned long int releasedNumber)
    {
        VectorCohortAging* tempentry;
        
        // insert into correct Male or Female list
        if (_vector_genetics.GetGender() == VectorGender::VECTOR_FEMALE) //female
        {
            // if unmated, put in Immature with progress 1, so that the females can mate with the local male population.  This will throw an exception if only one of the extra fields is mated.
            if( !_vector_genetics.IsMated() )
            {
                ImmatureQueues.push_front(VectorCohort::CreateCohort(1, releasedNumber, _vector_genetics));
            }
            else
            { 
                // already mated, so go in AdultQueues
                tempentry = VectorCohortAging::CreateCohort(0, 0, releasedNumber, _vector_genetics);
                AdultQueues.push_front(tempentry);
                queueIncrementTotalPopulation(tempentry, VectorStateEnum::STATE_ADULT);//update counter
            }
        }
        else
        {
            tempentry = VectorCohortAging::CreateCohort(0, 0, releasedNumber, _vector_genetics);
            MaleQueues.push_front(tempentry);
            queueIncrementTotalPopulation(tempentry);
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

#if 0
    template<class Archive>
    void VectorPopulationAging::serialize_inner(Archive & ar, const unsigned int file_version)
    { 
        // Register derived types - N/A

        // Serialize fields - N/A

        // Serialize base class - N/A
        ar & boost::serialization::base_object<Kernel::VectorPopulation>(*this);
    }

    template void VectorPopulationAging::serialize(boost::archive::binary_iarchive & ar, const unsigned int file_version);
    template void VectorPopulationAging::serialize(boost::archive::binary_oarchive & ar, const unsigned int file_version);
#endif
}

#if USE_BOOST_SERIALIZATION
#include <boost/serialization/export.hpp>
BOOST_CLASS_EXPORT(Kernel::VectorPopulationAging)

namespace Kernel {
    template< typename Archive >
    void serialize( Archive& ar, VectorPopulationAging &obj, unsigned int file_version )
    {
        ar & boost::serialization::base_object<VectorPopulation>(obj);
    }
    template void serialize(boost::archive::binary_iarchive & ar, VectorPopulationAging&, const unsigned int file_version);
    template void serialize(boost::archive::binary_oarchive & ar, VectorPopulationAging&, const unsigned int file_version);
}
#endif
