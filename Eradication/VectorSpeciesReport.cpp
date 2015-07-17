/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"
#include "VectorSpeciesReport.h"

#include <map>
#include <string>

#include "BoostLibWrapper.h"
#include "Debug.h"
#include "Environment.h"
#include "Exceptions.h"
#include "NodeVector.h"
#include "Sugar.h"

static const char * _module           = "VectorSpeciesReport";        // Module name for logging
static const std::string _report_name = "VectorSpeciesReport.json";   // Report output file name

#ifndef _WIN32
#define _countof(a) (sizeof(a)/sizeof(*(a)))
#endif

Kernel::IReport*
VectorSpeciesReport::CreateReport( const JsonConfigurable::tDynamicStringSet& rVectorSpeciesNames )
{
    return new VectorSpeciesReport( rVectorSpeciesNames );
}

VectorSpeciesReport::VectorSpeciesReport( const JsonConfigurable::tDynamicStringSet& rVectorSpeciesNames )
    : BinnedReport()
    , adult_vectors(nullptr)
    , infectious_vectors(nullptr)
    , daily_eir(nullptr)
    , daily_hbr(nullptr)
{
    LOG_DEBUG( "VectorSpeciesReport ctor\n" );
    if(rVectorSpeciesNames.empty())
    {
        throw Kernel::IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "Trying to initialize VectorSpeciesReport when there are no vectors in the simulation; this shouldn't be possible.");
    }

    const char* _axis_labels[]       = { "Vector Species" };
    const int   _num_bins_per_axis[] = { rVectorSpeciesNames.size() };

    report_name = _report_name;

    static_assert(_countof(_axis_labels) == _countof(_num_bins_per_axis), "Number of axis-labels must match number of axis bin-counts");

    axis_labels = std::vector<std::string>(_axis_labels, _axis_labels + (sizeof(_axis_labels) / sizeof(char*)));
    num_bins_per_axis = std::vector<int>(_num_bins_per_axis, _num_bins_per_axis + (sizeof(_num_bins_per_axis) / sizeof(int)));

    num_axes = _countof(_axis_labels);

    num_total_bins = 1;
    for (int i : num_bins_per_axis)
        num_total_bins *= i;

    // push back species binning
    values_per_axis.push_back( std::vector<float>( rVectorSpeciesNames.size(), 0 ) ); // not going to use this...
    friendly_names_per_axis.push_back( std::vector<std::string>( rVectorSpeciesNames.begin(), rVectorSpeciesNames.end() ) );
}

VectorSpeciesReport::~VectorSpeciesReport()
{
    delete adult_vectors;
    delete infectious_vectors;
    delete daily_eir;
    delete daily_hbr;
}

void VectorSpeciesReport::Initialize( unsigned int nrmSize )
{
    _nrmSize = nrmSize;
    release_assert( _nrmSize );

    initChannelBins();
}

void VectorSpeciesReport::initChannelBins()
{
    adult_vectors = new float[num_total_bins];
    infectious_vectors = new float[num_total_bins];
    daily_eir = new float[num_total_bins];
    daily_hbr = new float[num_total_bins];

    clearChannelsBins();
}

void VectorSpeciesReport::clearChannelsBins()
{
    memset(adult_vectors, 0, num_total_bins * sizeof(float));
    memset(infectious_vectors, 0, num_total_bins * sizeof(float));
    memset(daily_eir, 0, num_total_bins * sizeof(float));
    memset(daily_hbr, 0, num_total_bins * sizeof(float));
}

void VectorSpeciesReport::EndTimestep( float currentTime, float dt )
{
    Accumulate("Adult Vectors", adult_vectors);
    Accumulate("Infectious Vectors", infectious_vectors);
    Accumulate("Daily EIR", daily_eir);
    Accumulate("Daily HBR", daily_hbr);

    clearChannelsBins();
}

void VectorSpeciesReport::postProcessAccumulatedData()
{
    LOG_DEBUG( "postProcessAccumulatedData\n" );

    // careful with order, we normalize 'in place', so channels used as
    // normalization 'bases' should be normalized last.
    normalizeChannel("Infectious Vectors",    "Adult Vectors");
    normalizeChannel("Adult Vectors",         (float)_nrmSize);
    normalizeChannel("Daily EIR",             _nrmSize);
    normalizeChannel("Daily HBR",             (float)_nrmSize);
}

void VectorSpeciesReport::LogIndividualData( Kernel::IndividualHuman * individual )
{
    LOG_DEBUG( "VectorSpeciesReport::LogIndividualData\n" );
}

void VectorSpeciesReport::LogNodeData( Kernel::INodeContext * pNC )
{
    LOG_DEBUG( "VectorSpeciesReport::LogNodeData.\n" );

    int   bin_index = 0;
    Kernel::INodeVector * pNV = NULL;
    if( pNC->QueryInterface( GET_IID( Kernel::INodeVector ), (void**) &pNV ) != Kernel::s_OK )
    {
        throw Kernel::QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "pNC", "INodeVector", "INodeContext" );
    }

    // reverse order, since push_front is used in NodeVector::SetVectorPopulations(), why??
    auto& population_list = pNV->GetVectorPopulations();
    for (auto iterator = population_list.rbegin(); iterator != population_list.rend(); iterator++)
    {
        const auto vectorpopulation = *iterator;
        LOG_VALID_F( "bin_index = %d \t species name = %s\n", bin_index, vectorpopulation->get_SpeciesID().c_str() );
        adult_vectors[bin_index]      += (float)( vectorpopulation->getAdultCount() + vectorpopulation->getInfectedCount() + vectorpopulation->getInfectiousCount() );
        infectious_vectors[bin_index] += (float)( vectorpopulation->getInfectiousCount() );
        daily_eir[bin_index]          +=          vectorpopulation->GetEIRByPool(Kernel::VectorPoolIdEnum::BOTH_VECTOR_POOLS);
        daily_hbr[bin_index]          +=          vectorpopulation->GetHBRByPool(Kernel::VectorPoolIdEnum::BOTH_VECTOR_POOLS);
        bin_index++;
    }
}
