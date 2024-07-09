
#include "stdafx.h"

#include <string>
#include "ReportEventRecorder.h"
#include "Log.h"
#include "Exceptions.h"
#include "NodeEventContext.h"
#include "IndividualEventContext.h"
#include "FileSystem.h"
#include "SimulationEnums.h"
#include "ISimulation.h"
#include "EventTrigger.h"
#include "SimulationEventContext.h"

SETUP_LOGGING( "ReportEventRecorder" )

// These need to be after SETUP_LOGGING so that the LOG messages in the
// templates don't make GCC complain.
#include "BaseTextReportEventsTemplate.h"
#include "BaseReportEventRecorderTemplate.h"

namespace Kernel
{
    template std::string BaseReportEventRecorder< IIndividualEventBroadcaster,
                                                  IIndividualEventObserver,
                                                  IIndividualHumanEventContext,
                                                  EventTrigger,
                                                  EventTriggerFactory>::GetEnableParameterName();

    template void BaseTextReportEventsTemplate< IIndividualEventBroadcaster,
                                                IIndividualEventObserver,
                                                IIndividualHumanEventContext,
                                                EventTrigger >::Reduce();

    template void BaseTextReportEventsTemplate< IIndividualEventBroadcaster,
                                                IIndividualEventObserver,
                                                IIndividualHumanEventContext,
                                                EventTrigger >::UnregisterAllBroadcasters();

    const std::string ReportEventRecorder::ENABLE_PARAMETER_NAME   = "Report_Event_Recorder";
    const std::string ReportEventRecorder::EVENTS_LIST_NAME        = "Report_Event_Recorder_Events";
    const std::string ReportEventRecorder::EVENTS_LIST_DESC        =  Report_Event_Recorder_Events_DESC_TEXT;
    const std::string ReportEventRecorder::IGNORE_EVENTS_LIST_NAME = "Report_Event_Recorder_Ignore_Events_In_List";
    const std::string ReportEventRecorder::IGNORE_EVENTS_LIST_DESC =  Report_Event_Recorder_Ignore_Events_In_List_DESC_TEXT;

    GET_SCHEMA_STATIC_WRAPPER_IMPL(ReportEventRecorder,ReportEventRecorder)

    IReport* ReportEventRecorder::CreateReport()
    {
        return new ReportEventRecorder();
    }

    ReportEventRecorder::ReportEventRecorder( bool useYears )
        : BaseReportEventRecorder("ReportEventRecorder.csv")
        , m_PropertiesToReport()
        , m_PropertyChangeIPKeyString()
        , m_PropertyChangeIPKey()
        , m_ReportFilter( ENABLE_PARAMETER_NAME.c_str(), "Report_Event_Recorder", useYears, true, true )
    {
    }

    ReportEventRecorder::~ReportEventRecorder()
    {
    }

    void ReportEventRecorder::ConfigureOther( const Configuration * inputJson )
    {
        m_ReportFilter.ConfigureParameters( *this, inputJson );

        m_PropertiesToReport.value_source = IPKey::GetConstrainedStringConstraintKey(); 
        initConfigTypeMap("Report_Event_Recorder_Individual_Properties", &m_PropertiesToReport, Report_Event_Recorder_Individual_Properties_DESC_TEXT, ENABLE_PARAMETER_NAME.c_str() );

        initConfigTypeMap( "Report_Event_Recorder_PropertyChange_IP_Key_Of_Interest",
                           &m_PropertyChangeIPKeyString,
                           Report_Event_Recorder_PropertyChange_IP_Key_Of_Interest_DESC_TEXT, "" );
    }

    void ReportEventRecorder::CheckOther( const Configuration * inputJson )
    {
        m_ReportFilter.CheckParameters( inputJson );
    }

    void ReportEventRecorder::Initialize( unsigned int nrmSize )
    {
        for( auto key_name : m_PropertiesToReport )
        {
            IndividualProperty* p_ip = IPFactory::GetInstance()->GetIP( key_name, "Report_Event_Recorder_Individual_Properties", false );
            if( p_ip == nullptr )
            {
                std::stringstream ss;
                ss << "The IP Key (" << key_name << ") specified in 'Report_Event_Recorder_Individual_Properties' is unknown.\n"
                    << "Valid values are: " << IPFactory::GetInstance()->GetKeysAsString();
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }
        }
        if( !m_PropertyChangeIPKeyString.empty() )
        {
            IndividualProperty* p_ip = IPFactory::GetInstance()->GetIP( m_PropertyChangeIPKeyString, "Report_Event_Recorder_PropertyChange_IP_Key_Of_Interest", false );
            if( p_ip == nullptr )
            {
                std::stringstream ss;
                ss << "The IP Key (" << m_PropertyChangeIPKeyString << ") specified in 'Report_Event_Recorder_PropertyChange_IP_Key_Of_Interest' is unknown.\n"
                    << "Valid values are: " << IPFactory::GetInstance()->GetKeysAsString();
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }
            m_PropertyChangeIPKey = p_ip->GetKey<IPKey>();

            // -----------------------------------------------------------------------------------------
            // --- If the user is using the PropertyChange filter feature but did not add the event
            // --- to the event list, add it.
            // -----------------------------------------------------------------------------------------
            auto it = std::find( eventTriggerList.begin(), eventTriggerList.end(), EventTrigger::PropertyChange );
            if( it == eventTriggerList.end() )
            {
                eventTriggerList.push_back( EventTrigger::PropertyChange );
            }
        }
        m_ReportFilter.Initialize();

        BaseReportEventRecorder::Initialize( nrmSize );
    }

    void ReportEventRecorder::CheckForValidNodeIDs( const std::vector<ExternalNodeId_t>& demographicNodeIds )
    {
        m_ReportFilter.CheckForValidNodeIDs( GetReportName(), demographicNodeIds );
    }

    void ReportEventRecorder::UpdateEventRegistration( float currentTime,
                                                       float dt,
                                                       std::vector<INodeEventContext*>& rNodeEventContextList,
                                                       ISimulationEventContext* pSimEventContext )
    {
        bool is_valid_time = m_ReportFilter.IsValidTime( pSimEventContext->GetSimulationTime() );
        if( !is_registered && is_valid_time )
        {
            for( auto pNEC : rNodeEventContextList )
            {
                release_assert( pNEC );
                IIndividualEventBroadcaster* broadcaster = pNEC->GetIndividualEventBroadcaster();
                UpdateRegistration( broadcaster, true );
                broadcaster_list.push_back( broadcaster );
            }
            is_registered = true;
        }
        else if( is_registered && !is_valid_time )
        {
            UnregisterAllBroadcasters();
            is_registered = false;
        }
    }

    std::string ReportEventRecorder::GetHeader() const
    {
        std::stringstream header ;
        header << BaseReportEventRecorder::GetHeader()
               << "," << "Node_ID"
               << "," << "Event_Name"
               << "," << "Individual_ID"
               << "," << "Age"
               << "," << "Gender"
               << "," << "Infected"
               << "," << "Infectiousness" ;

        for (const auto& prop : m_PropertiesToReport)
        {
            header << "," << prop;
        }

        return header.str();
    }

    std::string ReportEventRecorder::GetOtherData( IIndividualHumanEventContext *context,
                                                   const EventTrigger& trigger )
    {
        int         id           = context->GetSuid().data;
        ExternalNodeId_t node_id = context->GetNodeEventContext()->GetExternalId();
        const char* event_name   = trigger.c_str();
        float       age          = context->GetAge();
        const char  gender       = (context->GetGender() == Gender::MALE) ? 'M' : 'F' ;
        bool        infected     = context->IsInfected();
        float       infectious   = context->GetInfectiousness() ;

        std::stringstream ss;
        ss << "," << node_id
           << "," << event_name
           << "," << id
           << "," << age
           << "," << gender
           << "," << infected
           << "," << infectious;

        // Report requested properties
        const auto * pProp = context->GetProperties();

        for (const auto& prop_name : m_PropertiesToReport)
        {
            IPKey key( prop_name );
            if( !pProp->Contains( key ) )
            {
                throw BadMapKeyException( __FILE__, __LINE__, __FUNCTION__, "properties", prop_name.c_str() );
            }
            ss << "," << pProp->Get( key ).GetValueAsString();
        }
        return ss.str();
    }

    std::string ReportEventRecorder::GetTime( IIndividualHumanEventContext* pEntity ) const
    {
        std::stringstream ss;
        ss << pEntity->GetNodeEventContext()->GetTime().time;
        return ss.str();
    }

    bool ReportEventRecorder::notifyOnEvent( IIndividualHumanEventContext *pEntity, const EventTrigger& trigger )
    {
        bool notify = true;
        if( (trigger == EventTrigger::PropertyChange) && m_PropertyChangeIPKey.IsValid() )
        {
            notify = (m_PropertyChangeIPKey == pEntity->GetInterventionsContext()->GetLastIPChange().GetKey<IPKey>());
        }

        notify = notify && m_ReportFilter.IsValidNode( pEntity->GetNodeEventContext() );
        notify = notify && m_ReportFilter.IsValidHuman( pEntity->GetIndividualHumanConst() );

        bool ret = true;
        if( notify )
        {
            ret = BaseReportEventRecorder::notifyOnEvent( pEntity, trigger );
        }
        return ret;
    }
}
