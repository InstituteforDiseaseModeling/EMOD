/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#ifdef ENABLE_TYPHOID

#include <numeric> // for std::accumulate
#include "ReportTyphoid.h" // for base class
#include "NodeTyphoid.h" // for base class
#include "Debug.h" // for base class
#include "IdmDateTime.h"

SETUP_LOGGING( "ReportTyphoid" )


namespace Kernel {

static const std::string _num_chronic_carriers_label     = "Number of Chronic Carriers";
static const std::string _num_subclinic_infections_label = "Number of New Sub-Clinical Infections";
static const std::string _num_acute_infections_label     = "Number of New Acute Infections";


GET_SCHEMA_STATIC_WRAPPER_IMPL(ReportTyphoid,ReportTyphoid)


ReportTyphoid::ReportTyphoid()
: recording( false )
, parent(nullptr)
, startYear(0.0f)
, stopYear(0.0f)
{
}

bool ReportTyphoid::Configure( const Configuration * inputJson )
{
#define MIN_TYPH_YEAR (1850)
    initConfigTypeMap( "Inset_Chart_Reporting_Start_Year", &startYear, Typhoid_Inset_Chart_Reporting_Start_Year_DESC_TEXT, MIN_TYPH_YEAR, MAX_YEAR, 0.0f );
    initConfigTypeMap( "Inset_Chart_Reporting_Stop_Year", &stopYear, Typhoid_Inset_Chart_Reporting_Stop_Year_DESC_TEXT , MIN_TYPH_YEAR, MAX_YEAR, 0.0f );
    bool ret = JsonConfigurable::Configure( inputJson );
    if( ret && !JsonConfigurable::_dryrun )
    {
        LOG_DEBUG_F( "Read in Start_Year (%f) and Stop_Year(%f).\n", startYear, stopYear );
        if( startYear >= stopYear )
        {
            throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, "Inset_Chart_Reporting_Start_Year", startYear, "Inset_Chart_Reporting_Stop_Year", stopYear );
        }
        channelDataMap.SetStartStopYears( startYear, stopYear );
    }
    return ret ;
}

void ReportTyphoid::BeginTimestep()
{
    if( recording ) // not sure what BeginTimestep even does for us
    {
        return ReportEnvironmental::BeginTimestep();
    }
}

void ReportTyphoid::setRecordingFlag()
{
    release_assert( parent );
    
    float currentYear = parent->GetSimulationTime().Year();
    LOG_DEBUG_F( "currentYear = %f.\n", currentYear );
    if( currentYear >= startYear && currentYear < stopYear )
    {
        recording = true;
    }
    else
    {
        recording = false;
        ReportEnvironmental::BeginTimestep(); // ??? can we clear anything accumulated in the first timestep that we no longer want?
    }
}

void ReportTyphoid::EndTimestep( float currentTime, float dt )
{
    setRecordingFlag();
    if( recording )
    {
        //ReportEnvironmental::EndTimestep( currentTime, dt ); 
        // bypass generic-level EndTimestep coz we don't care about those channels.
        BaseChannelReport::EndTimestep( currentTime, dt );
        ReportEnvironmental::EndTimestep( currentTime, dt );

        // Make sure we push at least one zero per timestep
        Accumulate( _num_chronic_carriers_label, 0 );
        Accumulate( _num_subclinic_infections_label, 0 );
        Accumulate( _num_acute_infections_label, 0 );
    }

    LOG_DEBUG_F( "recording = %d\n", recording );
}

void
ReportTyphoid::postProcessAccumulatedData()
{
    // Don't call into base class -- too much stuff we don't want. Just copy-paste a couple into here.
    LOG_DEBUG( "postProcessAccumulatedData\n" );
    normalizeChannel("Infected", _stat_pop_label);
    //addDerivedCumulativeSummaryChannel(_new_infections_label, "Cumulative Infections");
}

void
ReportTyphoid::populateSummaryDataUnitsMap(
    std::map<std::string, std::string> &units_map
)
{
    ReportEnvironmental::populateSummaryDataUnitsMap(units_map);
    
    // Additional malaria channels
    //units_map[_wpv1_prev_label]                 = _infected_fraction_label;
}

void
ReportTyphoid::LogIndividualData(
    IIndividualHuman * individual
)
{
    /*if( recording == false ) 
    {
        return;
    }*/

    ReportEnvironmental::LogIndividualData( individual );
    IIndividualHumanTyphoid* typhoid_individual = NULL;
    if( individual->QueryInterface( GET_IID( IIndividualHumanTyphoid ), (void**)&typhoid_individual ) != s_OK )
    {
        throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "individual", "IIndividualTyphoid", "IndividualHuman" );
    }

    auto mcw = individual->GetMonteCarloWeight();
    if( typhoid_individual->IsChronicCarrier( false ) )
    {
        //Accumulate( _num_chronic_carriers_label, mc_weight );
        chron_carriers_counter += mcw;
    }

    if( individual->IsInfected() )
    {
        if( typhoid_individual->IsSubClinical() )
        {
            subclinical_infections_counter += mcw;
        }
        else if( typhoid_individual->IsAcute() )
        {
            acute_infections_counter += mcw;
        }
    }
}

void
ReportTyphoid::LogNodeData(
    INodeContext * pNC
)
{
    if( parent == nullptr )
    {
        parent = pNC->GetParent();
        LOG_DEBUG_F( "Set parent to %x\n", parent );
        setRecordingFlag();
    }

    if( recording == false ) 
    {
        return;
    }

    ReportEnvironmental::LogNodeData( pNC );
    Accumulate( _num_chronic_carriers_label, chron_carriers_counter  );
    Accumulate( _num_subclinic_infections_label, subclinical_infections_counter );
    Accumulate( _num_acute_infections_label, acute_infections_counter );
    chron_carriers_counter = 0;
    subclinical_infections_counter = 0;
    acute_infections_counter = 0;
}

void ReportTyphoid::AccumulateSEIRW()
{
}

}

#endif // ENABLE_TYPHOID
