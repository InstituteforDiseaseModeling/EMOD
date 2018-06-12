/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <map>
#include <vector>
#include <string>

#include "IReport.h"
#include "Log.h"
#include "BoostLibWrapper.h"
#include "ChannelDataMap.h"

class BaseChannelReport : public Kernel::BaseReport
{
public:
    virtual ~BaseChannelReport() {};

    // -------------------
    // --- IReport Methods
    // -------------------
    virtual void Initialize( unsigned int nrmSize );

    virtual void BeginTimestep();
    virtual void LogNodeData( Kernel::INodeContext* pNC );
    virtual void EndTimestep( float currentTime, float dt );

    virtual void Reduce(); // user may call multiple times, each time the reduce will only apply to time steps accumulated since the last call
    virtual void Finalize();

    virtual std::string GetReportName() const;

    // ------------------
    // --- Other Methods
    // ------------------
    virtual void SetAugmentor( IChannelDataMapOutputAugmentor* pAugmentor );

protected:
    BaseChannelReport( const std::string& rReportName );

    virtual void Accumulate(const std::string& channel_name, float value);

    void normalizeChannel( const std::string &channel_name, const std::string &normalization_channel );
    void normalizeChannel( const std::string &channel_name, float normalization_value );

    virtual void addDerivedLogScaleSummaryChannel(const std::string& channel_name, const std::string& channel_log_name);
    virtual void addDerivedCumulativeSummaryChannel(const std::string& channel_name, const std::string& channel_cumulative_name);

    virtual void populateSummaryDataUnitsMap( std::map<std::string, std::string> &units_map ) = 0;
    virtual void postProcessAccumulatedData() = 0;

    std::string report_name;

    unsigned int _nrmSize;

    ChannelDataMap channelDataMap ;
};
