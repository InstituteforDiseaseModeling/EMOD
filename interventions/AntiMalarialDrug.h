/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "Drugs.h"

namespace Kernel
{
    typedef std::map<float,float> bodyweight_map_t;

    class AntimalarialDrug : public GenericDrug
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, AntimalarialDrug, IDistributableIntervention)

    public:
        /* clorton virtual */ bool Configure( const Configuration * ) /* clorton override */;
        AntimalarialDrug();
        virtual ~AntimalarialDrug();

        // ISupports
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;
        virtual void SetContextTo(IIndividualHumanContext *context) override;

        // IDistributableIntervention
        virtual bool Distribute(IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pCCO ) override;

        // IDrug
        virtual void  ConfigureDrugTreatment( IIndividualHumanInterventionsContext * ivc = nullptr ) override;  // read in PkPd and drug-killing parameters from SusceptibilityFlagsMalaria
        virtual std::string GetDrugName() const override;

        static float BodyWeightByAge(float age_in_days);
        static const float _adult_bodyweight_kg;

    protected:

        virtual void ResetForNextDose(float dt) override;

        // These have same names as analogous methods on container but are internal for the drug itself.
        float get_drug_IRBC_killrate();
        float get_drug_hepatocyte();
        float get_drug_gametocyte02();
        float get_drug_gametocyte34();
        float get_drug_gametocyteM();

        virtual void ApplyEffects() override;

        float drug_IRBC_killrate;
        float drug_hepatocyte;
        float drug_gametocyte02;
        float drug_gametocyte34;
        float drug_gametocyteM;
        IMalariaDrugEffectsApply * imda;

        jsonConfigurable::ConstrainedString drug_name;
        static bodyweight_map_t create_bodyweight_map();
        static const bodyweight_map_t bodyweight_map_;

        DECLARE_SERIALIZABLE(AntimalarialDrug);
    };
}
