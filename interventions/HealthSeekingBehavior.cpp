/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "HealthSeekingBehavior.h"

#include "InterventionEnums.h"
#include "InterventionFactory.h"
#include "HealthSeekingBehavior.h"
#include "NodeEventContext.h"    // for INodeEventContext (ICampaignCostObserver)
#include "SimulationConfig.h"
#include "IIndividualHumanContext.h"
#include "ISimulationContext.h"
#include "RANDOM.h"

SETUP_LOGGING( "SimpleHealthSeekingBehavior" )

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
        initConfig( "Event_Or_Config", use_event_or_config, inputJson, MetadataDescriptor::Enum("EventOrConfig", Event_Or_Config_DESC_TEXT, MDD_ENUM_ARGS( EventOrConfig ) ) );
        if( use_event_or_config == EventOrConfig::Event || JsonConfigurable::_dryrun )
        {
            initConfigTypeMap( "Actual_IndividualIntervention_Event", &actual_intervention_event, HSB_Actual_Intervention_Config_Event_DESC_TEXT );
        }

        if( use_event_or_config == EventOrConfig::Config || JsonConfigurable::_dryrun )
        {
            initConfigComplexType( "Actual_IndividualIntervention_Config", &actual_intervention_config, HSB_Actual_Intervention_Config_DESC_TEXT, "Event_Or_Config", "Config" );
        }

        initConfigTypeMap( "Tendency", &probability_of_seeking, HSB_Tendency_DESC_TEXT, 0.0, 1.0, 1.0 );
        initConfigTypeMap( "Single_Use", &single_use, HSB_Single_Use_DESC_TEXT, true ); //true means it will expire after a single use

        bool ret = BaseIntervention::Configure( inputJson );
        if( ret )
        {
            if( use_event_or_config == EventOrConfig::Config || JsonConfigurable::_dryrun )
            {
                InterventionValidator::ValidateIntervention( GetTypeName(),
                                                             InterventionTypeValidation::INDIVIDUAL,
                                                             actual_intervention_config._json, 
                                                             inputJson->GetDataLocation() );
            }
            if( !JsonConfigurable::_dryrun && 
                actual_intervention_event.IsUninitialized() &&
                (actual_intervention_config._json.Type() == ElementType::NULL_ELEMENT) )
            {
                const char* msg = "You must define either Actual_IndividualIntervention_Event or Actual_IndividualIntervention_Config";
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg );
            }
        }
        return ret ;
    }

    SimpleHealthSeekingBehavior::SimpleHealthSeekingBehavior()
    : BaseIntervention()
    , probability_of_seeking(0)
    , m_pCCO(nullptr)
    , single_use(true)
    , use_event_or_config( EventOrConfig::Event )
    , actual_intervention_event()
    {
    }

    // Each time this is called, the HSB intervention is going to decide for itself if
    // health should be sought. For start, just do it based on roll of the dice. If yes,
    // an intervention needs to be created (from 'actual' config) and distributed to the
    // individual who owns us.
    void SimpleHealthSeekingBehavior::Update( float dt )
    {
        if( !BaseIntervention::UpdateIndividualsInterventionStatus() ) return;

        if(expired) return; // don't give expired intervention.  should be cleaned up elsewhere anyways, though.

        LOG_DEBUG_F("Individual %d is seeking with tendency %f \n", parent->GetSuid().data, probability_of_seeking*dt);

        if( parent->GetRng()->SmartDraw( 1- pow( 1-probability_of_seeking, dt ) ) )
        //if( parent->GetRng()->e() < EXPCDF( probability_of_seeking * dt ) )
        {
            LOG_DEBUG_F("SimpleHealthSeekingBehavior is going to give the actual intervention to individual %d\n", parent->GetSuid().data );

            // Important: Use the instance method to obtain the intervention factory obj instead of static method to cross the DLL boundary
            IGlobalContext *pGC = nullptr;
            const IInterventionFactory* ifobj = nullptr;
            if (s_OK == parent->QueryInterface(GET_IID(IGlobalContext), (void**)&pGC))
            {
                ifobj = pGC->GetInterventionFactory();
            }
            if (!ifobj)
            {
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, "The pointer to IInterventionFactory object is not valid (could be DLL specific)" );
            }

            if( use_event_or_config == EventOrConfig::Event )
            {
                IIndividualEventBroadcaster* broadcaster = parent->GetEventContext()->GetNodeEventContext()->GetIndividualEventBroadcaster();
                LOG_DEBUG_F("SimpleHealthSeekingBehavior is broadcasting the actual intervention event to individual %d.\n", parent->GetSuid().data );
                broadcaster->TriggerObservers( parent->GetEventContext(), actual_intervention_event );
            }
            else if( actual_intervention_config._json.Type() != ElementType::NULL_ELEMENT )
            {
                //auto indiv_id = parent->GetInterventionsContext()->GetParent()->GetSuid().data;
                LOG_INFO_F("SimpleHealthSeekingBehavior is distributing actual intervention to individual %d.\n", parent->GetSuid().data );
                auto config = Configuration::CopyFromElement( (actual_intervention_config._json), "campaign" );
                IDistributableIntervention *di = const_cast<IInterventionFactory*>(ifobj)->CreateIntervention( config );
                delete config;
                config = nullptr;
/*
                // In theory this is a good addition, but I can't get it to trigger when I add other upstream
                // error checking and handling. And if we add it here, we should add it to the other places we do CreateIntervention's.
                IDistributableIntervention *di = nullptr;
                try
                {
                    auto tmp_config = Configuration::CopyFromElement( (actual_intervention_config._json) );
                    di = const_cast<IInterventionFactory*>(ifobj)->CreateIntervention( tmp_config );
                    delete tmp_config;
                    tmp_config = nullptr;
                }
                catch( json::Exception xcept )
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
            else
            {
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, "neither event or config defined" );
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
        BaseIntervention::SetContextTo( context );

        if (s_OK != context->GetEventContext()->GetNodeEventContext()->QueryInterface(GET_IID(ICampaignCostObserver), (void**)&m_pCCO) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent->GetEventContext()->GetNodeEventContext()", "ICampaignCostObserver", "INodeEventContext");
        }
    }

    //void SimpleHealthSeekingBehavior::Expire()
    //{
    //    expired = true;
    //    // notify campaign event observer
    //    if (m_pCCO != nullptr)
    //    {
    //        m_pCCO->notifyCampaignEventOccurred( (IBaseIntervention*)this, nullptr, parent );
    //    }
    //}

    REGISTER_SERIALIZABLE(SimpleHealthSeekingBehavior);

    void SimpleHealthSeekingBehavior::serialize(IArchive& ar, SimpleHealthSeekingBehavior* obj)
    {
        BaseIntervention::serialize(ar, obj);
        SimpleHealthSeekingBehavior& intervention = *obj;
        ar.labelElement("probability_of_seeking") & intervention.probability_of_seeking;
        ar.labelElement("use_event_or_config") & (uint32_t&)intervention.use_event_or_config;
        ar.labelElement("actual_intervention_config") & intervention.actual_intervention_config;
        ar.labelElement("actual_intervention_event") & intervention.actual_intervention_event;
        ar.labelElement("single_use") & intervention.single_use;
    }
}
