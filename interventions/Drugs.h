/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include "IDrug.h"
#include "InterventionEnums.h"      // return types
#include "InterventionFactory.h"    // macros that 'auto'-register classes
#include "Configure.h"              // base classes
#include "SimulationEnums.h"        // for PkPdModel

namespace Kernel
{
    struct ICampaignCostObserver;
    struct IMalariaDrugEffectsApply;

    class GenericDrug : public IDrug, public BaseIntervention
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, GenericDrug, IDistributableIntervention)

    public:
        bool Configure( const Configuration * );
        virtual ~GenericDrug();
        virtual int AddRef();
        virtual int Release();

        // ISupports
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject);

        // IDistributableIntervention
        virtual void SetContextTo(IIndividualHumanContext *context);
        virtual void Update(float dt);

        // IDrug
        virtual void  ConfigureDrugTreatment( IIndividualHumanInterventionsContext * ivc = NULL );
        virtual int   GetDrugType() const;
        virtual std::string GetDrugName() const;
        virtual DrugUsageType::Enum GetDrugUsageType();
        virtual float GetDrugReducedAcquire()  const;
        virtual float GetDrugReducedTransmit() const;
        virtual float GetDrugCurrentConcentration() const;
        virtual float GetDrugCurrentEfficacy() const;

    protected:
        GenericDrug();

        // interfaces for different PkPd models (i.e. FIXED_DURATION_CONSTANT_EFFECT, CONCENTRATION_VERSUS_TIME)
        virtual void SimpleUpdate(float dt);
        virtual void UpdateWithPkPd(float dt);

        virtual void ResetForNextDose(float dt);
        virtual void ApplyEffects(); // virtual, not part of interface
        virtual void PkPdParameterValidation();
        virtual void Expire();

        // context for this intervention--does not need to be reset upon migration, it is just for GiveDrug()
        IIndividualHumanContext *parent;

        int   drug_type;
        DrugUsageType::Enum dosing_type;
        PKPDModel::Enum durability_time_profile;
        float fast_decay_time_constant;
        float slow_decay_time_constant;
        float dosing_timer;        //time to next dose for ongoing treatment
        int   remaining_doses;     //number of doses left in treatment
        float time_between_doses;
        float fast_component;
        float slow_component;

        float current_efficacy;
        float current_concentration;
        float current_reducedacquire;
        float current_reducedtransmit;
        float pk_rate_mod;
        float Cmax;
        float Vd;
        float drug_c50;
        float fraction_defaulters;

    private:

#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
        friend class ::boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive &ar, GenericDrug& drug, const unsigned int v);
#endif
    };
}
