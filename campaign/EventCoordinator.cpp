
#include "stdafx.h"

#include "EventCoordinator.h"
#include "ObjectFactoryTemplates.h"
#include "Log.h"

SETUP_LOGGING( "EventCoordinator" )

 /*
    General Event Coordinator manager architecture

    Each simulation object belonging to a single simulation instance maintains a list of all distribution managers requested to be instantiated by the campaign file

    at each simulation timestep, all DMs are updated. this is a hook for simulation-wide information exchange and will eventually require some kind of messaging system. we'll leave that out for now 
    and see what the first implementations of dynamic campaigns need

    Campaign file structure:   
    {
        "Events" :
        [
            {
                "class" : "CampaignEvent",
                "start_day": <start day>
                "nodeset": 
                {
                    "class" : <node set class>
                    <nodeset data members>
                }

                "event_coordinator": 
                {
                    "class": <classname>
                    <ec-configuration>
                }
            },
            ...
        ]
    }


    Sequence:

    1. Simulation reads campaign file, calls a class factory to instantiate and configure each of the Campaign Events in the file requested

    2. Each campaign event instantiates the event coordinator and nodeset but does not activate

    3. Each campaig event is added to the simulation global queue

    4. When a campaign event's start day arrives, it is executed which
        - assigns nodes to the EC using the NodeSet
            - if static, can be separate from the DM description 
            - if reevaluatable (this might only make sense for extreme efficiency cases, probably not relevant), could be embedded in the DMs themselves
            - architecturally, theres no reason this couldn't be done in an outer dm wrapper with its own parameters controlling the reeval time
                - a country-level manager could work this way, by deploying local campaigns in nodes with specific properties
                    - requires the node property interfaces to be exposed, obviously

        - registers the EC as active

    5. the campaign event is removed from the queue

    6. Simulation Update() then calls Update and UpdateNodes() on all the ECs it has currently registered

    7. When an EC->IsFinished() return true, the EC is unregistered and released



    Integration testing strategy:

    - create an empty campaign file, get it to load and validate
    - get some traces on the event activation to make sure events are being dispatched at the right time

    */

namespace Kernel
{
    EventCoordinatorFactory* EventCoordinatorFactory::_instance = nullptr;
    template EventCoordinatorFactory* ObjectFactory<IEventCoordinator, EventCoordinatorFactory>::getInstance();
}



