/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "NodeTyphoidEventContext.h"
#include "TyphoidDefs.h"
#include "IWaningEffect.h"
#include "Interventions.h"
#include "IntranodeTransmissionTypes.h"

namespace Kernel
{
    class TyphoidWASH : public BaseNodeIntervention
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, TyphoidWASH, INodeDistributableIntervention)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_CONFIGURED(Outbreak)

    public:
        TyphoidWASH ();
        TyphoidWASH( const TyphoidWASH& );
        virtual ~TyphoidWASH();
        
        // INodeDistributableIntervention 
        virtual void SetContextTo(INodeEventContext *context) override;
        virtual bool Distribute(INodeEventContext *context, IEventCoordinator2* pEC = nullptr ) override;

        // IDistributableIntervention
        virtual void Update(float dt) override;

        // ISupports
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;

        TyphoidVaccineMode::Enum vaccine_mode;
        float effect;
        INodeTyphoidInterventionEffectsApply * itvc; // interventions container
        INodeEventContext *parent;
        IWaningEffect* changing_effect;

    protected:
        void getUpdatePointer();
        std::string targeted_individual_properties;
        bool use_property_targeting;
        // context for this intervention--does not need to be reset upon migration, it is just for GiveVaccine()

        //DECLARE_SERIALIZABLE(TyphoidWASH );
    };
}
