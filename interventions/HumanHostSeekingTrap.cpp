/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "HumanHostSeekingTrap.h"

#include <typeinfo>

#include "Contexts.h"                      // for IIndividualHumanContext, IIndividualHumanInterventionsContext
#include "InterventionEnums.h"
#include "InterventionFactory.h"
#include "VectorInterventionsContainer.h"  // for IVectorInterventionEffectsSetter methods

static const char* _module = "HumanHostSeekingTrap";

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(HumanHostSeekingTrap)
        HANDLE_INTERFACE(IConfigurable)
        HANDLE_INTERFACE(IDistributableIntervention)
        HANDLE_ISUPPORTS_VIA(IDistributableIntervention)
    END_QUERY_INTERFACE_BODY(HumanHostSeekingTrap)

    IMPLEMENT_FACTORY_REGISTERED(HumanHostSeekingTrap)

    HumanHostSeekingTrap::HumanHostSeekingTrap()
    {
        initSimTypes( 2, "VECTOR_SIM", "MALARIA_SIM" );
        initConfigTypeMap( "Cost_To_Consumer", &cost_per_unit, HST_Cost_To_Consumer_DESC_TEXT, 0, 999999, 3.75 );
        
    }

    HumanHostSeekingTrap::~HumanHostSeekingTrap()
    {
        delete killing_effect;
        delete attract_effect;
    }

    HumanHostSeekingTrap::HumanHostSeekingTrap( const HumanHostSeekingTrap& master )
    : BaseIntervention( master )
    {
        killing_config = master.killing_config;
        attract_config = master.attract_config;

        auto tmp_killing = Configuration::CopyFromElement( killing_config._json );
        auto tmp_attract = Configuration::CopyFromElement( attract_config._json );

        killing_effect = WaningEffectFactory::CreateInstance( tmp_killing );
        attract_effect = WaningEffectFactory::CreateInstance( tmp_attract );

        delete tmp_killing;
        delete tmp_attract;
        tmp_killing = nullptr;
        tmp_attract = nullptr;
    }

    bool
    HumanHostSeekingTrap::Configure(
        const Configuration * inputJson
    )
    {
        initConfigComplexType("Killing_Config",  &killing_config, HST_Killing_Config_DESC_TEXT );
        initConfigComplexType("Attract_Config",  &attract_config, HST_Attract_Config_DESC_TEXT );
        bool configured = JsonConfigurable::Configure( inputJson );
        if( !JsonConfigurable::_dryrun )
        {
            auto tmp_killing = Configuration::CopyFromElement( killing_config._json );
            auto tmp_attract = Configuration::CopyFromElement( attract_config._json );

            killing_effect = WaningEffectFactory::CreateInstance( tmp_killing );
            attract_effect = WaningEffectFactory::CreateInstance( tmp_attract );

            delete tmp_killing;
            delete tmp_attract;
            tmp_killing = nullptr;
            tmp_attract = nullptr;
        }
        return configured;
    }

    bool
    HumanHostSeekingTrap::Distribute(
        IIndividualHumanInterventionsContext *context,
        ICampaignCostObserver * const pCCO
    )
    {
        if (s_OK != context->QueryInterface(GET_IID(IVectorInterventionEffectsSetter), (void**)&ivies) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "IVectorInterventionEffectsSetter", "IIndividualHumanInterventionsContext" );
        }
        context->PurgeExisting( typeid(*this).name() );
        return BaseIntervention::Distribute( context, pCCO );
    }

    void HumanHostSeekingTrap::Update( float dt )
    {
        killing_effect->Update(dt);
        attract_effect->Update(dt);
        current_killingrate = killing_effect->Current();
        current_attractrate = attract_effect->Current();

        // Effects of human host-seeking trap are updated with indoor-home artificial-diet interfaces in VectorInterventionsContainer::Update.
        // Attraction rate diverts indoor feeding attempts from humans to trap; killing rate kills a fraction of diverted feeding attempts.
        ivies->UpdateArtificialDietAttractionRate( current_attractrate );
        ivies->UpdateArtificialDietKillingRate( current_killingrate );
    }

    void HumanHostSeekingTrap::SetContextTo(
        IIndividualHumanContext *context
    )
    {
        if (s_OK != context->GetInterventionsContext()->QueryInterface(GET_IID(IVectorInterventionEffectsSetter), (void**)&ivies) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "IVectorInterventionEffectsSetter", "IIndividualHumanContext" );
        }
    }

    REGISTER_SERIALIZABLE(HumanHostSeekingTrap);

    void HumanHostSeekingTrap::serialize(IArchive& ar, HumanHostSeekingTrap* obj)
    {
        BaseIntervention::serialize( ar, obj );
        HumanHostSeekingTrap& trap = *obj;
        ar.labelElement("current_attractrate") & trap.current_attractrate;
        ar.labelElement("current_killingrate") & trap.current_killingrate;
        ar.labelElement("attract_effect") & trap.attract_effect;
        ar.labelElement("killing_effect") & trap.killing_effect;
    }
}

