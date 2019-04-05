/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#ifdef ENABLE_TYPHOID

#include "BinnedReportTyphoid.h"

#include <map>
#include <string>

#include "BoostLibWrapper.h"
#include "Environment.h"
#include "Exceptions.h"
#include "Sugar.h"
#include "TyphoidDefs.h"
#include "IndividualTyphoid.h"
#include "Common.h"

using namespace std;
using namespace json;
//using namespace Kernel;

namespace Kernel {

// Module name for logging
SETUP_LOGGING( "BinnedReportTyphoid" )

static const std::string _num_chronic_carriers_label     = "Number of Chronic Carriers";
static const std::string _num_subclinic_infections_label = "Number of New Sub-Clinical Infections";
static const std::string _num_acute_infections_label     = "Number of New Acute Infections";

Kernel::IReport*
BinnedReportTyphoid::CreateReport()
{
    return new BinnedReportTyphoid();
}

// Derived constructor calls base constructor to initialized reduced timesteps etc. 
BinnedReportTyphoid::BinnedReportTyphoid() 
    : BinnedReport()
    , carrier_bins( nullptr )
    , subclinical_bins( nullptr )
    , acute_bins( nullptr )
{
    LOG_DEBUG( "BinnedReportTyphoid ctor\n" );
    _num_age_bins = 100;

    _age_bin_friendly_names.resize( _num_age_bins );
    memset( _age_bin_upper_values, 0, sizeof( float ) * 100 );
    for( int idx = 0; idx < _num_age_bins ; idx++ )
    {
        _age_bin_upper_values[idx] = DAYSPERYEAR*(idx+1);
        _age_bin_friendly_names[idx] = std::to_string( idx );
    }
}

BinnedReportTyphoid::~BinnedReportTyphoid()
{
    delete[] carrier_bins;
    delete[] subclinical_bins;
    delete[] acute_bins;
}

void BinnedReportTyphoid::initChannelBins()
{
    carrier_bins = new float[num_total_bins];
    subclinical_bins = new float[num_total_bins];
    acute_bins = new float[num_total_bins];

    BinnedReport::initChannelBins();

    clearChannelsBins();
}

void BinnedReportTyphoid::clearChannelsBins()
{
    memset(carrier_bins, 0, num_total_bins * sizeof(float));
    memset(subclinical_bins, 0, num_total_bins * sizeof(float));
    memset(acute_bins, 0, num_total_bins * sizeof(float));
}

void BinnedReportTyphoid::EndTimestep( float currentTime, float dt )
{
    if (currentTime<6570){
        num_timesteps--;
        return;
    }

    Accumulate( _num_chronic_carriers_label,     carrier_bins );
    Accumulate( _num_subclinic_infections_label, subclinical_bins );
    Accumulate( _num_acute_infections_label,     acute_bins );

    BinnedReport::EndTimestep( currentTime, dt );

    clearChannelsBins();
}

void  BinnedReportTyphoid::LogIndividualData( IIndividualHuman * individual )
{
    LOG_DEBUG( "BinnedReportTyphoid::LogIndividualData\n" );

    // Look ma, I can copy-paste code...(from ReportTyphoid.cpp, let's refactor)
    Kernel::IIndividualHumanTyphoid* typhoid_individual = NULL;
    if( individual->QueryInterface( GET_IID( Kernel::IIndividualHumanTyphoid ), (void**)&typhoid_individual ) != Kernel::s_OK )
    {
        throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "individual", "IIndividualTyphoid", "IndividualHuman" );
    }

    auto mc_weight = individual->GetMonteCarloWeight();
    int bin_index = calcBinIndex(individual);

    if( typhoid_individual->IsChronicCarrier() )
    {
        carrier_bins[ bin_index ] += mc_weight;
    }

    if( individual->IsInfected() )
    {
        if( typhoid_individual->IsSubClinical() )
        {
            subclinical_bins[ bin_index ] += mc_weight;
        }
        else if( typhoid_individual->IsAcute() )
        {
            acute_bins[ bin_index ] += mc_weight;
        }
    }

    BinnedReport::LogIndividualData(individual);
}

void BinnedReportTyphoid::postProcessAccumulatedData()
{
}

}

#endif // ENABLE_TYPHOID
