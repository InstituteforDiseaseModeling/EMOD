
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


