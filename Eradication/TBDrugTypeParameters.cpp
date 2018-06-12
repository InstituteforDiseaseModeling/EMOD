/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "TBDrugTypeParameters.h"

#include "Exceptions.h"
#include "Log.h"

SETUP_LOGGING( "TBDTP" )

namespace Kernel
{
    TBDrugTypeParameters::TBDrugTypeParameters(
        const std::string& tb_drug_name
    ) : 
    TB_drug_inactivation_rate(0.0),
    TB_drug_cure_rate(0.0),
    TB_drug_resistance_rate(0.0),
    TB_drug_relapse_rate(0.0),
    TB_drug_mortality_rate(0.0)
    {
        initConfigTypeMap("TB_Drug_Inactivation_Rate", &TB_drug_inactivation_rate, TB_Drug_Inactivation_Rate_DESC_TEXT, 0.0, 1.0, 1.0);
        initConfigTypeMap("TB_Drug_Cure_Rate", &TB_drug_cure_rate, TB_Drug_Cure_Rate_DESC_TEXT, 0.0, 1.0, 1.0);
        initConfigTypeMap("TB_Drug_Resistance_Rate", &TB_drug_resistance_rate, TB_Drug_Resistance_Rate_DESC_TEXT, 0.0, 1.0, 0.0); // 0.0 means no resistance 
        initConfigTypeMap("TB_Drug_Relapse_Rate", &TB_drug_relapse_rate, TB_Drug_Relapse_Rate_DESC_TEXT, 0.0, 1.0, 0.0); // 0.0 means no relapse 
        initConfigTypeMap("TB_Drug_Mortality_Rate", &TB_drug_mortality_rate, TB_Drug_Mortality_Rate_DESC_TEXT, 0.0, 1.0, 0.0); // 0.0 means no mortality 
        initConfigTypeMap("TB_Drug_Primary_Decay_Time_Constant", &TB_drug_primary_decay_time_constant, TB_Drug_Primary_Decay_Time_Constant_DESC_TEXT, 0.0, 100000.0, 0.0);
    }

    TBDrugTypeParameters::~TBDrugTypeParameters()
    { LOG_DEBUG( "dtor\n" );
    }

    TBDrugTypeParameters* TBDrugTypeParameters::CreateTBDrugTypeParameters(
        const Configuration * inputJson,
        const std::string& tb_drug_name
    )
    { 
        LOG_DEBUG( "Create TBDrugTypeParameters\n" );

        TBDrugTypeParameters* params = _new_ TBDrugTypeParameters(tb_drug_name);
        if( !JsonConfigurable::_dryrun )
        {
            auto tmp_config = Configuration::CopyFromElement( (*EnvPtr->Config)["TB_Drug_Params"][tb_drug_name.c_str()], EnvPtr->Config->GetDataLocation() );
            params->Configure( tmp_config );
            delete tmp_config;
            tmp_config = nullptr;
        }
        LOG_DEBUG( "END CreateTBDrugTypeParameters\n" );
        return params;

    }

    bool
    TBDrugTypeParameters::Configure(
        const ::Configuration *config
    )
    {
        LOG_DEBUG( "Configure\n" );

        return JsonConfigurable::Configure( config );
    }

    QueryResult
    TBDrugTypeParameters::QueryInterface(
        iid_t iid, void **ppvObject
    )
    {
        throw NotYetImplementedException(  __FILE__, __LINE__, __FUNCTION__, "Should not get here" );
    }
}

