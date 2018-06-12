/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once
#include "Configure.h"

namespace Kernel 
{
    ENUM_DEFINE(TBDrugType,
        ENUM_VALUE_SPEC(DOTS                    , 1)
        ENUM_VALUE_SPEC(DOTSImproved            , 2)
        ENUM_VALUE_SPEC(EmpiricTreatment        , 3)
        ENUM_VALUE_SPEC(FirstLineCombo          , 4)
        ENUM_VALUE_SPEC(SecondLineCombo         , 5)
        ENUM_VALUE_SPEC(ThirdLineCombo          , 6)
        ENUM_VALUE_SPEC(LatentTreatment         , 7))

    class TBDrugTypeParameters : public JsonConfigurable
    {
        friend class SimulationConfig;
        friend class AntiTBPropDepDrug;
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
    public:
        static TBDrugTypeParameters* CreateTBDrugTypeParameters( const Configuration * inputJson, const std::string& tb_drug_name );

        TBDrugTypeParameters( const std::string& tb_drug_name );
        virtual ~TBDrugTypeParameters();
        bool Configure( const ::Configuration *json );
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject);

    protected:
        float TB_drug_inactivation_rate;
        float TB_drug_cure_rate;
        float TB_drug_resistance_rate;
        float TB_drug_relapse_rate;
        float TB_drug_mortality_rate;
        float TB_drug_primary_decay_time_constant;

    private:
        TBDrugType::Enum _drugType;
    };
}
