
#pragma once
#include "ISociety.h"
#include "Relationship.h"
#include "IRelationshipManager.h"
#include "IPairFormationParameters.h"
#include "IPairFormationRateTable.h"
#include "IPairFormationAgent.h"
#include "IPairFormationFlowController.h"
#include "IPairFormationStats.h"
#include "IConcurrency.h"
#include "Configure.h"

namespace Kernel
{

    class ConcurrencyConfiguration;

    class SocietyImpl : public ISociety
    {
        GET_SCHEMA_STATIC_WRAPPER(SocietyImpl)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING();
        DECLARE_QUERY_INTERFACE();

    public:
        static ISociety* Create(IRelationshipManager*);
        virtual void BeginUpdate() override;
        virtual void UpdatePairFormationRates( const IdmDateTime& rCurrentTime, float dt ) override;
        virtual void UpdatePairFormationAgents( const IdmDateTime& rCurrentTime, float dt ) override;

        virtual const IPairFormationRateTable* GetRates(RelationshipType::Enum) override;
        virtual IPairFormationAgent* GetPFA(RelationshipType::Enum) override;
        virtual IPairFormationStats* GetStats(RelationshipType::Enum) override;
        virtual IPairFormationFlowController* GetController( RelationshipType::Enum ) override;

        virtual void SetParameters( RANDOMBASE* pRNG, IIdGeneratorSTI* pIdGen, const Configuration* config ) override;
        virtual IRelationshipParameters* GetRelationshipParameters( RelationshipType::Enum type ) override;

        virtual IConcurrency* GetConcurrency() override;

        virtual void SetOverrideRelationshipFormationRate( RelationshipType::Enum relType, float rate ) override;
        virtual void SetOverrideCoitalActRate( RelationshipType::Enum relType, float rate ) override;
        virtual void SetOverrideCondomUsageProbability( RelationshipType::Enum relType, const Sigmoid* pOverride ) override;
        virtual void SetOverrideRelationshipDuration( RelationshipType::Enum relType, float heterogeniety, float scale ) override;

        // ---------------------------
        // --- JsonConfiurable Methods
        // ---------------------------
        virtual bool Configure(const Configuration *config) override;

    protected:
        SocietyImpl(IRelationshipManager* pmgr = nullptr );
        virtual ~SocietyImpl();

        IRelationshipManager* relationship_manager;

        IRelationshipParameters*        rel_params[    RelationshipType::COUNT ];
        IPairFormationParameters*       form_params[   RelationshipType::COUNT ];
        IPairFormationRateTable*        rates[         RelationshipType::COUNT ];
        IPairFormationStats*            stats[         RelationshipType::COUNT ];
        IPairFormationAgent*            pfa[           RelationshipType::COUNT ];
        IPairFormationFlowController*   controller[    RelationshipType::COUNT ];

        ConcurrencyConfiguration* p_concurrency;

        float pfa_selection_threshold ;
    };
}