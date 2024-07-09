
#pragma once

#include <string>

#include "ISupports.h"
#include "NodeEventContextHost.h"
#include "INodeSTIInterventionEffectsApply.h"

namespace Kernel
{
    class NodeSTIEventContextHost : public NodeEventContextHost
                                  , public INodeSTIInterventionEffectsApply
    {
        IMPLEMENT_NO_REFERENCE_COUNTING()

    public:
        NodeSTIEventContextHost(Node* _node);
        virtual ~NodeSTIEventContextHost();
  
        virtual QueryResult QueryInterface(iid_t iid, void** pinstance) override;
       
        virtual void UpdateInterventions(float dt) override;
        virtual bool GiveIntervention( INodeDistributableIntervention* iv ) override;

        // INodeSTIInterventionEffectsApply
        virtual void StageExistingForPurgingIfQualifies( const std::string& iv_name,
                                                         node_intervention_qualify_function_t qual_func ) override;
        virtual void SetOverrideRelationshipFormationRate( RelationshipType::Enum relType, float rate ) override;
        virtual void SetOverrideCoitalActRate( RelationshipType::Enum relType, float rate ) override;
        virtual void SetOverrideCondomUsageProbabiity( RelationshipType::Enum relType, const Sigmoid* pOverride ) override;
        virtual void SetOverrideRelationshipDuration( RelationshipType::Enum relType, float heterogeniety, float scale ) override;

        // other
        void ApplyStagedInterventions();

    protected: 
        std::vector<float>           m_OverrideRelationshipForamtionRate;
        std::vector<float>           m_OverrideCoitalActRate;
        std::vector<const Sigmoid*>  m_OverrideCondomUsage;
        std::vector<float>           m_OverrideDurationHeterogeniety;
        std::vector<float>           m_OverrideDurationScale;

        std::vector<INodeDistributableIntervention*> m_StagedNodeInterventionsToPurge;
        std::vector<INodeDistributableIntervention*> m_StagedInterventionsToAdd;

        // hate to have this but I need it when i call Update() on the staged intervention
        float m_DT;
        
    private:
        NodeSTIEventContextHost() : NodeSTIEventContextHost(nullptr) { }
    };
}
