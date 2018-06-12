/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include <map>
#include <numeric>
#include "BaseChannelReport.h"
#include "Configuration.h"
#include "Debug.h"
#include "Sugar.h"
#include "Environment.h"
#include "FileSystem.h"
#include "Exceptions.h"
#include "ProgVersion.h"
#include "BoostLibWrapper.h"
#include "RapidJsonImpl.h"
#include "Serializer.h"

using namespace std;
using namespace json;

SETUP_LOGGING( "BaseChannelReport" )

BaseChannelReport::BaseChannelReport( const std::string& rReportName )
    : BaseReport()
    , report_name( rReportName )
    , _nrmSize( 0 )
    , channelDataMap()
{
}

void
BaseChannelReport::Initialize( unsigned int nrmSize )
{
    _nrmSize = nrmSize;
    release_assert( _nrmSize );
}

void BaseChannelReport::BeginTimestep()
{
    channelDataMap.IncreaseChannelLength( 1 );
}

void BaseChannelReport::LogNodeData(
    Kernel::INodeContext * pNC
    )
{
}

void BaseChannelReport::EndTimestep( float currentTime, float dt )
{

}

void BaseChannelReport::Reduce()
{
    LOG_DEBUG( "Reduce\n" );
    channelDataMap.Reduce();
}

void
BaseChannelReport::Finalize()
{
    LOG_DEBUG( "Finalize\n" );

    postProcessAccumulatedData();

    std::map<std::string, std::string> units_map;
    populateSummaryDataUnitsMap(units_map);

    channelDataMap.WriteOutput( report_name, units_map );
}

std::string BaseChannelReport::GetReportName() const
{
    return report_name;
}

void BaseChannelReport::Accumulate( const std::string& channel_name, float value )
{
    channelDataMap.Accumulate( channel_name, value );
}

void
BaseChannelReport::normalizeChannel(
    const std::string &channel_name,
    float normalization_value
)
{
    channelDataMap.normalizeChannel( channel_name, normalization_value );
}

void
BaseChannelReport::normalizeChannel(
    const std::string &channel_name,
    const std::string &normalization_channel_name
)
{
    channelDataMap.normalizeChannel( channel_name, normalization_channel_name );
}

void
BaseChannelReport::addDerivedLogScaleSummaryChannel( 
    const std::string& source_channel_name,
    const std::string& log_channel_name
)
{
    channelDataMap.addDerivedLogScaleSummaryChannel( source_channel_name, log_channel_name );
}

void BaseChannelReport::addDerivedCumulativeSummaryChannel(
    const std::string& source_channel_name,
    const std::string& cumulative_channel_name)
{
    channelDataMap.addDerivedCumulativeSummaryChannel( source_channel_name, cumulative_channel_name );
}

void BaseChannelReport::SetAugmentor( IChannelDataMapOutputAugmentor* pAugmentor )
{
    channelDataMap.SetAugmentor( pAugmentor );
}

#if 0
template<class Archive>
void serialize(Archive &ar, Report& report, const unsigned int v)
{
    ar & report.timesteps_reduced;
    ar & report.channelDataMap;
    ar & report._nrmSize;
}
#endif
