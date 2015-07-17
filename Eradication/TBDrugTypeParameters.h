/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include "Configure.h"
#include "InterventionEnums.h"

namespace Kernel {
    class TBDrugTypeParameters : public JsonConfigurable
    {
        friend class SimulationConfig;
        friend class AntiTBPropDepDrug;
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
    public:
        TBDrugTypeParameters( const std::string& tb_drug_name );
        static TBDrugTypeParameters* CreateTBDrugTypeParameters( const std::string& tb_drug_name );
        virtual ~TBDrugTypeParameters();
        bool Configure( const ::Configuration *json );
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject);
        typedef map< std::string, const TBDrugTypeParameters* > tTBDTPMap;
        static tTBDTPMap _tbdtMap;


    protected:
        float TB_drug_inactivation_rate;
        float TB_drug_clearance_rate;
        float TB_drug_resistance_rate;
        float TB_drug_relapse_rate;
        float TB_drug_mortality_rate;
        float TB_drug_primary_decay_time_constant;

    private:
        TBDrugType::Enum _drugType;
    };
}
