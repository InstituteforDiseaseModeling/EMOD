/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once
#include "TriggeredEventCoordinator.h"
#include "IDistribution.h"
#include "Timers.h"

namespace Kernel
{
    class DelayEventCoordinator : public TriggeredEventCoordinator
    {
        DECLARE_FACTORY_REGISTERED_EXPORT( EventCoordinatorFactory, DelayEventCoordinator, IEventCoordinator )
        DECLARE_CONFIGURED( DelayEventCoordinator )
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

    public:
        DelayEventCoordinator();
        ~DelayEventCoordinator();
        void Callback( float dt );
        void UpdateNodes( float dt ) override;
        virtual void Update( float dt ) override;
        virtual bool notifyOnEvent( IEventCoordinatorEventContext * pEntity, const EventTriggerCoordinator & trigger ) override;

    private:
        CountdownTimer remaining_delay_days;
        IDistribution* delay_distribution;
    };
}
