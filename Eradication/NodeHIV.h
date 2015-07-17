/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once
#include "NodeSTI.h"
#include "IndividualHIV.h" // for serialization only
#include "TransmissionGroupsFactory.h"
#include "Relationship.h"
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

        // void Initialize();

        // Factory methods
        virtual IndividualHuman *createHuman(suids::suid suid, float monte_carlo_weight, float initial_age, int gender, float above_poverty);

        //virtual void SetupIntranodeTransmission();
        //virtual void Update( float dt );
        //virtual void processEmigratingIndividual( IndividualHuman *individual );
        //virtual IndividualHuman* NodeHIV::processImmigratingIndividual( IndividualHuman* movedind );

        // INodeHIV
        // virtual const vector<RelationshipStartInfo>& GetNewRelationships() const;
        // virtual const std::vector<RelationshipEndInfo>& GetTerminatedRelationships() const;

        // NodeSTI
        // std::multimap< unsigned long, int > migratedIndividualToRelationshipIdMap;

    private:

#if USE_BOOST_SERIALIZATION
        friend class boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive & ar, NodeHIV& node, const unsigned int file_version);
#endif
    };
}
