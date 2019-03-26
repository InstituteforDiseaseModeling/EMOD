/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "BroadcasterImpl.h"
#include "EventTrigger.h"
#include "EventTriggerNode.h"
#include "EventTriggerCoordinator.h"

SETUP_LOGGING( "BroadcasterImpl" )

// moved after SETUP_LOGGING to make Linux Debug happy with LOG_DEBUG statements
#include "BroadcasterImplTemplate.h"

namespace Kernel
{
    template class BroadcasterImpl< ICoordinatorEventObserver,
                                    IEventCoordinatorEventContext,
                                    EventTriggerCoordinator,
                                    EventTriggerCoordinatorFactory >;

    template class BroadcasterImpl< INodeEventObserver,
                                    INodeEventContext,
                                    EventTriggerNode,
                                    EventTriggerNodeFactory >;

    template class BroadcasterImpl< IIndividualEventObserver,
                                    IIndividualHumanEventContext,
                                    EventTrigger,
                                    EventTriggerFactory >;
}


