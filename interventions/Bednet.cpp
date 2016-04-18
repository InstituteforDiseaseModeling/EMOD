/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "Bednet.h"

#include <typeinfo>

#include "Contexts.h"                      // for IIndividualHumanContext, IIndividualHumanInterventionsContext
#include "InterventionFactory.h"
#include "VectorInterventionsContainer.h"  // for IBednetConsumer methods
#include "Log.h"
#include "IndividualEventContext.h"
#include "NodeEventContext.h"

static const char* _module = "SimpleBednet";

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(SimpleBednet)
        HANDLE_INTERFACE(IConfigurable)
        HANDLE_INTERFACE(IDistributableIntervention)
        HANDLE_ISUPPORTS_VIA(IDistributableIntervention)
    END_QUERY_INTERFACE_BODY(SimpleBednet)

    IMPLEMENT_FACTORY_REGISTERED(SimpleBednet)
    
    SimpleBednet::SimpleBednet( const SimpleBednet& master )
    : BaseIntervention( master )
    {
        killing_config  = master.killing_config;
        blocking_config = master.blocking_config;

        auto tmp_killing  = Configuration::CopyFromElement( killing_config._json  );
        auto tmp_blocking = Configuration::CopyFromElement( blocking_config._json );

        killing_effect  = WaningEffectFactory::CreateInstance( tmp_killing  );
        blocking_effect = WaningEffectFactory::CreateInstance( tmp_blocking );

        delete tmp_killing;
        delete tmp_blocking;
        tmp_killing  = nullptr;
        tmp_blocking = nullptr;
    }

    SimpleBednet::SimpleBednet()
    : killing_effect( nullptr )
    , blocking_effect( nullptr )
    {
        initSimTypes( 2, "MALARIA_SIM", "VECTOR_SIM" );
        initConfigTypeMap( "Cost_To_Consumer", &cost_per_unit, SB_Cost_To_Consumer_DESC_TEXT, 0, 999999, 3.75 );
    }

    SimpleBednet::~SimpleBednet()
    {
        delete killing_effect;
        delete blocking_effect;
    }

    bool
    SimpleBednet::Configure(
        const Configuration * inputJson
    )
    {
        initConfig( "Bednet_Type", bednet_type, inputJson, MetadataDescriptor::Enum("Bednet_Type", SB_Bednet_Type_DESC_TEXT, MDD_ENUM_ARGS(BednetType)) );
        initConfigComplexType("Killing_Config",  &killing_config, SB_Killing_Config_DESC_TEXT );
        initConfigComplexType("Blocking_Config",  &blocking_config, SB_Blocking_Config_DESC_TEXT );
        bool configured = JsonConfigurable::Configure( inputJson );
        if( !JsonConfigurable::_dryrun )
        {
            auto tmp_killing  = Configuration::CopyFromElement( killing_config._json  );
            auto tmp_blocking = Configuration::CopyFromElement( blocking_config._json );

            killing_effect  = WaningEffectFactory::CreateInstance( tmp_killing  );
            blocking_effect = WaningEffectFactory::CreateInstance( tmp_blocking );

            delete tmp_killing;
            delete tmp_blocking;
            tmp_killing  = nullptr;
            tmp_blocking = nullptr;
        }
        return configured;
    }

    bool
    SimpleBednet::Distribute(
        IIndividualHumanInterventionsContext *context,
        ICampaignCostObserver * const pCCO
    )
    {
        if (s_OK != context->QueryInterface(GET_IID(IBednetConsumer), (void**)&ibc) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "IBednetConsumer", "IIndividualHumanInterventionsContext" );
        }
        context->PurgeExisting( typeid(*this).name() );
        bool ret = BaseIntervention::Distribute( context, pCCO );
        if( ret && !on_distributed_event.IsUninitialized() && (on_distributed_event != NO_TRIGGER_STR) )
        {
            INodeTriggeredInterventionConsumer* broadcaster = nullptr;
            if (s_OK != context->GetParent()->GetEventContext()->GetNodeEventContext()->QueryInterface(GET_IID(INodeTriggeredInterventionConsumer), (void**)&broadcaster))
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, 
                                               "parent->GetEventContext()->GetNodeEventContext()", 
                                               "INodeTriggeredInterventionConsumer", 
                                               "INodeEventContext" );
            }
            broadcaster->TriggerNodeEventObserversByString( context->GetParent()->GetEventContext(), on_distributed_event );
        }
        return ret ;
    }

    void SimpleBednet::Update( float dt )
    {
        killing_effect->Update(dt);
        blocking_effect->Update(dt);
        float current_killingrate = killing_effect->Current();
        float current_blockingrate = blocking_effect->Current();
        LOG_DEBUG_F( "current_killingrate = %f\n", current_killingrate );
        LOG_DEBUG_F( "current_blockingrate = %f\n", current_blockingrate );
        ibc->UpdateProbabilityOfKilling( current_killingrate );
        ibc->UpdateProbabilityOfBlocking( current_blockingrate );
    }

    void SimpleBednet::SetContextTo(
        IIndividualHumanContext *context
    )
    {
        if (s_OK != context->GetInterventionsContext()->QueryInterface(GET_IID(IBednetConsumer), (void**)&ibc) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "IBednetConsumer", "IIndividualHumanContext" );
        }
    }

/*
    Kernel::QueryResult SimpleBednet::QueryInterface( iid_t iid, void **ppinstance )
    {
        assert(ppinstance);

        if ( !ppinstance )
            return e_NULL_POINTER;

        ISupports* foundInterface;

        if ( iid == GET_IID(IBednet))
            foundInterface = static_cast<IBednet*>(this);
        // -->> add support for other I*Consumer interfaces here <<--
        else if ( iid == GET_IID(ISupports))
            foundInterface = static_cast<ISupports*>(static_cast<IBednet*>(this));
        else
            foundInterface = 0;

        QueryResult status;
        if ( !foundInterface )
            status = e_NOINTERFACE;
        else
        {
            //foundInterface->AddRef();           // not implementing this yet!
            status = s_OK;
        }

        *ppinstance = foundInterface;
        return status;

    }*/

    REGISTER_SERIALIZABLE(SimpleBednet);

    void SimpleBednet::serialize(IArchive& ar, SimpleBednet* obj)
    {
        SimpleBednet& bednet = *obj;
        ar.labelElement("blocking_effect") & bednet.blocking_effect;
        ar.labelElement("killing_effect") & bednet.killing_effect;
        ar.labelElement("bednet_type") & (uint32_t&)bednet.bednet_type;
    }
}
