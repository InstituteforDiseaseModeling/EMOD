/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#ifdef ENABLE_TB

#include <functional>
#include <map>
#include "BoostLibWrapper.h"

#include "INodeContext.h"
#include "SpatialReportTB.h"
#include "IndividualTB.h"
#include "Sugar.h"
#include "Environment.h"
#include "Exceptions.h"
#include "SimulationConfig.h"
#include "ProgVersion.h"

using namespace std;

SETUP_LOGGING( "SpatialReportTB" )

namespace Kernel {

GET_SCHEMA_STATIC_WRAPPER_IMPL(SpatialReportTB,SpatialReportTB)

/////////////////////////
// Initialization methods
/////////////////////////
IReport*
SpatialReportTB::CreateReport()
{
    return _new_ SpatialReportTB();
}

SpatialReportTB::SpatialReportTB()
: SpatialReport()
, active_tb_prevalence_info(            "Active_TB_Prevalence",             "fraction")
, latent_tb_prevalence_info(            "Latent_TB_Prevalence",             "fraction")
, mdr_tb_prevalence_info(               "MDR_TB_Prevalence",                "fraction")
, active_mdr_tb_prevalence_info(        "Active_MDR_TB_Prevalence",         "fraction")
, tb_immune_fraction_info(              "TB_Immune_Fraction",               "fraction")
, new_active_tb_infections_info(        "New_Active_TB_Infections",         "")
, newly_cleared_tb_infections_info(     "Newly_Cleared_TB_Infections",      "")
    , new_active_TB_infections(0.0f)
    , newly_cleared_tb_infections(0.0f)
    , active_TB_persons(0.0f)
    , latent_TB_persons(0.0f)
    , MDR_TB_persons(0.0f)
    , active_MDR_TB_persons(0.0f)
    , TB_immune_persons(0.0f)
{
}

void SpatialReportTB::populateChannelInfos(tChanInfoMap &channel_infos)
{
    SpatialReport::populateChannelInfos(channel_infos);

    channel_infos[ active_tb_prevalence_info.name ] = &active_tb_prevalence_info;
    channel_infos[ latent_tb_prevalence_info.name ] = &latent_tb_prevalence_info;
    channel_infos[ mdr_tb_prevalence_info.name ] = &mdr_tb_prevalence_info;
    channel_infos[ active_mdr_tb_prevalence_info.name ] = &active_mdr_tb_prevalence_info;
    channel_infos[ tb_immune_fraction_info.name ] = &tb_immune_fraction_info;
    channel_infos[ new_active_tb_infections_info.name ] = &new_active_tb_infections_info;
    channel_infos[ newly_cleared_tb_infections_info.name ] = &newly_cleared_tb_infections_info;
}

void
SpatialReportTB::LogIndividualData(
    Kernel::IIndividualHuman* individual
)
{
    LOG_DEBUG( "LogIndividualData in SpatialReportTB\n" );

    float monte_carlo_weight = (float)individual->GetMonteCarloWeight();
    const Kernel::IndividualHumanTB* individual_tb = static_cast<const Kernel::IndividualHumanTB*>(individual);

    //Immunity
    if (individual_tb->IsImmune() && !individual->IsInfected())
    {
        TB_immune_persons += monte_carlo_weight;
    }

    // newinfectionstate
    NewInfectionState::_enum nis = individual->GetNewInfectionState();
    if( nis == NewInfectionState::NewlyActive )
    {
        new_active_TB_infections += monte_carlo_weight;
    }
    if( nis == NewInfectionState::NewlyCleared )
    {
        newly_cleared_tb_infections += monte_carlo_weight;
    }

    //Latent people
    if ( individual_tb->HasLatentInfection() ) 
    {
        latent_TB_persons += monte_carlo_weight;
    }

    //Active people
    if ( individual_tb->HasActiveInfection() )
    {
        active_TB_persons += monte_carlo_weight;
    
        //active mdr people
        if (individual_tb->IsMDR() )
        {
            active_MDR_TB_persons += monte_carlo_weight;
        }

    }

    //MDR both latent and active
    if ( individual_tb->IsMDR() )
    {
        MDR_TB_persons += monte_carlo_weight;
    }
}


void
SpatialReportTB::LogNodeData(
    Kernel::INodeContext * pNC
)
{
    SpatialReport::LogNodeData(pNC);

    auto nodeid = pNC->GetExternalID();

    if(active_tb_prevalence_info.enabled)
    {
        Accumulate(active_tb_prevalence_info.name, nodeid, active_TB_persons);
        active_TB_persons = 0.0f;
    }

    if(latent_tb_prevalence_info.enabled)
    {
        Accumulate(latent_tb_prevalence_info.name, nodeid, latent_TB_persons);
        latent_TB_persons = 0.0f;
    }

    if(mdr_tb_prevalence_info.enabled)
    {
        Accumulate(mdr_tb_prevalence_info.name, nodeid, MDR_TB_persons);
        MDR_TB_persons = 0.0f;
    }

    if(active_mdr_tb_prevalence_info.enabled)
    {
        Accumulate(active_mdr_tb_prevalence_info.name, nodeid, active_MDR_TB_persons);
        active_MDR_TB_persons = 0.0f;
    }

    if(tb_immune_fraction_info.enabled)
    {
        Accumulate(tb_immune_fraction_info.name, nodeid, TB_immune_persons);
        TB_immune_persons = 0.0f;
    }

    if(new_active_tb_infections_info.enabled)
    {
        Accumulate(new_active_tb_infections_info.name, nodeid, new_active_TB_infections);
        new_active_TB_infections = 0.0f;
    }

    if(newly_cleared_tb_infections_info.enabled)
    {
        Accumulate(new_active_tb_infections_info.name, nodeid, newly_cleared_tb_infections); 
        newly_cleared_tb_infections = 0.0f;
    }
}

void
SpatialReportTB::postProcessAccumulatedData()
{
    SpatialReport::postProcessAccumulatedData();

    // Normalize TB-specific summary data channels
    if( active_tb_prevalence_info.enabled )
        normalizeChannel(active_tb_prevalence_info.name, population_info.name);

    if( latent_tb_prevalence_info.enabled )
        normalizeChannel(latent_tb_prevalence_info.name, population_info.name);

    if( mdr_tb_prevalence_info.enabled )
        normalizeChannel(mdr_tb_prevalence_info.name, population_info.name);

    if( active_mdr_tb_prevalence_info.enabled )
        normalizeChannel(active_mdr_tb_prevalence_info.name, population_info.name);

    if( tb_immune_fraction_info.enabled )
        normalizeChannel(tb_immune_fraction_info.name, population_info.name);
}

#if 0
template<class Archive>
void serialize(Archive &ar, SpatialReportTB& report, const unsigned int v)
{
    ar & report.timesteps_reduced;
    ar & report.channelDataMap;
    ar & report._nrmSize;
}
#endif

}

#endif // ENABLE_TB
