/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"

#include "Debug.h"
#include "ReportHIV.h"
#include "NodeHIV.h"
#include "SusceptibilityHIV.h"
#include "InfectionHIV.h"
#include "HIVInterventionsContainer.h"
#include "NodeEventContext.h"
#include "SimulationConfig.h" // to iterate over listed_events

static const char* _module = "ReportHIV";

#define CD4_THRESHOLD 200

namespace Kernel {

    static const char* _num_hiv_acute_label          = "Number of (untreated) Individuals with Acute HIV";
    static const char* _num_hiv_latent_label         = "Number of (untreated) Individuals with Latent HIV";
    static const char* _num_hiv_aids_label           = "Number of (untreated) Individuals with AIDS";
    static const char* _num_hiv_cd4_hi_on_ART_label  = "Number of Individuals HIV+ w/ CD4 >= 200 (on-ART)";
    static const char* _num_hiv_cd4_hi_non_ART_label = "Number of Individuals HIV+ w/ CD4 >= 200 (non-ART)";
    static const char* _num_hiv_cd4_lo_on_ART_label  = "Number of Individuals HIV+ w/ CD4 < 200 (on-ART)";
    static const char* _num_hiv_cd4_lo_non_ART_label = "Number of Individuals HIV+ w/ CD4 < 200 (non-ART)";
    static const char* _num_on_ART_label             = "Number of Individuals on ART";
    static const char* _num_ART_dropouts_label       = "Number of ART dropouts (cumulative)";
    static const char* _num_events_label             = "Number of Events" ;

    std::string GetEventPerChannelLabel( const std::string& trig )
    {
        std::string label = _num_events_label + std::string(" - ") + trig;
        return label ;
    }

    GET_SCHEMA_STATIC_WRAPPER_IMPL(ReportHIV,ReportHIV)

    ReportHIV::ReportHIV()
        : num_acute(0)
        , num_latent(0)
        , num_aids(0)
        , num_hiv_cd4_hi_non_ART(0)
        , num_hiv_cd4_hi_on_ART(0)
        , num_hiv_cd4_lo_non_ART(0)
        , num_hiv_cd4_lo_on_ART(0)
        , num_on_ART(0)
        , num_ART_dropouts(0)
        , num_events(0)
        , event_to_counter_map()
        , eventTriggerList()
        , ntic_list()
    {
        //std::cout << "ReportHIV created." << std::endl;

        // ------------------------------------------------------------------------------------------------
        // --- Since this report will be listening for events, it needs to increment its reference count
        // --- so that it is 1.  Other objects will be AddRef'ing and Release'ing this report/observer
        // --- so it needs to start with a refcount of 1.
        // ------------------------------------------------------------------------------------------------
        AddRef();
    }

    ReportHIV::~ReportHIV()
    {
    }

    void
    ReportHIV::populateSummaryDataUnitsMap(
        std::map<std::string, std::string> &units_map
    )
    {
        ReportSTI::populateSummaryDataUnitsMap(units_map);
        units_map[ _num_hiv_acute_label          ] = "";
        units_map[ _num_hiv_latent_label         ] = "";
        units_map[ _num_hiv_aids_label           ] = "";
        units_map[ _num_hiv_cd4_hi_non_ART_label ] = "";
        units_map[ _num_hiv_cd4_hi_on_ART_label  ] = "";
        units_map[ _num_hiv_cd4_lo_non_ART_label ] = "";
        units_map[ _num_hiv_cd4_lo_on_ART_label  ] = "";

        // if event_to_counter_map is empty, then we are recording all events
        if( event_to_counter_map.size() <= 0 )
        {
            units_map[ _num_events_label ] = "";
        }
        else
        {
            for( auto trig : eventTriggerList )
            {
                std::string label = GetEventPerChannelLabel( trig ) ;
                units_map[ label ] = "";
            }
        }
    }

    bool ReportHIV::Configure( const Configuration* inputJson )
    {
        initConfigTypeMap( "Report_HIV_Event_Channels_List", &eventTriggerList, Report_HIV_Event_Channels_List_DESC_TEXT, "<configuration>:Listed_Events.*", GET_CONFIGURABLE(SimulationConfig)->listed_events );

        bool ret = JsonConfigurable::Configure( inputJson );

        if( ret && JsonConfigurable::_dryrun == false )
        {
            // ---------------------------------------------------------------------
            // --- If the user does not specify a list of specific events to count,
            // --- then we want to count all of the events (except EveryUpdate)
            // ---------------------------------------------------------------------
            if( eventTriggerList.size() <= 0 )
            {

                for( auto &trig: GET_CONFIGURABLE(SimulationConfig)->listed_events )
                {
                    if( trig != "EveryUpdate" &&
                        trig != "EveryTimeStep"
                      )
                    {
                        LOG_INFO_F( "Adding %s to eventTriggerList.\n", trig.c_str() );
                        eventTriggerList.push_back( trig );
                    }
                }
            }
            else
            {
                // ----------------------------------------------------
                // --- If the user provided a list of specific events, 
                // --- then we want to count those specific events.
                // --- We use a map to keep track of the event to counter.
                // --- If the map is empty, then we are counting all events.
                // ----------------------------------------------------
                for( auto trig : eventTriggerList )
                {
                    event_to_counter_map[trig] = 0 ;
                }
            }
        }
        return ret ;
    }

    void ReportHIV::UpdateEventRegistration( float currentTime, 
                                             float dt, 
                                             std::vector<INodeEventContext*>& rNodeEventContextList ) 
    {
        if( ntic_list.size() > 0 )
        {
            return ;
        }

        for( auto pNEC : rNodeEventContextList )
        {
            INodeTriggeredInterventionConsumer* pNTIC = nullptr;
            if( pNEC->QueryInterface( GET_IID(INodeTriggeredInterventionConsumer), (void**)&pNTIC ) != s_OK )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "pNEC", "INodeTriggeredInterventionConsumer", "INodeEventContext" );
            }
            release_assert( pNTIC );

            for( auto trig : eventTriggerList )
            {
                LOG_INFO_F( "ReportHIV is registering to listen to event %s\n", trig.c_str() );
                pNTIC->RegisterNodeEventObserverByString( this, trig );
            }
            ntic_list.push_back( pNTIC );
        }
    }

    void
    ReportHIV::LogIndividualData(
        IndividualHuman* individual
    )
    {
        ReportSTI::LogIndividualData( individual );
        IIndividualHumanHIV* hiv_individual = NULL;
        if( individual->QueryInterface( GET_IID( IIndividualHumanHIV ), (void**)&hiv_individual ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "individual", "IIndividualHIV", "IndividualHuman" );
        }

        if( hiv_individual->GetHIVInfection() )
        {
            auto mcw = individual->GetMonteCarloWeight();
            auto cd4 = hiv_individual->GetHIVSusceptibility()->GetCD4count();
            if( cd4 >= CD4_THRESHOLD )
            {
                if( hiv_individual->GetHIVInterventionsContainer()->OnArtQuery() == true )
                {
                    num_hiv_cd4_hi_on_ART += mcw;
                }
                else
                { 
                    num_hiv_cd4_hi_non_ART += mcw;
                }
            }
            else
            {
                if( hiv_individual->GetHIVInterventionsContainer()->OnArtQuery() == true )
                {
                    num_hiv_cd4_lo_on_ART += mcw;
                }
                else
                { 
                    num_hiv_cd4_lo_non_ART += mcw;
                }
            }

            switch( hiv_individual->GetHIVInfection()->GetStage() )
            {
                case HIVInfectionStage::ACUTE:
                    num_acute += mcw;
                break;

                case HIVInfectionStage::LATENT:
                    num_latent += mcw;
                break;

                case HIVInfectionStage::AIDS:
                    num_aids += mcw;
                break;

                case HIVInfectionStage::ON_ART:
                    num_on_ART += mcw;
                break;

                default:
                    throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__, "hiv_individual->GetHIVInfection()->GetStage()", hiv_individual->GetHIVInfection()->GetStage() );
                break;
            }

            if( hiv_individual->GetHIVInterventionsContainer()->GetArtStatus() == ARTStatus::OFF_BY_DROPOUT )
            {
                num_ART_dropouts += mcw;
            }
        }
    }

    void ReportHIV::EndTimestep( float currentTime, float dt )
    {
        ReportSTI::EndTimestep( currentTime, dt );

        Accumulate( _num_hiv_acute_label, num_acute );
        Accumulate( _num_hiv_latent_label, num_latent );
        Accumulate( _num_hiv_aids_label, num_aids );
        Accumulate( _num_hiv_cd4_hi_non_ART_label, num_hiv_cd4_hi_non_ART );
        Accumulate( _num_hiv_cd4_hi_on_ART_label, num_hiv_cd4_hi_on_ART );
        Accumulate( _num_hiv_cd4_lo_non_ART_label, num_hiv_cd4_lo_non_ART );
        Accumulate( _num_hiv_cd4_lo_on_ART_label, num_hiv_cd4_lo_on_ART );
        Accumulate( _num_on_ART_label, num_on_ART );
        Accumulate( _num_ART_dropouts_label, num_ART_dropouts );

        if( event_to_counter_map.size() <= 0 )
        {
            Accumulate( _num_events_label, num_events ); 
            num_events = 0 ;
        }
        else
        {
            for( auto trig : eventTriggerList )
            {
                std::string label = GetEventPerChannelLabel( trig ) ;
                Accumulate( label, event_to_counter_map[ trig ] ); 
                event_to_counter_map[ trig ] = 0 ;
            }
        }
                 
        num_acute =  0;
        num_latent = 0;
        num_aids =   0;
        num_hiv_cd4_lo_non_ART = 0;
        num_hiv_cd4_lo_on_ART = 0;
        num_hiv_cd4_hi_non_ART = 0;
        num_hiv_cd4_hi_on_ART = 0;
        num_on_ART = 0;
        num_ART_dropouts = 0;
    }

    void
    ReportHIV::Reduce()
    {
        ReportSTI::Reduce();

        // make sure we are unregistered before objects start being deleted
        for( auto p_ntic : ntic_list )
        {
            for( auto trig : eventTriggerList )
            {
                p_ntic->UnregisterNodeEventObserverByString( this, trig  );
            }
        }
        ntic_list.clear();
    }

    bool ReportHIV::notifyOnEvent( IIndividualHumanEventContext *context, 
                                   const std::string& StateChange )
    {
        LOG_DEBUG_F( "notifyOnEvent: %s\n", StateChange.c_str() );
        // no elements in the map implies that we are counting all of the events.
        if( event_to_counter_map.size() <= 0 )
        {
            num_events++;
        }
        else
        {
            event_to_counter_map[ StateChange ]++ ;
        }
        return true ;
    }
}

