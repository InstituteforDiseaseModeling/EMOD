/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "AntiTBDrug.h"

namespace Kernel
{
    class DrugTypeByProperty : public JsonConfigurable, public IComplexJsonConfigurable
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        friend class AntiTBPropDepDrug;
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) { return e_NOINTERFACE; }
        public:
            DrugTypeByProperty() {}

            virtual void ConfigureFromJsonAndKey( const Configuration* inputJson, const std::string& key ) override;
            virtual json::QuickBuilder GetSchema() override;
            virtual bool  HasValidDefault() const override { return false; }

        protected:
            std::map< std::string, std::string > prop2drugMap;
    };

    class AntiTBPropDepDrug : public AntiTBDrug
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, AntiTBPropDepDrug, IDistributableIntervention);

    public:
        AntiTBPropDepDrug();
        virtual ~AntiTBPropDepDrug();
        virtual bool Configure( const Configuration * ) override;

        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;
        virtual void  ConfigureDrugTreatment( IIndividualHumanInterventionsContext * ivc = nullptr ) override;

    protected:
        DrugTypeByProperty drug_type_by_property;
        bool enable_state_specific_tx;

        DECLARE_SERIALIZABLE(AntiTBPropDepDrug);
    };
}

