/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <string>
#include <list>
#include <vector>
#include <queue>
#include <functional>
#include "BoostLibWrapper.h"
#include "ISupports.h"
#include "Configuration.h"
#include "suids.hpp"
#include "RANDOM.h"
#include "IdmDateTime.h"
#include "INodeContext.h"

namespace Kernel
{
    struct INodeEventContext;
    struct IEventCoordinator;
    class CampaignEvent;

    struct IDMAPI ISimulationEventContext : public ISupports
    {
        // node access
        typedef std::function<bool (suids::suid, INodeEventContext*)> node_visit_function_t; // return true if node was visited

        virtual void VisitNodes(node_visit_function_t func) = 0;
        virtual INodeEventContext* GetNodeEventContext(suids::suid node_id) = 0;

        // registration
        virtual void RegisterEventCoordinator(IEventCoordinator* iec) = 0;

        //////////////////////////////////////////////////////////////////////////
        // pass through from ISimulationContext
        // time services
        virtual IdmDateTime GetSimulationTime() const = 0;
        virtual int GetSimulationTimestep() const = 0;

        // random number services
        virtual RANDOMBASE* GetRng() =0;
    };

    class Simulation;

    // The SimulationEventContextHost implements functionality properly belonging to the Simulation class but it split out manually to make development easier. like, you know, what partial class declarations are for.
    class SimulationEventContextHost : public ISimulationEventContext
    {
        DECLARE_QUERY_INTERFACE()
        IMPLEMENT_NO_REFERENCE_COUNTING() 

    public:
        SimulationEventContextHost(Simulation* _sim);
        SimulationEventContextHost();
        virtual ~SimulationEventContextHost();

        //////////////////////////////////////////////////////////////////////////
        // ISimulationEventContext
        virtual void VisitNodes(node_visit_function_t func);
        virtual INodeEventContext* GetNodeEventContext(suids::suid node_id);

        // registration
        virtual void RegisterEventCoordinator(IEventCoordinator* iec);

        //////////////////////////////////////////////////////////////////////////
        // pass through from ISimulationContext
        // time services
        virtual IdmDateTime GetSimulationTime() const;
        virtual int GetSimulationTimestep() const;

        // random number services
        virtual RANDOMBASE* GetRng();

        //////////////////////////////////////////////////////////////////////////

        // host implementation
        virtual void Update(float dt);
        void LoadCampaignFromFile(const std::string & campaignfile, const std::vector<ExternalNodeId_t>& nodeIds_demographics);
        std::string campaign_filename;

    protected:
        Simulation* sim;

        std::list<IEventCoordinator*> event_coordinators;

        struct campaign_event_comparison
        {
            bool operator()(const CampaignEvent* _Left, const CampaignEvent* _Right) const;
        };

        std::priority_queue< CampaignEvent*, std::vector<CampaignEvent*>, campaign_event_comparison>  event_queue;

        void propagateContextToDependents();
    };
}
