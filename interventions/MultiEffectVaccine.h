/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

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
        virtual void SetContextTo( IIndividualHumanContext *context ) override;
        virtual bool NeedsInfectiousLoopUpdate() const;

        // ISupports
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;


    protected:
        // context for this intervention--does not need to be reset upon migration, it is just for GiveVaccine()
        IIndividualHumanContext *parent;

        IWaningEffect* acquire_effect;
        IWaningEffect* transmit_effect;
        IWaningEffect* mortality_effect;

        DECLARE_SERIALIZABLE(MultiEffectVaccine);
    };
}
