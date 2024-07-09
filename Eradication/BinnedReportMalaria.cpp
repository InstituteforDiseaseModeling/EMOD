
#include "stdafx.h"
#include "BinnedReportMalaria.h"

#include <map>

#include "Environment.h"
#include "Exceptions.h"
#include "Sugar.h"
#include "Malaria.h"
#include "MalariaContexts.h"
#include "ReportUtilitiesMalaria.h"

using namespace std;
using namespace json;

namespace Kernel {

SETUP_LOGGING( "BinnedReportMalaria" )


static const string _mean_parasitemia_label( "Mean Parasitemia" );

BEGIN_QUERY_INTERFACE_BODY( BinnedReportMalaria )
    HANDLE_INTERFACE( IReportMalariaDiagnostics )
    HANDLE_ISUPPORTS_VIA( IReport )
END_QUERY_INTERFACE_BODY( BinnedReportMalaria )

IReport*
BinnedReportMalaria::CreateReport()
{
    return new BinnedReportMalaria();
}

// Derived constructor calls base constructor to initialized reduced timesteps etc. 
BinnedReportMalaria::BinnedReportMalaria()
    : BinnedReport()
    , m_DetectionThresholds()
    , infection_detected_bins()
    , mean_parasitemia_bins(nullptr)
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
    for( auto& r_pair_name_bins : infection_detected_bins )
    {
        delete[] r_pair_name_bins.second;
    }
    delete[] mean_parasitemia_bins;
    delete[] new_clinical_cases_bins;
    delete[] new_severe_cases_bins;
    delete[] msp_variant_bins;
    delete[] nonspec_variant_bins;
    delete[] pfemp1_variant_bins;
    delete[] ss_msp_variant_bins;
    delete[] ss_nonspec_variant_bins;
    delete[] ss_pfemp1_variant_bins;
}

void BinnedReportMalaria::SetDetectionThresholds( const std::vector<float>& rDetectionThresholds )
{
    m_DetectionThresholds = rDetectionThresholds;

}

void BinnedReportMalaria::initChannelBins()
{
    BinnedReport::initChannelBins();

    infection_detected_bins.push_back( std::make_pair( "Blood Smear Parasite Positive"  , new float[ num_total_bins ] ) );
    infection_detected_bins.push_back( std::make_pair( "Blood Smear Gametocyte Positive", new float[ num_total_bins ] ) );
    infection_detected_bins.push_back( std::make_pair( "PCR Parasites Positive"         , new float[ num_total_bins ] ) );
    infection_detected_bins.push_back( std::make_pair( "PCR Gametocytes Positive"       , new float[ num_total_bins ] ) );
    infection_detected_bins.push_back( std::make_pair( "PfHRP2 Positive"                , new float[ num_total_bins ] ) );
    infection_detected_bins.push_back( std::make_pair( "True Positive"                  , new float[ num_total_bins ] ) );
    infection_detected_bins.push_back( std::make_pair( "Fever Positive"                 , new float[ num_total_bins ] ) );

    release_assert( MalariaDiagnosticType::pairs::count() == infection_detected_bins.size() );
    release_assert( m_DetectionThresholds.size() == infection_detected_bins.size() );

    mean_parasitemia_bins    = new float[ num_total_bins ];
    new_clinical_cases_bins  = new float[ num_total_bins ];
    new_severe_cases_bins    = new float[ num_total_bins ];
    msp_variant_bins         = new float[ num_total_bins ];
    nonspec_variant_bins     = new float[ num_total_bins ];
    pfemp1_variant_bins      = new float[ num_total_bins ];
    ss_msp_variant_bins      = new float[ num_total_bins ];
    ss_nonspec_variant_bins  = new float[ num_total_bins ];
    ss_pfemp1_variant_bins   = new float[ num_total_bins ];

    clearChannelsBins();
}

void BinnedReportMalaria::clearChannelsBins()
{
    for( auto& r_pair_name_bins : infection_detected_bins )
    {
        memset( r_pair_name_bins.second, 0, num_total_bins * sizeof( float ) );
    }
    memset( mean_parasitemia_bins   , 0, num_total_bins * sizeof(float) );
    memset( new_clinical_cases_bins , 0, num_total_bins * sizeof(float) );
    memset( new_severe_cases_bins   , 0, num_total_bins * sizeof(float) );
    memset( msp_variant_bins        , 0, num_total_bins * sizeof(float) );
    memset( nonspec_variant_bins    , 0, num_total_bins * sizeof(float) );
    memset( pfemp1_variant_bins     , 0, num_total_bins * sizeof(float) );
    memset( ss_msp_variant_bins     , 0, num_total_bins * sizeof(float) );
    memset( ss_nonspec_variant_bins , 0, num_total_bins * sizeof(float) );
    memset( ss_pfemp1_variant_bins  , 0, num_total_bins * sizeof(float) );
}

void BinnedReportMalaria::EndTimestep( float currentTime, float dt )
{
    for( auto& r_pair_name_bins : infection_detected_bins )
    {
        Accumulate( r_pair_name_bins.first, r_pair_name_bins.second );
    }
    Accumulate( _mean_parasitemia_label,                         mean_parasitemia_bins   );
    Accumulate( "New Clinical Cases",                            new_clinical_cases_bins );
    Accumulate( "New Severe Cases",                              new_severe_cases_bins   );
    Accumulate( "Sum MSP Variant Fractions",                     msp_variant_bins        );
    Accumulate( "Sum Non-Specific Variant Fractions",            nonspec_variant_bins    );
    Accumulate( "Sum PfEMP1 Variant Fractions",                  pfemp1_variant_bins     );
    Accumulate( "Sum of Squared MSP Variant Fractions",          ss_msp_variant_bins     );
    Accumulate( "Sum of Squared Non-Specific Variant Fractions", ss_nonspec_variant_bins );
    Accumulate( "Sum of Squared PfEMP1 Variant Fractions",       ss_pfemp1_variant_bins  );
    
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

    IMalariaHumanContext* individual_malaria = nullptr;
    if( individual->QueryInterface( GET_IID( IMalariaHumanContext), (void**) &individual_malaria ) != Kernel::s_OK )
    {
        throw Kernel::QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "individual", "IndividualHuman", "IMalariaHumanContext" );
    }

    Kernel::IMalariaSusceptibility* susc_malaria = individual_malaria->GetMalariaSusceptibilityContext();

    float msp_frac     = susc_malaria->get_fraction_of_variants_with_antibodies(MalariaAntibodyType::MSP1);
    float nonspec_frac = susc_malaria->get_fraction_of_variants_with_antibodies(MalariaAntibodyType::PfEMP1_minor);
    float pfemp1_frac  = susc_malaria->get_fraction_of_variants_with_antibodies(MalariaAntibodyType::PfEMP1_major);

    msp_variant_bins[        bin_index ] += mc_weight * msp_frac;
    nonspec_variant_bins[    bin_index ] += mc_weight * nonspec_frac;
    pfemp1_variant_bins[     bin_index ] += mc_weight * pfemp1_frac;

    ss_msp_variant_bins[     bin_index ] += mc_weight * msp_frac     * msp_frac;
    ss_nonspec_variant_bins[ bin_index ] += mc_weight * nonspec_frac * nonspec_frac;
    ss_pfemp1_variant_bins[  bin_index ] += mc_weight * pfemp1_frac  * pfemp1_frac;

    if (individual->IsInfected())
    {
        std::vector<float> detected( infection_detected_bins.size(), 0.0 );
        ReportUtilitiesMalaria::LogIndividualMalariaInfectionAssessment( individual,
                                                                         m_DetectionThresholds,
                                                                         detected,
                                                                         mean_parasitemia_bins[ bin_index ] );
        for( int i = 0; i < infection_detected_bins.size(); ++i )
        {
            infection_detected_bins[ i ].second[ bin_index ] += detected[ i ];
        }

        if ( individual_malaria->HasClinicalSymptomNew(ClinicalSymptomsEnum::CLINICAL_DISEASE) )
            new_clinical_cases_bins[bin_index] += mc_weight;

        if ( individual_malaria->HasClinicalSymptomNew(ClinicalSymptomsEnum::SEVERE_DISEASE) )
            new_severe_cases_bins[bin_index] += mc_weight;
    }
}

void BinnedReportMalaria::postProcessAccumulatedData()
{
    normalizeChannel( _mean_parasitemia_label, infection_detected_bins[ MalariaDiagnosticType::BLOOD_SMEAR_PARASITES ].first );
    channelDataMap.ExponentialValues( _mean_parasitemia_label );
}

}
