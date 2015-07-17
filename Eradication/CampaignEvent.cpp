/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"
#include <string>
#include <list>
#include <vector>
#include <functional>

#include "ISupports.h"
#include "Debug.h"
#include "EnumSupport.h"
#include "SimpleTypemapRegistration.h"
#include "Environment.h"
//#include "CajunIncludes.h"
#include "Sugar.h"
//#include "ValidationLog.h"
#include "Log.h"
#include "Configuration.h"
#include "Configure.h"
#include "ConfigurationImpl.h"
#include "CampaignEvent.h"
#include "EventCoordinator.h"
#include "FactorySupport.h"

#include "StandardEventCoordinator.h"
#include "SimpleEventCoordinator.h"

static const char * _module = "CampaignEvent";
json::Object Kernel::CampaignEventFactory::ceSchema;

namespace Kernel
{
    using namespace std;
    // CampaignEventFactory
    ICampaignEventFactory * CampaignEventFactory::_instance = NULL;
    CampaignEvent* CampaignEventFactory::CreateInstance(const Configuration * config)
    {
        CampaignEvent *ce = CreateInstanceFromSpecs<CampaignEvent>(config, getRegisteredClasses(), false);
        release_assert(ce);

        if (ce && !JsonConfigurable::_dryrun)
        {           
            // now try to instantiate and configure the NodeSet and the EventCoordinator

            auto ns_config = Configuration::CopyFromElement( ce->nodeset_config._json );
            ce->nodeset = NodeSetFactory::CreateInstance( ns_config );
            delete ns_config;

            auto ec_config = Configuration::CopyFromElement((ce->event_coordinator_config._json));
            ce->event_coordinator = EventCoordinatorFactory::CreateInstance( ec_config );
            delete ec_config;

            if (!ce->nodeset)
            {
                throw FactoryCreateFromJsonException( __FILE__, __LINE__, __FUNCTION__, std::string( json::QuickInterpreter(ce->nodeset_config._json)["class"].As<String>()).c_str() );
            }
            if (!ce->event_coordinator)
            {
                throw FactoryCreateFromJsonException(__FILE__, __LINE__, __FUNCTION__, string( json::QuickInterpreter(ce->event_coordinator_config._json)["class"].As<String>()).c_str());
            }

            if (!ce->nodeset || !ce->event_coordinator)
            {               
                ce->Release();
                return NULL;
            }
        }
        return ce;
    }

    void
    CampaignEventFactory::Register(string classname, instantiator_function_t _if)  {  getRegisteredClasses()[classname] = _if;  }

    support_spec_map_t&
    CampaignEventFactory::getRegisteredClasses() { static support_spec_map_t registered_classes; return registered_classes; }

    json::QuickBuilder
    CampaignEventFactory::GetSchema()
    {
        // Iterate over all registrants, instantiate using function pointer, call Configure
        // but in 'don't QI' mode.
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
                auto *pCE = CreateInstanceFromSpecs<CampaignEvent>(fakeConfig, getRegisteredClasses(), false);
                release_assert( pCE );
                json::QuickBuilder* schema = &dynamic_cast<JsonConfigurable*>(pCE)->GetSchema();
                (*schema)[std::string("class")] = json::String( class_name );
                ceSchema[class_name] = *schema;
            }
            catch( DetailedException &e )
            {
                std::ostringstream msg;
                msg << "ConfigException creating intervention for GetSchema: " 
                    << e.what()
                    << std::endl;
                LOG_INFO( msg.str().c_str() );
            }
            catch( json::Exception &e )
            {
                std::ostringstream msg;
                msg << "json Exception creating intervention for GetSchema: "
                    << e.what()
                    << std::endl;
                LOG_INFO( msg.str().c_str() );
            }
        }
        LOG_DEBUG( "Returning from GetSchema.\n" );
        json::QuickBuilder retSchema = json::QuickBuilder(ceSchema);
        return retSchema;
    }

    // CampaignEvent

    IMPLEMENT_FACTORY_REGISTERED(CampaignEvent)

    QuickBuilder CampaignEvent::GetSchema()
    {
        return QuickBuilder( jsonSchemaBase );
    }

    IMPL_QUERY_INTERFACE1(CampaignEvent, IConfigurable)

    CampaignEvent::CampaignEvent()
    : nodeset(NULL)
    , event_coordinator(NULL)
    , event_index(0)
    {
    }

    bool
    CampaignEvent::Configure(
        const Configuration * inputJson
    )
    {
        initConfigTypeMap( "Start_Day", &start_day, Start_Day_DESC_TEXT, 0 );
        initConfigComplexType( "Nodeset_Config", &nodeset_config, Nodeset_Config_DESC_TEXT );
        initConfigComplexType( "Event_Coordinator_Config", &event_coordinator_config, Event_Coordinator_Config_DESC_TEXT );
        return JsonConfigurable::Configure( inputJson );
    }

    void CampaignEvent::Dispatch( ISimulationEventContext *isec )
    {
        event_coordinator->SetContextTo(isec);

        // this activates the event_coordinator for the nodeset
        ISimulationEventContext::node_visit_function_t visit_func = 
            [this](suids::suid node_id, INodeEventContext *nec) -> bool
            {
                if (nodeset->Contains(nec))
                {
                    event_coordinator->AddNode(node_id);
                    return true;
                }
                else
                {
                    return false;
                }
            };
       
        isec->VisitNodes(visit_func); // add all the nodes
        isec->RegisterEventCoordinator(event_coordinator);

    }

    CampaignEvent::~CampaignEvent()
    {
        if (event_coordinator) { event_coordinator->Release(); event_coordinator = NULL; }
        if (nodeset) { nodeset->Release(); nodeset = NULL; }
    }

    float
    CampaignEvent::GetStartDay() const { return start_day; }

    int
    CampaignEvent::GetEventIndex() const { return event_index; }

    void
    CampaignEvent::SetEventIndex(int index) { event_index = index; }

    //
    // CampaignEventByYear class here.
    //
    IMPL_QUERY_INTERFACE1(CampaignEventByYear, IConfigurable)
    IMPLEMENT_FACTORY_REGISTERED(CampaignEventByYear)

    CampaignEventByYear::CampaignEventByYear()
    {
    }

    bool
    CampaignEventByYear::Configure(
        const Configuration * inputJson
    )
    {
        float start_year;
        initConfigTypeMap( "Start_Year", &start_year, Start_Year_DESC_TEXT, 0 );
        initConfigComplexType( "Nodeset_Config", &nodeset_config, Nodeset_Config_DESC_TEXT );
        initConfigComplexType( "Event_Coordinator_Config", &event_coordinator_config, Event_Coordinator_Config_DESC_TEXT );

        // Bypasss CampaignEvent base class so that we don't break without Start_Day!
        bool ret = JsonConfigurable::Configure( inputJson );
        // Throw in some error handling here. Base_Year may not be present. 
        float base_year = IdmDateTime::_base_year;
        /*if( base_year == 0 )
        {
            throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, "Base_Year", "0", "Start_Year", std::to_string( start_year ).c_str() );
        }*/
        start_day = (start_year - base_year) * DAYSPERYEAR;
        return ret;
    }

    QuickBuilder CampaignEventByYear::GetSchema()
    {
        return QuickBuilder( jsonSchemaBase );
    }

    CampaignEventByYear::~CampaignEventByYear()
    {
    }

#if USE_JSON_SERIALIZATION

    // IJsonSerializable Interfaces
    void CampaignEvent::JSerialize( IJsonObjectAdapter* root, JSerializer* helper ) const
    {
        root->BeginObject();

        root->Insert("nodeset");

        // We should do this better by using QI later
        string nsClassName = json::QuickInterpreter(nodeset_config._json)["class"].As<String>();
        if (nsClassName.compare("NodeSetAll") == 0)
        {
            ((NodeSetAll*)nodeset)->JSerialize(root, helper);
        }
        else if (nsClassName.compare("NodeSetPolygon") == 0)
        {
            ((NodeSetPolygon*)nodeset)->JSerialize(root, helper);
        }
        else if (nsClassName.compare("NodeSetNodeList") == 0)
        {
            ((NodeSetNodeList*)nodeset)->JSerialize(root, helper);
        }
        else
        {
            throw GeneralConfigurationException(__FILE__, __LINE__, __FUNCTION__, nsClassName.c_str());
        }

        root->Insert("event_coordinator");

        string ecClassName = json::QuickInterpreter(event_coordinator_config._json)["class"].As<String>();
        if (ecClassName.compare("StandardInterventionDistributionEventCoordinator") == 0)
        {
            ((StandardInterventionDistributionEventCoordinator*)event_coordinator)->JSerialize(root, helper);
        }
        else if (ecClassName.compare("SimpleInterventionDistributionEventCoordinator") == 0)
        {
            ((SimpleInterventionDistributionEventCoordinator*)event_coordinator)->JSerialize(root, helper);
        }
        else
        {
            throw GeneralConfigurationException(__FILE__, __LINE__, __FUNCTION__, ecClassName.c_str());
        }

        root->Insert("start_day", start_day);
        root->Insert("event_index", event_index);

        root->EndObject();
    }

    void CampaignEvent::JDeserialize( IJsonObjectAdapter* root, JSerializer* helper )
    {
    }
#endif
}

#if USE_BOOST_SERIALIZATION
BOOST_CLASS_EXPORT(Kernel::CampaignEvent)
namespace Kernel {
    template<class Archive>
    void serialize(Archive &ar, CampaignEvent& event, const unsigned int v)
    {
        ar & event.nodeset;
        ar & event.event_coordinator;
        ar & event.start_day;
        ar & event.event_index;
        ar & event.distribution_duration;
    }
}
#endif
