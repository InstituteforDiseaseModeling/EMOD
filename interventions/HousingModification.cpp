/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "HousingModification.h"

#include <typeinfo>

#include "Contexts.h"                      // for IIndividualHumanContext, IIndividualHumanInterventionsContext
#include "InterventionEnums.h"
#include "InterventionFactory.h"
#include "VectorInterventionsContainer.h"  // for IHousingModificationConsumer methods
#include "Log.h"

static const char* _module = "SimpleHousingModification";

namespace Kernel
{
    IMPLEMENT_FACTORY_REGISTERED(SimpleHousingModification)
    IMPLEMENT_FACTORY_REGISTERED(IRSHousingModification)
    IMPLEMENT_FACTORY_REGISTERED(ScreeningHousingModification)
    IMPLEMENT_FACTORY_REGISTERED(SpatialRepellentHousingModification)
    IMPLEMENT_FACTORY_REGISTERED(ArtificialDietHousingModification)
    IMPLEMENT_FACTORY_REGISTERED(InsectKillingFenceHousingModification)

    REGISTER_SERIALIZABLE(SimpleHousingModification);
    REGISTER_SERIALIZABLE(IRSHousingModification);
    REGISTER_SERIALIZABLE(ScreeningHousingModification);
    REGISTER_SERIALIZABLE(SpatialRepellentHousingModification);
    REGISTER_SERIALIZABLE(ArtificialDietHousingModification);
    REGISTER_SERIALIZABLE(InsectKillingFenceHousingModification);

    void SimpleHousingModification::serialize(IArchive& ar, SimpleHousingModification* obj)
    {
        SimpleHousingModification& mod = *obj;
        ar.labelElement("blocking_effect") & mod.blocking_effect;
        ar.labelElement("killing_effect") & mod.killing_effect;
    }

    void IRSHousingModification::serialize(IArchive& ar, IRSHousingModification* obj)
    {
        SimpleHousingModification::serialize(ar, obj);
    }

    void ScreeningHousingModification::serialize(IArchive& ar, ScreeningHousingModification* obj)
    {
        SimpleHousingModification::serialize(ar, obj);
    }

    void SpatialRepellentHousingModification::serialize(IArchive& ar, SpatialRepellentHousingModification* obj)
    {
        SimpleHousingModification::serialize(ar, obj);
    }

    void ArtificialDietHousingModification::serialize(IArchive& ar, ArtificialDietHousingModification* obj)
    {
        SimpleHousingModification::serialize(ar, obj);
    }

    void InsectKillingFenceHousingModification::serialize(IArchive& ar, InsectKillingFenceHousingModification* obj)
    {
        SimpleHousingModification::serialize(ar, obj);
    }

    bool
    SimpleHousingModification::Configure(
        const Configuration * inputJson
    )
    {
        initConfigComplexType("Killing_Config", &killing_config, HM_Killing_Config_DESC_TEXT );
        initConfigComplexType("Blocking_Config", &blocking_config, HM_Blocking_Config_DESC_TEXT );
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
        return JsonConfigurable::Configure( inputJson );
    }

    SimpleHousingModification::SimpleHousingModification()
        : killing_effect( nullptr )
        , blocking_effect( nullptr )
    {
        initSimTypes( 2, "VECTOR_SIM", "MALARIA_SIM" );
        initConfigTypeMap("Cost_To_Consumer", &cost_per_unit, HM_Cost_To_Consumer_DESC_TEXT, 0, 999999, 8.0);
    }

    SimpleHousingModification::~SimpleHousingModification()
    {
        delete killing_effect;
        delete blocking_effect;
    }

    SimpleHousingModification::SimpleHousingModification( const SimpleHousingModification& master )
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

    bool
    SimpleHousingModification::Distribute(
        IIndividualHumanInterventionsContext *context,
        ICampaignCostObserver * const pCCO
    )
    {
        if (s_OK != context->QueryInterface(GET_IID(IHousingModificationConsumer), (void**)&ihmc) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "IHousingModificationConsumer", "IIndividualHumanInterventionsContext" );
        }
        context->PurgeExisting( typeid(*this).name() );
        return BaseIntervention::Distribute( context, pCCO );
    }

    void SimpleHousingModification::SetContextTo(
        IIndividualHumanContext *context
    )
    {
        LOG_DEBUG("SimpleHousingModification::SetContextTo (probably deserializing)\n");
        if (s_OK != context->GetInterventionsContext()->QueryInterface(GET_IID(IHousingModificationConsumer), (void**)&ihmc) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "IHousingModificationConsumer", "IIndividualHumanContext" );
        }

    }

    void SimpleHousingModification::Update( float dt )
    {
        killing_effect->Update(dt);
        blocking_effect->Update(dt);
        float current_killingrate = killing_effect->Current();
        float current_blockingrate = blocking_effect->Current();

        if( ihmc )
        {
            ihmc->ApplyHouseBlockingProbability( current_blockingrate );
            ihmc->UpdateProbabilityOfScreenKilling( current_killingrate );
        }
        else
        {
            throw NullPointerException( __FILE__, __LINE__, __FUNCTION__, "ihmc" );
        }
    }

    BEGIN_QUERY_INTERFACE_BODY(SimpleHousingModification)
        HANDLE_INTERFACE(IConfigurable)
        HANDLE_INTERFACE(IDistributableIntervention)
        HANDLE_ISUPPORTS_VIA(IDistributableIntervention)
    END_QUERY_INTERFACE_BODY(SimpleHousingModification)
/*
    Kernel::QueryResult SimpleHousingModification::QueryInterface( iid_t iid, void **ppinstance )
    {
        assert(ppinstance);

        if ( !ppinstance )
            return e_NULL_POINTER;

        ISupports* foundInterface;

        if ( iid == GET_IID(IHousingModification))
            foundInterface = static_cast<IHousingModification*>(this);
        // -->> add support for other I*Consumer interfaces here <<--

        else if ( iid == GET_IID(ISupports))
            foundInterface = static_cast<ISupports*>(static_cast<IHousingModification*>(this));
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
}

