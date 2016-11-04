/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "AntiMalarialDrug.h"

#include "Exceptions.h"
#include "IndividualEventContext.h"
#include "MalariaDrugTypeParameters.h"
#include "MalariaInterventionsContainer.h"  // for IMalariaDrugEffectsApply methods
#include "SimulationConfig.h"               // for global-context access to PKPDmodel (!)
#include "MalariaParameters.h"

static const char * _module = "AntimalarialDrug";

namespace Kernel
{
    const float AntimalarialDrug::_adult_bodyweight_kg = 50.0f;
    const bodyweight_map_t AntimalarialDrug::bodyweight_map_ = create_bodyweight_map();

    BEGIN_QUERY_INTERFACE_DERIVED(AntimalarialDrug, GenericDrug)
        //HANDLE_INTERFACE(IMalariaDrugEffects)
    END_QUERY_INTERFACE_DERIVED(AntimalarialDrug, GenericDrug)
    IMPLEMENT_FACTORY_REGISTERED(AntimalarialDrug)

    AntimalarialDrug::~AntimalarialDrug()
    { }

    float
    AntimalarialDrug::get_drug_IRBC_killrate()
    {
        return current_efficacy * drug_IRBC_killrate;
    }

    float AntimalarialDrug::get_drug_hepatocyte()
    {
        return current_efficacy * drug_hepatocyte;
    }

    float
    AntimalarialDrug::get_drug_gametocyte02()
    {
        return current_efficacy * drug_gametocyte02;
    }

    float
    AntimalarialDrug::get_drug_gametocyte34()
    {
        return current_efficacy * drug_gametocyte34;
    }

    float
    AntimalarialDrug::get_drug_gametocyteM()
    {
        return current_efficacy * drug_gametocyteM;
    }

    std::string
    AntimalarialDrug::GetDrugName() const
    {
        return drug_type;
    }

    bool
    AntimalarialDrug::Configure(
        const Configuration * inputJson
    )
    {
        initConfigTypeMap("Cost_To_Consumer", &cost_per_unit, AMDRUG_Cost_To_Consumer_DESC_TEXT, 0, 999999, 10);
        drug_type.constraints = "<configuration>:Malaria_Drug_Params.*";
        // would be nice to be able to set constraint_params so that this ConstrainedString could
        // validate itself, but not there yet.
        initConfigTypeMap( "Drug_Type", &drug_type, AMDRUG_Drug_Type_DESC_TEXT);
        initConfig( "Dosing_Type", dosing_type, inputJson, MetadataDescriptor::Enum("Dosing_Type", AMDRUG_Dosing_Type_DESC_TEXT, MDD_ENUM_ARGS(DrugUsageType)) );
        return JsonConfigurable::Configure( inputJson );
    }

    AntimalarialDrug::AntimalarialDrug()
        : GenericDrug(),
        drug_IRBC_killrate(0),
        drug_hepatocyte(0),
        drug_gametocyte02(0),
        drug_gametocyte34(0),
        drug_gametocyteM(0),
        imda(nullptr)
    {
        initSimTypes( 1, "MALARIA_SIM" );
    }

    bool
    AntimalarialDrug::Distribute(
        IIndividualHumanInterventionsContext *context,
        ICampaignCostObserver * pCCO
    )
    {
        if (s_OK != context->QueryInterface(GET_IID(IMalariaDrugEffectsApply), (void**)&imda) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "IMalariaDrugEffectsApply", "IIndividualHumanInterventionsContext" );
        }

        // just add in another Drug to list, can later check the person's records and apply accordingly (TODO)
        return GenericDrug::Distribute( context, pCCO );
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

    void AntimalarialDrug::ConfigureDrugTreatment( IIndividualHumanInterventionsContext * ivc )
    {
        auto mdtMap = GET_CONFIGURABLE(SimulationConfig)->malaria_params->MalariaDrugMap;
        if( mdtMap.find( drug_type ) == mdtMap.end() )
        {
            throw BadMapKeyException( __FILE__, __LINE__, __FUNCTION__, "mdtMap", drug_type.c_str() );
        }

        auto drug_params = mdtMap.at( drug_type );

        if(dosing_type == (DrugUsageType::FullTreatmentCourse) ||
           dosing_type == (DrugUsageType::FullTreatmentNewDetectionTech) ||
           dosing_type == (DrugUsageType::FullTreatmentParasiteDetect) ||
           dosing_type == (DrugUsageType::FullTreatmentWhenSymptom))
        {
            remaining_doses = drug_params->drug_fulltreatment_doses;
        }
        else if(dosing_type == (DrugUsageType::SingleDose) ||
                dosing_type == (DrugUsageType::SingleDoseNewDetectionTech) ||
                dosing_type == (DrugUsageType::SingleDoseParasiteDetect) ||
                dosing_type == (DrugUsageType::SingleDoseWhenSymptom))
        {
            remaining_doses = 1;
        }
        else if (dosing_type == (DrugUsageType::Prophylaxis))
        {
            remaining_doses = 1000; //hack for prophylaxis--person keeps taking 1000 doses, because this lasts for several years.  This will be replaced with a different usage pattern when we reintroduce compliance
        }

        time_between_doses = drug_params->drug_dose_interval;
        Cmax = drug_params->drug_Cmax;

        // Optional age-dependent dosing
        float bodyweight_exponent = drug_params->bodyweight_exponent;
        DoseMap::dose_map_t fractional_dose_by_upper_age = drug_params->dose_map.fractional_dose_by_upper_age;
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

        Vd = drug_params->drug_Vd;
        drug_c50 = drug_params->drug_pkpd_c50;
        drug_IRBC_killrate = drug_params->max_drug_IRBC_kill;
        drug_gametocyte02  = drug_params->drug_gametocyte02_killrate;
        drug_gametocyte34  = drug_params->drug_gametocyte34_killrate;
        drug_gametocyteM   = drug_params->drug_gametocyteM_killrate;
        drug_hepatocyte    = drug_params->drug_hepatocyte_killrate;

        durability_time_profile = GET_CONFIGURABLE(SimulationConfig)->malaria_params->PKPD_model;
        fast_decay_time_constant = drug_params->drug_decay_T1;
        slow_decay_time_constant = drug_params->drug_decay_T2;

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
        imda->ApplyDrugVaccineReducedAcquireEffect( GetDrugReducedAcquire() );
        imda->ApplyDrugVaccineReducedTransmitEffect( GetDrugReducedTransmit() );
        imda->ApplyDrugIRBCKillRateEffect( get_drug_IRBC_killrate() );
        imda->ApplyDrugHepatocyteEffect( get_drug_hepatocyte() );
        imda->ApplyDrugGametocyte02Effect( get_drug_gametocyte02() );
        imda->ApplyDrugGametocyte34Effect( get_drug_gametocyte34() );
        imda->ApplyDrugGametocyteMEffect( get_drug_gametocyteM() );
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
        ar.labelElement("drug_IRBC_killrate") & drug.drug_IRBC_killrate;
        ar.labelElement("drug_hepatocyte") & drug.drug_hepatocyte;
        ar.labelElement("drug_gametocyte02") & drug.drug_gametocyte02;
        ar.labelElement("drug_gametocyte34") & drug.drug_gametocyte34;
        ar.labelElement("drug_gametocyteM") & drug.drug_gametocyteM;
        ar.labelElement("drug_type") & (std::string&)drug.drug_type;
    }
}
