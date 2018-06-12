/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "BoostLibWrapper.h"
#include "Sugar.h"
#include "Debug.h"
#include "Environment.h"
#include "EventCoordinator.h"
#include "Configuration.h"
#include "ConfigurationImpl.h"
#include "FactorySupport.h"
#include "InterventionFactory.h"

#include "Log.h"

SETUP_LOGGING( "EventCoordinator" )

json::Object Kernel::EventCoordinatorFactory::ecSchema;

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

    // TODO: FINISH PORTING THIS TO NEW MECHANISM
/*    IEventCoordinator* EventCoordinatorFactory::CreateInstance( const Configuration* config )
    {
        // TODO: make this mechanism fancier, implement and access a global class factory registration method
        CI_CREATE_SPEC_LIST(spec_list,
            CI_SUPPORT_SPEC(SimpleInterventionDistributionEventCoordinator));
        return CreateInstanceFromSpecs<IEventCoordinator>(config, spec_list, true);

    }*/
    IEventCoordinatorFactory * EventCoordinatorFactory::_instance = nullptr;
    bool EventCoordinatorFactory::doThisOnce = false;
    IEventCoordinator* EventCoordinatorFactory::CreateInstance( const Configuration *config )
    {
        if( !doThisOnce )
        {
#ifdef WIN32
            WIN32_FIND_DATA ffd;
            std::wstring eventCoordDir = std::wstring( L"event_coordinators\\*" );

            HANDLE hFind = FindFirstFile(eventCoordDir.c_str(), &ffd);
            do
            {
                if ( !(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ) // ignore . and ..
                {
                    std::wstring dllPath = std::wstring( L"event_coordinators\\" ) + ffd.cFileName;
                    // must end in .dll
                    if( dllPath.find( L".dll" ) == std::string::npos ||
                        dllPath.find( L".dll" ) != dllPath.length()-4 )
                    {
                        LOG_DEBUG_F( "%S is not a DLL\n", dllPath );
                        continue;
                    }
                    HMODULE ecDll = LoadLibrary( dllPath.c_str() );
                    if( ecDll == nullptr )
                    {
                        LOG_WARN_F("Failed to load dll %S\n", ffd.cFileName);
                    }
                    else
                    {
                        typedef int (*callProc)(IEventCoordinatorFactory*, IInterventionFactory *);
                        callProc _callProc = (callProc)GetProcAddress( ecDll, "RegisterWithFactory" );
                        if( _callProc != nullptr )
                        {
                            (_callProc)( EventCoordinatorFactory::getInstance(), InterventionFactory::getInstance() );
                        }
                        else
                        {
                            LOG_WARN("GetProcAddr failed for RegisterWithFactory.\n");
                        }
                    }
                }
            }
            while (FindNextFile(hFind, &ffd) != 0);
            doThisOnce = true;
#else
#warning "Need to implement linux version of loading event coordinators as shared objects."
#endif
        }

        LOG_DEBUG("EventCoordinatorFactory::CreateInstance about to create instance from specs now that we have registered dll with factory?\n");
        return CreateInstanceFromSpecs<IEventCoordinator>(config, getRegisteredClasses(), true);
    }

    json::QuickBuilder
    EventCoordinatorFactory::GetSchema()
    {
        // Iterate over all registrangs, instantiate using function pointer (or functor?), call Configure
        // but with special 'don't error out' mode.
        support_spec_map_t& registrants = getRegisteredClasses();

        JsonConfigurable::_dryrun = true;
        for (auto& entry : registrants)
        {
            const std::string& class_name = entry.first;
            LOG_DEBUG_F("class_name = %s\n", class_name.c_str());
            json::Object fakeJson;
            fakeJson["class"] = json::String(class_name);
            Configuration * fakeConfig = Configuration::CopyFromElement( fakeJson );
            try
            {
                auto pEC = CreateInstanceFromSpecs<IEventCoordinator>(fakeConfig, getRegisteredClasses(), true);
                release_assert( pEC );
                json::QuickBuilder* schema = &dynamic_cast<JsonConfigurable*>(pEC)->GetSchema();
                (*schema)[std::string("class")] = json::String( class_name );
                ecSchema[class_name] = *schema;
            }
            catch( DetailedException &e )
            {
                std::ostringstream msg;
                msg << "ConfigException creating intervention for GetSchema: " 
                    << e.what()
                    << std::endl;
                LOG_INFO( msg.str().c_str() );
            }
            catch( const json::Exception &e )
            {
                std::ostringstream msg;
                msg << "json Exception creating intervention for GetSchema: "
                    << e.what()
                    << std::endl;
                LOG_INFO( msg.str().c_str() );
            }
            LOG_DEBUG( "Done with that class....\n" );
            delete fakeConfig;
            fakeConfig = nullptr;
        }
        LOG_DEBUG( "Returning from GetSchema.\n" );
        json::QuickBuilder retSchema = json::QuickBuilder(ecSchema);
        return retSchema;
    }
}



