/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#ifdef ENABLE_TBHIV

#include "TBHIVConfigurableTBdrug.h"
#include "Contexts.h"
#include "IndividualEventContext.h"
#include "TBContexts.h"
#include "Debug.h"                        // for release-assert
#include "SimulationConfig.h"
#include "TBParameters.h"
#include "TBHIVParameters.h"
#include "TBDrugTypeParameters.h" //for TBDrugTypes
#include "TBHIVDrugTypeParameters.h"
#include "IIndividualHumanHIV.h" //for HIV status
#include "HIVInterventionsContainer.h" // for ART

SETUP_LOGGING( "TBHIVConfigurableTBdrug" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED(TBHIVConfigurableTBdrug, AntiTBDrug)
    END_QUERY_INTERFACE_DERIVED(TBHIVConfigurableTBdrug, AntiTBDrug)

    IMPLEMENT_FACTORY_REGISTERED(TBHIVConfigurableTBdrug)


    TBHIVConfigurableTBdrug::TBHIVConfigurableTBdrug()
    : AntiTBDrug()
    , TB_drug_inactivation_rate_mdr(0)
    , TB_drug_inactivation_rate_hiv(0)
    , TB_drug_cure_rate_mdr(0)
    , TB_drug_cure_rate_hiv(0)
    , TB_drug_mortality_rate_mdr(0)
    , TB_drug_relapse_rate_mdr(0)
    , TB_drug_relapse_rate_hiv(0)
    , TB_drug_resistance_rate_hiv(0)
    , latent_efficacy_multiplier(1.0)
    , active_efficacy_multiplier(1.0)
    {
        initSimTypes( 1, "TBHIV_SIM" );
        current_efficacy = 1.0; //
        remaining_doses = 1;  // 0 value zeros out current_efficacy
    }

    TBHIVConfigurableTBdrug::~TBHIVConfigurableTBdrug()
    { 
        //delete drug_type_by_property;  
    }

    int  TBHIVConfigurableTBdrug::MDRHIVHierarchy() const
    {

        IIndividualHumanTB* tb_patient = nullptr;
        if (parent->QueryInterface(GET_IID(IIndividualHumanTB), (void**)&tb_patient) != s_OK)
        {
            throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "individual", "IIndvidualHumanTB", "IndividualHuman");
        }

        IIndividualHumanHIV* hiv_patient = nullptr;
        if (parent->QueryInterface(GET_IID(IIndividualHumanHIV), (void**)&hiv_patient) != s_OK)
        {
            throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "individual", "IIndvidualHumanHIV", "IndividualHuman");
        }

        if (tb_patient->IsMDR())
        {
            if (tb_patient->HasActiveInfection())
            {
                return TBHIVConfigurabeDrugState::ActiveMDR;
            }
            else;
            {
                return TBHIVConfigurabeDrugState::LatentMDR;
            }

        }
        else if (hiv_patient->HasHIV() && !hiv_patient->GetHIVInterventionsContainer()->OnArtQuery())
        {
            if (tb_patient->HasActiveInfection())
            {
                return TBHIVConfigurabeDrugState::ActiveHIVPosOffART;
            }
            else
            {
                return TBHIVConfigurabeDrugState::LatentHIVPosOffART;
            }
        }
        else
        {
            if (tb_patient->HasActiveInfection())
            {
                return TBHIVConfigurabeDrugState::ActiveHIVNegorPosOnART;
            }
            else
            {
                return TBHIVConfigurabeDrugState::LatentHIVNegorPosOnART;
            }

        }


    }
    bool
    TBHIVConfigurableTBdrug::Configure(
        const Configuration * inputJson
    )
    {
        initConfigTypeMap("TB_Drug_Name", &drug_name_string);
		initConfigTypeMap("Latency_Multiplier", &latent_efficacy_multiplier, TB_Latent_Efficacy_Multiplier_DESC_TEXT, 0, 1.0, 1.0);
		initConfigTypeMap("Active_Multiplier", &active_efficacy_multiplier, TB_Active_Efficacy_Multiplier_DESC_TEXT, 0, 1.0, 1.0);
        return GenericDrug::Configure( inputJson );

    }

    float TBHIVConfigurableTBdrug::GetDrugInactivationRate() const
    {
        int status = MDRHIVHierarchy();

        switch (status) 
        {
        case TBHIVConfigurabeDrugState::ActiveHIVPosOffART : return current_efficacy * TB_drug_inactivation_rate_hiv * active_efficacy_multiplier;
            break; //unnecessary but cosmetic
        case TBHIVConfigurabeDrugState::LatentHIVPosOffART:  return current_efficacy * TB_drug_inactivation_rate_hiv * latent_efficacy_multiplier;
            break;
        case TBHIVConfigurabeDrugState::ActiveHIVNegorPosOnART:  return current_efficacy * TB_drug_inactivation_rate * active_efficacy_multiplier;
            break;
        case TBHIVConfigurabeDrugState::LatentHIVNegorPosOnART:  return current_efficacy * TB_drug_inactivation_rate * latent_efficacy_multiplier;
            break;
        case TBHIVConfigurabeDrugState::ActiveMDR:  return current_efficacy * TB_drug_inactivation_rate_mdr * active_efficacy_multiplier;
            break;
        case TBHIVConfigurabeDrugState::LatentMDR:  return current_efficacy * TB_drug_inactivation_rate_mdr * latent_efficacy_multiplier;
            break;
        default: throw BadEnumInSwitchStatementException(__FILE__, __LINE__, __FUNCTION__, "TBHIVConfigurableDrugState", status, TBHIVConfigurabeDrugState::pairs::lookup_key( status ) );
        
        }
        
    }

    float TBHIVConfigurableTBdrug::GetDrugClearanceRate() const
    {
        int status = MDRHIVHierarchy();

        switch (status)
        {
        case TBHIVConfigurabeDrugState::ActiveHIVPosOffART: return current_efficacy * TB_drug_cure_rate_hiv * active_efficacy_multiplier;
            break; //unnecessary but cosmetic
        case TBHIVConfigurabeDrugState::LatentHIVPosOffART:  return current_efficacy * TB_drug_cure_rate_hiv * latent_efficacy_multiplier;
            break;
        case TBHIVConfigurabeDrugState::ActiveHIVNegorPosOnART:  return current_efficacy * TB_drug_cure_rate * active_efficacy_multiplier;
            break;
        case TBHIVConfigurabeDrugState::LatentHIVNegorPosOnART:  return current_efficacy * TB_drug_cure_rate * latent_efficacy_multiplier;
            break;
        case TBHIVConfigurabeDrugState::ActiveMDR:  return current_efficacy * TB_drug_cure_rate_mdr * active_efficacy_multiplier;
            break;
        case TBHIVConfigurabeDrugState::LatentMDR:  return current_efficacy * TB_drug_cure_rate_mdr * latent_efficacy_multiplier;
            break;
        default: throw BadEnumInSwitchStatementException(__FILE__, __LINE__, __FUNCTION__, "TBHIVConfigurableDrugState", status, TBHIVConfigurabeDrugState::pairs::lookup_key( status ) );

        }
    }

    float TBHIVConfigurableTBdrug::GetDrugResistanceRate() const
    {
        int status = MDRHIVHierarchy();

        switch (status)
        {
        case TBHIVConfigurabeDrugState::ActiveHIVPosOffART: return current_efficacy * TB_drug_resistance_rate_hiv;
            break; 
        case TBHIVConfigurabeDrugState::LatentHIVPosOffART:  return 0.0f;
            break;
        case TBHIVConfigurabeDrugState::ActiveHIVNegorPosOnART:  return current_efficacy * TB_drug_resistance_rate;
            break;
        case TBHIVConfigurabeDrugState::LatentHIVNegorPosOnART: return 0.0f;
            break;
        case TBHIVConfigurabeDrugState::ActiveMDR:  return  0.0f;
            break;
        case TBHIVConfigurabeDrugState::LatentMDR:  return 0.0f;
            break;
        default: throw BadEnumInSwitchStatementException(__FILE__, __LINE__, __FUNCTION__, "TBHIVConfigurableDrugState", status, TBHIVConfigurabeDrugState::pairs::lookup_key( status ) );

        }
    }

    float TBHIVConfigurableTBdrug::GetDrugRelapseRate() const
    {
        int status = MDRHIVHierarchy();

        switch (status)
        {
        case TBHIVConfigurabeDrugState::ActiveHIVPosOffART: return current_efficacy * TB_drug_relapse_rate_hiv;
            break; 
        case TBHIVConfigurabeDrugState::LatentHIVPosOffART:  return 0.0f;
            break;
        case TBHIVConfigurabeDrugState::ActiveHIVNegorPosOnART:  return current_efficacy * TB_drug_relapse_rate;
            break;
        case TBHIVConfigurabeDrugState::LatentHIVNegorPosOnART:  return 0.0f;
            break;
        case TBHIVConfigurabeDrugState::ActiveMDR:  return current_efficacy * TB_drug_relapse_rate_mdr;
            break;
        case TBHIVConfigurabeDrugState::LatentMDR:  return 0.0f;
            break;
        default: throw BadEnumInSwitchStatementException(__FILE__, __LINE__, __FUNCTION__, "TBHIVConfigurableDrugState", status, TBHIVConfigurabeDrugState::pairs::lookup_key( status ) );

        }
      
    }

    float TBHIVConfigurableTBdrug::GetDrugMortalityRate() const
    {
        int status = MDRHIVHierarchy();

        switch (status)
        {
        case TBHIVConfigurabeDrugState::ActiveHIVPosOffART: return current_efficacy * TB_drug_mortality_rate_hiv;
            break; //unnecessary but cosmetic
        case TBHIVConfigurabeDrugState::LatentHIVPosOffART:  return 0.0f;
            break;
        case TBHIVConfigurabeDrugState::ActiveHIVNegorPosOnART:  return current_efficacy * TB_drug_mortality_rate;
            break;
        case TBHIVConfigurabeDrugState::LatentHIVNegorPosOnART:  return 0.0f;
            break;
        case TBHIVConfigurabeDrugState::ActiveMDR:  return current_efficacy * TB_drug_mortality_rate_mdr;
            break;
        case TBHIVConfigurabeDrugState::LatentMDR:  return 0.0f;
            break;
        default: throw BadEnumInSwitchStatementException(__FILE__, __LINE__, __FUNCTION__, "TBHIVConfigurableDrugState", status, TBHIVConfigurabeDrugState::pairs::lookup_key( status ) );

        }

    }


    void TBHIVConfigurableTBdrug::ConfigureDrugTreatment( IIndividualHumanInterventionsContext * ivc )  
    {
        current_efficacy = 1;
        IIndividualHumanTB* tb_patient = nullptr;
        if ( ivc->GetParent()->QueryInterface( GET_IID(IIndividualHumanTB), (void**) &tb_patient ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "individual", "IIndvidualHumanTB2", "IndividualHuman" );
        }

        auto tbdtMap = GET_CONFIGURABLE(SimulationConfig)->tbhiv_params->TBHIVDrugMap; 

        LOG_DEBUG_F("Read in the tbdt map, the drug type is %s \n", drug_name_string.c_str());
        if (tbdtMap[drug_name_string] == nullptr)
        {
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "You used a drug which is not in the TBHIV_Drug_Types." );
        }
        release_assert( tbdtMap[drug_name_string] );

        fast_decay_time_constant = tbdtMap[drug_name_string]->TB_drug_primary_decay_time_constant;

        LOG_DEBUG_F( "Using fast_decay_time_constant value of %f\n", fast_decay_time_constant );

        TB_drug_inactivation_rate = tbdtMap[drug_name_string]->TB_drug_inactivation_rate;
        TB_drug_cure_rate = tbdtMap[drug_name_string]->TB_drug_cure_rate;
        TB_drug_resistance_rate = tbdtMap[drug_name_string]->TB_drug_resistance_rate;
        TB_drug_relapse_rate = tbdtMap[drug_name_string]->TB_drug_relapse_rate;
        TB_drug_mortality_rate = tbdtMap[drug_name_string]->TB_drug_mortality_rate;

        TB_drug_cure_rate_hiv = tbdtMap[drug_name_string]->TB_drug_cure_rate_hiv;
        TB_drug_cure_rate_mdr = tbdtMap[drug_name_string]->TB_drug_cure_rate_mdr;

        TB_drug_inactivation_rate_mdr = tbdtMap[drug_name_string]->TB_drug_inactivation_rate_mdr;
        TB_drug_inactivation_rate_hiv = tbdtMap[drug_name_string]->TB_drug_inactivation_rate_hiv;

        TB_drug_relapse_rate_hiv = tbdtMap[drug_name_string]->TB_drug_relapse_rate_hiv;
        TB_drug_relapse_rate_mdr = tbdtMap[drug_name_string]->TB_drug_relapse_rate_mdr;

        TB_drug_mortality_rate_hiv = tbdtMap[drug_name_string]->TB_drug_mortality_rate_hiv;
        TB_drug_mortality_rate_mdr = tbdtMap[drug_name_string]->TB_drug_inactivation_rate_mdr;

        TB_drug_resistance_rate_hiv = tbdtMap[drug_name_string]->TB_drug_resistance_rate_hiv;

        current_reducedacquire = tbdtMap[drug_name_string]->TB_reduced_acquire;
        current_reducedtransmit = tbdtMap[drug_name_string]->TB_reduced_transmit;

        LOG_DEBUG_F("Finished reading in the map: TB_drug_cure_rate = %f.\n", TB_drug_cure_rate );

        //in base class AntiTBDrug, sets current_efficacy to 1 and broadcast that you are starting an intervention
        return AntiTBDrug::ConfigureDrugTreatment( ivc ); 
    }


    REGISTER_SERIALIZABLE(TBHIVConfigurableTBdrug);

    void TBHIVConfigurableTBdrug::serialize(IArchive& ar, TBHIVConfigurableTBdrug* obj)
    {
        AntiTBDrug::serialize(ar, obj);
        TBHIVConfigurableTBdrug& drug = *obj;
        ar.labelElement("TB_drug_inactivation_rate_mdr") & drug.TB_drug_inactivation_rate_mdr;
        ar.labelElement("TB_drug_inactivation_rate_hiv") & drug.TB_drug_inactivation_rate_hiv;
        ar.labelElement("TB_drug_cure_rate_mdr") & drug.TB_drug_cure_rate_mdr;
        ar.labelElement("TB_drug_cure_rate_hiv") & drug.TB_drug_cure_rate_hiv;
        ar.labelElement("TB_drug_mortality_rate_mdr") & drug.TB_drug_mortality_rate_mdr;
        ar.labelElement("TB_drug_mortality_rate_hiv") & drug.TB_drug_mortality_rate_hiv;
        ar.labelElement("TB_drug_relapse_rate_mdr") & drug.TB_drug_relapse_rate_mdr;
        ar.labelElement("TB_drug_relapse_rate_hiv") & drug.TB_drug_relapse_rate_hiv;
        ar.labelElement("TB_drug_resistance_rate_hiv") & drug.TB_drug_relapse_rate_hiv;
        ar.labelElement("latent_efficacy_multiplier")  & drug.latent_efficacy_multiplier;
        ar.labelElement("active_efficacy_multiplier")  & drug.active_efficacy_multiplier;
    }
}

#endif
