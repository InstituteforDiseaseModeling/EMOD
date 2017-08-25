/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#ifdef ENABLE_TB

#include "AntiTBDrug.h"

#include "Contexts.h"                  // for IIndividualHumanContext, IIndividualHumanInterventionsContext
#include "Debug.h"                     // for IIndividualHumanContext, IIndividualHumanInterventionsContext
#include "TBInterventionsContainer.h"  // for ITBDrugEffectsApply methods
#include "NodeEventContext.h"          // for INodeEventContext (ICampaignCostObserver)
#include "EventTrigger.h"

SETUP_LOGGING( "AntiTBDrug" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED(AntiTBDrug, GenericDrug)
    END_QUERY_INTERFACE_DERIVED(AntiTBDrug, GenericDrug)

    IMPLEMENT_FACTORY_REGISTERED(AntiTBDrug)

    AntiTBDrug::AntiTBDrug()
    : GenericDrug()
    , TB_drug_inactivation_rate(0)
    , TB_drug_clearance_rate(0)
    , TB_drug_resistance_rate(0)
    , TB_drug_relapse_rate(0)
    , TB_drug_mortality_rate(0)
    , itbda(nullptr)
    , m_pCCO(nullptr)
    {
        initSimTypes( 2, "TB_SIM", "TBHIV_SIM" );
    }


    float AntiTBDrug::GetDrugInactivationRate() const
    {
        return current_efficacy * TB_drug_inactivation_rate;
    }

    float AntiTBDrug::GetDrugClearanceRate() const
    {
        LOG_DEBUG_F( "TB_drug_clearance_rate = %f \n", current_efficacy * TB_drug_clearance_rate );
        return current_efficacy * TB_drug_clearance_rate;
    }

    float AntiTBDrug::GetDrugResistanceRate() const
    {
        LOG_DEBUG_F( "TB_drug_resistance_rate = %f \n", current_efficacy * TB_drug_resistance_rate ); //TODO: is it right to multiply by current_efficacy?
        return current_efficacy * TB_drug_resistance_rate;
    }

    float AntiTBDrug::GetDrugRelapseRate() const
    {
        LOG_DEBUG_F( "TB_drug_relapse_rate = %f \n", current_efficacy * TB_drug_relapse_rate ); //TODO: is it right to multiply by current_efficacy?
        return current_efficacy * TB_drug_relapse_rate;
    }

    float AntiTBDrug::GetDrugMortalityRate() const
    {
        LOG_DEBUG_F( "TB_drug_mortality_rate = %f \n", current_efficacy * TB_drug_mortality_rate ); //TODO: is it right to multiply by current_efficacy?
        return current_efficacy * TB_drug_mortality_rate;
    }

    int
    AntiTBDrug::GetDrugType() const
    {
        return drug_type;
    }

    std::string
    AntiTBDrug::GetDrugName() const
    {
        return TBDrugType::pairs::lookup_key(drug_type);
    }

    bool
    AntiTBDrug::Configure(
        const Configuration * inputJson
    )
    {
        initConfig( "Drug_Type", drug_type, inputJson, MetadataDescriptor::Enum("Drug_Type", TB_Drug_Type_DESC_TEXT, MDD_ENUM_ARGS(TBDrugType)));
        initConfigTypeMap("TB_Drug_Inactivation_Rate", &TB_drug_inactivation_rate, TB_Drug_Inactivation_Rate_DESC_TEXT, 0.0, 1.0, 0.0);
        initConfigTypeMap("TB_Drug_Clearance_Rate", &TB_drug_clearance_rate, TB_Drug_Clearance_Rate_DESC_TEXT, 0.0, 1.0, 0.0);
        initConfigTypeMap("TB_Drug_Resistance_Rate", &TB_drug_resistance_rate, TB_Drug_Resistance_Rate_DESC_TEXT, 0.0, 1.0, 0.0); // 0.0 means no resistance
        initConfigTypeMap("TB_Drug_Relapse_Rate", &TB_drug_relapse_rate, TB_Drug_Relapse_Rate_DESC_TEXT, 0.0, 1.0, 0.0); // 0.0 means no relapse
        initConfigTypeMap("TB_Drug_Mortality_Rate", &TB_drug_mortality_rate, TB_Drug_Mortality_Rate_DESC_TEXT, 0.0, 1.0, 0.0); // 0.0 means no death

        initConfigTypeMap("Reduced_Transmit", &current_reducedtransmit, TB_Drug_Reduced_Transmit_DESC_TEXT, 0.0, 1.0, 1.0 ); //1.0 means it is 100% reduced

        return GenericDrug::Configure( inputJson );
    }

    bool
    AntiTBDrug::Distribute(
        IIndividualHumanInterventionsContext *context,
        ICampaignCostObserver * pCCO
    )
    {
        if (s_OK != context->QueryInterface(GET_IID(ITBDrugEffectsApply), (void**)&itbda) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "ITBDrugEffectsApply", "IIndividualHumanInterventionsContext" );
        }
        m_pCCO = pCCO;
        return GenericDrug::Distribute( context, pCCO );
    }

    void
    AntiTBDrug::SetContextTo(
        IIndividualHumanContext *context
    )
    {
        if (s_OK != context->GetInterventionsContext()->QueryInterface(GET_IID(ITBDrugEffectsApply), (void**)&itbda) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "ITBDrugEffectsApply", "IIndividualHumanContext" );
        }

        if (s_OK != context->GetEventContext()->GetNodeEventContext()->QueryInterface(GET_IID(ICampaignCostObserver), (void**)&m_pCCO) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent->GetEventContext()->GetNodeEventContext()", "ICampaignCostObserver", "INodeEventContext");
        }

        return GenericDrug::SetContextTo( context );
    }

    void AntiTBDrug::ConfigureDrugTreatment( IIndividualHumanInterventionsContext * ivc )  // set current_efficacy = 1
    {
        current_efficacy = 1.0;
        LOG_DEBUG_F( "Individual %d starting the drug, set current_efficacy to 1 \n", parent->GetSuid().data );

        //Update the person's tx naive flag to false and broadcast that they started the drugregimen in the TBInterventionsContainer
        release_assert(itbda);
        itbda->UpdateTreatmentStatus( EventTrigger::TBStartDrugRegimen );
    }

    void AntiTBDrug::ApplyEffects()
    {
        assert(itbda);
        itbda->ApplyDrugVaccineReducedAcquireEffect(GetDrugReducedAcquire());
        itbda->ApplyDrugVaccineReducedTransmitEffect(GetDrugReducedTransmit());

        TBDrugEffects_t effects;
        effects.clearance_rate = GetDrugClearanceRate();
        effects.inactivation_rate = GetDrugInactivationRate();
        effects.resistance_rate = GetDrugResistanceRate();
        effects.relapse_rate = GetDrugRelapseRate();
        effects.mortality_rate = GetDrugMortalityRate();
        itbda->ApplyTBDrugEffects( effects, (TBDrugType::Enum)drug_type );
    }

    void AntiTBDrug::Expire()
    {
        GenericDrug::Expire();
        LOG_DEBUG_F( "Individual %d finished the drug \n", parent->GetSuid().data );

        //Update the person's failed flag to false in the TBInterventionsContainer
        release_assert(itbda);
        itbda->UpdateTreatmentStatus( EventTrigger::TBStopDrugRegimen );

        // notify campaign event observer
        if (m_pCCO != nullptr)
        {
            m_pCCO->notifyCampaignEventOccurred( (IBaseIntervention*)this, nullptr, parent );
        }
    }

    REGISTER_SERIALIZABLE(AntiTBDrug);

    void AntiTBDrug::serialize(IArchive& ar, AntiTBDrug* obj)
    {
        GenericDrug::serialize(ar, obj);
        AntiTBDrug& drug = *obj;
        ar.labelElement("TB_drug_inactivation_rate") & drug.TB_drug_inactivation_rate;
        ar.labelElement("TB_drug_clearance_rate") & drug.TB_drug_clearance_rate;
        ar.labelElement("TB_drug_resistance_rate") & drug.TB_drug_resistance_rate;
        ar.labelElement("TB_drug_relapse_rate") & drug.TB_drug_relapse_rate;
        ar.labelElement("TB_drug_mortality_rate") & drug.TB_drug_mortality_rate;
    }
}

#endif // ENABLE_TB
