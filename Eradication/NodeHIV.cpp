/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"

#include "NodeHIV.h"
#include "Debug.h"

#include "IndividualHIV.h"
#include "RelationshipReporting.h"
#include "SimulationSTI.h"

static const char * _module = "NodeHIV";

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED(NodeHIV, NodeSTI)
        HANDLE_INTERFACE(INodeHIV)
    END_QUERY_INTERFACE_DERIVED(NodeHIV, NodeSTI)

    NodeHIV::NodeHIV(ISimulationContext *_parent_sim, suids::suid node_suid)
        : NodeSTI(_parent_sim, node_suid)
    {
    }

    NodeHIV::NodeHIV()
        : NodeSTI()
    {
    }

    NodeHIV::~NodeHIV(void)
    {
    }

    NodeHIV *NodeHIV::CreateNode(ISimulationContext *_parent_sim, suids::suid node_suid)
    {
        NodeHIV *newnode = _new_ NodeHIV(_parent_sim, node_suid);
        newnode->Initialize();

        return newnode;
    }

    IndividualHuman *NodeHIV::createHuman(suids::suid suid, float monte_carlo_weight, float initial_age, int gender, float above_poverty)
    {
        return IndividualHumanHIV::CreateHuman(this, suid, monte_carlo_weight, initial_age, gender,  above_poverty);
    }

/*
    const vector<RelationshipStartInfo>& NodeHIV::GetNewRelationships() const
    {
        return new_relationships;
    }

    const std::vector<RelationshipEndInfo>& NodeHIV::GetTerminatedRelationships() const
    {
        return terminated_relationships;
    }
*/
}

#if USE_BOOST_SERIALIZATION
#include "IndividualHIV.h"
BOOST_CLASS_EXPORT(Kernel::NodeHIV)
namespace Kernel {
    template<class Archive>
    void serialize(Archive & ar, NodeHIV& node, const unsigned int file_version)
    {
        // Register derived types
        //ar.template register_type<IndividualHumanHIV>();

        // Serialize base class
        ar &boost::serialization::base_object<Node>(node);    
    }
}
#endif

