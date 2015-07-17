/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"
#include "BehaviorPfa.h"
#include "IIndividualHuman.h"
#include "Exceptions.h"
#include "SimulationEnums.h"    // Gender::
#include "AssortivityFactory.h"

#include "Log.h"
#include "Debug.h"
static const char * _module = "BehaviorPfa";

namespace Kernel {

    void BehaviorPfa::SetUpdatePeriod( float update_period )
    {
        LOG_DEBUG_F("%s( %f )\n", __FUNCTION__, update_period);
        if (update_period < 0.0f)
        {
            throw OutOfRangeException(__FILE__, __LINE__, __FUNCTION__, "update_period", update_period, 0.0f);
        }

        m_update_period = update_period;
    }

    void BehaviorPfa::AddIndividual( IIndividualHumanSTI* sti_person )
    {
        IIndividualHuman* person = NULL;
        if (sti_person->QueryInterface(GET_IID(IIndividualHuman), (void**)&person) != s_OK)
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "sti_person", "IIndividualHuman*", "IIndividualHumanSTI*");
        }
        int agebin_index = parameters->BinIndexForAgeAndSex(person->GetAge(), person->GetGender());

        LOG_DEBUG_F("%s( [ %f, %c, %d ])\n", __FUNCTION__, person->GetAge(), person->GetGender() == Gender::MALE ? 'M' : 'F', person->GetSuid().data);

        if (person->GetGender() == Gender::MALE)
        {
            m_all_males.push_back(sti_person);
            AddToPopulation(m_male_population.at(agebin_index), sti_person, agebin_index);
#ifdef _DEBUG
            new_males[agebin_index]++;
#endif
        }
        else
        {
            AddToPopulation(m_female_population.at(agebin_index), sti_person, agebin_index);
#ifdef _DEBUG
            new_females[agebin_index]++;
#endif
        }
    }

    void BehaviorPfa::RemoveIndividual( IIndividualHumanSTI* sti_person )
    {
        IIndividualHuman* person = NULL;
        if (sti_person->QueryInterface(GET_IID(IIndividualHuman), (void**)&person) != s_OK)
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "sti_person", "IIndividualHuman*", "IIndividualHumanSTI*");
        }
        int agebin_index = parameters->BinIndexForAgeAndSex(person->GetAge(), person->GetGender());

        LOG_DEBUG_F("%s( [ %f, %c, %d ])\n", __FUNCTION__, person->GetAge(), person->GetGender() == Gender::MALE ? 'M' : 'F', person->GetSuid().data);

        if (person->GetGender() == Gender::MALE)
        {
            m_all_males.remove((sti_person));
            release_assert( RemoveFromPopulation(sti_person, m_male_population) );
        }
        else
        {
            release_assert( RemoveFromPopulation(sti_person, m_female_population) );
        }
    }

    void BehaviorPfa::Update( const IdmDateTime& rCurrentTime, float dt )
    {
        LOG_DEBUG_F("%s()\n", __FUNCTION__);
        m_pAssortivity->Update( rCurrentTime, dt );

        m_time_since_last_update += dt;
        if (m_time_since_last_update >= m_update_period)
        {
            if (LOG_LEVEL(INFO))
            {
                LOG_INFO_F( "%s: new individuals since last update:\n", __FUNCTION__ );
                cout << "[ ";
                for (auto count : new_males) {
                    cout << count << ' ';
                }
                cout << ']' << endl;
                cout << "[ ";
                for (auto count : new_females) {
                    cout << count << ' ';
                }
                cout << ']' << endl;
            }

            UpdateQueueLengths( m_QueueLengthsBefore );

            int total_male_queue_index = -1 ;
            std::vector<int> male_queue_index(20,-1) ;
            int new_relationships = 0;
            for (human_list_t::iterator it = m_all_males.begin(); it != m_all_males.end(); /*it++*/)
            {
                IIndividualHumanSTI* sti_male = *it;
                IIndividualHuman* male = NULL;
                if (sti_male->QueryInterface(GET_IID(IIndividualHuman), (void**)&male))
                {
                    throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "sti_male", "IIndividualHuman", "IIndividualHumanSTI" );
                }
                int male_agebin_index = parameters->BinIndexForAgeAndSex(male->GetAge(), Gender::MALE);

                total_male_queue_index++ ;
                male_queue_index[ male_agebin_index ]++ ;

                float cumulative_probability = 0.0f;
                auto& joint_probability_table = parameters->JointProbabilityTable();
                int female_agebin_count = parameters->GetFemaleAgeBinCount();
                for (int female_agebin_index = 0; female_agebin_index < female_agebin_count; female_agebin_index++)
                {
                    if (m_female_population[female_agebin_index].size() > 0)
                    {
                        float joint_probability         = joint_probability_table.at(male_agebin_index)[female_agebin_index];
                        preference[female_agebin_index] = joint_probability;
                        cumulative_probability         += joint_probability;
                    }
                    else
                    {
                        preference[female_agebin_index] = 0.0f;
                    }
                }

                float current_time = male->GetParent()->GetTime().time ;
                LOG_DEBUG_F("Eligibility: time, male age, male_inf,male id, male bin index, total queue index, bin queue index, cum prob, match:    %4.1f, %5.2f, %d, %5d, %2d, %3d, %2d, %4.3f, %d\n",
                    current_time,
                    (male->GetAge()/365.0f),
                    male->IsInfected(),
                    male->GetSuid().data,
                    male_agebin_index,
                    total_male_queue_index,
                    male_queue_index[ male_agebin_index ],
                    cumulative_probability,
                    (cumulative_probability > m_cum_prob_threshold) );

                if (cumulative_probability > m_cum_prob_threshold)
                {
                    float dice_roll = m_rng->e() * cumulative_probability;
                    int female_agebin_index = 0;
                    float cumulative_preference = preference[female_agebin_index];
                    while (dice_roll > cumulative_preference)
                    {
                        female_agebin_index++;
                        cumulative_preference += preference[female_agebin_index];
                    }

                    IIndividualHumanSTI* sti_female = m_pAssortivity->SelectPartner( sti_male, m_female_population[female_agebin_index] );

                    // CHECK IF sti_male and sti_female are already in a relationship!
                    if( (sti_female == nullptr) || areInRelationship( sti_male, sti_female ) )
                    {
                        it++;
                        continue;
                    }
                    LOG_DEBUG_F("%s: - new relationship (%d, %d)\n", __FUNCTION__, male->GetSuid().data, sti_female->GetSuid().data);
                    release_assert( RemoveFromPopulation(sti_female, m_female_population) );
                    release_assert( RemoveFromPopulation(sti_male, m_male_population) );

                    // Formed a relationship, remove from eligible population
                    human_list_t::iterator current = it++;
                    m_all_males.erase(current);

                    // Form new relationship
                    relationship_fn(sti_male, sti_female);
                    new_relationships++;
                }
                else
                {
                    // No Suitable partner was found, go on to next
                    it++;
                }
            }

            UpdateQueueLengths( m_QueueLengthsAfter );

            m_time_since_last_update = 0.0f;
            LOG_INFO_F( "%s: %d new relationships formed this update.\n", __FUNCTION__, new_relationships );
            {
                memset( new_males.data(), 0, new_males.size() * sizeof(int) );
                memset( new_females.data(), 0, new_females.size() * sizeof(int) );
            }
        }
    }

    void BehaviorPfa::Print(const char *rel_type) const
    {
        LOG_WARN_F("female %s PFA queue lengths:\t", rel_type);
        stringstream ss;

        ss << "[ ";
        int female_agebin_count = parameters->GetFemaleAgeBinCount();
        for (int female_agebin_index = 0; female_agebin_index < female_agebin_count; female_agebin_index++) {
            ss << m_female_population[female_agebin_index].size()  << ' ';
        }
        ss << "]" << endl;
        LOG_WARN_F(ss.str().c_str());
        ss.str("");

        LOG_WARN_F("male %s PFA queue lengths:\t", rel_type);
        ss << "[ ";
        int male_agebin_count = parameters->GetMaleAgeBinCount();
        for (int male_agebin_index = 0; male_agebin_index < male_agebin_count; male_agebin_index++) {
            ss << m_male_population[male_agebin_index].size()  << ' ';
        }

        ss << "]" << endl;
        LOG_WARN_F(ss.str().c_str());
        ss.str("");
    }

    const map<int, vector<float>>& BehaviorPfa::GetAgeBins()
    {
        LOG_DEBUG_F("%s()\n", __FUNCTION__);
        return parameters->GetAgeBins();
    }

    const map<int, vector<float>>& BehaviorPfa::GetDesiredFlow()
    {
        LOG_DEBUG_F("%s()\n", __FUNCTION__);
        return desired_flow;
    }

    const map<int, vector<int>>& BehaviorPfa::GetQueueLengthsBefore()
    {
        return m_QueueLengthsBefore ;
    }

    const map<int, vector<int>>& BehaviorPfa::GetQueueLengthsAfter()
    {
        return m_QueueLengthsAfter ;
    }

    IPairFormationAgent* BehaviorPfa::CreatePfa( const Configuration* pConfig, 
                                                 const IPairFormationParameters* params, 
                                                 float updatePeriod, 
                                                 float selectionThreshold,
                                                 RANDOMBASE *prng, 
                                                 RelationshipCreator relationship_fn )
    {
        BehaviorPfa* pfa =  _new_ BehaviorPfa( params, updatePeriod, selectionThreshold, prng, relationship_fn );
        pfa->Configure( pConfig );
        return pfa ;
    }

    BehaviorPfa::BehaviorPfa( const IPairFormationParameters* params, 
                              float updatePeriod, 
                              float selectionThreshold, 
                              RANDOMBASE *prng, 
                              RelationshipCreator creator )
        : m_update_period(0.0f)
        , m_cum_prob_threshold(selectionThreshold)
        , m_time_since_last_update(0.0f)
        , m_all_males()
        , m_male_population(params->GetMaleAgeBinCount())
        , m_female_population(params->GetFemaleAgeBinCount())
        , m_population_map()
        , preference(params->GetFemaleAgeBinCount())
        , desired_flow()
        , parameters(params)
        , m_rng(prng)
        , relationship_fn(creator)
        , m_pAssortivity( AssortivityFactory::CreateAssortivity( params->GetRelationshipType(), prng ) )
        , m_QueueLengthsBefore()
        , m_QueueLengthsAfter()
        , new_males()
        , new_females()
    {
        release_assert( m_pAssortivity != nullptr );

        SetUpdatePeriod( updatePeriod );  // performs checks on value before setting

        auto& agebins = parameters->GetAgeBins();
        for (int sex = Gender::MALE; sex <= Gender::FEMALE; sex++)
        {
            int bin_count = agebins.at(sex).size();
            desired_flow[sex].resize(bin_count);
            m_QueueLengthsBefore[sex].resize(bin_count);
            m_QueueLengthsAfter[sex].resize(bin_count);
        }
        new_males.resize(agebins.at(Gender::MALE).size());
        new_females.resize(agebins.at(Gender::FEMALE).size());
    }

    BehaviorPfa::~BehaviorPfa()
    {
        delete m_pAssortivity ;
    }

    bool BehaviorPfa::Configure(const Configuration *config)
    {
        auto assort_config = Configuration::CopyFromElement( (*config)["Assortivity"] );
        
        bool ret = m_pAssortivity->Configure( assort_config );

        delete assort_config ;

        return ret ;
    }

    void BehaviorPfa::AddToPopulation( human_list_t &age_bin_list, IIndividualHumanSTI* sti_person, int age_bin_index )
    {
        LOG_DEBUG_F("%s( [ %d ] )\n", __FUNCTION__, sti_person->GetSuid().data);
        age_bin_list.push_back(sti_person);
        human_list_t::iterator it = age_bin_list.end();
        m_population_map[sti_person] = map_entry_t::pair(age_bin_index, --it);
    }

    bool BehaviorPfa::RemoveFromPopulation( IIndividualHumanSTI* sti_person, population_t &target_population )
    {
        LOG_DEBUG_F("%s( [ %d ] ) - ", __FUNCTION__, sti_person->GetSuid().data);
        bool found = false;

        if (m_population_map.find(sti_person) != m_population_map.end())
        {
            LOG_DEBUG("found and erasing.\n");
            map_entry_t& map_entry = m_population_map.at(sti_person);
            target_population.at(map_entry.first).erase(map_entry.second);
            m_population_map.erase(sti_person);
            found = true;
        }
        else
        {
            LOG_DEBUG("not found.\n");
        }

        return found;
    }

    // small utility function that helps PFA not put a couple into a duplicate relationship
    bool
    BehaviorPfa::areInRelationship(
        IIndividualHumanSTI* person1, // would rather make this const but GetRelationships is non-const
        const IIndividualHumanSTI* person2
    )
    const
    {
        bool existing_relationship = false;
        auto male_rels = person1->GetRelationships();
        for( auto rel: male_rels )
        {
            if( rel->FemalePartner()->GetSuid().data == person2->GetSuid().data )
            {
                LOG_WARN_F( "PFA attempted to create duplicate relationship between male individual %d and female individual %d\n", person1->GetSuid().data, person2->GetSuid().data );
                existing_relationship = true;
                break; // preserve single exit point from function
            }
        }
        return existing_relationship;
    }

    void BehaviorPfa::UpdateQueueLengths( map<int, vector<int>>& rQueueLengths )
    {
        for( int age_bin_index = 0 ; age_bin_index < m_male_population.size() ; age_bin_index++ )
        {
            rQueueLengths[ Gender::MALE ][age_bin_index] = m_male_population.at( age_bin_index ).size() ;
        }

        for( int age_bin_index = 0 ; age_bin_index < m_female_population.size() ; age_bin_index++ )
        {
            rQueueLengths[ Gender::FEMALE ][age_bin_index] = m_female_population.at( age_bin_index ).size() ;
        }
    }
}
