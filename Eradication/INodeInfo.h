/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <stdafx.h>

#include "suids.hpp"
#include "INodeContext.h"

namespace Kernel
{
    // INodeInfo is an interface to an obect that contains a subset of information
    // about a node that other nodes might need.  For example, an intervention might
    // want to make a decision based on how many people are in a node.
    struct INodeInfo
    {
    public:
        virtual ~INodeInfo() {}

        virtual void Update( INodeContext* pNC ) = 0;

        virtual const suids::suid& GetSuid()       const = 0;
        virtual ExternalNodeId_t   GetExternalID() const = 0;

        virtual int   GetRank()             const = 0;
        virtual float GetPopulation()       const = 0;
        virtual float GetLongitudeDegrees() const = 0;
        virtual float GetLatitudeDegrees()  const = 0;

        virtual void serialize( IArchive& ar, bool firstTime ) = 0;
    };

    // A factory is needed since some simulations might want to have different
    // data available in the node info object.
    struct INodeInfoFactory
    {
        virtual INodeInfo* CreateNodeInfo() = 0;
        virtual INodeInfo* CreateNodeInfo( int rank, INodeContext* pNC ) = 0;
    };

    // This interface is part of a feature that allows an intervention to send
    // events to other nodes.  The intervention can implement this interface
    // and then Simulation will use it to determine if the event should be sent
    // to the given node.
    struct INodeQualifier
    {
        virtual bool Qualifies( const INodeInfo& rni ) const = 0;
    };
}
