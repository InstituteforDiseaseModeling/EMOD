/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

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

        virtual void SetParameters( IIdGeneratorSTI* pIdGen, const Configuration* config ) override;
        virtual IRelationshipParameters* GetRelationshipParameters( RelationshipType::Enum type ) override;

        virtual IConcurrency* GetConcurrency() override;

        // ---------------------------
        // --- JsonConfiurable Methods
        // ---------------------------
        virtual bool Configure(const Configuration *config) override;

    protected:
        SocietyImpl(IRelationshipManager* pmgr = nullptr );
        virtual ~SocietyImpl();

        IRelationshipManager* relationship_manager;

        IRelationshipParameters*        rel_params[    RelationshipType::COUNT ];
        const IPairFormationParameters* form_params[   RelationshipType::COUNT ];
        IPairFormationRateTable*        rates[         RelationshipType::COUNT ];
        IPairFormationStats*            stats[         RelationshipType::COUNT ];
        IPairFormationAgent*            pfa[           RelationshipType::COUNT ];
        IPairFormationFlowController*   controller[    RelationshipType::COUNT ];

        ConcurrencyConfiguration* p_concurrency;

        float pfa_selection_threshold ;
    };
}