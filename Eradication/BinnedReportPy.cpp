/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#pragma warning(disable:4996)

#ifdef ENABLE_PYTHON

#include "BinnedReportPy.h"

#include <map>
#include <string>

#include "BoostLibWrapper.h"
#include "Environment.h"
#include "Exceptions.h"
#include "Sugar.h"
#include "IndividualPy.h"
#include "Common.h"

using namespace std;
using namespace json;
using namespace Kernel;

SETUP_LOGGING( "BinnedReportPy" )

namespace Kernel {

Kernel::IReport*
BinnedReportPy::CreateReport()
{
    return new BinnedReportPy();
}

// Derived constructor calls base constructor to initialized reduced timesteps etc. 
BinnedReportPy::BinnedReportPy() 
    : BinnedReport()
{
    LOG_DEBUG( "BinnedReportPy ctor\n" );
    _num_age_bins = 100;

    _age_bin_friendly_names.resize( _num_age_bins );
    memset( _age_bin_upper_values, 0, sizeof( float ) * 100 );
    for( int idx = 0; idx < _num_age_bins ; idx++ )
    {
        _age_bin_upper_values[idx] = 365.0f*(idx+1);
        _age_bin_friendly_names[idx] = std::to_string( idx );
    }
}

BinnedReportPy::~BinnedReportPy()
{
}

void BinnedReportPy::initChannelBins()
{
    BinnedReport::initChannelBins();

    clearChannelsBins();
}

void BinnedReportPy::clearChannelsBins()
{
}

void BinnedReportPy::EndTimestep( float currentTime, float dt )
{
    BinnedReport::EndTimestep( currentTime, dt );

    clearChannelsBins();
}

void BinnedReportPy::LogIndividualData( IIndividualHuman * individual )
{
    LOG_DEBUG( "BinnedReportPy::LogIndividualData\n" );

    // Look ma, I can copy-paste code...(from ReportPy.cpp, let's refactor)
    Kernel::IIndividualHumanPy* typhoid_individual = NULL;
    if( individual->QueryInterface( GET_IID( Kernel::IIndividualHumanPy ), (void**)&typhoid_individual ) != Kernel::s_OK )
    {
        throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "individual", "IIndividualPy", "IndividualHuman" );
    }

    auto mc_weight = individual->GetMonteCarloWeight();
    int bin_index = calcBinIndex(individual);

    BinnedReport::LogIndividualData(individual);
}

void BinnedReportPy::postProcessAccumulatedData()
{
}

}
#endif // ENABLE_PYTHON
