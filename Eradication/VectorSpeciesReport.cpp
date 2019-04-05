/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "VectorSpeciesReport.h"

#include <map>
#include <string>

#include "BoostLibWrapper.h"
#include "Debug.h"
#include "Environment.h"
#include "Exceptions.h"
#include "VectorContexts.h"
#include "Sugar.h"
#include "INodeContext.h"

SETUP_LOGGING( "VectorSpeciesReport" )

static const std::string _report_name = "VectorSpeciesReport.json";   // Report output file name

static const std::string _label_adult_vectors(        "Adult Vectors Per Node"                      );
static const std::string _label_infectious_vectors(   "Percent Infectious Vectors"                  );
static const std::string _label_daily_eir(            "Daily EIR"                                   );
static const std::string _label_daily_hbr(            "Daily HBR"                                   );
static const std::string _label_dead_vectors_before(  "Percent Vectors Died Before Feeding"         );
static const std::string _label_dead_vectors_indoor(  "Percent Vectors Died During Indoor Feeding"  );
static const std::string _label_dead_vectors_outdoor( "Percent Vectors Died During Outdoor Feeding" );


#ifndef _WIN32
#define _countof(a) (sizeof(a)/sizeof(*(a)))
#endif
namespace Kernel {

Kernel::IReport*
VectorSpeciesReport::CreateReport( const Kernel::jsonConfigurable::tDynamicStringSet& rVectorSpeciesNames )
{
    return new VectorSpeciesReport( rVectorSpeciesNames );
}

VectorSpeciesReport::VectorSpeciesReport( const Kernel::jsonConfigurable::tDynamicStringSet& rVectorSpeciesNames )
    : BinnedReport( _report_name )
    , adult_vectors(nullptr)
    , infectious_vectors(nullptr)
    , daily_eir(nullptr)
    , daily_hbr( nullptr )
    , dead_vectors_before( nullptr )
    , dead_vectors_indoor( nullptr )
    , dead_vectors_outdoor( nullptr )
{
    LOG_DEBUG( "VectorSpeciesReport ctor\n" );
    if(rVectorSpeciesNames.empty())
    {
        throw Kernel::IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "Trying to initialize VectorSpeciesReport when there are no vectors in the simulation; this shouldn't be possible.");
    }

    const char* _axis_labels[]       = { "Vector Species" };
    const int   _num_bins_per_axis[] = { int(rVectorSpeciesNames.size()) };

    static_assert(_countof(_axis_labels) == _countof(_num_bins_per_axis), "Number of axis-labels must match number of axis bin-counts");

    axis_labels = std::vector<std::string>(_axis_labels, _axis_labels + (sizeof(_axis_labels) / sizeof(char*)));
    num_bins_per_axis = std::vector<int>(_num_bins_per_axis, _num_bins_per_axis + (sizeof(_num_bins_per_axis) / sizeof(int)));

    num_axes = _countof(_axis_labels);

    num_total_bins = 1;
    for (int i : num_bins_per_axis)
        num_total_bins *= i;

    // push back species binning
    values_per_axis.push_back( std::vector<float>( rVectorSpeciesNames.size(), 0 ) ); // not going to use this...
    // friendly_names_per_axis.push_back( std::vector<std::string>( rVectorSpeciesNames.begin(), rVectorSpeciesNames.end() ) );
    _age_bin_friendly_names = std::vector<std::string>( rVectorSpeciesNames.begin(), rVectorSpeciesNames.end() );
}

VectorSpeciesReport::~VectorSpeciesReport()
{
    delete adult_vectors;
    delete infectious_vectors;
    delete daily_eir;
    delete daily_hbr;
    delete dead_vectors_before;
    delete dead_vectors_indoor;
    delete dead_vectors_outdoor;
}

void VectorSpeciesReport::Initialize( unsigned int nrmSize )
{
    _nrmSize = nrmSize;
    release_assert( _nrmSize );

    initChannelBins();
}

void VectorSpeciesReport::initChannelBins()
{
    adult_vectors        = new float[ num_total_bins ];
    infectious_vectors   = new float[ num_total_bins ];
    daily_eir            = new float[ num_total_bins ];
    daily_hbr            = new float[ num_total_bins ];
    dead_vectors_before  = new float[ num_total_bins ];
    dead_vectors_indoor  = new float[ num_total_bins ];
    dead_vectors_outdoor = new float[ num_total_bins ];

    clearChannelsBins();
}

void VectorSpeciesReport::clearChannelsBins()
{
    memset( adult_vectors,        0, num_total_bins * sizeof(float) );
    memset( infectious_vectors,   0, num_total_bins * sizeof(float) );
    memset( daily_eir,            0, num_total_bins * sizeof(float) );
    memset( daily_hbr,            0, num_total_bins * sizeof(float) );
    memset( dead_vectors_before,  0, num_total_bins * sizeof(float) );
    memset( dead_vectors_indoor,  0, num_total_bins * sizeof(float) );
    memset( dead_vectors_outdoor, 0, num_total_bins * sizeof(float) );
}

void VectorSpeciesReport::EndTimestep( float currentTime, float dt )
{
    Accumulate( _label_adult_vectors,        adult_vectors        );
    Accumulate( _label_infectious_vectors,   infectious_vectors   );
    Accumulate( _label_daily_eir,            daily_eir            );
    Accumulate( _label_daily_hbr,            daily_hbr            );
    Accumulate( _label_dead_vectors_before,  dead_vectors_before  );
    Accumulate( _label_dead_vectors_indoor,  dead_vectors_indoor  );
    Accumulate( _label_dead_vectors_outdoor, dead_vectors_outdoor );

    clearChannelsBins();
}

void VectorSpeciesReport::postProcessAccumulatedData()
{
    LOG_DEBUG( "postProcessAccumulatedData\n" );

    // careful with order, we normalize 'in place', so channels used as
    // normalization 'bases' should be normalized last.
    normalizeChannel( _label_infectious_vectors, _label_adult_vectors );

    normalizeChannelWithLastTimestep( _label_dead_vectors_before,  _label_adult_vectors );
    normalizeChannelWithLastTimestep( _label_dead_vectors_indoor,  _label_adult_vectors );
    normalizeChannelWithLastTimestep( _label_dead_vectors_outdoor, _label_adult_vectors );

    normalizeChannel( _label_adult_vectors,        (float)_nrmSize );
    normalizeChannel( _label_daily_eir,            (float)_nrmSize );
    normalizeChannel( _label_daily_hbr,            (float)_nrmSize );
}

void VectorSpeciesReport::normalizeChannelWithLastTimestep(
    const std::string &channel_name,
    const std::string &normalization_channel_name )
{
    const ChannelDataMap::channel_data_t& channel_data          = channelDataMap.GetChannel( channel_name               );
    const ChannelDataMap::channel_data_t& normalization_channel = channelDataMap.GetChannel( normalization_channel_name );

    if( normalization_channel.size() == channel_data.size() )
    {
        // ----------------------------------------------------------------------------------------
        // --- Sets all values to zero - In particular, its sets the values for the first timestep
        // --- NOTE: there are num_total_bins per timestep so the first num_total_bins values are
        // --- for the first timestep.
        // ----------------------------------------------------------------------------------------
        ChannelDataMap::channel_data_t normalized_data( channel_data.size(), 0.0 );

        // -------------------------------------------------------
        // --- start at num_total_bins to skip the first timestep
        // -------------------------------------------------------
        for( int i = num_total_bins ; i < channel_data.size() ; ++i )
        {
            float normalization = normalization_channel[ i - num_total_bins ];
            if( normalization != 0.0f )
            {
                normalized_data[ i ] = channel_data[ i ] / normalization;
            }
        }
        channelDataMap.SetChannelData( channel_name, normalized_data );
    }
    else
    {
        std::ostringstream ss;
        ss << "The channel to be normalized (" << channel_name << ") and the normalizing channel (" << normalization_channel_name << ") must have the same length.  "
            << channel_name << "=" << channel_data.size() << ", " << normalization_channel_name << "=" << normalization_channel.size();
        throw Kernel::IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
    }
}


void VectorSpeciesReport::LogIndividualData( Kernel::IIndividualHuman* individual )
{
    LOG_DEBUG( "VectorSpeciesReport::LogIndividualData\n" );
}

void VectorSpeciesReport::LogNodeData( Kernel::INodeContext * pNC )
{
    LOG_DEBUG( "VectorSpeciesReport::LogNodeData.\n" );

    int   bin_index = 0;
    Kernel::INodeVector * pNV = nullptr;
    if( pNC->QueryInterface( GET_IID( Kernel::INodeVector ), (void**) &pNV ) != Kernel::s_OK )
    {
        throw Kernel::QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "pNC", "INodeVector", "INodeContext" );
    }

    // reverse order, since push_front is used in NodeVector::SetVectorPopulations(), why??
    const VectorPopulationReportingList_t& population_list = pNV->GetVectorPopulationReporting();
    for (auto iterator = population_list.rbegin(); iterator != population_list.rend(); iterator++)
    {
        const auto vectorpopulation = *iterator;
        LOG_VALID_F( "bin_index = %d \t species name = %s\n", bin_index, vectorpopulation->get_SpeciesID().c_str() );
        adult_vectors[bin_index]        += float( vectorpopulation->getAdultCount() + vectorpopulation->getInfectedCount( nullptr ) + vectorpopulation->getInfectiousCount( nullptr ) );
        infectious_vectors[bin_index]   += float( vectorpopulation->getInfectiousCount( nullptr ) );
        daily_eir[bin_index]            +=        vectorpopulation->GetEIRByPool(Kernel::VectorPoolIdEnum::BOTH_VECTOR_POOLS);
        daily_hbr[bin_index]            +=        vectorpopulation->GetHBRByPool(Kernel::VectorPoolIdEnum::BOTH_VECTOR_POOLS);
        dead_vectors_before[bin_index]  += float( vectorpopulation->getNumDiedBeforeFeeding() );
        dead_vectors_indoor[bin_index]  += float( vectorpopulation->getNumDiedDuringFeedingIndoor() );
        dead_vectors_outdoor[bin_index] += float( vectorpopulation->getNumDiedDuringFeedingOutdoor() );
        bin_index++;
    }
}

}
