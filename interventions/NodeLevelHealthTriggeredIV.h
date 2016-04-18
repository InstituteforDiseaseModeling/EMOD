/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <string>
#include <list>
#include <vector>

#include "BoostLibWrapper.h"

#include "Interventions.h"
#include "Configuration.h"
#include "InterventionFactory.h"
#include "InterventionEnums.h"
#include "NodeEventContext.h"
#include "Configure.h"
#include "DemographicRestrictions.h"
#include "EventTrigger.h"

namespace Kernel
{
    class NodeLevelHealthTriggeredIV : public IIndividualEventObserver, public BaseNodeIntervention
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, NodeLevelHealthTriggeredIV, INodeDistributableIntervention)

    public:        
        NodeLevelHealthTriggeredIV();
        virtual ~NodeLevelHealthTriggeredIV();
        virtual int AddRef() override;
        virtual int Release() override;
        virtual bool Configure( const Configuration* config ) override;
        virtual bool ConfigureTriggers( const Configuration* config );

        // INodeDistributableIntervention
        virtual bool Distribute( INodeEventContext *pNodeEventContext, IEventCoordinator2 *pEC ) override;
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;
        virtual void SetContextTo(INodeEventContext *context) override;
        virtual void Update(float dt) override;

        // IIndividualEventObserver
        virtual bool notifyOnEvent( IIndividualHumanEventContext *context, const std::string& StateChange ) override;

    protected:
        INodeEventContext* parent;
        std::vector<std::string>   m_trigger_conditions;
        float max_duration;
        float duration;
        DemographicRestrictions demographic_restrictions;
        bool m_disqualified_by_coverage_only;
        float blackout_period ;
        float blackout_time_remaining ;
        EventTrigger blackout_event_trigger ;
        bool notification_occured ;
        std::map<std::string,std::set<int>> event_occured_map ;
        std::map<suids::suid,bool> event_occurred_while_resident_away;
        IndividualInterventionConfig actual_intervention_config;
        IDistributableIntervention *_di;

        virtual bool qualifiesToGetIntervention( const IIndividualHumanEventContext * pIndividual );
        virtual float getDemographicCoverage() const;
        virtual void onDisqualifiedByCoverage( IIndividualHumanEventContext *pIndiv );
    };
}
