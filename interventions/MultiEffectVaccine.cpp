/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "MultiEffectVaccine.h"
#include "InterventionsContainer.h"  // for IVaccineConsumer methods

static const char* _module = "MultiEffectVaccine";

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(MultiEffectVaccine)
        HANDLE_INTERFACE(IConfigurable)
        HANDLE_INTERFACE(IDistributableIntervention)
        HANDLE_INTERFACE(IVaccine)
        HANDLE_INTERFACE(IBaseIntervention)
        HANDLE_ISUPPORTS_VIA(IDistributableIntervention)
        HANDLE_INTERFACE(IBaseIntervention)
    END_QUERY_INTERFACE_BODY(MultiEffectVaccine)

    IMPLEMENT_FACTORY_REGISTERED(MultiEffectVaccine)

    bool
    MultiEffectVaccine::Configure(
        const Configuration * inputJson
    )
    {
        initConfigTypeMap("Vaccine_Take", &vaccine_take, SV_Vaccine_Take_DESC_TEXT, 0.0, 1.0, 1.0 ); 
        initConfigComplexType("Acquire_Config",  &acquire_config, MEV_Acquire_Config_DESC_TEXT );
        initConfigComplexType("Transmit_Config",  &transmit_config, MEV_Transmit_Config_DESC_TEXT );
        initConfigComplexType("Mortality_Config", &mortality_config, MEV_Mortality_Config_DESC_TEXT  );

        bool configured = BaseIntervention::Configure( inputJson );
        if( !JsonConfigurable::_dryrun )
        {
            auto tmp_acquire   = Configuration::CopyFromElement( acquire_config._json   );
            auto tmp_transmit  = Configuration::CopyFromElement( transmit_config._json  );
            auto tmp_mortality = Configuration::CopyFromElement( mortality_config._json );

            acquire_effect   = WaningEffectFactory::CreateInstance( tmp_acquire   );
            transmit_effect  = WaningEffectFactory::CreateInstance( tmp_transmit  );
            mortality_effect = WaningEffectFactory::CreateInstance( tmp_mortality );

            delete tmp_acquire;
            delete tmp_transmit;
            delete tmp_mortality;
            tmp_acquire   = nullptr;
            tmp_transmit  = nullptr;
            tmp_mortality = nullptr;
        }
        LOG_DEBUG_F( "Vaccine configured with type %d and take %f.\n", vaccine_type, vaccine_take );
        return configured;
    }

    MultiEffectVaccine::MultiEffectVaccine() 
    : acquire_effect( nullptr )
    , transmit_effect( nullptr )
    , mortality_effect( nullptr )
    {
    }

    MultiEffectVaccine::MultiEffectVaccine( const MultiEffectVaccine& master )
    {
        vaccine_take = master.vaccine_take;
        cost_per_unit = master.cost_per_unit;
        acquire_config = master.acquire_config;
        transmit_config = master.transmit_config;
        mortality_config = master.mortality_config;

        auto tmp_acquire   = Configuration::CopyFromElement( acquire_config._json   );
        auto tmp_transmit  = Configuration::CopyFromElement( transmit_config._json  );
        auto tmp_mortality = Configuration::CopyFromElement( mortality_config._json );

        acquire_effect   = WaningEffectFactory::CreateInstance( tmp_acquire   );
        transmit_effect  = WaningEffectFactory::CreateInstance( tmp_transmit  );
        mortality_effect = WaningEffectFactory::CreateInstance( tmp_mortality );

        delete tmp_acquire;
        delete tmp_transmit;
        delete tmp_mortality;
        tmp_acquire   = nullptr;
        tmp_transmit  = nullptr;
        tmp_mortality = nullptr;
    }

    MultiEffectVaccine::~MultiEffectVaccine()
    {
        delete acquire_effect;
        delete transmit_effect;
        delete mortality_effect;
    }

    void MultiEffectVaccine::Update( float dt )
    {
        release_assert(ivc);

        acquire_effect->Update(dt);
        current_reducedacquire = acquire_effect->Current();
        ivc->UpdateVaccineAcquireRate( current_reducedacquire );

        transmit_effect->Update(dt);
        current_reducedtransmit  = transmit_effect->Current();
        ivc->UpdateVaccineTransmitRate( current_reducedtransmit );

        mortality_effect->Update(dt);
        current_reducedmortality  = mortality_effect->Current(); 
        ivc->UpdateVaccineMortalityRate( current_reducedmortality );
    }


    REGISTER_SERIALIZABLE(MultiEffectVaccine);

    void MultiEffectVaccine::serialize(IArchive& ar, MultiEffectVaccine* obj)
    {
        SimpleVaccine::serialize( ar, obj );
        MultiEffectVaccine& vaccine = *obj;
        ar.labelElement("acquire_effect")                 & vaccine.acquire_effect;
        ar.labelElement("transmit_effect")                 & vaccine.transmit_effect;
        ar.labelElement("mortality_effect")                 & vaccine.mortality_effect;
    }
}
