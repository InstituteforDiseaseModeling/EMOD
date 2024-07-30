
#pragma once
#include "Node.h"
#include "IndividualSTI.h" // for serialization only
#include "INodeSTI.h"

namespace Kernel
{
    class RelationshipGroups;

    class NodeSTI : public Node, public INodeSTI, public IActionStager
    {
        GET_SCHEMA_STATIC_WRAPPER(NodeSTI)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING();
        DECLARE_QUERY_INTERFACE();

    public:
        virtual ~NodeSTI(void);
        static NodeSTI *CreateNode(ISimulationContext *_parent_sim, ExternalNodeId_t externalNodeId, suids::suid node_suid);

        virtual void Initialize() override;
        virtual void SetupEventContextHost() override;
        virtual bool Configure( const Configuration* config ) override;

    protected:
        NodeSTI();
        NodeSTI(ISimulationContext *_parent_sim, ExternalNodeId_t externalNodeId, suids::suid node_suid);

        IRelationshipManager* relMan;
        ISociety* society;
        RelationshipGroups* pRelationshipGroups;

        virtual void SetParameters( NodeDemographicsFactory *demographics_factory, ClimateFactory *climate_factory ) override;

        // Factory methods
        virtual IIndividualHuman* createHuman( suids::suid suid, float monte_carlo_weight, float initial_age, int gender) override;

        // INodeSTI
        virtual /*const?*/ IRelationshipManager* GetRelationshipManager() /*const?*/ override;
        virtual ISociety* GetSociety() override;
        virtual IActionStager* GetActionStager() override;
        virtual void DepositFromIndividual( const IStrainIdentity& rStrain, const CoitalAct& rCoitalAct ) override;
        virtual RANDOMBASE* GetRng() override;

        // IActionStager
        virtual void StageIntervention( IIndividualHumanEventContext* pHuman, IDistributableIntervention* pIntervention ) override;
        virtual void StageEvent( IIndividualHumanEventContext* pHuman, const EventTrigger& rTrigger ) override;

        virtual void PostUpdate() override;
        virtual void SetupIntranodeTransmission() override;
        virtual void Update( float dt ) override;
        virtual void processEmigratingIndividual( IIndividualHuman* individual ) override;
        virtual IIndividualHuman* processImmigratingIndividual( IIndividualHuman* movedind ) override;

        DECLARE_SERIALIZABLE(NodeSTI);

    private:
        float pfa_burnin_duration;
        std::vector<std::pair<IIndividualHumanEventContext*, IDistributableIntervention*>> staged_interventions;
        std::vector<std::pair<IIndividualHumanEventContext*, EventTrigger>> staged_events;
    };
}
