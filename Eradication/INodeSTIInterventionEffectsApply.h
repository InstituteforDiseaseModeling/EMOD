
#pragma once

#include <string>

#include "ISupports.h"
#include "RelationshipType.h"

namespace Kernel
{
    struct INodeDistributableIntervention;
    struct Sigmoid;
    
    typedef std::function<bool (INodeDistributableIntervention*)> node_intervention_qualify_function_t;

    struct INodeSTIInterventionEffectsApply : public ISupports
    {
        // A node-level intervention that is changing Society should use this method
        // to remove any existing interventions that are also impacting society for
        // the same relationships.  This method should be used instead of PurgeExisting()
        // so that the settings are changed at the same time the new parameters are updated.
        // See NodeSTIEventContextHost::ApplyStagedInterventions() for more information.
        virtual void StageExistingForPurgingIfQualifies( const std::string& iv_name,
                                                         node_intervention_qualify_function_t qual_func ) = 0;

        virtual void SetOverrideRelationshipFormationRate( RelationshipType::Enum relType, float rate ) = 0;
        virtual void SetOverrideCoitalActRate( RelationshipType::Enum relType, float rate ) = 0;
        virtual void SetOverrideCondomUsageProbabiity( RelationshipType::Enum relType, const Sigmoid* pOverride ) = 0;
        virtual void SetOverrideRelationshipDuration( RelationshipType::Enum relType, float heterogeniety, float scale ) = 0;
    };
}
