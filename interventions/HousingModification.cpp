/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

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

SETUP_LOGGING( "SimpleHousingModification" )

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
        WaningConfig   killing_config;
        WaningConfig   blocking_config;

        initConfigComplexType("Killing_Config", &killing_config, HM_Killing_Config_DESC_TEXT );
        initConfigComplexType("Blocking_Config", &blocking_config, HM_Blocking_Config_DESC_TEXT );
        bool configured = BaseIntervention::Configure( inputJson );
        if( !JsonConfigurable::_dryrun && configured )
        {
            killing_effect  = WaningEffectFactory::CreateInstance( killing_config  );
            blocking_effect = WaningEffectFactory::CreateInstance( blocking_config );
        }
        return configured;
    }

    SimpleHousingModification::SimpleHousingModification()
    : BaseIntervention()
    , killing_effect(nullptr)
    , blocking_effect(nullptr)
    , ihmc(nullptr)
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
    , killing_effect( nullptr )
    , blocking_effect( nullptr )
    , ihmc( nullptr )
    {
        if( master.killing_effect != nullptr )
        {
            killing_effect = master.killing_effect->Clone();
        }
        if( master.blocking_effect != nullptr )
        {
            blocking_effect = master.blocking_effect->Clone();
        }
    }

    bool
    SimpleHousingModification::Distribute(
        IIndividualHumanInterventionsContext *context,
        ICampaignCostObserver * const pCCO
    )
    {
        if( AbortDueToDisqualifyingInterventionStatus( context->GetParent() ) )
        {
            return false;
        }

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
        BaseIntervention::SetContextTo( context );
        if( killing_effect != nullptr )
        {
            killing_effect->SetContextTo( context );
        }
        if( blocking_effect != nullptr )
        {
            blocking_effect->SetContextTo( context );
        }

        LOG_DEBUG("SimpleHousingModification::SetContextTo (probably deserializing)\n");
        if (s_OK != context->GetInterventionsContext()->QueryInterface(GET_IID(IHousingModificationConsumer), (void**)&ihmc) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "IHousingModificationConsumer", "IIndividualHumanContext" );
        }
    }

    void SimpleHousingModification::Update( float dt )
    {
        if( !BaseIntervention::UpdateIndividualsInterventionStatus() ) return;

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
}

