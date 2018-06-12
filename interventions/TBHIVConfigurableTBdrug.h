/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "AntiTBDrug.h"

namespace Kernel
{
    ENUM_DEFINE(TBHIVConfigurabeDrugState,
        ENUM_VALUE_SPEC(ActiveMDR, 0)
        ENUM_VALUE_SPEC(LatentMDR, 1)
        ENUM_VALUE_SPEC(ActiveHIVPosOffART, 2)
        ENUM_VALUE_SPEC(LatentHIVPosOffART, 3)
        ENUM_VALUE_SPEC(ActiveHIVNegorPosOnART, 4)
        ENUM_VALUE_SPEC(LatentHIVNegorPosOnART, 5)
        ENUM_VALUE_SPEC(TBHIVCONFIGURABLEDRUGSTATECOUNT, 6))

    class TBHIVConfigurableTBdrug : public AntiTBDrug
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, TBHIVConfigurableTBdrug, IDistributableIntervention);

    public:
        TBHIVConfigurableTBdrug();
        virtual ~TBHIVConfigurableTBdrug();
        virtual bool Configure( const Configuration * ) override;

        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;
        virtual void  ConfigureDrugTreatment( IIndividualHumanInterventionsContext * ivc = nullptr ) override;

    protected:
        std::string drug_name_string;

        virtual float GetDrugInactivationRate() const;
        virtual float GetDrugClearanceRate() const;
        virtual float GetDrugResistanceRate() const;
        virtual float GetDrugRelapseRate() const;
        virtual float GetDrugMortalityRate() const;

        int MDRHIVHierarchy() const;
      
        float TB_drug_inactivation_rate_mdr;
        float TB_drug_inactivation_rate_hiv;

        float TB_drug_cure_rate_mdr;
        float TB_drug_cure_rate_hiv;

        float TB_drug_resistance_rate_hiv;

        float TB_drug_relapse_rate_mdr;
        float TB_drug_relapse_rate_hiv;

        float TB_drug_mortality_rate_hiv;
        float TB_drug_mortality_rate_mdr;

        //Distingusih between inactivation and clearance between latent and active( relapse and mortality and resistance apply only to ACTIVe
        // So these should be specified as such in campaign) 
        float latent_efficacy_multiplier;  
        float active_efficacy_multiplier;

        DECLARE_SERIALIZABLE(TBHIVConfigurableTBdrug);
    };
}

