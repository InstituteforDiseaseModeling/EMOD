/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "IdmApi.h"
#include "IdmDateTime.h"
#include "Configure.h"
#include "IRelationship.h"
#include "Properties.h"


namespace Kernel {

    struct IIdGeneratorSTI;
    struct IPairFormationRateTable;
    struct IPairFormationAgent;
    struct IPairFormationStats;
    struct IRelationshipParameters;
    struct IConcurrency;

    struct IDMAPI ISociety : public JsonConfigurable
    {
        virtual void BeginUpdate() = 0;
        virtual void UpdatePairFormationRates( const IdmDateTime& rCurrentTime, float dt) = 0;
        virtual void UpdatePairFormationAgents( const IdmDateTime& rCurrentTime, float dt ) = 0;
        
        virtual const IPairFormationRateTable* GetRates(RelationshipType::Enum) = 0;
        virtual IPairFormationAgent* GetPFA(RelationshipType::Enum) = 0;
        virtual IPairFormationStats* GetStats(RelationshipType::Enum) = 0;

        virtual void SetParameters( IIdGeneratorSTI* pIdGen, const tPropertiesDistrib& rPropertiesDist, const Configuration* config ) = 0;
        virtual IRelationshipParameters* GetRelationshipParameters( RelationshipType::Enum type ) = 0;

        virtual IConcurrency* GetConcurrency() = 0;

        // JsonConfigurable - making public
        virtual bool Configure(const Configuration *config) = 0;

        virtual ~ISociety() {}
    };
}