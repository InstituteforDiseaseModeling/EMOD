/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <string>
#include <list>
#include <vector>

#include "BoostLibWrapper.h"

#include "ISupports.h"
#include "FactorySupport.h"
#include "Configure.h"
#include "NodeSet.h"

namespace Kernel
{
    struct INodeEventContext;
    struct ISimulationEventContext;

    class ICampaignEventFactory
    {
    public:
        virtual void Register(string classname, instantiator_function_t _if) = 0;
        virtual json::QuickBuilder GetSchema() = 0;
    };            


    class CampaignEvent;

    class CampaignEventFactory : public ICampaignEventFactory
    {
    public:
        static ICampaignEventFactory * getInstance() { return _instance ? _instance : _instance = new CampaignEventFactory(); }

        static CampaignEvent* CreateInstance(const Configuration * config);
        void Register(string classname, instantiator_function_t _if);
        virtual json::QuickBuilder GetSchema();

    protected:
        static support_spec_map_t& getRegisteredClasses();

    private:
        static ICampaignEventFactory * _instance;
        static json::Object ceSchema;
    };


    struct IEventCoordinator;

    class CampaignEvent : public JsonConfigurable
    {
        DECLARE_FACTORY_REGISTERED(CampaignEventFactory, CampaignEvent, IConfigurable)

    public:
        friend class CampaignEventFactory;
        DECLARE_CONFIGURED(CampaignEvent)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()  
        DECLARE_QUERY_INTERFACE()

        CampaignEvent();
        virtual ~CampaignEvent();
        float GetStartDay() const;
        int   GetEventIndex() const;
        void  SetEventIndex(int index);
        void CheckForValidNodeIDs(const std::vector<ExternalNodeId_t>& demographic_node_ids);
        float GetDistributionDuration() const;

        void Dispatch(ISimulationEventContext *isec); // called when the event is to happen

    protected:
        float start_day;
        int event_index; // to avoid the priority queue from arbitrarily swapping the order of events on the same start day
        INodeSet *nodeset;
        IEventCoordinator *event_coordinator; // TODO: eventually try to instantiate and then serialize just this, once a solution for global type registration is worked out
        NodeSetConfig nodeset_config;
        EventConfig event_coordinator_config;
    };
}
