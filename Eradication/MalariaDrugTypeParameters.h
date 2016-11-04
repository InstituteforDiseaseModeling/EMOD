/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "Configure.h"

namespace Kernel 
{
    ENUM_DEFINE(MalariaDrugType,
        ENUM_VALUE_SPEC(Artemisinin             , 1)
        ENUM_VALUE_SPEC(Chloroquine             , 2)
        ENUM_VALUE_SPEC(Quinine                 , 3)
        ENUM_VALUE_SPEC(SP                      , 4)
        ENUM_VALUE_SPEC(Primaquine              , 5)
        ENUM_VALUE_SPEC(Artemether_Lumefantrine , 6)
        ENUM_VALUE_SPEC(GenTransBlocking        , 7)
        ENUM_VALUE_SPEC(GenPreerythrocytic      , 8)
        ENUM_VALUE_SPEC(Tafenoquine             , 9))

    class SimulationConfig;
    class DoseMap : public JsonConfigurable, public IComplexJsonConfigurable
    {
        // We need the following two lines because we inherit from JsonConfigurable
        // which provides a little more functionality than is needed by these little
        // "class types".
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) { return e_NOINTERFACE; }

        public:
            DoseMap() {}
            typedef std::map<float,float> dose_map_t;
            dose_map_t fractional_dose_by_upper_age;
            virtual void ConfigureFromJsonAndKey( const Configuration* inputJson, const std::string& key ) override;
            virtual json::QuickBuilder GetSchema() override;
    };

    class MalariaDrugTypeParameters : public JsonConfigurable
    {
        friend class SimulationConfig;
        friend class AntimalarialDrug;
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()

    public:
        MalariaDrugTypeParameters( const std::string& drugType );
        static MalariaDrugTypeParameters* CreateMalariaDrugTypeParameters( const Configuration* inputJson, const std::string& drugType );
        virtual ~MalariaDrugTypeParameters();
        bool Configure( const ::Configuration *json );
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject);

        typedef map< std::string, MalariaDrugTypeParameters* > tMDTPMap;

    protected:
        void Initialize(const std::string& drugType);

        float max_drug_IRBC_kill;
        float drug_hepatocyte_killrate;
        float drug_gametocyte02_killrate;
        float drug_gametocyte34_killrate;
        float drug_gametocyteM_killrate;
        float drug_pkpd_c50;
        float drug_Cmax;
        float drug_Vd;
        float drug_decay_T1;
        float drug_decay_T2;
        int  drug_fulltreatment_doses;
        float drug_dose_interval;

        float bodyweight_exponent;
        DoseMap dose_map;

    private:
        std::string _drugType;
    };
}
