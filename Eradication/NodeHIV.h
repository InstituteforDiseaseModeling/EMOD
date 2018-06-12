/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once
#include "NodeSTI.h"
#include "INodeHIV.h"

namespace Kernel
{
    class NodeHIV : public NodeSTI, public INodeHIV
    {
        DECLARE_QUERY_INTERFACE()
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()

    public:
        virtual ~NodeHIV(void);
        static NodeHIV *CreateNode(ISimulationContext *_parent_sim, suids::suid node_suid);

    protected:
        NodeHIV();
        NodeHIV(ISimulationContext *_parent_sim, suids::suid node_suid);

        // virtual void Initialize() override;

        // Factory methods
        virtual IIndividualHuman* createHuman( suids::suid suid, float monte_carlo_weight, float initial_age, int gender) override;

        //virtual void SetupIntranodeTransmission();
        //virtual void Update( float dt );
        //virtual void processEmigratingIndividual( IIndividualHuman *individual );
        //virtual IIndividualHuman* NodeHIV::processImmigratingIndividual( IIndividualHuman* movedind );

        // INodeHIV
        // virtual const vector<RelationshipStartInfo>& GetNewRelationships() const;
        // virtual const std::vector<RelationshipEndInfo>& GetTerminatedRelationships() const;

        // NodeSTI
        // std::multimap< unsigned long, int > migratedIndividualToRelationshipIdMap;

        DECLARE_SERIALIZABLE(NodeHIV);
    };
}
