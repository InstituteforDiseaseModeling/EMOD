
#pragma once

#include "IRelationship.h"
#include "Sigmoid.h"

namespace Kernel 
{
    struct IDMAPI IRelationshipParameters
    {
        virtual RelationshipType::Enum GetType() const = 0;

        virtual float   GetCoitalActRate()                const = 0;
        virtual float   GetDurationWeibullHeterogeneity() const = 0;
        virtual float   GetDurationWeibullScale()         const = 0;
        virtual const Sigmoid& GetCondomUsage()           const = 0;

        // the size of these vectors should be equal with a one-to-one corespondence between the action and the probability.
        virtual const std::vector<RelationshipMigrationAction::Enum>& GetMigrationActions() const = 0;
        virtual const std::vector<float>& GetMigrationActionsCDF() const = 0;

        virtual void SetOverrideCoitalActRate( float overrideRate ) = 0;
        virtual void SetOverrideCondomUsageProbability( const Sigmoid* pOverride ) = 0;
        virtual void SetOverrideRelationshipDuration( float heterogeniety, float scale ) = 0;
    };
}