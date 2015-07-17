/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include "Configure.h"
#include "InterventionEnums.h"

namespace Kernel {
    class SimulationConfig;
    class DoseMap : public JsonConfigurable
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
            virtual void ConfigureFromJsonAndKey( const Configuration* inputJson, const std::string& key );
            virtual json::QuickBuilder GetSchema();
    };

    class MalariaDrugTypeParameters : public JsonConfigurable
    {
        friend class SimulationConfig;
        friend class AntimalarialDrug;
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()

    public:
        MalariaDrugTypeParameters( const std::string& drugType );
        static MalariaDrugTypeParameters* CreateMalariaDrugTypeParameters( const std::string& drugType );
        virtual ~MalariaDrugTypeParameters();
        bool Configure( const ::Configuration *json );
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject);

        typedef map< std::string, MalariaDrugTypeParameters* > tMDTPMap;
        static tMDTPMap _mdtMap;

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
