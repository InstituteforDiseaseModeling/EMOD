/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include "AntiTBDrug.h"

namespace Kernel
{
    class DrugTypeByProperty : public JsonConfigurable
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        friend class AntiTBPropDepDrug;
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) { return e_NOINTERFACE; }
        public:
            DrugTypeByProperty() {}

            virtual void ConfigureFromJsonAndKey( const Configuration* inputJson, const std::string& key );
            virtual json::QuickBuilder GetSchema();

        protected:
            std::map< std::string, std::string > prop2drugMap;
    };

    class AntiTBPropDepDrug : public AntiTBDrug
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, AntiTBPropDepDrug, IDistributableIntervention);

    public:
        bool Configure( const Configuration * );
        AntiTBPropDepDrug();
        virtual ~AntiTBPropDepDrug();

        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject);
        virtual void  ConfigureDrugTreatment( IIndividualHumanInterventionsContext * ivc = NULL );

    protected:
        DrugTypeByProperty drug_type_by_property;
        bool enable_state_specific_tx;

    private:
#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
        // Serialization
        friend class ::boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive &ar, AntiTBPropDepDrug& drug, const unsigned int v);
#endif
    };
}

