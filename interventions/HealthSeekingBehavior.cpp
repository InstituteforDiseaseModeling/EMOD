/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"
#include "HealthSeekingBehavior.h"

#include "InterventionEnums.h"
#include "InterventionFactory.h"
#include "NodeEventContext.h"    // for INodeEventContext (ICampaignCostObserver)

static const char * _module = "SimpleHealthSeekingBehavior";

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(SimpleHealthSeekingBehavior)
        HANDLE_INTERFACE(IConfigurable)
        HANDLE_INTERFACE(IBaseIntervention)
        HANDLE_INTERFACE(IDistributableIntervention)
        HANDLE_ISUPPORTS_VIA(IDistributableIntervention)
    END_QUERY_INTERFACE_BODY(SimpleHealthSeekingBehavior)

    IMPLEMENT_FACTORY_REGISTERED(SimpleHealthSeekingBehavior)

    bool
    SimpleHealthSeekingBehavior::Configure(
        const Configuration * inputJson
    )
    {
        EventOrConfig::Enum use_event_or_config;
        initConfig( "Event_Or_Config", use_event_or_config, inputJson, MetadataDescriptor::Enum("EventOrConfig", Event_Or_Config_DESC_TEXT, MDD_ENUM_ARGS( EventOrConfig ) ) );
        if( use_event_or_config == EventOrConfig::Event || JsonConfigurable::_dryrun )
        {
            initConfigTypeMap( "Actual_IndividualIntervention_Event", &actual_intervention_event, HSB_Actual_Intervention_Config_Event_DESC_TEXT, NO_TRIGGER_STR );
        }

        if( use_event_or_config == EventOrConfig::Config || JsonConfigurable::_dryrun )
        {
            initConfigComplexType( "Actual_IndividualIntervention_Config", &actual_intervention_config, HSB_Actual_Intervention_Config_DESC_TEXT, "Event_Or_Config", "Config" );
        }

        initConfigTypeMap( "Tendency", &probability_of_seeking, HSB_Tendency_DESC_TEXT, 0.0, 1.0, 1.0 );
        initConfigTypeMap( "Single_Use", &single_use, HSB_Single_Use_DESC_TEXT, true ); //true means it will expire after a single use

        bool ret = JsonConfigurable::Configure( inputJson );
        if( ret && (use_event_or_config == EventOrConfig::Config || JsonConfigurable::_dryrun) )
        {
            InterventionValidator::ValidateIntervention( actual_intervention_config._json );
        }
        return ret ;
    }

    SimpleHealthSeekingBehavior::SimpleHealthSeekingBehavior()
    : parent(nullptr)
    , probability_of_seeking(0)
    , m_pCCO(nullptr)
    , single_use(true)
    , actual_intervention_event( "NoTrigger" )
    {
    }

    // Each time this is called, the HSB intervention is going to decide for itself if
    // health should be sought. For start, just do it based on roll of the dice. If yes,
    // an intervention needs to be created (from 'actual' config) and distributed to the 
    // individual who owns us.
    void SimpleHealthSeekingBehavior::Update( float dt )
    {
        bool wasDistributed = false;
        
        if(expired) return; // don't give expired intervention.  should be cleaned up elsewhere anyways, though.

        LOG_DEBUG_F("Individual %d is seeking with tendency %f \n", parent->GetSuid().data, probability_of_seeking*dt);

        if( parent->GetRng()->e() < probability_of_seeking * dt)
        {
            LOG_DEBUG_F("SimpleHealthSeekingBehavior is going to give the actual intervention to individual %d\n", parent->GetSuid().data );
            
            // Important: Use the instance method to obtain the intervention factory obj instead of static method to cross the DLL boundary
            IGlobalContext *pGC = NULL;
            const IInterventionFactory* ifobj = NULL;
            if (s_OK == parent->QueryInterface(GET_IID(IGlobalContext), (void**)&pGC))
            {
                ifobj = pGC->GetInterventionFactory();
            }
            if (!ifobj)
            {
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, "The pointer to IInterventionFactory object is not valid (could be DLL specific)" );
            }

            if( actual_intervention_event != "NoTrigger" )
            {
                INodeTriggeredInterventionConsumer* broadcaster = nullptr;
                if (s_OK != parent->GetEventContext()->GetNodeEventContext()->QueryInterface(GET_IID(INodeTriggeredInterventionConsumer), (void**)&broadcaster))
                {
                    throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent->GetEventContext()->GetNodeEventContext()", "INodeTriggeredInterventionConsumer", "INodeEventContext" );
                }
                LOG_DEBUG_F("SimpleHealthSeekingBehavior is broadcasting the actual intervention event to individual %d.\n", parent->GetSuid().data );
                broadcaster->TriggerNodeEventObserversByString( parent->GetEventContext(), actual_intervention_event );

            }
            else
            {
                //auto indiv_id = parent->GetInterventionsContext()->GetParent()->GetSuid().data;
                LOG_DEBUG_F("SimpleHealthSeekingBehavior is distributing actual intervention to individual %d.\n", parent->GetSuid().data );
                auto config = Configuration::CopyFromElement( (actual_intervention_config._json) );
                IDistributableIntervention *di = const_cast<IInterventionFactory*>(ifobj)->CreateIntervention( config ); 
                delete config;
/*
                // In theory this is a good addition, but I can't get it to trigger when I add other upstream
                // error checking and handling. And if we add it here, we should add it to the other places we do CreateIntervention's.
                IDistributableIntervention *di = nullptr;
                try
                {
                    di = const_cast<IInterventionFactory*>(ifobj)->CreateIntervention(Configuration::CopyFromElement( (actual_intervention_config._json) )); 
                }
                catch( json::Exception &except )
                {
                    std::ostringstream msg;
                    msg << "Nested json for actual_intervention_config was empty or corrupted: ";
                    json::Writer::Write( actual_intervention_config._json, msg );                     //LOG_DEBUG_F( "%s", msg.str().c_str() );
                    throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
                }
*/
                // Now make sure cost gets reported back to node
                ICampaignCostObserver* pICCO;
                if (s_OK == parent->GetEventContext()->GetNodeEventContext()->QueryInterface(GET_IID(ICampaignCostObserver), (void**)&pICCO) )
                { 
                    m_pCCO = pICCO;
                }
                else
                {
                    throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent->GetEventContext()->GetNodeEventContext()", "ICampaignCostObserver", "INodeEventContext");
                }

                //Expire();
    
                if (s_OK == parent->GetEventContext()->GetNodeEventContext()->QueryInterface(GET_IID(ICampaignCostObserver), (void**)&pICCO) )
                {
                    LOG_DEBUG_F( "Got di object; calling Distribute.\n" );
                    di->Distribute( parent->GetInterventionsContext(), pICCO );
                    pICCO->notifyCampaignEventOccurred( (IBaseIntervention*)di, (IBaseIntervention*)this, parent );
                }
                else
                {
                    throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent->GetEventContext()->GetNodeEventContext()", "ICampaignCostObserver", "INodeEventContext");
                }
            }
            if( single_use )
            {
                LOG_DEBUG_F( "HSB for individual %d is expiring after single use.\n", parent->GetSuid().data );
                expired = true;
            }
            else
            {
                LOG_DEBUG_F( "HSB for individual %d will not expire after use.\n", parent->GetSuid().data );
            }
        }
        else
        {
            LOG_DEBUG("SimpleHealthSeekingBehavior did not distribute\n");
        }
    }
    
    void SimpleHealthSeekingBehavior::SetContextTo(IIndividualHumanContext *context) 
    { 
        parent = context; // for rng
 
        if (s_OK != context->GetEventContext()->GetNodeEventContext()->QueryInterface(GET_IID(ICampaignCostObserver), (void**)&m_pCCO) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent->GetEventContext()->GetNodeEventContext()", "ICampaignCostObserver", "INodeEventContext");
        }

    }

    void SimpleHealthSeekingBehavior::Expire()
    {
        expired = true;
        // notify campaign event observer
        if (m_pCCO != NULL)
        {
            m_pCCO->notifyCampaignEventOccurred( (IBaseIntervention*)this, NULL, parent );
        }   

    }
}

#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
BOOST_CLASS_EXPORT(Kernel::SimpleHealthSeekingBehavior)

namespace Kernel {
    // Would like to put all serialization stuff here but having these in headers
    // is posing a problem due to multiple subclasses. :(
    //REGISTER_SERIALIZATION_VOID_CAST(SimpleHealthSeekingBehavior, IDistributableIntervention);

    template<class Archive>
    void serialize(Archive &ar, SimpleHealthSeekingBehavior& obj, const unsigned int v)
    {
        static const char * _module = "SimpleHealthSeekingBehavior";
        LOG_DEBUG("(De)serializing SimpleHealthSeekingBehavior\n");

        boost::serialization::void_cast_register<SimpleHealthSeekingBehavior, IDistributableIntervention>();
        ar & (std::string) obj.actual_intervention_event;
        ar & obj.actual_intervention_config;
        ar & obj.probability_of_seeking;
        ar & obj.single_use;
        ar & boost::serialization::base_object<Kernel::BaseIntervention>(obj);
    }
}
#endif
