
#include "stdafx.h"
#include "DrugModelAntiMalarial.h"

#include "Exceptions.h"
#include "IIndividualHumanContext.h"
#include "IndividualEventContext.h"
#include "MalariaDrugTypeParameters.h"
#include "Interventions.h"

SETUP_LOGGING( "DrugModelAntiMalarial" )

namespace Kernel
{
    const float DrugModelAntiMalarial::_adult_bodyweight_kg = 50.0f;
    const DrugModelAntiMalarial::bodyweight_map_t DrugModelAntiMalarial::bodyweight_map_ = create_bodyweight_map();

    BEGIN_QUERY_INTERFACE_DERIVED( DrugModelAntiMalarial, DrugModel )
        HANDLE_INTERFACE( IMalariaDrugEffects )
    END_QUERY_INTERFACE_DERIVED( DrugModelAntiMalarial, DrugModel )

    DrugModelAntiMalarial::DrugModelAntiMalarial( const std::string& rName, const MalariaDrugTypeParameters* pParams )
        : DrugModel( rName )
        , pMalariaDrugTypeParameters( pParams )
    {
    }

    DrugModelAntiMalarial::~DrugModelAntiMalarial()
    {
    }

    DrugModel* DrugModelAntiMalarial::Clone()
    {
        return new DrugModelAntiMalarial( *this );
    }

    float DrugModelAntiMalarial::CalculateModifiedEfficacy( const IStrainIdentity& rStrain )
    {
        float modifier = 1.0;
        if( durability_time_profile == PKPDModel::CONCENTRATION_VERSUS_TIME )
        {
            modifier = GetDrugParams()->GetResistantModifiers().GetC50( rStrain );
        }
        return CalculateEfficacy( modifier * this->drug_c50, start_concentration, end_concentration );
    }

    float DrugModelAntiMalarial::get_drug_IRBC_killrate( const IStrainIdentity& rStrain )
    {
        float modifier = GetDrugParams()->GetResistantModifiers().GetMaxKilling( rStrain );
        float efficacy = CalculateModifiedEfficacy( rStrain );
        return modifier * efficacy * GetDrugParams()->GetMaxDrugIRBCKill();
    }

    float DrugModelAntiMalarial::get_drug_hepatocyte( const IStrainIdentity& rStrain )
    {
        float efficacy = CalculateModifiedEfficacy( rStrain );
        return efficacy * GetDrugParams()->GetKillRateHepatocyte();
    }

    float DrugModelAntiMalarial::get_drug_gametocyte02( const IStrainIdentity& rStrain )
    {
        float efficacy = CalculateModifiedEfficacy( rStrain );
        return efficacy * GetDrugParams()->GetKillRateGametocyte02();
    }

    float DrugModelAntiMalarial::get_drug_gametocyte34( const IStrainIdentity& rStrain )
    {
        float efficacy = CalculateModifiedEfficacy( rStrain );
        return efficacy * GetDrugParams()->GetKillRateGametocyte34();
    }

    float DrugModelAntiMalarial::get_drug_gametocyteM( const IStrainIdentity& rStrain )
    {
        float efficacy = CalculateModifiedEfficacy( rStrain );
        return efficacy * GetDrugParams()->GetKillRateGametocyteM();
    }

    void DrugModelAntiMalarial::ConfigureDrugTreatment( IIndividualHumanInterventionsContext * ivc )
    {
        Cmax = GetDrugParams()->GetCMax();

        // Optional age-dependent dosing
        float bodyweight_exponent = GetDrugParams()->GetBodyWeightExponent();
        const DoseMap& r_dose_map = GetDrugParams()->GetDoseMap();
        if( bodyweight_exponent > 0 || (r_dose_map.Size() > 0) )
        {
            float age_in_days = ivc->GetParent()->GetEventContext()->GetAge();
            float bodyweight = BodyWeightByAge( age_in_days );

            float fractional_dose = r_dose_map.GetFractionalDose( age_in_days );

            float Cmax_multiplier = pow( _adult_bodyweight_kg / bodyweight, bodyweight_exponent ) * fractional_dose;
            LOG_DEBUG_F( "Age=%d yr, weight=%d kg, dosing=%0.2f, Cmax_mult=%0.2f\n", int( age_in_days / DAYSPERYEAR ), int( bodyweight ), fractional_dose, Cmax_multiplier );
            Cmax *= Cmax_multiplier;
        }

        // Set other DrugModel parameters to those in MalariaDrugTypeParameters
        durability_time_profile  = GetDrugParams()->GetPKPDModel();
        Vd                       = GetDrugParams()->GetVd();
        drug_c50                 = GetDrugParams()->GetPkpdC50();
        fast_decay_time_constant = GetDrugParams()->GetDecayT1();
        slow_decay_time_constant = GetDrugParams()->GetDecayT2();

        PkPdParameterValidation();
    }

    DrugModelAntiMalarial::bodyweight_map_t DrugModelAntiMalarial::create_bodyweight_map()
    {
        // C++11 initialization would be nicer
        std::map<float, float> m;

        // WHO 2006 growth chart (median)
        m[ 0.0f ] = 3.3464f;  // birth
        m[ 180.0f ] = 7.934f;   // 6mo
        m[ 365.0f ] = 9.6479f;  // 1yr
        m[ 730.0f ] = 12.1515f; // 2yr
        m[ 1460.0f ] = 16.3489f; // 4yr

                                 // CDC growth chart (median)
        m[ 2190.0f ] = 21.0f;    // 6yr
        m[ 2920.0f ] = 26.0f;    // 8yr
        m[ 3650.0f ] = 32.0f;    // 10yr
        m[ 4380.0f ] = 40.5f;    // 12yr

                                 // 20yr (used in original Cmax fitting)
        m[ 7300.0f ] = _adult_bodyweight_kg;

        return m;
    }

    float DrugModelAntiMalarial::BodyWeightByAge( float age_in_days )
    {
        auto it = bodyweight_map_.upper_bound( age_in_days );

        if( it == bodyweight_map_.end() )
        {
            return _adult_bodyweight_kg;
        }

        float upper_age = it->first;
        float upper_bw = it->second;
        if( it == bodyweight_map_.begin() )
        {
            LOG_DEBUG( "Upperbound is first element.\n" );
            return upper_bw;
        }
        // Linear interpolation
        --it;
        float lower_age = it->first;
        float lower_bw = it->second;

        return lower_bw + (upper_bw - lower_bw)*(age_in_days - lower_age) / (upper_age - lower_age);
    }

    const MalariaDrugTypeParameters* DrugModelAntiMalarial::GetDrugParams()
    {
        // could be nullptr if was created from serialization
        if( pMalariaDrugTypeParameters == nullptr )
        {
            pMalariaDrugTypeParameters = &(MalariaDrugTypeCollection::GetInstance()->GetDrug( drug_name ));
        }
        return pMalariaDrugTypeParameters;
    }


    REGISTER_SERIALIZABLE( DrugModelAntiMalarial );

    void DrugModelAntiMalarial::serialize( IArchive& ar, DrugModelAntiMalarial* obj )
    {
        DrugModel::serialize( ar, obj );
        DrugModelAntiMalarial& drug = *obj;

        // let GetDrugParams() set pMalariaDrugTypeParameters when the config has been read
    }
}
