/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "Interventions.h"
#include "InterventionsContainer.h"
#include "TBDrugTypeParameters.h"

namespace Kernel
{
    // this container becomes a help implementation member of the IndividualHumanTB class 
    // it needs to implement consumer interfaces for all the relevant intervention types

    struct TBDrugEffects_t
    {
        float clearance_rate;
        float inactivation_rate;
        float resistance_rate;
        float relapse_rate;
        float mortality_rate;

        static void serialize(IArchive&, TBDrugEffects_t&);
    };

    typedef std::map <TBDrugType::Enum, TBDrugEffects_t> TBDrugEffectsMap_t;

    struct ITBDrugEffects : public ISupports
    {
        virtual TBDrugEffectsMap_t GetDrugEffectsMap() = 0;
        virtual ~ITBDrugEffects() { }
    };

    struct ITBDrugEffectsApply : public ISupports
    {
        virtual void ApplyDrugVaccineReducedAcquireEffect( float rate ) = 0;
        virtual void ApplyDrugVaccineReducedTransmitEffect( float rate ) = 0;
        virtual void ApplyTBDrugEffects( TBDrugEffects_t effects, TBDrugType::Enum drug_type ) = 0;
        virtual void UpdateTreatmentStatus( const EventTrigger& new_treatment_status ) = 0;
        virtual ~ITBDrugEffectsApply() { }
    };

    struct ITBInterventionsContainer : public ISupports
    {
        virtual int GetNumTBDrugsActive() = 0;
        virtual bool GetTxNaiveStatus() const = 0;
        virtual bool GetTxFailedStatus() const = 0;
        virtual bool GetTxEverRelapsedStatus() const = 0;
    };

    struct IHealthSeekingBehaviorUpdateEffectsApply : public ISupports
    {
        virtual void UpdateHealthSeekingBehaviors( float new_probability_of_seeking) = 0; //this function is called by HSBUpdate to the InterventionsContainer
    };

    class TBInterventionsContainer : public InterventionsContainer,
        public ITBDrugEffects,
        public ITBDrugEffectsApply,
        public ITBInterventionsContainer,
        public IHealthSeekingBehaviorUpdateEffectsApply

    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()

    public:
        TBInterventionsContainer();
        virtual ~TBInterventionsContainer();

        // ISupports
        virtual QueryResult QueryInterface(iid_t iid, void** pinstance) override;

        // ITBDrugEffectsApply
        virtual void ApplyDrugVaccineReducedAcquireEffect( float rate ) override;
        virtual void ApplyDrugVaccineReducedTransmitEffect( float rate ) override;
        virtual void ApplyTBDrugEffects( TBDrugEffects_t effects, TBDrugType::Enum drug_type ) override;
        virtual void UpdateTreatmentStatus( const EventTrigger& new_treatment_status ) override;
        
        //functions in the ITBInterventionsContainer
        virtual int GetNumTBDrugsActive() override; //this function needs to be non-const so it can call GetInterventionsByInterface
        virtual bool GetTxNaiveStatus() const override;
        virtual bool GetTxFailedStatus() const override;
        virtual bool GetTxEverRelapsedStatus() const override;

        //functions in IHealthSeekingBehaviorUpdateEffectsApply
        virtual void UpdateHealthSeekingBehaviors(float new_probability_of_seeking) override;

        virtual void InfectiousLoopUpdate( float dt ) override;

    protected:
        virtual TBDrugEffectsMap_t GetDrugEffectsMap() override;

        //virtual void PropagateContextToDependents(); // pass context to interventions if they need it

        TBDrugEffectsMap_t TB_drug_effects;

        bool m_is_tb_tx_naive_TBIVC;
        bool m_failed_tx_TBIVC;
        bool m_ever_relapsed_TBIVC;

        DECLARE_SERIALIZABLE(TBInterventionsContainer);
    };
}

