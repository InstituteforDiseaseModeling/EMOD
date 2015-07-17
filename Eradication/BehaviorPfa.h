/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include "IPairFormationAgent.h"
#include <queue>
#include <list>
#include <vector>
#include <map>
#include "IPairFormationParameters.h"
#include "RANDOM.h"
#include "IRelationship.h"
#include "IAssortivity.h"

namespace Kernel {

    class IDMAPI BehaviorPfa : public IPairFormationAgent 
    {
    public:
        static IPairFormationAgent* CreatePfa( const Configuration* pConfig,
                                               const IPairFormationParameters* params,
                                               float updatePeriod,
                                               float selectionThreshold,
                                               RANDOMBASE* prng, 
                                               RelationshipCreator rc );

        virtual void SetUpdatePeriod(float);
        virtual void AddIndividual(IIndividualHumanSTI*);

        virtual void RemoveIndividual(IIndividualHumanSTI*);

        virtual void Update( const IdmDateTime& rCurrentTime, float dt );
        virtual const map<int, vector<float>>& GetAgeBins();
        virtual const map<int, vector<float>>& GetDesiredFlow();
        virtual const map<int, vector<int>>& GetQueueLengthsBefore();
        virtual const map<int, vector<int>>& GetQueueLengthsAfter();

        virtual void Print(const char *rel_type) const;

        int GetNumPopulationTotal() const { return m_population_map.size() ; }

        int GetNumMalesInBin(   int ageBinIndex ) const { return m_male_population[   ageBinIndex ].size() ; }
        int GetNumFemalesInBin( int ageBinIndex ) const { return m_female_population[ ageBinIndex ].size() ; }

        typedef list<IIndividualHumanSTI*> human_list_t;
        const human_list_t& GetMalesInBin(   int ageBinIndex ) const { return m_male_population[   ageBinIndex ] ; }
        const human_list_t& GetFemalesInBin( int ageBinIndex ) const { return m_female_population[ ageBinIndex ] ; }

        // ---------------------------
        // --- JsonConfiurable Methods
        // ---------------------------
        virtual bool Configure(const Configuration *config);

#if USE_JSON_SERIALIZATION
        // For JSON serialization
        virtual void JSerialize( Kernel::IJsonObjectAdapter* root, Kernel::JSerializer* helper ) const {}
        virtual void JDeserialize( Kernel::IJsonObjectAdapter* root, Kernel::JSerializer* helper ) {}
#endif

        // ---------------------
        // --- ISupport Methods
        // ---------------------
        virtual Kernel::QueryResult QueryInterface(Kernel::iid_t iid, void **ppvObject) { return Kernel::e_NOINTERFACE; }
        virtual int32_t AddRef()  { return -1 ; }
        virtual int32_t Release() { return -1 ; }
    protected:
        BehaviorPfa( const IPairFormationParameters*, 
                     float updatePeriod, 
                     float selectionThreshold,
                     RANDOMBASE*, 
                     RelationshipCreator );

        virtual ~BehaviorPfa();
        bool areInRelationship( IIndividualHumanSTI * person1, const IIndividualHumanSTI* person2 ) const;

        float m_update_period;
        float m_cum_prob_threshold ;
        float m_time_since_last_update;

        typedef vector<human_list_t> population_t;
        typedef pair<int, human_list_t::iterator> map_entry_t;
        typedef map<IIndividualHumanSTI*, map_entry_t> population_map_t;

#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        human_list_t m_all_males;
        population_t m_male_population;
        population_t m_female_population;
        population_map_t m_population_map;
#pragma warning( pop )

    private:
        void AddToPopulation( human_list_t& age_bin_list, IIndividualHumanSTI* sti_person, int age_bin_index );
        bool RemoveFromPopulation( IIndividualHumanSTI* sti_person, population_t& target_population );
        void UpdateQueueLengths( map<int, vector<int>>& rQueueLengths );

#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        vector<float> preference;
        map<int, vector<float>> desired_flow;

        const IPairFormationParameters* parameters;
        RANDOMBASE* m_rng;
        RelationshipCreator relationship_fn;
        IAssortivity* m_pAssortivity ;

         map<int, vector<int>> m_QueueLengthsBefore ;
         map<int, vector<int>> m_QueueLengthsAfter ;

        // DEBUGGING/VALIDATION
        vector<int> new_males;
        vector<int> new_females;
#pragma warning( pop )
    };
}
