/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once
#include "Node.h"
#include "IndividualSTI.h" // for serialization only
#include "INodeSTI.h"

namespace Kernel
{
    class NodeSTI : public Node, public INodeSTI
    {
        GET_SCHEMA_STATIC_WRAPPER(NodeSTI)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING();
        DECLARE_QUERY_INTERFACE();

    public:
        virtual ~NodeSTI(void);
        static NodeSTI *CreateNode(ISimulationContext *_parent_sim, suids::suid node_suid);

        virtual void Initialize() override;
        virtual bool Configure( const Configuration* config ) override;

        void GetGroupMembershipForIndividual_STI( const RouteList_t& route, std::map<std::string, uint32_t>* properties, TransmissionGroupMembership_t* membershipOut );

    protected:
        NodeSTI();
        NodeSTI(ISimulationContext *_parent_sim, suids::suid node_suid);

        IRelationshipManager* relMan;
        ISociety* society;

        virtual void SetParameters( NodeDemographicsFactory *demographics_factory, ClimateFactory *climate_factory, bool white_list_enabled ) override;

        // Factory methods
        virtual IIndividualHuman* createHuman( suids::suid suid, float monte_carlo_weight, float initial_age, int gender) override;

        // INodeContext
        virtual act_prob_vec_t DiscreteGetTotalContagion(const TransmissionGroupMembership_t* membership) override;

        // INodeSTI
        virtual /*const?*/ IRelationshipManager* GetRelationshipManager() /*const?*/ override;
        virtual ISociety* GetSociety() override;

        virtual void SetupIntranodeTransmission() override;
        virtual void Update( float dt ) override;
        virtual void processEmigratingIndividual( IIndividualHuman* individual ) override;
        virtual IIndividualHuman* NodeSTI::processImmigratingIndividual( IIndividualHuman* movedind ) override;

        DECLARE_SERIALIZABLE(NodeSTI);

    private:
        float pfa_burnin_duration;
    };
}
