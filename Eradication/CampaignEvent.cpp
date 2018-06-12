/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include <string>
#include <functional>

#include "ISupports.h"
#include "Debug.h"
#include "EnumSupport.h"
#include "Environment.h"
#include "Sugar.h"
#include "Log.h"
#include "Configuration.h"
#include "Configure.h"
#include "ConfigurationImpl.h"
#include "CampaignEvent.h"
#include "EventCoordinator.h"
#include "FactorySupport.h"

// Note: These includes appear to be necessary for EMODule build to be aware
// of thse ECs. Need to move away from prior knowledge of specific ECs.
#include "GroupEventCoordinator.h"
#include "StandardEventCoordinator.h"
#include "CoverageByNodeEventCoordinator.h"
#include "CalendarEventCoordinator.h"
#include "NodeSet.h"

SETUP_LOGGING( "CampaignEvent" )
json::Object Kernel::CampaignEventFactory::ceSchema;

namespace Kernel
{
    // silly hack to get GCC not to resolve these whole classes away as unused.
    //IMPLEMENT_FACTORY_REGISTERED(NodeSetAll)
    //IMPLEMENT_FACTORY_REGISTERED(NodeSetNodeList)
    //IMPLEMENT_FACTORY_REGISTERED(NodeSetPolygon)

    using namespace std;
    // CampaignEventFactory
    ICampaignEventFactory * CampaignEventFactory::_instance = nullptr;
    CampaignEvent* CampaignEventFactory::CreateInstance(const Configuration * config)
    {
        CampaignEvent *ce = CreateInstanceFromSpecs<CampaignEvent>(config, getRegisteredClasses(), false);
        release_assert(ce);

        if (ce && !JsonConfigurable::_dryrun)
        {           
            // now try to instantiate and configure the NodeSet and the EventCoordinator

            auto ns_config = Configuration::CopyFromElement( ce->nodeset_config._json, config->GetDataLocation() );
            ce->nodeset = NodeSetFactory::CreateInstance( ns_config );
            delete ns_config;
            ns_config = nullptr;

            auto ec_config = Configuration::CopyFromElement( (ce->event_coordinator_config._json), config->GetDataLocation() );
            ce->event_coordinator = EventCoordinatorFactory::CreateInstance( ec_config );
            delete ec_config;
            ec_config = nullptr;

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
                return nullptr;
            }

            // make sure the start day for the coordinator makes sense
            ce->event_coordinator->CheckStartDay( ce->GetStartDay() );
        }
        return ce;
    }

    void CampaignEventFactory::Register(string classname, instantiator_function_t _if)  {  getRegisteredClasses()[classname] = _if;  }

    support_spec_map_t& CampaignEventFactory::getRegisteredClasses() { static support_spec_map_t registered_classes; return registered_classes; }

    json::QuickBuilder CampaignEventFactory::GetSchema()
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
            catch( const json::Exception &e )
            {
                std::ostringstream msg;
                msg << "json Exception creating intervention for GetSchema: "
                    << e.what()
                    << std::endl;
                LOG_INFO( msg.str().c_str() );
            }
            delete fakeConfig;
            fakeConfig = nullptr;
        }
        LOG_DEBUG( "Returning from GetSchema.\n" );
        json::QuickBuilder retSchema = json::QuickBuilder(ceSchema);
        return retSchema;
    }

    // CampaignEvent

    IMPLEMENT_FACTORY_REGISTERED(CampaignEvent)

    IMPL_QUERY_INTERFACE1(CampaignEvent, IConfigurable)

    CampaignEvent::CampaignEvent()
        : start_day(0.0f)
        , event_index(0)
        , nodeset(nullptr)
        , event_coordinator(nullptr)
        , nodeset_config()
        , event_coordinator_config()
    {
    }

    bool CampaignEvent::Configure(const Configuration * inputJson)
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
        if (event_coordinator) { event_coordinator->Release(); event_coordinator = nullptr;} 
        if (nodeset) { nodeset->Release(); nodeset = nullptr; }
    }

    float CampaignEvent::GetStartDay() const { return start_day; }

    int CampaignEvent::GetEventIndex() const { return event_index; }

    void CampaignEvent::SetEventIndex(int index) { event_index = index; }

    void CampaignEvent::CheckForValidNodeIDs(const std::vector<ExternalNodeId_t>& demographic_node_ids)
    {
        std::vector<ExternalNodeId_t> nodes_missing_in_demographics = nodeset->IsSubset(demographic_node_ids);
        if (!nodes_missing_in_demographics.empty())
        {
            std::stringstream nodes_missing_in_demographics_str;
            std::copy(nodes_missing_in_demographics.begin(), nodes_missing_in_demographics.end(), ostream_iterator<int>(nodes_missing_in_demographics_str, " "));  // list of missing nodes

            std::stringstream error_msg;
            error_msg << std::string("Found NodeIDs in the campaign that are missing in demographics: ") << nodes_missing_in_demographics_str.str();
            error_msg << std::string(". Only nodes configured in demographics can be used in a campaign.");

            throw InvalidInputDataException(__FILE__, __LINE__, __FUNCTION__, error_msg.str().c_str());
        }
    }

}

#if 0
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
