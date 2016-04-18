/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#ifdef ENABLE_TB

#include "TBInterventionsContainer.h"
#include "TBContexts.h"

#include "Drugs.h" // for IDrug interface
#include "InterventionFactory.h"
#include "Log.h"
#include "Sugar.h"
#include "IHealthSeekingBehavior.h" //for IHealthSeekingBehavior interface
#include "NodeEventContext.h"    // for INodeEventContext (ICampaignCostObserver)

static const char * _module = "TBInterventionsContainer";

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED(TBInterventionsContainer, InterventionsContainer)
        HANDLE_INTERFACE(ITBDrugEffectsApply)
        HANDLE_INTERFACE(ITBDrugEffects)
        HANDLE_INTERFACE(ITBInterventionsContainer)
        HANDLE_INTERFACE(IHealthSeekingBehaviorUpdateEffectsApply)
    END_QUERY_INTERFACE_DERIVED(TBInterventionsContainer, InterventionsContainer)

    TBInterventionsContainer::TBInterventionsContainer() :
        InterventionsContainer()
        , m_is_tb_tx_naive_TBIVC(true)
        , m_failed_tx_TBIVC(false)
        , m_ever_relapsed_TBIVC(false)
    {
    }

    TBInterventionsContainer::~TBInterventionsContainer()
    {
    }
    
    void TBInterventionsContainer::Update(float dt)
    {
        TB_drug_effects.clear();
        InterventionsContainer::Update(dt);
    }

    void TBInterventionsContainer::UpdateHealthSeekingBehaviors(float new_probability_of_seeking)
    {
        IHealthSeekingBehavior * IHSB = nullptr;

        //this section for counting the number of HSB interventions in the interventionslist
        //in future clean up so that this function doesn't have hard coded intervention class names
        std::string classname = "Kernel::HealthSeekingBehaviorUpdateable";
#ifdef WIN32
        classname = "class " + classname;
#endif
        std::list<IDistributableIntervention*> list_of_HSB = GetInterventionsByType(classname);
        LOG_DEBUG_F("Number of HSBUpdateable in intervention list %d\n", list_of_HSB.size()); 
        
        if (list_of_HSB.size() == 0)
        {
            LOG_DEBUG("no HSBUpdateable to update \n");
        }
        else
        {
            //this section for counting how updatomg each of the non-expired HSB with the new_probability_of_seeking
            for (auto active_HSB : list_of_HSB)
            {
                if (s_OK == active_HSB->QueryInterface(GET_IID(IHealthSeekingBehavior), (void **)&IHSB))
                {
                    IHSB->UpdateProbabilityofSeeking( new_probability_of_seeking ); 
                }
                else
                {
                    throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "active_HSB", "IHealthSeekingBehavior", "IIndividualHumanContext" );
                }
            }
        }
    }


    int TBInterventionsContainer::GetNumTBDrugsActive()
    {
        IDrug * ITB_drug = nullptr;

        //this section for counting the number of AntiTBDrug interventions in the interventionslist
        std::list<IDistributableIntervention*> list_of_tb_drugs = GetInterventionsByType("class Kernel::AntiTBDrug");
        LOG_DEBUG_F("Number of AntiTBDrug in intervention list %d\n",  list_of_tb_drugs.size()); 
        
        //this section for counting how many of the AntiTBDrug interventions have efficacy > 0 (not expired)
        int num_tb_drugs_active = 0;
        for (auto tb_drug_on_board : list_of_tb_drugs)
        {
            if (s_OK == tb_drug_on_board->QueryInterface(GET_IID(IDrug), (void **)&ITB_drug))
            {
                if (ITB_drug->GetDrugCurrentEfficacy() > 0)
                {
                    num_tb_drugs_active +=1;
                }
            }
        }
        LOG_DEBUG_F("Number of AntiTBDrug in intervention list that are active %d\n", num_tb_drugs_active);

        //this section for counting the number of AntiTBPropDepDrug interventions in the interventionslist
        std::list<IDistributableIntervention*> list_of_propdep_tb_drugs = GetInterventionsByType("class Kernel::AntiTBPropDepDrug");
        LOG_DEBUG_F("Number of AntiTBPropDepDrug in intervention list %d\n", list_of_propdep_tb_drugs.size()); 

        //this section for counting how many of the AntiTBPropDepDrug interventions have efficacy > 0 (not expired)
        for (auto tb_drug_on_board : list_of_propdep_tb_drugs)
        {
            if (s_OK == tb_drug_on_board->QueryInterface(GET_IID(IDrug), (void **)&ITB_drug))
            {
                if (ITB_drug->GetDrugCurrentEfficacy() > 0)
                {
                    num_tb_drugs_active +=1;
                }
            }
        }
        LOG_DEBUG_F("Number of AntiTBDrug and AntiTBPropDepDrug in intervention list that are active %d\n", num_tb_drugs_active);
        
        return num_tb_drugs_active;
    }
   
    void TBInterventionsContainer::ApplyDrugVaccineReducedAcquireEffect(
        float prob
    )
    {
        drugVaccineReducedAcquire  *= (1.0f-prob);
    }

    void TBInterventionsContainer::ApplyDrugVaccineReducedTransmitEffect(
        float prob
    )
    {
        drugVaccineReducedTransmit *= (1.0f-prob);
    }

    void TBInterventionsContainer::ApplyTBDrugEffects(
        TBDrugEffects_t effects, TBDrugType::Enum drug_type
    )
    {
        TB_drug_effects[drug_type].clearance_rate    += effects.clearance_rate;
        TB_drug_effects[drug_type].inactivation_rate += effects.inactivation_rate;
        TB_drug_effects[drug_type].resistance_rate += effects.resistance_rate;
        TB_drug_effects[drug_type].relapse_rate += effects.relapse_rate;
        TB_drug_effects[drug_type].mortality_rate += effects.mortality_rate;

    }

    void TBInterventionsContainer::UpdateTreatmentStatus(IndividualEventTriggerType::Enum new_treatment_status ) 
    {
        //this function is called when the drug is started and stopped/expires
        
        //first get the pointer to the person, parent is the generic individual
        IIndividualHumanTB2* tb_patient = nullptr;
        if ( parent->QueryInterface( GET_IID(IIndividualHumanTB2), (void**) &tb_patient ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "individual", "IIndvidualHumanTB2", "IndividualHuman" );
        }
        LOG_DEBUG_F( "Individual %d disease state is active %d, latent %d, pending relapse %d \n", parent->GetSuid().data,tb_patient->HasActiveInfection(), tb_patient->HasLatentInfection(), tb_patient->HasPendingRelapseInfection() );
        
        INodeTriggeredInterventionConsumer* broadcaster = nullptr;
        if (s_OK != parent->GetEventContext()->GetNodeEventContext()->QueryInterface(GET_IID(INodeTriggeredInterventionConsumer), (void**)&broadcaster))
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent->GetEventContext()->GetNodeEventContext()", "INodeTriggeredInterventionConsumer", "INodeEventContext" );
        }

        switch (new_treatment_status)
        {
            case IndividualEventTriggerType::TBStartDrugRegimen:
                LOG_DEBUG_F( "Individual %d starting the drug, broadcasting that this person started \n", parent->GetSuid().data );
                broadcaster->TriggerNodeEventObservers( parent->GetEventContext(), IndividualEventTriggerType::TBStartDrugRegimen );
        
                LOG_DEBUG("Started drug regimen, update tx_naive flag to false in TB IVC \n");
                m_is_tb_tx_naive_TBIVC = false;
                break;

            case IndividualEventTriggerType::TBStopDrugRegimen:
                //figure out if they failed or relapsed now that they have stopped their drug regimen
                //future should check if this works with multiple drugs on board at the same time

                //if the person still has active disease at the time this drug expires, they have failed
                if (tb_patient->HasActiveInfection() )
                {
                    LOG_DEBUG_F( "Individual %d finished the drug but still has active disease, broadcasting that this person failed \n", parent->GetSuid().data );
                    broadcaster->TriggerNodeEventObservers( parent->GetEventContext(), IndividualEventTriggerType::TBFailedDrugRegimen );

                    //Update the person's failed flag to false in the TBInterventionsContainer
                    LOG_DEBUG("Finished drug regimen but failed, update failed flag to true in TB IVC \n");
                    m_failed_tx_TBIVC = true;
                }
                else if (tb_patient->HasPendingRelapseInfection() )
                {
                    LOG_DEBUG_F( "Individual %d finished the drug but is pending relapse broadcasting that this person is pending relapse\n", parent->GetSuid().data );
                    broadcaster->TriggerNodeEventObservers( parent->GetEventContext(), IndividualEventTriggerType::TBRelapseAfterDrugRegimen );
            
                    //Update the person's failed flag to false in the TBInterventionsContainer
                    LOG_DEBUG("Finished drug regimen but now pending relapse, update ever relapsed flag to true in TB IVC \n");
                    m_ever_relapsed_TBIVC = true;
                }
                else
                {
                    LOG_DEBUG("the person finished the drug but did not fail or relapse");
                }
                break;

            default:
                LOG_DEBUG("the person finished the drug but did not fail or relapse");
                break;
        }
    }

    bool TBInterventionsContainer::GetTxNaiveStatus() const
    {
        return m_is_tb_tx_naive_TBIVC;
    }

    bool TBInterventionsContainer::GetTxFailedStatus() const
    {
        return m_failed_tx_TBIVC;
    }

    bool TBInterventionsContainer::GetTxEverRelapsedStatus() const
    {
        return m_ever_relapsed_TBIVC;
    }

    void TBInterventionsContainer::GiveDrug(IDrug* drug)
    {
        drug->ConfigureDrugTreatment( this );
    }

    // For now, before refactoring Drugs to work in new way, just check if the intervention is a
    // Drug, and if so, add to drugs list. In future, there will be no drugs list, just interventions.
    bool TBInterventionsContainer::GiveIntervention(
        IDistributableIntervention * pIV
    )
    {
        // NOTE: Calling this AFTER the QI/GiveDrug crashes!!! Both win and linux. Says SetContextTo suddenly became a pure virtual.
        pIV->SetContextTo( parent ); 
        IDrug * pDrug = nullptr;
        if( s_OK == pIV->QueryInterface(GET_IID(IDrug), (void**) &pDrug) )
        {
            LOG_DEBUG("Getting a drug\n");
            GiveDrug( pDrug );
        }

        return InterventionsContainer::GiveIntervention( pIV );
    }

    TBDrugEffectsMap_t TBInterventionsContainer::GetDrugEffectsMap() 
    { 
        return TB_drug_effects; 
    }

    REGISTER_SERIALIZABLE(TBInterventionsContainer);

    void TBDrugEffects_t::serialize(IArchive& ar, TBDrugEffects_t& effects)
    {
        ar.startObject();
            ar.labelElement("clearance_rate") & effects.clearance_rate;
            ar.labelElement("inactivation_rate") & effects.inactivation_rate;
            ar.labelElement("resistance_rate") & effects.resistance_rate;
            ar.labelElement("relapse_rate") & effects.relapse_rate;     // Boost serialization didn't include this member.
            ar.labelElement("mortality_rate") & effects.mortality_rate; // Boost serialization didn't include this member.
        ar.endObject();
    }

    // clorton TODO - could serialize(map<A, B>) be templatized in IArchive.h?
    void serialize(IArchive& ar, TBDrugEffectsMap_t& map)
    {
        size_t count = ar.IsWriter() ? map.size() : -1;
        ar.startArray(count);
        if (ar.IsWriter())
        {
            for (auto& entry : map)
            {
                ar.startObject();
                    ar.labelElement("key") & (uint32_t&)entry.first;
                    ar.labelElement("value") & entry.second; // clorton Kernel::serialize(ar, entry.second);
                ar.endObject();
            }
        }
        else
        {
            for (size_t i = 0; i < count; i++)
            {
                TBDrugType::Enum type;
                TBDrugEffects_t effects;
                ar.startObject();
                    ar.labelElement("key") & (uint32_t&)type;
                    ar.labelElement("value") & effects; // clorton Kernel::serialize(ar, effects);
                ar.endObject();
                map[type] = effects;
            }
        }
        ar.endArray();
    }

    void TBInterventionsContainer::serialize(IArchive& ar, TBInterventionsContainer* obj)
    {
        InterventionsContainer::serialize(ar, obj);
        TBInterventionsContainer& interventions = *obj;
        ar.labelElement("TB_drug_effects"); Kernel::serialize(ar, interventions.TB_drug_effects);
        ar.labelElement("m_is_tb_tx_naive_TBIVC") & interventions.m_is_tb_tx_naive_TBIVC;
        ar.labelElement("m_failed_tx_TBIVC") & interventions.m_failed_tx_TBIVC;
        ar.labelElement("m_ever_relapsed_TBIVC") & interventions.m_ever_relapsed_TBIVC;
    }
}

#endif // ENABLE_TB
