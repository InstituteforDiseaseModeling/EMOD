/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once


#ifdef ENABLE_TB
#include "Interventions.h"
#include "InterventionEnums.h"
#include "InterventionsContainer.h"
#include "SimpleTypemapRegistration.h"

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

#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
    // Serialization
    friend class ::boost::serialization::access;
    template<class Archive>
    friend void serialize(Archive &ar, TBDrugEffects_t& drugeffects, const unsigned int v);
#endif // BOOST

    };

    typedef std::map <TBDrugType::Enum, TBDrugEffects_t> TBDrugEffectsMap_t;

    struct ITBDrugEffects : public ISupports
    {
        virtual TBDrugEffectsMap_t GetDrugEffectsMap() = 0;
        virtual TBDrugTypeParameters::tTBDTPMap& GetTBdtParams() = 0;
        virtual ~ITBDrugEffects() { }
    };

    struct ITBDrugEffectsApply : public ISupports
    {
        virtual void ApplyDrugVaccineReducedAcquireEffect( float rate ) = 0;
        virtual void ApplyDrugVaccineReducedTransmitEffect( float rate ) = 0;
        virtual void ApplyTBDrugEffects( TBDrugEffects_t effects, TBDrugType::Enum drug_type ) = 0;
        virtual void UpdateTreatmentStatus( IndividualEventTriggerType::Enum new_treatment_status ) = 0;
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

    struct IDrug;
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
        virtual QueryResult QueryInterface(iid_t iid, void** pinstance);
        virtual bool GiveIntervention( IDistributableIntervention * pIV );

        // ITBDrugEffectsApply
        virtual void ApplyDrugVaccineReducedAcquireEffect( float rate );
        virtual void ApplyDrugVaccineReducedTransmitEffect( float rate );
        virtual void ApplyTBDrugEffects( TBDrugEffects_t effects, TBDrugType::Enum drug_type );
        virtual void UpdateTreatmentStatus( IndividualEventTriggerType::Enum new_treatment_status );
        
        virtual void Update(float dt); // hook to update interventions if they need it

        virtual TBDrugTypeParameters::tTBDTPMap& GetTBdtParams();

        //functions in the ITBInterventionsContainer
        virtual int GetNumTBDrugsActive(); //this function needs to be non-const so it can call GetInterventionsByType
        virtual bool GetTxNaiveStatus() const;
        virtual bool GetTxFailedStatus() const;
        virtual bool GetTxEverRelapsedStatus() const; 

        //functions in IHealthSeekingBehaviorUpdateEffectsApply
        virtual void UpdateHealthSeekingBehaviors(float new_probability_of_seeking);

    protected:
        virtual TBDrugEffectsMap_t GetDrugEffectsMap();

        //virtual void PropagateContextToDependents(); // pass context to interventions if they need it
        void GiveDrug(IDrug* drug);

        TBDrugEffectsMap_t TB_drug_effects;

        bool m_is_tb_tx_naive_TBIVC;
        bool m_failed_tx_TBIVC;
        bool m_ever_relapsed_TBIVC;

    private:
#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
        friend class ::boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive &ar, TBInterventionsContainer& container, const unsigned int v);
#endif
    };
}
#endif
