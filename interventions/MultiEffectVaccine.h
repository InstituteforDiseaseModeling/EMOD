/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "Vaccine.h"

namespace Kernel
{ 
    class MultiEffectVaccine : public SimpleVaccine
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, MultiEffectVaccine, IDistributableIntervention)

    public:
        MultiEffectVaccine();
        MultiEffectVaccine( const MultiEffectVaccine& );
        virtual ~MultiEffectVaccine();
        virtual int AddRef() override { return BaseIntervention::AddRef(); }
        virtual int Release() override { return BaseIntervention::Release(); }
        virtual bool Configure( const Configuration* pConfig ) override;

        // IDistributableIntervention
        virtual void Update(float dt) override;

        // ISupports
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;


    protected:
        // context for this intervention--does not need to be reset upon migration, it is just for GiveVaccine()
        IIndividualHumanContext *parent;

        float current_reducedacquire;
        float current_reducedtransmit;
        float current_reducedmortality;
        WaningConfig   acquire_config;
        IWaningEffect* acquire_effect;
        WaningConfig   transmit_config;
        IWaningEffect* transmit_effect;
        WaningConfig   mortality_config;
        IWaningEffect* mortality_effect;

        DECLARE_SERIALIZABLE(MultiEffectVaccine);
    };
}
