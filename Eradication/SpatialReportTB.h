/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <list>
#include <map>
#include <vector>
#include <string>
#include <fstream>

#include "SpatialReport.h"
#include "BoostLibWrapper.h"

namespace Kernel {

class SpatialReportTB : public SpatialReport
{
    GET_SCHEMA_STATIC_WRAPPER(SpatialReportTB)

public:
    static IReport* CreateReport();
    virtual ~SpatialReportTB() { }
 
    virtual void LogIndividualData( Kernel::IIndividualHuman* individual );
    virtual void LogNodeData( Kernel::INodeContext * pNC );

protected:
    SpatialReportTB();

    virtual void postProcessAccumulatedData();
    virtual void populateChannelInfos(tChanInfoMap &channel_infos);

    ChannelInfo active_tb_prevalence_info;
    ChannelInfo latent_tb_prevalence_info;
    ChannelInfo mdr_tb_prevalence_info;
    ChannelInfo active_mdr_tb_prevalence_info;
    ChannelInfo tb_immune_fraction_info;
    ChannelInfo new_active_tb_infections_info;
    ChannelInfo newly_cleared_tb_infections_info;

    // counters for LogIndividualData stuff
    float new_active_TB_infections;
    float newly_cleared_tb_infections;
    float active_TB_persons;
    float latent_TB_persons;
    float MDR_TB_persons;
    float active_MDR_TB_persons;
    float TB_immune_persons;
};
}
