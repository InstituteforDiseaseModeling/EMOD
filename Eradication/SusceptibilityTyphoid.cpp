/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include <string>

#include "SusceptibilityTyphoid.h"
#include "IndividualTyphoid.h"
#include "SimulationConfig.h"
#include "RANDOM.h"

SETUP_LOGGING( "SusceptibilityTyphoid" )

#ifdef ENABLE_TYPHOID

namespace Kernel
{

    GET_SCHEMA_STATIC_WRAPPER_IMPL(Typhoid.Susceptibility, SusceptibilityTyphoidConfig)
        BEGIN_QUERY_INTERFACE_BODY(SusceptibilityTyphoidConfig)
    END_QUERY_INTERFACE_BODY(SusceptibilityTyphoidConfig)

    bool
    SusceptibilityTyphoidConfig::Configure(
        const Configuration* config
    )
    {
        LOG_DEBUG("Configure\n");
        return JsonConfigurable::Configure(config);
    }

    BEGIN_QUERY_INTERFACE_BODY(SusceptibilityTyphoid)
        HANDLE_INTERFACE(ISusceptibilityTyphoid)
        HANDLE_INTERFACE(ISusceptibilityTyphoidReportable)
        END_QUERY_INTERFACE_BODY(SusceptibilityTyphoid)

    /*
    SusceptibilityTyphoid::SusceptibilityTyphoid()
      : SusceptibilityEnvironmental()
    {
    }
    */

    SusceptibilityTyphoid::SusceptibilityTyphoid(IIndividualHumanContext *context)
    : SusceptibilityEnvironmental(context)
    {
        // Everything initialized to 0 in Initialize
    }

    void SusceptibilityTyphoid::Initialize(float _age, float _immmod, float _riskmod)
    {
        SusceptibilityEnvironmental::Initialize(_age, _immmod, _riskmod);

        if (_age == 0.0f)
        {
            mod_acquire = 0.0f;
            LOG_DEBUG_F("Newborn being made immune for now.\n");
            // throws exception on error, no return type. 
        }
        LOG_DEBUG_F("Initializing Typhoid immunity object for new individual: id=%lu, age=%f, immunity modifier=%f, risk modifier=%f\n", parent->GetSuid().data, _age, mod_acquire, _riskmod);
    }

    SusceptibilityTyphoid::~SusceptibilityTyphoid(void)
    {
    }

    SusceptibilityTyphoid *SusceptibilityTyphoid::CreateSusceptibility(IIndividualHumanContext *context, float age, float immmod, float riskmod)
    {
        //LOG_INFO_F( "Creating Typhoid immunity object for new individual: age=%f, immunity modifier=%f, risk modifier=%f\n", age, immmod, riskmod );
        SusceptibilityTyphoid *newsusceptibility = _new_ SusceptibilityTyphoid(context);
        newsusceptibility->Initialize(age, immmod, riskmod);

        return newsusceptibility;
    }

    void SusceptibilityTyphoid::Update(float dt)
    {
        age += dt; // tracks age for immune purposes
        // if cross N year age boundary

        //end of maternal antibodies at 6 months
        float age_boundary = 0.5f * DAYSPERYEAR;
        if( ( age >= age_boundary ) && ( ( age - dt ) < age_boundary ) && ( mod_acquire == 0 ) )
        {
            if( parent->GetRng()->SmartDraw( IndividualHumanTyphoidConfig::typhoid_6month_susceptible_fraction ) )
            {
                LOG_DEBUG_F("id %lu 6-month old being made susceptible.\n", parent->GetSuid().data);
                mod_acquire = 1.0f;
            }
        }

        float age_boundary_2 = 3 * DAYSPERYEAR;
        if( ( age >= age_boundary_2 ) && ( ( age - dt ) < age_boundary_2 ) && ( mod_acquire == 0 ) )
        {
            if( parent->GetRng()->SmartDraw( IndividualHumanTyphoidConfig::typhoid_3year_susceptible_fraction ) )
            {
                LOG_DEBUG_F("id %lu 3-yo being made susceptible.\n", parent->GetSuid().data);
                mod_acquire = 1.0f;
            }
        }

        float age_boundary_3 = 6 * DAYSPERYEAR;
        if( ( age >= age_boundary_3 ) && ( ( age - dt ) < age_boundary_3 ) && ( mod_acquire == 0 ) )
        {
            if( parent->GetRng()->SmartDraw( IndividualHumanTyphoidConfig::typhoid_6year_susceptible_fraction ) )
            {
                LOG_DEBUG_F("id %lu 6-yo being made susceptible.\n", parent->GetSuid().data);
                mod_acquire = 1.0f;
            }
        } 

#define SUSPCEPT_INTRO_YEAR (20)
#define LAMBDA_THRESHOLD (1000)
        if (IndividualHumanTyphoidConfig::typhoid_exposure_lambda < LAMBDA_THRESHOLD )
        {
            float twenty_years_old_days = SUSPCEPT_INTRO_YEAR*DAYSPERYEAR;
            if( ( age < twenty_years_old_days ) && mod_acquire == 0 )
            {
                double perc = 0.0f;
                double perc1 = 0.0f;
                double perc2 = 0.0f;
                double num = 0.0f;
                double denom = 0.0f;
                float lambda = IndividualHumanTyphoidConfig::typhoid_exposure_lambda; 

                perc2 = 1.0f - ((twenty_years_old_days - age) / (age*lambda + twenty_years_old_days ));
                perc1 = 1.0f - ((twenty_years_old_days - (age-1)) / ((age-1)*lambda + twenty_years_old_days ));
                perc = (perc2 - perc1) / (1 - perc1);

                if( parent->GetRng()->SmartDraw( perc ) )
                {
                    mod_acquire = 1.0f;
                    LOG_VALID_F("id %lu , age %f being made susceptible.\n", parent->GetSuid().data, age);
                    //LOG_INFO_F("SUSCEPTIBLE %f %f\n", perc, mod_acquire);
                }
            } 
        }

        float intro_year = 10; // magic number?
        if (IndividualHumanTyphoidConfig::typhoid_exposure_lambda < LAMBDA_THRESHOLD )
        {
            intro_year = SUSPCEPT_INTRO_YEAR;
        }
        float age_boundary_4 = intro_year * DAYSPERYEAR;
        if (age >= age_boundary_4 && age - dt < age_boundary_4 && mod_acquire == 0)
        {
            LOG_DEBUG_F("id %lu Schoolkids being made susceptible.\n", parent->GetSuid().data);
            mod_acquire = 1.0f;
        } 

        if( ( age > age_boundary_4 + dt ) && ( mod_acquire == 0 ) )
        {
            LOG_INFO_F("SOMEONE WAS MISSED AGE %f CUTOFF DAY %f MODACQUIRE %f\n", age, age_boundary_4 + dt, mod_acquire);
        }
    }

}

#endif // ENABLE_TYPHOID
