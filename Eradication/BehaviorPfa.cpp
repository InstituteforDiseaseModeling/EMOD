
#include "stdafx.h"
#include "BehaviorPfa.h"
#include "IIndividualHuman.h"
#include "Exceptions.h"
#include "SimulationEnums.h"    // Gender::
#include "AssortivityFactory.h"
#include "RANDOM.h"
#include "INodeContext.h"

#include "Log.h"
#include "Debug.h"

SETUP_LOGGING( "BehaviorPfa" )

namespace Kernel 
{
    BEGIN_QUERY_INTERFACE_BODY(BehaviorPfa)
    END_QUERY_INTERFACE_BODY(BehaviorPfa)

    void BehaviorPfa::AddIndividual( IIndividualHumanSTI* sti_person )
    {
        IIndividualHuman* person = sti_person->GetIndividualHuman();

        int agebin_index = parameters->BinIndexForAgeAndSex(person->GetAge(), person->GetGender());

        LOG_DEBUG_F("%s( [ %f, %c, %d ])\n", __FUNCTION__, person->GetAge(), person->GetGender() == Gender::MALE ? 'M' : 'F', person->GetSuid().data);

        if (person->GetGender() == Gender::MALE)
        {
            m_all_males.push_back(sti_person);
            AddToPopulation(m_male_population.at(agebin_index), sti_person, agebin_index);
            new_males[agebin_index]++; //DEBUG
        }
        else
        {
            AddToPopulation(m_female_population.at(agebin_index), sti_person, agebin_index);
            new_females[agebin_index]++; //DEBUG
        }

        sti_person->GetIndividualHuman()->BroadcastEvent( EventTrigger::PFA_Entered );        
    }

    void BehaviorPfa::RemoveIndividual( IIndividualHumanSTI* sti_person )
    {
        IIndividualHuman* person = sti_person->GetIndividualHuman();

        int agebin_index = parameters->BinIndexForAgeAndSex(person->GetAge(), person->GetGender());

        LOG_DEBUG_F("%s( [ %f, %c, %d ])\n", __FUNCTION__, person->GetAge(), person->GetGender() == Gender::MALE ? 'M' : 'F', person->GetSuid().data);

        if (person->GetGender() == Gender::MALE)
        {
            m_all_males.remove((sti_person));
            release_assert( RemoveFromPopulation(sti_person, m_male_population, true) );
        }
        else
        {
            release_assert( RemoveFromPopulation(sti_person, m_female_population, true) );
        }

        sti_person->GetIndividualHuman()->BroadcastEvent( EventTrigger::PFA_Exited );
    }

    void BehaviorPfa::Update( const IdmDateTime& rCurrentTime, float dt )
    {
        LOG_DEBUG_F("%s()\n", __FUNCTION__);
        m_pAssortivity->Update( rCurrentTime, dt );

        m_time_since_last_update += dt;
        if( m_time_since_last_update >= parameters->UpdatePeriod() )
        {
            if (LOG_LEVEL(DEBUG))
            {
                LOG_DEBUG_F( "%s: new individuals since last update:\n", __FUNCTION__ );
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
            std::vector<int> male_queue_index( parameters->GetMaleAgeBinCount(), -1 );
            int new_relationships = 0;
            for (human_list_t::iterator it = m_all_males.begin(); it != m_all_males.end(); /*it++*/)
            {
                IIndividualHumanSTI* sti_male = *it;
                IIndividualHuman* male = sti_male->GetIndividualHuman();

                male->BroadcastEvent( EventTrigger::PFA_SeekingPartner );
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
                        if( m_female_population[ female_agebin_index ].front()->EnterRelationshipNow() )
                        {
                            joint_probability *= 10.0f; //increase probabilty of this female age bin to be selected
                        }
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
                    release_assert( RemoveFromPopulation(sti_female, m_female_population, false) );
                    release_assert( RemoveFromPopulation(sti_male, m_male_population, false) );

                    // Formed a relationship, remove from eligible population
                    human_list_t::iterator current = it++;
                    m_all_males.erase(current);

                    // Form new relationship
                    relationship_fn( sti_male, sti_female, false, nullptr );

                    
                    sti_male->GetIndividualHuman()->BroadcastEvent( EventTrigger::PFA_FoundPartner );
                    sti_female->GetIndividualHuman()->BroadcastEvent( EventTrigger::PFA_FoundPartner );

                    new_relationships++;
                }
                else
                {                                        
                    sti_male->GetIndividualHuman()->BroadcastEvent( EventTrigger::PFA_NoPartnerFound );                                       
                    
                    // No Suitable partner was found, go on to next
                    it++;
                }
            }

            UpdateQueueLengths( m_QueueLengthsAfter );

            m_time_since_last_update = 0.0f;
            LOG_DEBUG_F( "%s: %d new relationships formed this update.\n", __FUNCTION__, new_relationships );
            {
                memset( new_males.data(), 0, new_males.size() * sizeof(int) );
                memset( new_females.data(), 0, new_females.size() * sizeof(int) );
            }
        }
    }

    void BehaviorPfa::BeginUpdate()
    {
    }

    void BehaviorPfa::RegisterIndividual( IIndividualHumanSTI* pIndividualSti )
    {
        // ----------------------------------------------------------------------------------
        // --- NOTE:  The checks for size.  The map and the vector must have the same number
        // --- of entries.  If a person were added twice, they could appear in the vector
        // --- twice but the map once.
        // ----------------------------------------------------------------------------------

        if( pIndividualSti->GetIndividualHuman()->IsDead() )
        {
            return;
        }
        if( pIndividualSti->GetIndividualHuman()->GetGender() == Gender::MALE )
        {
            release_assert( m_RegisteredIDtoIndexMapMales.count( pIndividualSti->GetSuid().data ) == 0 );
            m_RegisteredMales.push_back( pIndividualSti );
            m_RegisteredIDtoIndexMapMales.insert( std::make_pair( pIndividualSti->GetSuid().data, (m_RegisteredMales.size() -1) ) );
            release_assert( m_RegisteredMales.size() == m_RegisteredIDtoIndexMapMales.size() );
        }
        else
        {
            release_assert( m_RegisteredIDtoIndexMapFemales.count( pIndividualSti->GetSuid().data ) == 0 );
            m_RegisteredFemales.push_back( pIndividualSti );
            m_RegisteredIDtoIndexMapFemales.insert( std::make_pair( pIndividualSti->GetSuid().data, (m_RegisteredFemales.size() -1) ) );
            release_assert( m_RegisteredFemales.size() == m_RegisteredIDtoIndexMapFemales.size() );
        }
    }

    void RemoveFromRegistration( IIndividualHumanSTI* pIndividualSti,
                                 std::vector<IIndividualHumanSTI*>& rRegistered,
                                 std::map<uint32_t, uint32_t>& rIDtoIndexMap )
    {
        if( rIDtoIndexMap.count( pIndividualSti->GetSuid().data ) == 0 )
        {
            return;
        }
        release_assert( rRegistered.size() > 0 );

        uint32_t index = rIDtoIndexMap.at( pIndividualSti->GetSuid().data );
        if( index != (rRegistered.size() - 1) )
        {
            uint32_t last_id = rRegistered.back()->GetSuid().data;
            rIDtoIndexMap[ last_id ] = index;
            rRegistered[ index ] = rRegistered.back();
        }
        rIDtoIndexMap.erase( pIndividualSti->GetSuid().data );
        rRegistered.pop_back();
        release_assert( rRegistered.size() == rIDtoIndexMap.size() );
    }

    void BehaviorPfa::UnregisterIndividual( IIndividualHumanSTI* pIndividualSti )
    {
        if( pIndividualSti->GetIndividualHuman()->GetGender() == Gender::MALE )
        {
            RemoveFromRegistration( pIndividualSti, m_RegisteredMales, m_RegisteredIDtoIndexMapMales );
        }
        else
        {
            RemoveFromRegistration( pIndividualSti, m_RegisteredFemales, m_RegisteredIDtoIndexMapFemales );
        }
    }

    bool BehaviorPfa::StartNonPfaRelationship( IIndividualHumanSTI* pIndividualSti,
                                               const IPKeyValue& rPartnerHasIP,
                                               Sigmoid* pCondomUsage )
    {
        IIndividualHuman* p_individual = pIndividualSti->GetIndividualHuman();

        std::vector<IIndividualHumanSTI*>* p_possible_partners;
        if( p_individual->GetGender() == Gender::MALE )
        {
            p_possible_partners = & m_RegisteredFemales;
        }
        else
        {
            p_possible_partners = & m_RegisteredMales;
        }

        bool partner_found = false;
        int num_attempts = 0;
        while( !partner_found && (num_attempts < p_possible_partners->size()) )
        {
            int selected_index = int( this->m_rng->e() * float(p_possible_partners->size()) );
            IIndividualHumanSTI* p_new_partner = (*p_possible_partners)[ selected_index ];

            if( p_new_partner == nullptr ) continue; // can be null due to death

            bool has_ip = true;
            if( rPartnerHasIP.IsValid() )
            {
                has_ip = p_new_partner->GetIndividualHuman()->GetProperties()->Contains(rPartnerHasIP);
            }

            if( has_ip && !areInRelationship(pIndividualSti, p_new_partner) )
            {
                partner_found = true;

                if( pIndividualSti->GetIndividualHuman()->GetGender() == Gender::MALE )
                    relationship_fn( pIndividualSti, p_new_partner, true, pCondomUsage );
                else
                    relationship_fn( p_new_partner, pIndividualSti, true, pCondomUsage );
            }

            ++num_attempts;
        }

        return partner_found;
    }


    void BehaviorPfa::Print(const char *rel_type) const
    {
        LOG_DEBUG_F("female %s PFA queue lengths:\t", rel_type);
        stringstream ss;

        ss << "[ ";
        int female_agebin_count = parameters->GetFemaleAgeBinCount();
        for (int female_agebin_index = 0; female_agebin_index < female_agebin_count; female_agebin_index++) {
            ss << m_female_population[female_agebin_index].size()  << ' ';
        }
        ss << "]" << endl;
        LOG_DEBUG_F(ss.str().c_str());
        ss.str("");

        LOG_DEBUG_F("male %s PFA queue lengths:\t", rel_type);
        ss << "[ ";
        int male_agebin_count = parameters->GetMaleAgeBinCount();
        for (int male_agebin_index = 0; male_agebin_index < male_agebin_count; male_agebin_index++) {
            ss << m_male_population[male_agebin_index].size()  << ' ';
        }

        ss << "]" << endl;
        LOG_DEBUG_F(ss.str().c_str());
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
                                                 float selectionThreshold,
                                                 RANDOMBASE *prng, 
                                                 RelationshipCreator relationship_fn )
    {
        BehaviorPfa* pfa =  _new_ BehaviorPfa( params, selectionThreshold, prng, relationship_fn );
        pfa->Configure( pConfig );
        return pfa ;
    }

    BehaviorPfa::BehaviorPfa()
        : m_cum_prob_threshold(0.0f)
        , m_time_since_last_update(0.0f)
        , m_RegisteredMales()
        , m_RegisteredFemales()
        , m_RegisteredIDtoIndexMapMales()
        , m_RegisteredIDtoIndexMapFemales()
        , m_all_males()
        , m_male_population()
        , m_female_population()
        , m_population_map()
        , preference()
        , desired_flow()
        , parameters(nullptr)
        , m_rng(nullptr)
        , relationship_fn(nullptr)
        , m_pAssortivity( nullptr )
        , m_QueueLengthsBefore()
        , m_QueueLengthsAfter()
        , new_males()
        , new_females()
    {
    }

    BehaviorPfa::BehaviorPfa( const IPairFormationParameters* params, 
                              float selectionThreshold, 
                              RANDOMBASE *prng, 
                              RelationshipCreator creator )
        : m_cum_prob_threshold(selectionThreshold)
        , m_time_since_last_update(0.0f)
        , m_RegisteredMales()
        , m_RegisteredFemales()
        , m_RegisteredIDtoIndexMapMales()
        , m_RegisteredIDtoIndexMapFemales()
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

        // these will need to hold all the people of the gender so allocating for 600k
        m_RegisteredMales.reserve(300000);
        m_RegisteredFemales.reserve(300000);
    }

    BehaviorPfa::~BehaviorPfa()
    {
        delete m_pAssortivity ;
    }

    bool BehaviorPfa::Configure(const Configuration *config)
    {
        auto assort_config = Configuration::CopyFromElement( (*config)["Assortivity"], config->GetDataLocation() );
        bool ret = m_pAssortivity->Configure( assort_config );

        delete assort_config ;

        return ret ;
    }

    void BehaviorPfa::AddToPopulation( human_list_t &age_bin_list, IIndividualHumanSTI* sti_person, int age_bin_index )
    {
        LOG_DEBUG_F("%s( [ %d ] )\n", __FUNCTION__, sti_person->GetSuid().data);
        human_list_t::iterator it;
        if( sti_person->EnterPfaNow() )
        {
            age_bin_list.push_front( sti_person ); // If we want the person to enter the PFA immediately, we put them at the front of the queue so that it happens sooner.
            it = age_bin_list.begin();             // It still does not guarantee that they get into a relationship, but greatly increases the probability.                                        
        }
        else
        {
            age_bin_list.push_back(sti_person);
            it = age_bin_list.end();
            --it;
        }

        pair_t pair = pair_t::pair(age_bin_index, it);

        if (m_population_map.find(sti_person) != m_population_map.end())
        {
            // Found, add iterator to iterator vector
            LOG_DEBUG_F("%s( [ %d ] ) FOUND, size is %d\n", __FUNCTION__, sti_person->GetSuid().data, m_population_map[sti_person].size());
            m_population_map[sti_person].push_back( pair );
        } else {
            // Not found, create new entry.
            LOG_DEBUG_F("%s( [ %d ] ) NOT FOUND\n", __FUNCTION__, sti_person->GetSuid().data);
            map_entry_t map_entry;
            map_entry.push_back( pair );
            m_population_map[sti_person] = map_entry;
        }
    }

    bool BehaviorPfa::RemoveFromPopulation( IIndividualHumanSTI* sti_person, population_t &target_population, bool remove_all)
    {
        LOG_DEBUG_F("%s( [ %d ] ) - ", __FUNCTION__, sti_person->GetSuid().data);
        bool found = false;

        if (m_population_map.find(sti_person) != m_population_map.end())
        {
            LOG_DEBUG("found and erasing.\n");
            map_entry_t& map_entry = m_population_map.at(sti_person);

            bool keep_going = true;
            while( keep_going && map_entry.size() > 0 )
            {
                target_population.at(map_entry.back().first).erase(map_entry.back().second);
                map_entry.pop_back();
                found = true;
                keep_going = remove_all;
            }

            if( map_entry.size() == 0)
            {
                m_population_map.erase(sti_person);
            }
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
            if( rel->GetFemalePartnerId().data == person2->GetSuid().data )
            {
                LOG_DEBUG_F( "PFA attempted to create duplicate relationship between male individual %d and female individual %d\n", person1->GetSuid().data, person2->GetSuid().data );
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

    REGISTER_SERIALIZABLE(BehaviorPfa);

    void BehaviorPfa::serialize(IArchive& ar, BehaviorPfa* obj)
    {
        BehaviorPfa& pfa = *obj;
        ar.labelElement("m_cum_prob_threshold"    ) & pfa.m_cum_prob_threshold;
        ar.labelElement("m_time_since_last_update") & pfa.m_time_since_last_update;

        // preference is a member variable only so we don't allocate the lenght of the vector every time.
        //ar.labelElement("preference"              ) & pfa.preference;

        ar.labelElement("desired_flow"            ) & pfa.desired_flow;
        ar.labelElement("m_QueueLengthsBefore"    ) & pfa.m_QueueLengthsBefore;
        ar.labelElement("m_QueueLengthsAfter"     ) & pfa.m_QueueLengthsAfter;
        ar.labelElement("new_males"               ) & pfa.new_males;
        ar.labelElement("new_females"             ) & pfa.new_females;

        //typedef list<IIndividualHumanSTI*> human_list_t;
        //typedef vector<human_list_t> population_t;
        //typedef pair<int, human_list_t::iterator> map_entry_t;
        //typedef map<IIndividualHumanSTI*, map_entry_t> population_map_t;

        //human_list_t m_all_males;
        //population_t m_male_population;
        //population_t m_female_population;
        //population_map_t m_population_map;


        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // !!! Needs to be set during serialization
        //const IPairFormationParameters* parameters;
        //RANDOMBASE* m_rng;
        //RelationshipCreator relationship_fn;
        //IAssortivity* m_pAssortivity ;
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    }
}
