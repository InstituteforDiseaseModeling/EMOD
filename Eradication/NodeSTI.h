/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once
#include "Node.h"
#include "IndividualSTI.h" // for serialization only
#include "TransmissionGroupsFactory.h"
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

        void Initialize();
        virtual bool Configure( const Configuration* config );

    protected:
        NodeSTI();
        NodeSTI(ISimulationContext *_parent_sim, suids::suid node_suid);

        IRelationshipManager* relMan;
        ISociety* society;

        virtual void SetMonteCarloParameters(float indsamplerate = 1.0f, int nummininf = 0);
        virtual void SetParameters(NodeDemographicsFactory *demographics_factory, ClimateFactory *climate_factory);

        // Factory methods
        virtual IndividualHuman *createHuman( suids::suid suid, float monte_carlo_weight, float initial_age, int gender, float above_poverty);

        // INodeContext
        virtual act_prob_vec_t DiscreteGetTotalContagion(const TransmissionGroupMembership_t* membership);

        // INodeSTI
        virtual /*const?*/ IRelationshipManager* GetRelationshipManager() /*const?*/;
        virtual ISociety* GetSociety();

        virtual void SetupIntranodeTransmission();
        virtual void Update( float dt );
        virtual void processEmigratingIndividual( IndividualHuman *individual );
        virtual IndividualHuman* NodeSTI::processImmigratingIndividual( IndividualHuman* movedind );

        std::multimap< unsigned long, int > migratedIndividualToRelationshipIdMap;

    private:
        float pfa_burnin_duration;

#if USE_BOOST_SERIALIZATION
        friend class boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive & ar, NodeSTI& node, const unsigned int file_version);
#endif
    };
}
