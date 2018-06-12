/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "BinnedReportMalaria.h"

#include <map>

#include "Environment.h"
#include "Exceptions.h"
#include "Sugar.h"
#include "Malaria.h"
#include "MalariaContexts.h"

using namespace std;
using namespace json;

namespace Kernel {

SETUP_LOGGING( "BinnedReportMalaria" )

IReport*
BinnedReportMalaria::CreateReport()
{
    return new BinnedReportMalaria();
}

// Derived constructor calls base constructor to initialized reduced timesteps etc. 
BinnedReportMalaria::BinnedReportMalaria()
    : BinnedReport()
    , parasite_positive_bins(nullptr)
    , fever_positive_bins(nullptr)
    , mean_parasitemia_bins(nullptr)
    , new_diagnostic_prev_bins(nullptr)
    , new_clinical_cases_bins(nullptr)
    , new_severe_cases_bins(nullptr)
    , msp_variant_bins(nullptr)
    , nonspec_variant_bins(nullptr)
    , pfemp1_variant_bins(nullptr)
    , ss_msp_variant_bins(nullptr)
    , ss_nonspec_variant_bins(nullptr)
    , ss_pfemp1_variant_bins(nullptr)
{
}

BinnedReportMalaria::~BinnedReportMalaria()
{
    delete[] parasite_positive_bins;
    delete[] fever_positive_bins;
    delete[] mean_parasitemia_bins;
    delete[] new_diagnostic_prev_bins;
    delete[] new_clinical_cases_bins;
    delete[] new_severe_cases_bins;
    delete[] msp_variant_bins;
    delete[] nonspec_variant_bins;
    delete[] pfemp1_variant_bins;
    delete[] ss_msp_variant_bins;
    delete[] ss_nonspec_variant_bins;
    delete[] ss_pfemp1_variant_bins;
}

void BinnedReportMalaria::initChannelBins()
{
    BinnedReport::initChannelBins();

    parasite_positive_bins   = new float[num_total_bins];
    fever_positive_bins      = new float[num_total_bins];
    mean_parasitemia_bins    = new float[num_total_bins];
    new_diagnostic_prev_bins = new float[num_total_bins];
    new_clinical_cases_bins  = new float[num_total_bins];
    new_severe_cases_bins    = new float[num_total_bins];
    msp_variant_bins         = new float[num_total_bins];
    nonspec_variant_bins     = new float[num_total_bins];
    pfemp1_variant_bins      = new float[num_total_bins];
    ss_msp_variant_bins      = new float[num_total_bins];
    ss_nonspec_variant_bins  = new float[num_total_bins];
    ss_pfemp1_variant_bins   = new float[num_total_bins];

    clearChannelsBins();
}

void BinnedReportMalaria::clearChannelsBins()
{
    memset(parasite_positive_bins  , 0, num_total_bins * sizeof(float));
    memset(fever_positive_bins     , 0, num_total_bins * sizeof(float));
    memset(mean_parasitemia_bins   , 0, num_total_bins * sizeof(float));
    memset(new_diagnostic_prev_bins, 0, num_total_bins * sizeof(float));
    memset(new_clinical_cases_bins , 0, num_total_bins * sizeof(float));
    memset(new_severe_cases_bins   , 0, num_total_bins * sizeof(float));
    memset(msp_variant_bins        , 0, num_total_bins * sizeof(float));
    memset(nonspec_variant_bins    , 0, num_total_bins * sizeof(float));
    memset(pfemp1_variant_bins     , 0, num_total_bins * sizeof(float));
    memset(ss_msp_variant_bins     , 0, num_total_bins * sizeof(float));
    memset(ss_nonspec_variant_bins , 0, num_total_bins * sizeof(float));
    memset(ss_pfemp1_variant_bins  , 0, num_total_bins * sizeof(float));
}

void BinnedReportMalaria::EndTimestep( float currentTime, float dt )
{
    Accumulate("Parasite Positive", parasite_positive_bins);
    Accumulate("Fever Positive", fever_positive_bins);
    Accumulate("Mean Parasitemia", mean_parasitemia_bins);
    Accumulate("New Diagnostic Prevalence", new_diagnostic_prev_bins);
    Accumulate("New Clinical Cases", new_clinical_cases_bins);
    Accumulate("New Severe Cases", new_severe_cases_bins);
    Accumulate("Sum MSP Variant Fractions", msp_variant_bins);
    Accumulate("Sum Non-Specific Variant Fractions", nonspec_variant_bins);
    Accumulate("Sum PfEMP1 Variant Fractions", pfemp1_variant_bins);
    Accumulate("Sum of Squared MSP Variant Fractions", ss_msp_variant_bins);
    Accumulate("Sum of Squared Non-Specific Variant Fractions", ss_nonspec_variant_bins);
    Accumulate("Sum of Squared PfEMP1 Variant Fractions", ss_pfemp1_variant_bins);
    
    BinnedReport::EndTimestep( currentTime, dt );

    clearChannelsBins();
}

void BinnedReportMalaria::LogIndividualData( Kernel::IIndividualHuman* individual )
{
    LOG_DEBUG( "BinnedReportMalaria::LogIndividualData\n" );

    BinnedReport::LogIndividualData(individual);

    // Get individual weight and bin variables
    float mc_weight    = float(individual->GetMonteCarloWeight());

    int bin_index = calcBinIndex(individual);
    LOG_DEBUG_F( "bin_index = %d\n", bin_index );

    Kernel::IMalariaHumanContext* individual_malaria = nullptr;
    if( individual->QueryInterface( GET_IID( Kernel::IMalariaHumanContext), (void**) &individual_malaria ) != Kernel::s_OK )
    {
        throw Kernel::QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "individual", "IndividualHuman", "IMalariaHumanContext" );
    }

    Kernel::IMalariaSusceptibility* susc_malaria = individual_malaria->GetMalariaSusceptibilityContext();

    float msp_frac     = susc_malaria->get_fraction_of_variants_with_antibodies(MalariaAntibodyType::MSP1);
    float nonspec_frac = susc_malaria->get_fraction_of_variants_with_antibodies(MalariaAntibodyType::PfEMP1_minor);
    float pfemp1_frac  = susc_malaria->get_fraction_of_variants_with_antibodies(MalariaAntibodyType::PfEMP1_major);
    msp_variant_bins[bin_index]     += mc_weight * msp_frac;
    nonspec_variant_bins[bin_index] += mc_weight * nonspec_frac;
    pfemp1_variant_bins[bin_index]  += mc_weight * pfemp1_frac;
    ss_msp_variant_bins[bin_index]     += mc_weight * msp_frac * msp_frac;
    ss_nonspec_variant_bins[bin_index] += mc_weight * nonspec_frac * nonspec_frac;
    ss_pfemp1_variant_bins[bin_index]  += mc_weight * pfemp1_frac * pfemp1_frac;

    if (individual->IsInfected())
    {
        float tempParasiteCount = individual_malaria->CheckParasiteCountWithTest(MALARIA_TEST_BLOOD_SMEAR);

        if ( tempParasiteCount > 0 )
        {
            parasite_positive_bins[bin_index] += mc_weight;
            mean_parasitemia_bins[bin_index] += mc_weight * log(tempParasiteCount);
        }

        if(individual_malaria->CheckForParasitesWithTest(MALARIA_TEST_NEW_DIAGNOSTIC))
            new_diagnostic_prev_bins[bin_index] += mc_weight;

        if ( individual_malaria->HasFever() )
            fever_positive_bins[bin_index] += mc_weight;

        if ( individual_malaria->HasClinicalSymptom(ClinicalSymptomsEnum::CLINICAL_DISEASE) )
            new_clinical_cases_bins[bin_index] += mc_weight;

        if ( individual_malaria->HasClinicalSymptom(ClinicalSymptomsEnum::SEVERE_DISEASE) )
            new_severe_cases_bins[bin_index] += mc_weight;
    }
}

void BinnedReportMalaria::postProcessAccumulatedData()
{
    normalizeChannel("Mean Parasitemia", "Parasite Positive");
    channelDataMap.ExponentialValues( "Mean Parasitemia" );
}

}
