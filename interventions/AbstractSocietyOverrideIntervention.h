
#pragma once


#include "Interventions.h"
#include "Configuration.h"
#include "InterventionFactory.h"
#include "Configure.h"
#include "ISocietyOverrideIntervention.h"

namespace Kernel
{
    struct INodeSTIInterventionEffectsApply;

    // Node-level interventions that are overriding parameters within Society
    // should use this base class.  It will help with the required staging
    // when adding an intervention and the removal existing ones.
    // See NodeSTIEventContextHost::ApplyStagedInterventions() for more information.
    class AbstractSocietyOverrideIntervention : public BaseNodeIntervention
                                              , public ISocietyOverrideIntervention
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()

    public:        
        AbstractSocietyOverrideIntervention();
        AbstractSocietyOverrideIntervention( const AbstractSocietyOverrideIntervention& );
        virtual ~AbstractSocietyOverrideIntervention();

        // INodeDistributableIntervention
        virtual bool Configure( const Configuration * config ) override;
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;
        virtual bool Distribute(INodeEventContext *context, IEventCoordinator2* pEC=nullptr) override; 
        virtual void Update(float dt) override;

        // ISocietyOverrideIntervention
        virtual RelationshipType::Enum GetRelationshipType() const override;

    protected:
        virtual void ApplyOverride() = 0;

        RelationshipType::Enum m_RelationshipType;
        float m_OverrideCoitalActRate;
         
        INodeSTIInterventionEffectsApply *m_pINSIC;
    };

}

