/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "RANDOM.h"
#include "Log.h"
#include "ReportUtilitiesMalaria.h"

SETUP_LOGGING("ReportUtilitiesMalaria")

using namespace Kernel;

// Evaluates polynomial of form : f(x) = p( 0 ) * x ^ (n - 1) + p( 1 ) *x ^ (n - 2)…
// where n = the length of the vector p
float ReportUtilitiesMalaria::PolyVal( const std::vector<double>& p, float x )
{
    float px = 0.0;
    int n = p.size();

    for (int i = 0; i < n; i++)
    {
        px += p[i] * pow(x, n - (i + 1));
    }

    return px;
}

// Converting +ve microscopic fields of view out of 200 slide views to parasite densities
float ReportUtilitiesMalaria::FieldsOfViewToDensity( float positive_fields )
{
    float PfPR_smeared = 0.0;
    float uL_per_field = float( 0.5 ) / float( 200.0 );
    if( positive_fields != 200 )
    {
        PfPR_smeared = -(1.0 / uL_per_field) * log( 1 - positive_fields / 200.0 );  //expected density given positive fields
    }
    else
    {
        PfPR_smeared = FLT_MAX;
    }

    return PfPR_smeared;
}

// Adding uncertainty to PCR derived densities a la Walker et al. (2015)
float ReportUtilitiesMalaria::NASBADensityWithUncertainty( RANDOMBASE * rng, float true_density )
{
    if( true_density <= 1.0e-4 )
    {
        return 0;
    }

    // ---------------------------------------------------------------------------------------------------------------------
    // --- The following algorithm is based on the following paper:
    // ---
    // --- "Improving statistical inference on pathogen densities estimated by quantitative molecular methods :
    // --- malaria gametocytaemia as a case study",
    // --- Martin Walker, Maria - Gloria Basanez, Andre Lin Ouedraogo, Cornelus Hermsen, Teun Bousema and Thomas S Churcher
    // --- BMC Bioinformatics 2015 16:5.
    // ---
    // --- https://doi.org/10.1186/s12859-014-0402-2
    // ---------------------------------------------------------------------------------------------------------------------

    std::vector<double> poly = { 2.30465631e-03, -4.63489922e-02, 4.86245414e-01, -2.84016039e+00, 7.40082984e+00 }; // From Walker et al.
    float log10_true_densities = log10(true_density);

    float log10_true_densities_pv = log10_true_densities;
    if( log10_true_densities_pv > 3.0 )
    {
        // Cap the value for PolyVal so values don't get too large.
        log10_true_densities_pv = 3.0;
    }

    float log10_sigma = ReportUtilitiesMalaria::PolyVal( poly, log10_true_densities_pv + 3 ) / 2.0; // +3 for ml to uL, 2 for 95% CI --> 1-sigma

    float random_variable = rng->eGauss();
    float log10_measured_densities = log10_true_densities + log10_sigma * random_variable;

    float measured_densities = 0.0;
    // Cap the minimum value so it doesn't get too small
    if( log10_measured_densities >= -3.0 )
    {
        measured_densities = pow(10, log10_measured_densities);
    }

    return measured_densities;
}

// Smears infectiousness values using a binomial draw given 40 draws
float ReportUtilitiesMalaria::BinomialInfectiousness( RANDOMBASE * rng, float infec )
{
    if (infec <= 0)
    {
        return 0;
    }

    float n_draws = 40.0;
    float infec_smeared = rng->binomial_approx2(n_draws, infec);

    if (infec_smeared / n_draws < 1e-4)
    {
        infec_smeared = 0.0;
    }

    return infec_smeared / n_draws;
}
