/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "TBDrugTypeParameters.h"
#include "TBHIVDrugTypeParameters.h"
#include "Exceptions.h"
#include "Log.h"

SETUP_LOGGING( "TBHIVDTP" )


namespace Kernel
{
    TBHIVDrugTypeParameters::TBHIVDrugTypeParameters(
        const std::string& tb_drug_name
    ) : 
    TBDrugTypeParameters(tb_drug_name),
    TB_drug_inactivation_rate_mdr(0.0),
    TB_drug_inactivation_rate_hiv(0.0),
    TB_drug_cure_rate_mdr(0.0),
    TB_drug_cure_rate_hiv(0.0),
    TB_drug_resistance_rate_hiv(0.0),
    TB_drug_relapse_rate_mdr(0.0),
    TB_drug_relapse_rate_hiv(0.0),
    TB_drug_mortality_rate_mdr(0.0),
    TB_drug_mortality_rate_hiv(0.0),
    TB_reduced_acquire(0.0),
    TB_reduced_transmit(0.0)
    {
        initConfigTypeMap( "TB_Drug_Inactivation_Rate_HIV", &TB_drug_inactivation_rate_hiv, TB_Drug_Inactivation_Rate_HIV_DESC_TEXT, 0.0, 1.0, 1.0, "Enable_Coinfection" );
        initConfigTypeMap( "TB_Drug_Inactivation_Rate_MDR", &TB_drug_inactivation_rate_mdr, TB_Drug_Inactivation_Rate_MDR_DESC_TEXT, 0.0, 1.0, 1.0, "Enable_Coinfection" );

        initConfigTypeMap( "TB_Drug_Cure_Rate_HIV", &TB_drug_cure_rate_hiv, TB_Drug_Cure_Rate_HIV_DESC_TEXT, 0.0, 1.0, 1.0, "Enable_Coinfection" );
        initConfigTypeMap( "TB_Drug_Cure_Rate_MDR", &TB_drug_cure_rate_mdr, TB_Drug_Cure_Rate_MDR_DESC_TEXT, 0.0, 1.0, 1.0, "Enable_Coinfection" );

        initConfigTypeMap( "TB_Drug_Resistance_Rate_HIV", &TB_drug_resistance_rate_hiv, TB_Drug_Resistance_Rate_HIV_DESC_TEXT, 0.0, 1.0, 0.0, "Enable_Coinfection" );

        initConfigTypeMap( "TB_Drug_Relapse_Rate_MDR", &TB_drug_relapse_rate_mdr, TB_Drug_Relapse_Rate_MDR_DESC_TEXT, 0.0, 1.0, 0.0, "Enable_Coinfection" );
        initConfigTypeMap( "TB_Drug_Relapse_Rate_HIV", &TB_drug_relapse_rate_hiv, TB_Drug_Relapse_Rate_HIV_DESC_TEXT, 0.0, 1.0, 0.0, "Enable_Coinfection" );

        initConfigTypeMap( "TB_Drug_Mortality_Rate_MDR", &TB_drug_mortality_rate_mdr, TB_Drug_Mortality_Rate_MDR_DESC_TEXT, 0.0, 1.0, 0.0, "Enable_Coinfection" );
        initConfigTypeMap( "TB_Drug_Mortality_Rate_HIV", &TB_drug_mortality_rate_hiv, TB_Drug_Mortality_Rate_HIV_DESC_TEXT, 0.0, 1.0, 0.0, "Enable_Coinfection" );

        initConfigTypeMap( "TB_Reduced_Transmit", &TB_reduced_transmit, TB_Reduced_Transmit_TBHIV_DESC_TEXT, 0.0, 1.0, 0.0, "Enable_Coinfection" );
        initConfigTypeMap( "TB_Reduced_Acquire", &TB_reduced_acquire, TB_Reduced_Acquire_TBHIV_DESC_TEXT, 0.0, 1.0, 0.0, "Enable_Coinfection" );
    }

    TBHIVDrugTypeParameters::~TBHIVDrugTypeParameters()
    { 
        LOG_DEBUG( "dtor\n" );
    }

    TBHIVDrugTypeParameters* TBHIVDrugTypeParameters::CreateTBHIVDrugTypeParameters(
        const Configuration * inputJson,
        const std::string& tb_drug_name
    )
    { 
        LOG_DEBUG( "Create TBHIVDrugTypeParameters\n" );

        TBHIVDrugTypeParameters* params = _new_ TBHIVDrugTypeParameters(tb_drug_name);
        if( !JsonConfigurable::_dryrun )
        {
            auto tmp_config = Configuration::CopyFromElement( (*EnvPtr->Config)["TBHIV_Drug_Params"][tb_drug_name.c_str()], EnvPtr->Config->GetDataLocation() );
            params->Configure( tmp_config );
            delete tmp_config;
            tmp_config = nullptr;
        }
        LOG_DEBUG( "END CreateTBHIVDrugTypeParameters\n" );
        return params;
    }

    /*bool
    TBHIVDrugTypeParameters::Configure(
        const ::Configuration *config
    )
    {
        LOG_DEBUG( "Configure\n" );

        return JsonConfigurable::Configure( config );
    } */

    QueryResult
    TBHIVDrugTypeParameters::QueryInterface(
        iid_t iid, void **ppvObject
    )
    {
        throw NotYetImplementedException(  __FILE__, __LINE__, __FUNCTION__, "Should not get here" );
    }
}

