/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <stdafx.h>
#include <string>
#include <vector>
#include <map>

#include "suids.hpp"
#include "EventTrigger.h"

namespace Kernel
{
    struct IArchive;

    // EventsForOtherNodes is an object that contains the information of events 
    // that get sent to other nodes.  It is used by Simulation to gather the events 
    // destined for other nodes and then the object is sent to the appropriate node.
    class EventsForOtherNodes
    {
    public:
        EventsForOtherNodes();
        ~EventsForOtherNodes();

        bool operator==( const EventsForOtherNodes& rthat ) const;
        bool operator!=( const EventsForOtherNodes& rthat ) const;

        void Add( const suids::suid& rNodeSuid, const EventTrigger& trigger );
        void Clear();
        void Print();
        void Update( const EventsForOtherNodes& rEfon );
        const std::map<suids::suid,std::vector<EventTrigger>>& GetMap() const { return m_NodeEventMap; }

        static void serialize( IArchive& ar, EventsForOtherNodes& obj );
    private:
        std::map<suids::suid,std::vector<EventTrigger>> m_NodeEventMap ;
    };
}
