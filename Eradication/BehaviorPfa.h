
#pragma once

#include "IPairFormationAgent.h"
#include <list>
#include <vector>
#include <map>
#include "IPairFormationParameters.h"
#include "IRelationship.h"
#include "IAssortivity.h"

namespace Kernel 
{
    class RANDOMBASE;

    class IDMAPI BehaviorPfa : public IPairFormationAgent 
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING();
        DECLARE_QUERY_INTERFACE();
    public:
        static IPairFormationAgent* CreatePfa( const Configuration* pConfig,
                                               const IPairFormationParameters* params,
                                               float selectionThreshold,
                                               RANDOMBASE* prng, 
                                               RelationshipCreator rc );

        // IPairFormationAgent
        virtual void BeginUpdate() override;
        virtual void AddIndividual(IIndividualHumanSTI*) override;
        virtual void RemoveIndividual(IIndividualHumanSTI*) override;
        virtual void Update( const IdmDateTime& rCurrentTime, float dt ) override;
        virtual void RegisterIndividual( IIndividualHumanSTI* pIndividualSti ) override;
        virtual void UnregisterIndividual( IIndividualHumanSTI* pIndividualSti ) override;
        virtual bool StartNonPfaRelationship( IIndividualHumanSTI* pIndividualSTI,
                                              const IPKeyValue& rPartnerHasIP,
                                              Sigmoid* pCondomUsage ) override;
        virtual const map<int, vector<float>>& GetAgeBins() override;
        virtual const map<int, vector<float>>& GetDesiredFlow() override;
        virtual const map<int, vector<int>>& GetQueueLengthsBefore() override;
        virtual const map<int, vector<int>>& GetQueueLengthsAfter() override;

        virtual void Print(const char *rel_type) const override;

        // Others
        int GetNumPopulationTotal() const { return m_population_map.size() ; }

        int GetNumMalesInBin(   int ageBinIndex ) const { return m_male_population[   ageBinIndex ].size() ; }
        int GetNumFemalesInBin( int ageBinIndex ) const { return m_female_population[ ageBinIndex ].size() ; }

        typedef list<IIndividualHumanSTI*> human_list_t;
        const human_list_t& GetMalesInBin(   int ageBinIndex ) const { return m_male_population[   ageBinIndex ] ; }
        const human_list_t& GetFemalesInBin( int ageBinIndex ) const { return m_female_population[ ageBinIndex ] ; }

        // ---------------------------
        // --- JsonConfiurable Methods
        // ---------------------------
        virtual bool Configure(const Configuration *config) override;

    protected:
        BehaviorPfa();
        BehaviorPfa( const IPairFormationParameters*, 
                     float selectionThreshold,
                     RANDOMBASE*, 
                     RelationshipCreator );

        virtual ~BehaviorPfa();
        bool areInRelationship( IIndividualHumanSTI * person1, const IIndividualHumanSTI* person2 ) const;

        float m_cum_prob_threshold ;
        float m_time_since_last_update;

        typedef vector<human_list_t> population_t;
        typedef pair<int, human_list_t::iterator> pair_t;
        typedef vector<pair_t> map_entry_t;
        typedef map<IIndividualHumanSTI*, map_entry_t> population_map_t;

        std::vector<IIndividualHumanSTI*> m_RegisteredMales;
        std::vector<IIndividualHumanSTI*> m_RegisteredFemales;
        std::map<uint32_t,uint32_t> m_RegisteredIDtoIndexMapMales;
        std::map<uint32_t,uint32_t> m_RegisteredIDtoIndexMapFemales;
        human_list_t m_all_males;
        population_t m_male_population;
        population_t m_female_population;
        population_map_t m_population_map;

    private:
        void AddToPopulation( human_list_t& age_bin_list, IIndividualHumanSTI* sti_person, int age_bin_index );
        bool RemoveFromPopulation( IIndividualHumanSTI* sti_person, population_t& target_population, bool remove_all );
        void UpdateQueueLengths( map<int, vector<int>>& rQueueLengths );

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

        DECLARE_SERIALIZABLE(BehaviorPfa);
    };
}
