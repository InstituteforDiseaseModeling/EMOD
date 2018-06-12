/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "Configure.h"
#include "TBDrugTypeParameters.h"

namespace Kernel 

{
    class TBDrugTypeParameters;
    class TBHIVDrugTypeParameters : public TBDrugTypeParameters
    {
        friend class SimulationConfig;
        friend class AntiTBPropDepDrug;
        friend class TBHIVConfigurableTBdrug;
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
    public:
        static TBHIVDrugTypeParameters* CreateTBHIVDrugTypeParameters( const Configuration * inputJson, const std::string& tb_drug_name );

        TBHIVDrugTypeParameters( const std::string& tb_drug_name );
        virtual ~TBHIVDrugTypeParameters();
       // bool Configure( const ::Configuration *json );
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject);

    protected:
        float TB_drug_inactivation_rate_mdr;
        float TB_drug_inactivation_rate_hiv;

        float TB_drug_cure_rate_mdr;
        float TB_drug_cure_rate_hiv;

        float TB_drug_resistance_rate_hiv;

        float TB_drug_relapse_rate_mdr;
        float TB_drug_relapse_rate_hiv;

        float TB_drug_mortality_rate_hiv;
        float TB_drug_mortality_rate_mdr;

        float TB_reduced_transmit;
        float TB_reduced_acquire;
    };
}
