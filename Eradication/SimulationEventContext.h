/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

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
#include "INodeContext.h"
#include "BroadcasterObserver.h"
#include "BroadcasterImpl.h"

namespace Kernel
{
    struct IdmDateTime;
    class EventTriggerCoordinator;
    class EventTriggerCoordinatorFactory;
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

        virtual ICoordinatorEventBroadcaster* GetCoordinatorEventBroadcaster() = 0;
        virtual INodeEventBroadcaster*        GetNodeEventBroadcaster() = 0;

        //////////////////////////////////////////////////////////////////////////
        // pass through from ISimulationContext
        // time services
        virtual const IdmDateTime& GetSimulationTime() const = 0;
        virtual int GetSimulationTimestep() const = 0;
    };

    class Simulation;

    // The SimulationEventContextHost implements functionality properly belonging to the Simulation class
    // but it split out manually to make development easier. like, you know, what partial class declarations are for.
    class SimulationEventContextHost : public ISimulationEventContext, 
                                       public ICoordinatorEventBroadcaster,
                                       public INodeEventBroadcaster
    {
        DECLARE_QUERY_INTERFACE()
        IMPLEMENT_NO_REFERENCE_COUNTING() 

    public:
        SimulationEventContextHost(Simulation* _sim);
        SimulationEventContextHost();
        virtual ~SimulationEventContextHost();

        //////////////////////////////////////////////////////////////////////////
        // ISimulationEventContext
        virtual void VisitNodes(node_visit_function_t func) override;
        virtual INodeEventContext* GetNodeEventContext(suids::suid node_id) override;

        // registration
        virtual void RegisterEventCoordinator(IEventCoordinator* iec) override;

        virtual ICoordinatorEventBroadcaster* GetCoordinatorEventBroadcaster() override;
        virtual INodeEventBroadcaster*        GetNodeEventBroadcaster() override;

        //////////////////////////////////////////////////////////////////////////
        // ICoordinatorEventBroadcaster
        virtual void RegisterObserver(   ICoordinatorEventObserver*     pObserver,    const EventTriggerCoordinator& trigger ) override;
        virtual void UnregisterObserver( ICoordinatorEventObserver*     pObserver,    const EventTriggerCoordinator& trigger ) override;
        virtual void TriggerObservers(   IEventCoordinatorEventContext* pCoordinator, const EventTriggerCoordinator& trigger ) override;

        //////////////////////////////////////////////////////////////////////////
        // INodeEventBroadcaster
        virtual void RegisterObserver(   INodeEventObserver* pObserver,         const EventTriggerNode& trigger ) override;
        virtual void UnregisterObserver( INodeEventObserver* pObserver,         const EventTriggerNode& trigger ) override;
        virtual void TriggerObservers(   INodeEventContext*  pNodeEventContext, const EventTriggerNode& trigger ) override;

        //////////////////////////////////////////////////////////////////////////
        // pass through from ISimulationContext
        // time services
        virtual const IdmDateTime& GetSimulationTime() const override;
        virtual int GetSimulationTimestep() const override;
        //////////////////////////////////////////////////////////////////////////

        // host implementation
        virtual void Update(float dt);
        void LoadCampaignFromFile(const std::string & campaignfile, const std::vector<ExternalNodeId_t>& nodeIds_demographics);
        std::string campaign_filename;

    protected:
        void propagateContextToDependents();

        Simulation* sim;
        std::list<IEventCoordinator*> event_coordinators;

        struct campaign_event_comparison
        {
            bool operator()(const CampaignEvent* _Left, const CampaignEvent* _Right) const;
        };

        std::priority_queue< CampaignEvent*, std::vector<CampaignEvent*>, campaign_event_comparison>  event_queue;

        CoordinatorEventBroadcaster coordinator_broadcaster_impl;
        NodeEventBroadcaster        node_broadcaster_impl;
    };
}
