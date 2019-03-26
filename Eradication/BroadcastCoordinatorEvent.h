/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "EventCoordinator.h"
#include "Configure.h"
#include "EventTriggerCoordinator.h"

namespace Kernel
{
    class BroadcastCoordinatorEvent : public JsonConfigurable, public IEventCoordinator, public IEventCoordinatorEventContext
    {
        DECLARE_FACTORY_REGISTERED_EXPORT( EventCoordinatorFactory, BroadcastCoordinatorEvent, IEventCoordinator )
    public:
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

        BroadcastCoordinatorEvent();
        virtual ~BroadcastCoordinatorEvent();

        virtual bool Configure( const Configuration * inputJson ) override;
        virtual QuickBuilder GetSchema() override;

        // IEventCoordinator methods
        virtual void SetContextTo( ISimulationEventContext *isec ) override;
        virtual void CheckStartDay( float campaignStartDay ) const override { };
        virtual void AddNode( const suids::suid& suid ) override;
        virtual void Update( float dt ) override;
        virtual void UpdateNodes( float dt ) override;
        virtual bool IsFinished() override;
        virtual IEventCoordinatorEventContext* GetEventContext() override { return this; }

        // IEventCoordinatorEventContext
        virtual const std::string& GetName() const override;
        virtual const IdmDateTime& GetTime() const override;
        virtual IEventCoordinator* GetEventCoordinator() override { return this; };

    protected:
        ISimulationEventContext* m_Parent;
        std::string              m_CoordinatorName;
        EventTriggerCoordinator  m_EventToBroadcast;
        float                    m_CampaignCost;
        INodeEventContext*       m_pNodeForCampaignCost;
    };
}

