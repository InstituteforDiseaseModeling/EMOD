/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

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
#include "Configure.h"

namespace Kernel {

    class SocietyImpl : public ISociety
    {
        GET_SCHEMA_STATIC_WRAPPER(SocietyImpl)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING();
        DECLARE_QUERY_INTERFACE();

    public:
        static ISociety* Create(IRelationshipManager*);
        virtual void BeginUpdate();
        virtual void UpdatePairFormationRates( const IdmDateTime& rCurrentTime, float dt );
        virtual void UpdatePairFormationAgents( const IdmDateTime& rCurrentTime, float dt );

        virtual const IPairFormationRateTable* GetRates(RelationshipType::Enum);
        virtual IPairFormationAgent* GetPFA(RelationshipType::Enum);
        virtual IPairFormationStats* GetStats(RelationshipType::Enum);

        virtual void SetParameters( const Configuration* config );

        // ---------------------------
        // --- JsonConfiurable Methods
        // ---------------------------
        virtual bool Configure(const Configuration *config);

#if USE_JSON_SERIALIZATION
        // For JSON serialization
        virtual void JSerialize( Kernel::IJsonObjectAdapter* root, Kernel::JSerializer* helper ) const {}
        virtual void JDeserialize( Kernel::IJsonObjectAdapter* root, Kernel::JSerializer* helper ) {}
#endif

    protected:
        SocietyImpl(IRelationshipManager* pmgr = nullptr );
        virtual ~SocietyImpl();

        IRelationshipManager* relationship_manager;

        const IPairFormationParameters* parameters[    RelationshipType::COUNT ];
        IPairFormationRateTable*        rates[         RelationshipType::COUNT ];
        IPairFormationStats*            stats[         RelationshipType::COUNT ];
        IPairFormationAgent*            pfa[           RelationshipType::COUNT ];
        IPairFormationFlowController*   controller[    RelationshipType::COUNT ];
        float                           base_rate[     RelationshipType::COUNT ];
        float                           update_period[ RelationshipType::COUNT ];

        float extra_relational_rate_ratio_male;
        float extra_relational_rate_ratio_female;
        float pfa_selection_threshold ;
    };
}