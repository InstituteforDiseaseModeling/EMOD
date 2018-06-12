/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "AntiMalarialDrug.h"

#include "Exceptions.h"
#include "IndividualEventContext.h"
#include "MalariaDrugTypeParameters.h"
#include "SimulationConfig.h"               // for global-context access to PKPDmodel (!)
#include "MalariaParameters.h"
#include "MalariaContexts.h"

SETUP_LOGGING( "AntimalarialDrug" )

namespace Kernel
{
    const float AntimalarialDrug::_adult_bodyweight_kg = 50.0f;
    const bodyweight_map_t AntimalarialDrug::bodyweight_map_ = create_bodyweight_map();

    BEGIN_QUERY_INTERFACE_DERIVED(AntimalarialDrug, GenericDrug)
        HANDLE_INTERFACE(IMalariaDrugEffects)
    END_QUERY_INTERFACE_DERIVED(AntimalarialDrug, GenericDrug)
    IMPLEMENT_FACTORY_REGISTERED(AntimalarialDrug)

    AntimalarialDrug::~AntimalarialDrug()
    {
    }

    float AntimalarialDrug::CalculateModifiedEfficacy( const IStrainIdentity& rStrain )
    {
        float modifier = 1.0;
        if( durability_time_profile == PKPDModel::CONCENTRATION_VERSUS_TIME )
        {
            modifier = pDrugResistantModifiers->GetC50( rStrain );
        }
        return GenericDrug::CalculateEfficacy( modifier * this->drug_c50, start_concentration, end_concentration );
    }

    float
    AntimalarialDrug::get_drug_IRBC_killrate( const IStrainIdentity& rStrain )
    {
        float modifier = pDrugResistantModifiers->GetMaxKilling( rStrain );
        float efficacy = CalculateModifiedEfficacy( rStrain );
        return modifier * efficacy * drug_IRBC_killrate;
    }

    float AntimalarialDrug::get_drug_hepatocyte( const IStrainIdentity& rStrain )
    {
        float efficacy = CalculateModifiedEfficacy( rStrain );
        return current_efficacy * drug_hepatocyte;
    }

    float
    AntimalarialDrug::get_drug_gametocyte02( const IStrainIdentity& rStrain )
    {
        float efficacy = CalculateModifiedEfficacy( rStrain );
        return efficacy * drug_gametocyte02;
    }

    float
    AntimalarialDrug::get_drug_gametocyte34( const IStrainIdentity& rStrain )
    {
        float efficacy = CalculateModifiedEfficacy( rStrain );
        return efficacy * drug_gametocyte34;
    }

    float
    AntimalarialDrug::get_drug_gametocyteM( const IStrainIdentity& rStrain )
    {
        float efficacy = CalculateModifiedEfficacy( rStrain );
        return efficacy * drug_gametocyteM;
    }

    DrugUsageType::Enum
    AntimalarialDrug::GetDrugUsageType()
    {
        return dosing_type;
    }

    bool
    AntimalarialDrug::Configure(
        const Configuration * inputJson
    )
    {
        jsonConfigurable::tFixedStringSet drug_name_set;
        if( !JsonConfigurable::_dryrun )
        {
            auto mdtMap = GET_CONFIGURABLE( SimulationConfig )->malaria_params->MalariaDrugMap;
            for( auto& rNameDrugPair : mdtMap )
            {
                drug_name_set.insert( rNameDrugPair.first );
            }
        }

        jsonConfigurable::ConstrainedString tmp_drug_name;
        tmp_drug_name.constraints = "<configuration>:Malaria_Drug_Params.*";
        tmp_drug_name.constraint_param = &drug_name_set;

        initConfigTypeMap("Cost_To_Consumer", &cost_per_unit, AMDRUG_Cost_To_Consumer_DESC_TEXT, 0, 999999, 10);
        initConfigTypeMap( "Drug_Type", &tmp_drug_name, AMDRUG_Drug_Type_DESC_TEXT);
        initConfig( "Dosing_Type", dosing_type, inputJson, MetadataDescriptor::Enum("Dosing_Type", AMDRUG_Dosing_Type_DESC_TEXT, MDD_ENUM_ARGS(DrugUsageType)) );

        bool ret = BaseIntervention::Configure( inputJson );
        if( ret && !JsonConfigurable::_dryrun )
        {
            if( tmp_drug_name.empty() || !inputJson->Exist( "Drug_Type" ) )
            {
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, "'Drug_Type' was not defined and it is a required parameter.");
            }
            drug_name = tmp_drug_name;

            auto mdtMap = GET_CONFIGURABLE( SimulationConfig )->malaria_params->MalariaDrugMap;
            if( mdtMap.find( drug_name ) == mdtMap.end() )
            {
                throw BadMapKeyException( __FILE__, __LINE__, __FUNCTION__, "mdtMap", drug_name.c_str() );
            }
            pMalariaDrugTypeParameters = mdtMap.at( drug_name );
        }
        return ret;
    }

    AntimalarialDrug::AntimalarialDrug()
        : GenericDrug()
        , dosing_type( DrugUsageType::SingleDose )
        , drug_IRBC_killrate(0)
        , drug_hepatocyte(0)
        , drug_gametocyte02(0)
        , drug_gametocyte34(0)
        , drug_gametocyteM(0)
        , pMalariaDrugTypeParameters(nullptr)
        , pDrugResistantModifiers(nullptr)
        , imda(nullptr)
    {
        initSimTypes( 1, "MALARIA_SIM" );
    }

    bool
    AntimalarialDrug::Distribute(
        IIndividualHumanInterventionsContext *context,
        ICampaignCostObserver * pCCO
    )
    {
        bool keep = false;

        IMalariaHumanContext* p_mhc = nullptr;
        if( context->GetParent()->QueryInterface( GET_IID( IMalariaHumanContext ), (void**)&p_mhc ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context->GetParent()", "IMalariaHumanContext", "IIndividualHumanContext" );
        }

        if( (GetDrugUsageType() == DrugUsageType::FullTreatmentParasiteDetect) ||
            (GetDrugUsageType() == DrugUsageType::SingleDoseParasiteDetect   ) )
        {
            if( p_mhc->CheckForParasitesWithTest( 1 ) )
            {
                keep = true;
            }
        }
        else if( (GetDrugUsageType() == DrugUsageType::FullTreatmentNewDetectionTech) ||
                 (GetDrugUsageType() == DrugUsageType::SingleDoseNewDetectionTech   ) )
        {
            if( p_mhc->CheckForParasitesWithTest( 2 ) )
            {
                keep = true;
            }
        }
        else if( (GetDrugUsageType() == DrugUsageType::FullTreatmentWhenSymptom) ||
                 (GetDrugUsageType() == DrugUsageType::SingleDoseWhenSymptom   ) )
        {
            if( p_mhc->HasFever() )
            {
                keep = true;
            }
        }
        else // not conditional, so just distribute the drug
        {
            keep = true;
        }

        if( keep )
        {
            if (s_OK != context->QueryInterface(GET_IID(IMalariaDrugEffectsApply), (void**)&imda) )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "IMalariaDrugEffectsApply", "IIndividualHumanInterventionsContext" );
            }

            // just add in another Drug to list, can later check the person's records and apply accordingly (TODO)
            return GenericDrug::Distribute( context, pCCO );
        }
        else
        {
            return false;
        }
    }

    void
    AntimalarialDrug::SetContextTo(
        IIndividualHumanContext *context
    )
    {
        release_assert( context );
        release_assert( context->GetInterventionsContext() );
        if (s_OK != context->GetInterventionsContext()->QueryInterface(GET_IID(IMalariaDrugEffectsApply), (void**)&imda) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context->GetInterventionsContext()", "IMalariaDrugEffectsApply", "IIndividualHumanInterventionsContext" );
        }

        return GenericDrug::SetContextTo( context );
    }

    int AntimalarialDrug::GetNumDoses() const
    {
        int num_doses = 0;
        if( (dosing_type == DrugUsageType::FullTreatmentCourse          ) ||
            (dosing_type == DrugUsageType::FullTreatmentNewDetectionTech) ||
            (dosing_type == DrugUsageType::FullTreatmentParasiteDetect  ) ||
            (dosing_type == DrugUsageType::FullTreatmentWhenSymptom     ) )
        {
            num_doses = pMalariaDrugTypeParameters->GetFullTreatmentDoses();
        }
        else if( (dosing_type == DrugUsageType::SingleDose                ) ||
                 (dosing_type == DrugUsageType::SingleDoseNewDetectionTech) ||
                 (dosing_type == DrugUsageType::SingleDoseParasiteDetect  ) ||
                 (dosing_type == DrugUsageType::SingleDoseWhenSymptom     ) )
        {
            num_doses = 1;
        }
        else if( dosing_type == (DrugUsageType::Prophylaxis) )
        {
            // hack for prophylaxis--person keeps taking 1000 doses, 
            // because this lasts for several years.  This will be 
            // replaced with a different usage pattern when we 
            // reintroduce compliance
            num_doses = 1000;
        }

        return num_doses;
    }

    void AntimalarialDrug::ConfigureDrugTreatment( IIndividualHumanInterventionsContext * ivc )
    {
        remaining_doses = GetNumDoses();
        time_between_doses = pMalariaDrugTypeParameters->GetDoseInterval();
        Cmax = pMalariaDrugTypeParameters->GetCMax();

        // Optional age-dependent dosing
        float bodyweight_exponent = pMalariaDrugTypeParameters->GetBodyWeightExponent();
        DoseMap::dose_map_t fractional_dose_by_upper_age = pMalariaDrugTypeParameters->GetDoseMap().fractional_dose_by_upper_age;
        if ( bodyweight_exponent > 0 || !fractional_dose_by_upper_age.empty() )
        {
            float age_in_days = ivc->GetParent()->GetEventContext()->GetAge();
            float bodyweight = BodyWeightByAge(age_in_days);

            auto it = fractional_dose_by_upper_age.upper_bound(age_in_days/DAYSPERYEAR);
            float fractional_dose = 1.0f;
            if (it != fractional_dose_by_upper_age.end())
            {
                fractional_dose = it->second;
            }

            float Cmax_multiplier = pow(_adult_bodyweight_kg/bodyweight, bodyweight_exponent) * fractional_dose;
            LOG_DEBUG_F("Age=%d yr, weight=%d kg, dosing=%0.2f, Cmax_mult=%0.2f\n", int(age_in_days/DAYSPERYEAR), int(bodyweight), fractional_dose, Cmax_multiplier);
            Cmax *= Cmax_multiplier;
        }

        Vd                      = pMalariaDrugTypeParameters->GetVd();
        drug_c50                = pMalariaDrugTypeParameters->GetPkpdC50();
        drug_IRBC_killrate      = pMalariaDrugTypeParameters->GetMaxDrugIRBCKill();
        drug_gametocyte02       = pMalariaDrugTypeParameters->GetKillRateGametocyte02();
        drug_gametocyte34       = pMalariaDrugTypeParameters->GetKillRateGametocyte34();
        drug_gametocyteM        = pMalariaDrugTypeParameters->GetKillRateGametocyteM();
        drug_hepatocyte         = pMalariaDrugTypeParameters->GetKillRateHepatocyte();
        pDrugResistantModifiers = &(pMalariaDrugTypeParameters->GetResistantModifiers());

        durability_time_profile = GET_CONFIGURABLE(SimulationConfig)->malaria_params->PKPD_model;
        fast_decay_time_constant = pMalariaDrugTypeParameters->GetDecayT1();
        slow_decay_time_constant = pMalariaDrugTypeParameters->GetDecayT2();

        PkPdParameterValidation();
    }

    void AntimalarialDrug::ResetForNextDose(float dt)
    {
        dosing_timer = time_between_doses;
        remaining_doses--;

        if (remaining_doses > 0 && time_between_doses < dt)
        {
            LOG_DEBUG_F("Time to next dose (%0.3f) is shorter than the time-step (%0.3f).\n", time_between_doses, dt);

            // Decrement remaining_doses further.  For example, if we are doing 1h timesteps for infected individuals but 24h for uninfected
            // and have 12h dosing (6 pills over 3 days), do 3 "double" pills over 3 days for 24h updaters instead of what otherwise would be 6 pills over 6 days
            int decrement_n_more_doses = int(dt/time_between_doses + 0.5)-1;
            remaining_doses -= min(remaining_doses, decrement_n_more_doses);
            LOG_DEBUG_F("Decrementing remaining doses by another %d\n", decrement_n_more_doses);
        }
    }

    void AntimalarialDrug::ApplyEffects()
    {
        assert(imda);

        imda->AddDrugEffects( this );
    }

    void AntimalarialDrug::Expire()
    {
        imda->RemoveDrugEffects( this );
        GenericDrug::Expire();
    }

    bodyweight_map_t AntimalarialDrug::create_bodyweight_map()
    {
        // C++11 initialization would be nicer
        std::map<float,float> m;

        // WHO 2006 growth chart (median)
        m[0.0f]    = 3.3464f;  // birth
        m[180.0f]  = 7.934f;   // 6mo
        m[365.0f]  = 9.6479f;  // 1yr
        m[730.0f]  = 12.1515f; // 2yr
        m[1460.0f] = 16.3489f; // 4yr

        // CDC growth chart (median)
        m[2190.0f] = 21.0f;    // 6yr
        m[2920.0f] = 26.0f;    // 8yr
        m[3650.0f] = 32.0f;    // 10yr
        m[4380.0f] = 40.5f;    // 12yr

        // 20yr (used in original Cmax fitting)
        m[7300.0f] = _adult_bodyweight_kg;

        return m;
    }

    float AntimalarialDrug::BodyWeightByAge(float age_in_days)
    {
        auto it = bodyweight_map_.upper_bound(age_in_days);

        if ( it == bodyweight_map_.end() )
        {
            return _adult_bodyweight_kg;
        }

        float upper_age = it->first;
        float upper_bw = it->second;
        if ( it == bodyweight_map_.begin() )
        {
            LOG_DEBUG("Upperbound is first element.\n");
            return upper_bw;
        }
        // Linear interpolation
        --it;
        float lower_age = it->first;
        float lower_bw = it->second;

        return lower_bw + (upper_bw-lower_bw)*(age_in_days-lower_age)/(upper_age-lower_age);
    }

    REGISTER_SERIALIZABLE(AntimalarialDrug);

    void AntimalarialDrug::serialize(IArchive& ar, AntimalarialDrug* obj)
    {
        GenericDrug::serialize(ar, obj);
        AntimalarialDrug& drug = *obj;
        ar.labelElement("dosing_type"       ) & (uint32_t&)drug.dosing_type;
        ar.labelElement("drug_IRBC_killrate") & drug.drug_IRBC_killrate;
        ar.labelElement("drug_hepatocyte"   ) & drug.drug_hepatocyte;
        ar.labelElement("drug_gametocyte02" ) & drug.drug_gametocyte02;
        ar.labelElement("drug_gametocyte34" ) & drug.drug_gametocyte34;
        ar.labelElement("drug_gametocyteM"  ) & drug.drug_gametocyteM;

        if( ar.IsReader() )
        {
            drug.pDrugResistantModifiers = &(GET_CONFIGURABLE( SimulationConfig )->malaria_params->MalariaDrugMap[ drug.drug_name ]->GetResistantModifiers());
        }
    }
}
