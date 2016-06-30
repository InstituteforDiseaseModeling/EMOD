/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/
#pragma once

#include "stdafx.h"
#include <typeinfo>

#include "NodeEventContext.h"
#include "NodeEventContextHost.h"
#include "Debug.h"
#include "NodeDemographics.h"
#include "Node.h"
#include "IIndividualHuman.h"

#include "Log.h"
#include "Exceptions.h"

static const char * _module = "NodeEventContext";

namespace Kernel
{
    NodeEventContextHost::NodeEventContextHost()
    : node(nullptr)
    {
        arrival_distribution_sources.clear();
    }

    NodeEventContextHost::NodeEventContextHost(Node* _node)
    : node(_node)
    {
        arrival_distribution_sources.clear();
    }

    // This was done with macros, but prefer actual code.
    Kernel::QueryResult NodeEventContextHost::QueryInterface( iid_t iid, void** ppinstance )
    {
        release_assert(ppinstance); // todo: add a real message: "QueryInterface requires a non-NULL destination!");

        if ( !ppinstance )
            return e_NULL_POINTER;

        ISupports* foundInterface;

        if ( iid == GET_IID(INodeEventContext)) 
            foundInterface = static_cast<INodeEventContext*>(this);
        else if (iid == GET_IID(INodeInterventionConsumer))
            foundInterface = static_cast<INodeInterventionConsumer*>(this);
        else if (iid == GET_IID(IOutbreakConsumer))
            foundInterface = static_cast<IOutbreakConsumer*>(this);
        else if (iid == GET_IID(ICampaignCostObserver))
            foundInterface = static_cast<ICampaignCostObserver*>(this);
        else if (iid == GET_IID(INodeTriggeredInterventionConsumer))
            foundInterface = static_cast<INodeTriggeredInterventionConsumer*>(this);
        // -->> add support for other I*Consumer interfaces here <<--      
        else if (iid == GET_IID(IGlobalContext))
            node->QueryInterface(iid, (void**) &foundInterface);
        else if (iid == GET_IID(INodeContext))
            foundInterface = (INodeContext*)node;
        else
            foundInterface = nullptr;

        QueryResult status = e_NOINTERFACE;
        if ( foundInterface )
        {
            foundInterface->AddRef();
            status = s_OK;
            //status = InterventionsContainer::QueryInterface(iid, (void**)&foundInterface);
        }

        *ppinstance = foundInterface;
        return status;
    }

    void
    NodeEventContextHost::SetContextTo(
        INodeContext* context
    )
    {
        
        PropagateContextToDependents();
    }

    // method 1 for VisitIndividuals uses a functor/lambda function
    void NodeEventContextHost::VisitIndividuals( individual_visit_function_t func )
    {
        for( auto individual : node->individualHumans)
        {
            // TODO: I would like this function to return bool so we can keep a total of 'successful' calls, but
            // compiler threw up on me when I tried that! :(
            func(individual->GetEventContext());
        }
    }

    // method 2 for VisitIndividuals uses an interface
    int
    NodeEventContextHost::VisitIndividuals(
        IVisitIndividual * pEventCoordinator,
        int limit // = -1
    )
    {
        int retTotal = 0;
        size_t nodePop = node->individualHumans.size();
        float probThreshold = 1.0;
        if( limit != -1 )
        {
            probThreshold = float(limit)/nodePop;
        }
        float nodeCostThisInterventionThisTimestep = 0.0f;
        for( auto individual : node->individualHumans)
        {
            /*
             * hard limit
            if( limit > -1 && retTotal >= limit )
            {
                break;
            }*/

            // Do probabilistically
            if( node->GetRng()->e() < probThreshold )
            {
                float costForThisIntervention = 0.0f;
                if( pEventCoordinator->visitIndividualCallback(individual->GetEventContext(), costForThisIntervention, this))
                {
                    retTotal++;
                    //nodeCostThisInterventionThisTimestep += (float)( costForThisIntervention * ih->GetMonteCarloWeight() );
                }
            }
        }

        IncrementCampaignCost( nodeCostThisInterventionThisTimestep );
        LOG_DEBUG_F("Node CampaignCost = %f\n", nodeCostThisInterventionThisTimestep);
        return retTotal;
    }

    void NodeEventContextHost::notifyCampaignExpenseIncurred(
        float expenseIncurred,
        const IIndividualHumanEventContext * pIndiv
    )
    {
        release_assert( node );
        release_assert( pIndiv );
        if( expenseIncurred > 0 )
        {
            LOG_DEBUG("Campaign expense was incurred\n");
        }
        IncrementCampaignCost( expenseIncurred * float(pIndiv->GetMonteCarloWeight()) );
    }

    // First cut of this function writes the intervention report to stdout. It is an abbreviated,
    // 1-line-per-intervention report.
    void NodeEventContextHost::notifyCampaignEventOccurred(
        /*const*/ ISupports * pDistributedIntervention,
        /*const*/ ISupports * pDistributor,// for tb, cool to know parent intervention that 'gave', including tendency if it's an HSB.
        /*const*/ IIndividualHumanContext * pDistributeeIndividual
    )
    {
        if( Environment::getInstance()->Log->CheckLogLevel(Logger::DEBUG, "EEL") )
        {
            // intervention recipient
            std::stringstream msg;
            float recipientAge = float(pDistributeeIndividual->GetEventContext()->GetAge());
            msg << "hum_id="
                << pDistributeeIndividual->GetSuid().data
                << ",ra="
                << recipientAge
                << std::endl;
            Environment::getInstance()->Log->LogF(Logger::DEBUG, "EEL","%s\n", msg.str().c_str() );
        }
    }

    const NodeDemographics& NodeEventContextHost::GetDemographics()
    {
        return node->demographics;
    }

    const suids::suid &
    NodeEventContextHost::GetId()
    const
    {
        return node->suid;
    }

    void NodeEventContextHost::ProcessArrivingIndividual( IIndividualHuman* ih )
    {
        for (auto& entry : arrival_distribution_sources)
        {
            for (int k = 0; k < entry.second; k++)
                entry.first->ProcessArriving(ih->GetEventContext());
        }
    }

    void NodeEventContextHost::ProcessDepartingIndividual( IIndividualHuman* ih )
    {
        //LOG_DEBUG( "ProcessDepartingIndividual\n" );
        for (auto& entry : departure_distribution_sources)
        {
            LOG_DEBUG( "ProcessDepartingIndividual for event coordinator\n" );
            for (int k = 0; k < entry.second; k++)
            {
                LOG_DEBUG( "ProcessDepartingIndividual: Calling ProcessDeparting for individual\n" );
                entry.first->ProcessDeparting(ih->GetEventContext());
            }
        }
    }

    void NodeEventContextHost::RegisterTravelDistributionSource( ITravelLinkedDistributionSource *tlds, TravelEventType type )
    {
        travel_distribution_source_map_t *sources = sourcesMapForType(type);        
        if (sources->find(tlds) != sources->end())
            (*sources)[tlds]++;
        else
            (*sources)[tlds] = 1;

        tlds->AddRef();
    }

    void NodeEventContextHost::UnregisterTravelDistributionSource( ITravelLinkedDistributionSource *tlds, TravelEventType type )
    {
        travel_distribution_source_map_t *sources = sourcesMapForType(type);        
        if (sources->find(tlds) != sources->end())
        {
            (*sources)[tlds]--;
            tlds->Release();

            if ((*sources)[tlds] == 0)
            {
                sources->erase(tlds);
            }
        } else
        {
            // unregistering something that wasnt registered...probably an error
        }
    }

    void NodeEventContextHost::RegisterNodeEventObserverByString(
        IIndividualEventObserver * pObserver,
        const std::string &trigger
    )
    {
        if( std::find( individual_event_observers[ trigger ].begin(), individual_event_observers[ trigger ].end(), pObserver ) != individual_event_observers[ trigger ].end() )
        {
            std::stringstream ss ;
            ss << "Trying to register an observer (" << typeid(*pObserver).name() << ") more than once to event " << trigger ;
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }

        INodeDistributableIntervention * ndi;
        if( s_OK == pObserver->QueryInterface(GET_IID(INodeDistributableIntervention), (void**)&ndi) )
        {
            // DMB - something in HINT needs this
            ndi->SetContextTo( node->GetEventContext() ); // this is probably unnecessarily circular (a GetContextPointer() function could be added?)
            ndi->Release();
        }

        LOG_DEBUG_F( "Observer is registering for event %s.\n", trigger.c_str() );
        individual_event_observers[ trigger ].push_back( pObserver );
        pObserver->AddRef();
    }

    void NodeEventContextHost::RegisterNodeEventObserver(
        IIndividualEventObserver * pObserver,
        const IndividualEventTriggerType::Enum &trigger_event
    )
    {
        std::string trigger = IndividualEventTriggerType::pairs::lookup_key(trigger_event);
        RegisterNodeEventObserverByString( pObserver, trigger );
    }

    void NodeEventContextHost::UnregisterNodeEventObserverByString(
        IIndividualEventObserver * pObserver,
        const std::string &trigger
    )
    {
        release_assert( individual_event_observers.find( trigger ) != individual_event_observers.end() );
        LOG_INFO( "[UnregisterIndividualEventObserver] Putting individual event observer into the disposed observers list .\n" );
        disposed_observers[ trigger ].push_back( pObserver );
    }

    void NodeEventContextHost::UnregisterNodeEventObserver(
        IIndividualEventObserver * pObserver,
        const IndividualEventTriggerType::Enum &trigger_event
    )
    {
        release_assert( pObserver );
        std::string trigger = IndividualEventTriggerType::pairs::lookup_key(trigger_event);
        UnregisterNodeEventObserverByString( pObserver, trigger );
    }

    void NodeEventContextHost::TriggerNodeEventObserversByString(
        IIndividualHumanEventContext *pIndiv,
        const std::string& trigger
    )
    {
        if( trigger == "NoTrigger" )
        {
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "trigger = NoTrigger. Nobody should be publishing NoTrigger. Please find root cause and fix." );
        }

        LOG_DEBUG_F( "Individual %d fired off event %s at time %f.\n", pIndiv->GetInterventionsContext()->GetParent()->GetSuid().data, trigger.c_str(), (float)GetTime().time );
        LOG_DEBUG_F( "We have %d observers of event %s.\n", individual_event_observers[ trigger ].size(), trigger.c_str() );
        for (auto observer : individual_event_observers[ trigger ] )
        {
            // ---------------------------------------------------------------------
            // --- Make sure the observer has not been requested to be unregistered
            // --- from being notified of events.
            // ---------------------------------------------------------------------
            bool notify = true ;
            if( disposed_observers.count( trigger ) > 0 )
            {
                auto& r_disposed_list = disposed_observers[ trigger ] ;
                auto it = std::find( r_disposed_list.begin(), r_disposed_list.end(), observer ) ;
                notify = (it == r_disposed_list.end()); // we want notify to be false if we find something
            }

            if( notify )
            {
                observer->notifyOnEvent( pIndiv, trigger ); 
            }
        }
    }

    void NodeEventContextHost::TriggerNodeEventObservers(
        IIndividualHumanEventContext *pIndiv,
        const IndividualEventTriggerType::Enum &StateChange
    )
    {
        std::string trigger = IndividualEventTriggerType::pairs::lookup_key(StateChange);
        TriggerNodeEventObserversByString( pIndiv, trigger );
    }

    void NodeEventContextHost::PropagateContextToDependents()
    {
//#ifdef NEC
        INodeEventContext *inec = node->GetEventContext(); // this is again probably unnecessarily circular
        for (auto &observer_nvp : individual_event_observers) // ieo is map of types to lists
        {
            for (auto &observer : observer_nvp.second) // observer_nvp.second is list, observer is intervention
            {
                // QI for NDI to reset context
                INodeDistributableIntervention * intervention = nullptr;
                if( s_OK == observer->QueryInterface(GET_IID(INodeDistributableIntervention), (void**)&intervention) )
                {
                    intervention->SetContextTo(inec);
                }
                else
                {
                    LOG_WARN( "Unable to get INodeDistributableIntervention interface from IIndividualEventObserver." );
                }
            }
        }

        for (auto intervention : node_interventions)
        {
            intervention->SetContextTo(inec);
        }
//#endif
    }

    void NodeEventContextHost::IncrementCampaignCost(float cost)
    {
        node->Campaign_Cost += cost;
    }


    void NodeEventContextHost::UpdateInterventions(float dt)
    {
        for (auto intervention : node_interventions)
        {
            intervention->Update(dt);
        }
        // do interventions registered as observers need to be Update-d??? Yes, that's how they timer-based expire themselves and unregister.
        INodeDistributableIntervention* ndi = nullptr;
        for (auto &observer_nvp : individual_event_observers)
        {
            // it would be sad if in the Update the observer get deleted from the list and messed up the iterator!
            for (auto &observer : observer_nvp.second)
            {
                if ( s_OK != observer->QueryInterface(GET_IID(INodeDistributableIntervention), (void**)&ndi) ) continue;
                release_assert(ndi);
                ndi->Update(dt);
                ndi->Release();
            }
        }
        DisposeOfUnregisteredObservers();
    }

    void NodeEventContextHost::DisposeOfUnregisteredObservers()
    {
        if( disposed_observers.size() > 0 )
        {
            LOG_DEBUG_F( "We have %d disposed_observers to clean up.\n", disposed_observers.size() );
        }

        for (auto &garbage : disposed_observers)
        {
            auto trigger = garbage.first;
            auto observer_list = garbage.second;
            for( auto observer : observer_list )
            {
                auto find_it = std::find( individual_event_observers[ trigger ].begin(), individual_event_observers[ trigger ].end(), observer );
                release_assert( find_it != individual_event_observers[ trigger ].end() );
                individual_event_observers[ trigger ].erase( find_it );
                LOG_INFO_F( "[UnregisterIndividualEventObserver] Removed individual event observer from list: now %d observers of event %s.\n",
                            individual_event_observers[ trigger ].size(),
                            trigger.c_str()
                          );
            }
        }
        disposed_observers.clear();
    }

    bool NodeEventContextHost::GiveIntervention( INodeDistributableIntervention* iv )
    {
        node_interventions.push_back( iv );
        iv->AddRef();

        IBaseIntervention * pBaseIV = nullptr;
        if( iv->QueryInterface( GET_IID(IBaseIntervention), (void**)&pBaseIV ) == s_OK ) 
        {
            IncrementCampaignCost( pBaseIV->GetCostPerUnit() );
            pBaseIV->Release();
        }
        else
        {
            LOG_WARN("Unsuccessful in querying for IBaseIntervention from INodeDistributableIntervention for passing Campaign_Cost back to Node.\n");
        }
        iv->SetContextTo( this );
        return true;
    }

    std::list<INodeDistributableIntervention*> NodeEventContextHost::GetInterventionsByType(const std::string& type_name)
    {
        std::list<INodeDistributableIntervention*> interventions_of_type;
        LOG_INFO_F("Looking for intervention of type %s", type_name.c_str());
        for (auto intervention : node_interventions)
        {
            std::string cur_iv_type_name = typeid( *intervention ).name();
            if( cur_iv_type_name == type_name )
            {
                LOG_INFO( "Found one..." );
                interventions_of_type.push_back( intervention );
            }
        }

        return interventions_of_type;
    }

    void NodeEventContextHost::PurgeExisting( const std::string& iv_name )
    {
        for (auto intervention : node_interventions)
        {
            std::string cur_iv_type_name = typeid( *intervention ).name();
            if( cur_iv_type_name == iv_name )
            {
                LOG_INFO_F("Found an existing intervention by that name (%s) which we are purging\n", iv_name.c_str());
                node_interventions.remove( intervention );
                delete intervention;
                break;
            }
        }
    }

    NodeEventContextHost::~NodeEventContextHost()
    {
        DisposeOfUnregisteredObservers();
        cleanupDistributionSourceMap(arrival_distribution_sources);
        cleanupDistributionSourceMap(departure_distribution_sources);

        for (auto &observer_map : individual_event_observers)
        {
            LOG_DEBUG_F( "Deleting %d observers.\n", observer_map.second.size() );
            
            for (auto &observer : observer_map.second)
            {
                dynamic_cast<ISupports*>(observer)->Release();
            }
        }

        for (auto intervention : node_interventions)
        {
            delete intervention;
        }
    }

    NodeEventContextHost::travel_distribution_source_map_t* NodeEventContextHost::sourcesMapForType( INodeEventContext::TravelEventType type )
    {
        travel_distribution_source_map_t *sources = nullptr;
        switch (type)
        {
        case Arrival: sources = &arrival_distribution_sources; break;
        case Departure: sources = &departure_distribution_sources; break;
        default: break;
        }
        return sources;
    }

    void NodeEventContextHost::cleanupDistributionSourceMap( travel_distribution_source_map_t &sources )
    {
        for (auto& entry : sources)
        {
            for (int k = 0; k < entry.second; k++)
                entry.first->Release();
        }

        sources.clear();
    }

    void NodeEventContextHost::AddImportCases(
        StrainIdentity* outbreak_strainID,
        float import_age,
        NaturalNumber num_cases_per_node
    )
    {
        for (int i = 0; i < num_cases_per_node; i++)
        {
            IIndividualHuman* new_individual = node->configureAndAddNewIndividual(1.0, import_age, 0.0, 0.5); // using age specified by Oubreak, but otherwise community demographics for import case (e.g. immune history)

            // 0 = incubation_period_override, outbreaks are instantaneously mature
            new_individual->AcquireNewInfection(outbreak_strainID, 0 ); 
        }
    }

    // These are all pass-throughs in order to get rid of GetNode() as an interface method.
    bool NodeEventContextHost::IsInPolygon(float* vertex_coords, int numcoords)
    {
        return node->IsInPolygon( vertex_coords, numcoords );
    }

    bool NodeEventContextHost::IsInPolygon( const json::Array &poly )
    {
        return node->IsInPolygon( poly );
    }

    bool NodeEventContextHost::IsInExternalIdSet( const tNodeIdList& nodelist )
    {
        return node->IsInExternalIdSet( nodelist );
    }

    bool NodeEventContextHost::GetUrban() const
    {
        return node->GetUrban();
    }

    IdmDateTime NodeEventContextHost::GetTime() const
    {
        return node->GetTime();
    }

    RANDOMBASE*  NodeEventContextHost::GetRng()
    {
        return node->GetRng();
    } 

    INodeContext* NodeEventContextHost::GetNodeContext()
    {
        return static_cast<INodeContext*>(node);
    }

    int NodeEventContextHost::GetIndividualHumanCount() const
    {
        size_t nodePop = node->individualHumans.size();
        return int(nodePop);
    }

    ExternalNodeId_t NodeEventContextHost::GetExternalId() const
    {
        ExternalNodeId_t nodeId = node->GetExternalID();
        return nodeId;
    }
}

#if 0
namespace Kernel {
    template<class Archive>
    void serialize(Archive &ar, NodeEventContextHost& nec, const unsigned int v)
    {
        ar & nec.node; // hope this works!
        ar & nec.arrival_distribution_sources;
        ar & nec.departure_distribution_sources;
        ar & nec.individual_event_observers;
        ar & nec.node_interventions;
    }
}
#endif
