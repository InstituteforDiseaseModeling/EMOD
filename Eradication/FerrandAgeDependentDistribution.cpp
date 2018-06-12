/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "math.h"
#include "Common.h"
#include "Log.h"
#include "Debug.h"

#include "FerrandAgeDependentDistribution.h"

SETUP_LOGGING( "FerrandAgeDependentDistribution" )

namespace Kernel
{
    GET_SCHEMA_STATIC_WRAPPER_IMPL(FerrandAgeDependentDistribution,FerrandAgeDependentDistribution)

    BEGIN_QUERY_INTERFACE_BODY(FerrandAgeDependentDistribution)
    END_QUERY_INTERFACE_BODY(FerrandAgeDependentDistribution)


    FerrandAgeDependentDistribution::FerrandAgeDependentDistribution()
    : JsonConfigurable()
    , m_child_beta(1.52f)
    , m_child_s(2.7f)
    , m_child_mu(16.0f)
    , m_child_alpha(0.57f)
    , m_child_age_years(15.0f)
    , m_max_age_years(70.0f)
    , m_adult_lambda_slope(-0.2717f)
    , m_adult_lambda_intercept(21.182f)
    , m_adult_kappa_intercept(2.0f)
    {
        // Ferrand, R. A. et al. AIDS among older children and adolescents in Southern Africa:
        // projecting the time course and magnitude of the epidemic. AIDS 23, 2039–2046 (2009).
        initConfigTypeMap( "HIV_Child_Survival_Rapid_Progressor_Rate",     &m_child_beta,             HIV_Child_Survival_Rapid_Progressor_Rate_DESC_TEXT    ,       0.0f,   1000.0f,  1.52f   );
        initConfigTypeMap( "HIV_Child_Survival_Slow_Progressor_Shape",     &m_child_s,                HIV_Child_Survival_Slow_Progressor_Shape_DESC_TEXT    ,       0.001f, 1000.0f,  2.7f    );
        initConfigTypeMap( "HIV_Child_Survival_Slow_Progressor_Scale",     &m_child_mu,               HIV_Child_Survival_Slow_Progressor_Scale_DESC_TEXT    ,       0.001f, 1000.0f, 16.0f    );
        initConfigTypeMap( "HIV_Child_Survival_Rapid_Progressor_Fraction", &m_child_alpha,            HIV_Child_Survival_Rapid_Progressor_Fraction_DESC_TEXT,       0.0f,      1.0f,  0.57f   );

        initConfigTypeMap( "HIV_Age_Max_for_Child_Survival_Function",      &m_child_age_years,        HIV_Age_Max_for_Child_Survival_Function_DESC_TEXT     ,       0.0f,     75.0f, 15.0f    );
        initConfigTypeMap( "HIV_Age_Max_for_Adult_Age_Dependent_Survival", &m_max_age_years,          HIV_Age_Max_for_Adult_Age_Dependent_Survival_DESC_TEXT,       0.0f,     75.0f, 70.0f    );

        initConfigTypeMap( "HIV_Adult_Survival_Scale_Parameter_Slope",     &m_adult_lambda_slope,     HIV_Adult_Survival_Scale_Parameter_Slope_DESC_TEXT    , -1000.0f,     1000.0f, -0.2717f );
        initConfigTypeMap( "HIV_Adult_Survival_Scale_Parameter_Intercept", &m_adult_lambda_intercept, HIV_Adult_Survival_Scale_Parameter_Intercept_DESC_TEXT,       0.001f, 1000.0f, 21.182f  );

        // Todd et al. assumes constant kappa = 2
        initConfigTypeMap( "HIV_Adult_Survival_Shape_Parameter",           &m_adult_kappa_intercept,  HIV_Adult_Survival_Shape_Parameter_DESC_TEXT          ,       0.001f, 1000.0f,  2.0f    );
    }

    bool FerrandAgeDependentDistribution::Configure( const Configuration* pInputJson )
    {
        return JsonConfigurable::Configure( pInputJson );
    }

    // takes in age in days and returns age in days
    float FerrandAgeDependentDistribution::invcdf(float rand, float age) 
    {
        float age_in_years = age / DAYSPERYEAR;
        float invcdf = 0.0;
        float prob = 0.0;

        if (age_in_years < m_child_age_years)
        {
            if (rand < m_child_alpha) 
            {
                prob = rand/m_child_alpha;
                invcdf = -log(prob)/m_child_beta;
            }
            else 
            {
                prob = (rand - m_child_alpha)/(1 - m_child_alpha);
                invcdf = m_child_mu * pow( -log(prob)/log(2.0f), 1/m_child_s);
            }
        } 
        else 
        {
            if(age_in_years > m_max_age_years)
            {
                age_in_years = m_max_age_years; 
            }

            prob = rand;
            float lambda = m_adult_lambda_slope * age_in_years + m_adult_lambda_intercept;
            float kappa =  m_adult_kappa_intercept;
            invcdf =  DistributionInvCDF(prob, lambda, kappa);
        }

        invcdf *= DAYSPERYEAR;

        LOG_DEBUG_F( "%s returning prognosis of %f (years) based on current age of %f (years)\n", __FUNCTION__, invcdf/DAYSPERYEAR, age_in_years );
        return invcdf;
    }


    float FerrandAgeDependentDistribution::DistributionInvCDF(float prob, float lambda, float kappa)
    {
        release_assert( prob != 1.0f );
        release_assert( kappa != 0.0 );

        return lambda*pow(-log(1.0f-prob), 1.0f/kappa);
    }

}