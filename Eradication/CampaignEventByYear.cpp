/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "CampaignEventByYear.h"
#include "SimulationSTI.h"
#include "SimulationConfig.h"

namespace Kernel
{
#ifndef DISABLE_STI
    //
    // CampaignEventByYear class here.
    //
    IMPL_QUERY_INTERFACE1(CampaignEventByYear, IConfigurable)
    IMPLEMENT_FACTORY_REGISTERED(CampaignEventByYear)

    CampaignEventByYear::CampaignEventByYear()
    {
    }

    bool
    CampaignEventByYear::Configure(
        const Configuration * inputJson
    )
    {
        if( !JsonConfigurable::_dryrun &&
            (GET_CONFIGURABLE( SimulationConfig )->sim_type != SimType::STI_SIM) &&
            (GET_CONFIGURABLE( SimulationConfig )->sim_type != SimType::HIV_SIM) )
        {
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "CampainEventByYear can only be used in STI and HIV simulations." );
        }

        float start_year;
        initConfigTypeMap( "Start_Year", &start_year, Start_Year_DESC_TEXT, MIN_YEAR, MAX_YEAR, MIN_YEAR );
        initConfigComplexType( "Nodeset_Config", &nodeset_config, Nodeset_Config_DESC_TEXT );
        initConfigComplexType( "Event_Coordinator_Config", &event_coordinator_config, Event_Coordinator_Config_DESC_TEXT );

        // Bypasss CampaignEvent base class so that we don't break without Start_Day!
        bool ret = JsonConfigurable::Configure( inputJson );
        start_day = (start_year - SimulationSTI::base_year) * DAYSPERYEAR;
        return ret;
    }

    CampaignEventByYear::~CampaignEventByYear()
    {
    }
#endif
}
