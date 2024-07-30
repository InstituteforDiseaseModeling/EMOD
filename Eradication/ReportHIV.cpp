
#include "stdafx.h"

#include "Debug.h"
#include "ReportHIV.h"
#include "NodeHIV.h"
#include "SusceptibilityHIV.h"
#include "InfectionHIV.h"
#include "IHIVInterventionsContainer.h"
#include "NodeEventContext.h"
#include "EventTrigger.h"
#include "IndividualHIV.h"

SETUP_LOGGING( "ReportHIV" )

#define CD4_THRESHOLD 200

namespace Kernel {

    GET_SCHEMA_STATIC_WRAPPER_IMPL(ReportHIV,ReportHIV)

    ReportHIV::ReportHIV()
        : num_hiv_acute_id()
        , num_hiv_latent_id()
        , num_hiv_untreated_aids_id()
        , num_hiv_treated_aids_id()
        , num_hiv_cd4_hi_on_ART_id()
        , num_hiv_cd4_hi_non_ART_id()
        , num_hiv_cd4_lo_on_ART_id()
        , num_hiv_cd4_lo_non_ART_id()
        , num_on_ART_id()
        , num_ART_dropouts_id()
        , num_events_id()
        , event_trigger_index_to_channel_id()
        , num_acute(0)
        , num_latent(0)
        , num_aids_without(0)
        , num_aids_with(0)
        , num_hiv_cd4_lo_non_ART(0)
        , num_hiv_cd4_hi_non_ART(0)
        , num_hiv_cd4_lo_on_ART(0)
        , num_hiv_cd4_hi_on_ART(0)
        , num_on_ART(0)
        , num_ART_dropouts(0)
        , num_events(0)
        , counting_all_events(false)
        , event_counter_vector()
        , eventTriggerList()
    {
        // ------------------------------------------------------------------------------------------------
        // --- Since this report will be listening for events, it needs to increment its reference count
        // --- so that it is 1.  Other objects will be AddRef'ing and Release'ing this report/observer
        // --- so it needs to start with a refcount of 1.
        // ------------------------------------------------------------------------------------------------
        AddRef();   // TODO - this should be virtual, but isn't because the constructor isn't finished yet...

        // this vector is indexed by the EventTrigger index
        event_trigger_index_to_channel_id.resize( EventTriggerFactory::GetInstance()->GetNumEventTriggers() );
        event_counter_vector.resize(              EventTriggerFactory::GetInstance()->GetNumEventTriggers() );

        num_hiv_acute_id          = AddChannel( "Number of (untreated) Individuals with Acute HIV"   );
        num_hiv_latent_id         = AddChannel( "Number of (untreated) Individuals with Latent HIV"  );
        num_hiv_untreated_aids_id = AddChannel( "Number of (untreated) Individuals with AIDS"        );
        num_hiv_treated_aids_id   = AddChannel( "Number of (treated) Individuals with AIDS"          );
        num_hiv_cd4_hi_on_ART_id  = AddChannel( "Number of Individuals HIV+ w/ CD4 >= 200 (on-ART)"  );
        num_hiv_cd4_hi_non_ART_id = AddChannel( "Number of Individuals HIV+ w/ CD4 >= 200 (non-ART)" );
        num_hiv_cd4_lo_on_ART_id  = AddChannel( "Number of Individuals HIV+ w/ CD4 < 200 (on-ART)"   );
        num_hiv_cd4_lo_non_ART_id = AddChannel( "Number of Individuals HIV+ w/ CD4 < 200 (non-ART)"  );
        num_on_ART_id             = AddChannel( "Number of Individuals on ART"                       );
        num_ART_dropouts_id       = AddChannel( "Number of ART dropouts (cumulative)"                );
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

        units_map[ num_hiv_acute_id.GetName()          ] = "";
        units_map[ num_hiv_latent_id.GetName()         ] = "";
        units_map[ num_hiv_untreated_aids_id.GetName() ] = "";
        units_map[ num_hiv_treated_aids_id.GetName()   ] = "";
        units_map[ num_hiv_cd4_hi_non_ART_id.GetName() ] = "";
        units_map[ num_hiv_cd4_hi_on_ART_id.GetName()  ] = "";
        units_map[ num_hiv_cd4_lo_non_ART_id.GetName() ] = "";
        units_map[ num_hiv_cd4_lo_on_ART_id.GetName()  ] = "";
        units_map[ num_on_ART_id.GetName()             ] = "";
        units_map[ num_ART_dropouts_id.GetName()       ] = "";

        if( counting_all_events )
        {
            units_map[ num_events_id.GetName()] = "";
        }
        else
        {
            for( auto trig : eventTriggerList )
            {
                const ChannelID& r_id = event_trigger_index_to_channel_id[ trig.GetIndex() ];
                units_map[ r_id.GetName()] = "";
            }
        }
    }

    bool ReportHIV::Configure( const Configuration* inputJson )
    {
        initConfigTypeMap( "Report_HIV_Event_Channels_List", &eventTriggerList, Report_HIV_Event_Channels_List_DESC_TEXT, "Enable_Default_Reporting" );

        bool ret = ReportSTI::Configure( inputJson );

        if( ret && JsonConfigurable::_dryrun == false )
        {
            // ---------------------------------------------------------------------
            // --- If the user does not specify a list of specific events to count,
            // --- then we want to count all of the events (except EveryUpdate)
            // ---------------------------------------------------------------------
            if( eventTriggerList.size() == 0 )
            {
                counting_all_events = true;

                std::vector<EventTrigger> all_triggers = EventTriggerFactory::GetInstance()->GetAllEventTriggers();
                for( auto trig : all_triggers )
                {
                    // we'll need better solution here as we add more built-in model events
                    if( (trig != EventTrigger::EveryUpdate) && (trig != EventTrigger::ExposureComplete ) ) 
                    {
                        LOG_DEBUG_F( "Adding %s to eventTriggerList.\n", trig.c_str() );
                        eventTriggerList.push_back( trig );
                        bool in_event_list = std::find( m_EventTriggerList.begin(),
                                                        m_EventTriggerList.end(),
                                                        trig ) != m_EventTriggerList.end();
                        if( !in_event_list )
                        {
                            m_EventTriggerList.push_back( trig );
                        }
                    }
                }
                num_events_id = AddChannel( "Number of Events" );
            }
            else
            {
                counting_all_events = false;
                for( auto trigger : eventTriggerList )
                {
                    bool in_event_list = std::find( m_EventTriggerList.begin(),
                                                    m_EventTriggerList.end(),
                                                    trigger ) != m_EventTriggerList.end();
                    if( !in_event_list )
                    {
                        m_EventTriggerList.push_back( trigger );
                    }
                    std::string label = std::string("Number of Events - ") + trigger.ToString();
                    ChannelID id = AddChannel( label );
                    event_trigger_index_to_channel_id[ trigger.GetIndex() ] = id;
                }
            }
        }
        return ret ;
    }

    void
    ReportHIV::LogIndividualData(
        IIndividualHuman* individual
    )
    {
        ReportSTI::LogIndividualData( individual );

        // --------------------------------------------------------------------------------
        // --- Switched to static_cast because this method was taking 12% of total sim time
        // --- and the QueryInterface() was taking most of that. 
        // --------------------------------------------------------------------------------
        IndividualHumanHIV* hiv_individual = static_cast<IndividualHumanHIV*>(individual);

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
                    if( hiv_individual->GetHIVInterventionsContainer()->OnArtQuery() )
                        num_aids_with += mcw;
                    else
                        num_aids_without += mcw;
                break;

                case HIVInfectionStage::ON_ART:
                    num_on_ART += mcw;
                break;

                default:
                    throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__,
                                                             "hiv_individual->GetHIVInfection()->GetStage()",
                                                             hiv_individual->GetHIVInfection()->GetStage(),
                                                             HIVInfectionStage::pairs::lookup_key( hiv_individual->GetHIVInfection()->GetStage() ) );
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

        Accumulate( num_hiv_acute_id,          num_acute              );
        Accumulate( num_hiv_latent_id,         num_latent             );
        Accumulate( num_hiv_untreated_aids_id, num_aids_without       );
        Accumulate( num_hiv_treated_aids_id,   num_aids_with          );
        Accumulate( num_hiv_cd4_hi_non_ART_id, num_hiv_cd4_hi_non_ART );
        Accumulate( num_hiv_cd4_hi_on_ART_id,  num_hiv_cd4_hi_on_ART  );
        Accumulate( num_hiv_cd4_lo_non_ART_id, num_hiv_cd4_lo_non_ART );
        Accumulate( num_hiv_cd4_lo_on_ART_id,  num_hiv_cd4_lo_on_ART  );
        Accumulate( num_on_ART_id,             num_on_ART             );
        Accumulate( num_ART_dropouts_id,       num_ART_dropouts       );

        if( counting_all_events )
        {
            Accumulate( num_events_id, num_events ); 
            num_events = 0 ;
        }
        else
        {
            for( auto trig : eventTriggerList )
            {
                int trig_index = trig.GetIndex();
                ChannelID& r_id = event_trigger_index_to_channel_id[ trig_index ];
                Accumulate( r_id, event_counter_vector[ trig_index ] ); 
                event_counter_vector[ trig_index ] = 0 ;
            }
        }
                 
        num_acute = 0;
        num_latent = 0;
        num_aids_without = 0;
        num_aids_with = 0;
        num_hiv_cd4_lo_non_ART = 0;
        num_hiv_cd4_lo_on_ART = 0;
        num_hiv_cd4_hi_non_ART = 0;
        num_hiv_cd4_hi_on_ART = 0;
        num_on_ART = 0;
        num_ART_dropouts = 0;
    }

    
    bool ReportHIV::notifyOnEvent( IIndividualHumanEventContext *context, 
                                   const EventTrigger& trigger )
    {
        ReportSTI::notifyOnEvent( context, trigger );

        // no elements in the map implies that we are counting all of the events.
        if( counting_all_events )
        {
            num_events++;
        }
        else
        {
            bool in_event_list = std::find( eventTriggerList.begin(),
                                            eventTriggerList.end(), trigger ) != eventTriggerList.end();
            if( in_event_list )
            {
                event_counter_vector[ trigger.GetIndex() ]++ ;
            }
        }
        return true ;
    }

    void ReportHIV::postProcessAccumulatedData()
    {
        ReportSTI::postProcessAccumulatedData();
        channelDataMap.RemoveChannel( susceptible_pop_id.GetName() );
        channelDataMap.RemoveChannel( exposed_pop_id.GetName()     );
        channelDataMap.RemoveChannel( infectious_pop_id.GetName()  );
        channelDataMap.RemoveChannel( recovered_pop_id.GetName()   );
        channelDataMap.RemoveChannel( waning_pop_id.GetName()      );
    }
}

