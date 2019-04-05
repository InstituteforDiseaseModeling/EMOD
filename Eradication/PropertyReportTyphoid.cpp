/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "IIndividualHuman.h"
#include "Node.h"
#include "PropertyReportTyphoid.h"
#include "TransmissionGroupsBase.h"
#include "SimulationTyphoid.h"
#include "SimulationEventContext.h"

using namespace std;
using namespace json;

SETUP_LOGGING( "PropertyReportTyphoid" )

static const std::string _report_name = "PropertyReportTyphoid.json";

namespace Kernel
{

GET_SCHEMA_STATIC_WRAPPER_IMPL( PropertyReportTyphoid, PropertyReportTyphoid )

/////////////////////////
// Initialization methods
/////////////////////////
IReport*
PropertyReportTyphoid::CreateReport()
{
    return _new_ PropertyReportTyphoid();
}

PropertyReportTyphoid::PropertyReportTyphoid()
    : PropertyReportEnvironmental( _report_name )
    , start_year( 0.0 )
    , stop_year( FLT_MAX )
    , is_collecting_data( false )
{
}

bool PropertyReportTyphoid::Configure( const Configuration* inputJson )
{
    initConfigTypeMap( "Property_Report_Start_Year", &start_year, Property_Report_Start_Year_DESC_TEXT, 0.0f, FLT_MAX, 0.0f,    "Enable_Property_Output" );
    initConfigTypeMap( "Property_Report_Stop_Year",  &stop_year,  Property_Report_Stop_Year_DESC_TEXT,  0.0f, FLT_MAX, FLT_MAX, "Enable_Property_Output" );

    bool ret = JsonConfigurable::Configure( inputJson );

    if( ret && !JsonConfigurable::_dryrun )
    {
        if( start_year < SimulationTyphoid::base_year )
        {
            start_year = SimulationTyphoid::base_year;
        }
        if( start_year >= stop_year )
        {
            throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                    "Property_Report_Start_Year", start_year,
                                                    "Property_Report_Stop_Year", stop_year,
                                                    "'Property_Report_Start_Year' must be less than 'Property_Report_Stop_Year'");
        }
        channelDataMap.SetStartStopYears( start_year, stop_year );
    }

    return ret;
}

void PropertyReportTyphoid::UpdateEventRegistration( float currentTime,
                                                     float dt,
                                                     std::vector<INodeEventContext*>& rNodeEventContextList,
                                                     ISimulationEventContext* pSimEventContext )
{
    float current_year = pSimEventContext->GetSimulationTime().Year();
    if( !is_collecting_data && (start_year <= current_year) && (current_year < stop_year) )
    {
        is_collecting_data = true;
    }
    else if( is_collecting_data && (current_year > stop_year) )
    {
        is_collecting_data = false;
    }

    if( is_collecting_data )
    {
        PropertyReport::UpdateEventRegistration( currentTime, dt, rNodeEventContextList, pSimEventContext );
    }
}

void PropertyReportTyphoid::BeginTimestep()
{
    if( is_collecting_data )
    {
        PropertyReport::BeginTimestep();
    }
}

bool PropertyReportTyphoid::IsCollectingIndividualData( float currentTime, float dt ) const
{
    return is_collecting_data;
}

void
PropertyReportTyphoid::LogIndividualData(
    Kernel::IIndividualHuman* individual
)
{
    if( !is_collecting_data ) return;

    PropertyReportEnvironmental::LogIndividualData( individual );
}

void
PropertyReportTyphoid::LogNodeData(
    Kernel::INodeContext * pNC
)
{
    if( !is_collecting_data ) return;

    PropertyReportEnvironmental::LogNodeData(pNC);

}

}

