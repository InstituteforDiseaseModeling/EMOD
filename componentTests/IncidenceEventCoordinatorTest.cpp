/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include <memory> // unique_ptr
#include "UnitTest++.h"
#include "componentTests.h"

#include "IncidenceEventCoordinator.h"
#include "IndividualHumanContextFake.h"
#include "IndividualHumanInterventionsContextFake.h"
#include "INodeContextFake.h"
#include "INodeEventContextFake.h"
#include "ISimulationEventContextFake.h"
#include "Node.h"
#include "Exceptions.h"
#include "SimulationConfig.h"
#include "IdmMpi.h"
#include "EventTrigger.h"

using namespace std;
using namespace Kernel;

// maybe these shouldn't be protected in Simulation.h
typedef boost::bimap<ExternalNodeId_t, suids::suid> nodeid_suid_map_t;
typedef nodeid_suid_map_t::value_type nodeid_suid_pair;


SUITE( IncidenceEventCoordinatorTest )
{
    static int m_NextId = 1;

    struct IecFixture
    {
        std::vector< IndividualHumanInterventionsContextFake* > m_hic_list;
        std::vector< IndividualHumanContextFake*              > m_human_list;
        IdmMpi::MessageInterface* m_pMpi;
        SimulationConfig* m_pSimulationConfig;
        EventTrigger m_ActionEvent1;
        EventTrigger m_ActionEvent2;

        IecFixture()
            : m_hic_list()
            , m_human_list()
            , m_pSimulationConfig( new SimulationConfig() )
        {
            Environment::Finalize();
            JsonConfigurable::ClearMissingParameters();
            JsonConfigurable::_useDefaults = true;

            m_pMpi = IdmMpi::MessageInterface::CreateNull();

            Environment::Finalize();
            Environment::setLogger( new SimpleLogger( Logger::tLevel::WARNING ) );
            int argc = 1;
            char* exeName = "componentTests.exe";
            char** argv = &exeName;
            string configFilename( "testdata/IncidenceEventCoordinatorTest/config.json" );
            string inputPath( "testdata/IncidenceEventCoordinatorTest" );
            string outputPath( "testdata/IncidenceEventCoordinatorTest" );
            string statePath( "testdata/IncidenceEventCoordinatorTest" );
            string dllPath( "" );
            Environment::Initialize( m_pMpi, configFilename, inputPath, outputPath, dllPath, false );

            Environment::setSimulationConfig( m_pSimulationConfig );

            EventTriggerFactory::DeleteInstance();
            EventTriggerFactory::GetInstance()->Configure( EnvPtr->Config );
            m_ActionEvent1 = EventTriggerFactory::GetInstance()->CreateUserEventTrigger( "Action_Event_1" );
            m_ActionEvent2 = EventTriggerFactory::GetInstance()->CreateUserEventTrigger( "Action_Event_2" );

            IPFactory::DeleteFactory();
            IPFactory::CreateFactory();

            std::map<std::string, float> ip_values;
            ip_values.insert( std::make_pair( "URBAN", 0.5f ) );
            ip_values.insert( std::make_pair( "RURAL", 0.5f ) );

            IPFactory::GetInstance()->AddIP( 1, "Location", ip_values );

            NPFactory::DeleteFactory();
            NPFactory::CreateFactory();

            std::map<std::string, float> np_values;
            np_values.insert( std::make_pair( "HIGH", 0.5f ) );
            np_values.insert( std::make_pair( "LOW", 0.5f ) );

            NPFactory::GetInstance()->AddNP( "Risk", np_values );
        }

        ~IecFixture()
        {
            for( auto hic : m_hic_list )
            {
                delete hic;
            }
            m_hic_list.clear();

            for( auto human : m_human_list )
            {
                delete human;
            }
            m_human_list.clear();

            EventTriggerFactory::DeleteInstance();
            IPFactory::DeleteFactory();
            JsonConfigurable::ClearMissingParameters();
            Environment::Finalize();
            JsonConfigurable::_useDefaults = false;
        }

        IIndividualHumanContext* CreateHuman(
            INodeContext* pnc,
            INodeEventContext* pnec,
            int gender,
            float ageDays,
            const std::string& rPropertyName = std::string(),
            const std::string& rPropertyValue = std::string() )
        {
            IndividualHumanInterventionsContextFake* p_hic = new IndividualHumanInterventionsContextFake();
            IndividualHumanContextFake* p_human = new IndividualHumanContextFake( p_hic, pnc, pnec, nullptr );

            p_human->SetId( m_NextId++ );
            p_human->SetGender( gender );
            p_human->SetAge( ageDays );
            p_human->GetProperties()->Add( IPKeyValue( rPropertyName, rPropertyValue ) );

            m_hic_list.push_back( p_hic );
            m_human_list.push_back( p_human );

            return (IIndividualHumanContext*)p_human;
        }

        INodeContext* CreateNode( int id, float baseAgeYears, std::string np_kv_str )
        {
            suids::suid s_id;
            s_id.data = id;

            INodeEventContextFake* p_nec = new INodeEventContextFake();
            p_nec->Initialize();
            INodeContextFake* p_nc = new INodeContextFake( s_id, p_nec );
            p_nc->GetNodeProperties().Add( NPKeyValue( np_kv_str ) );

            p_nec->Add( CreateHuman( p_nc, p_nec, Gender::FEMALE, (baseAgeYears-5) * DAYSPERYEAR, "Location", "URBAN" ) ); // filtered out by age

            p_nec->Add( CreateHuman( p_nc, p_nec, Gender::FEMALE, (baseAgeYears+1) * DAYSPERYEAR, "Location", "URBAN" ) ); // filtered out by gender
            p_nec->Add( CreateHuman( p_nc, p_nec, Gender::FEMALE, (baseAgeYears+2) * DAYSPERYEAR, "Location", "URBAN" ) ); // filtered out by gender

            p_nec->Add( CreateHuman( p_nc, p_nec, Gender::MALE, (baseAgeYears+1) * DAYSPERYEAR, "Location", "RURAL" ) ); // filtered out by IP-URBAN
            p_nec->Add( CreateHuman( p_nc, p_nec, Gender::MALE, (baseAgeYears+2) * DAYSPERYEAR, "Location", "RURAL" ) ); // filtered out by IP-URBAN

            p_nec->Add( CreateHuman( p_nc, p_nec, Gender::MALE, (baseAgeYears+0) * DAYSPERYEAR, "Location", "URBAN" ) );
            p_nec->Add( CreateHuman( p_nc, p_nec, Gender::MALE, (baseAgeYears+1) * DAYSPERYEAR, "Location", "URBAN" ) );
            p_nec->Add( CreateHuman( p_nc, p_nec, Gender::MALE, (baseAgeYears+2) * DAYSPERYEAR, "Location", "URBAN" ) );
            p_nec->Add( CreateHuman( p_nc, p_nec, Gender::MALE, (baseAgeYears+3) * DAYSPERYEAR, "Location", "URBAN" ) );
            p_nec->Add( CreateHuman( p_nc, p_nec, Gender::MALE, (baseAgeYears+4) * DAYSPERYEAR, "Location", "URBAN" ) );
            p_nec->Add( CreateHuman( p_nc, p_nec, Gender::MALE, (baseAgeYears+5) * DAYSPERYEAR, "Location", "URBAN" ) );
            p_nec->Add( CreateHuman( p_nc, p_nec, Gender::MALE, (baseAgeYears+6) * DAYSPERYEAR, "Location", "URBAN" ) );
            p_nec->Add( CreateHuman( p_nc, p_nec, Gender::MALE, (baseAgeYears+7) * DAYSPERYEAR, "Location", "URBAN" ) );
            p_nec->Add( CreateHuman( p_nc, p_nec, Gender::MALE, (baseAgeYears+8) * DAYSPERYEAR, "Location", "URBAN" ) );
            p_nec->Add( CreateHuman( p_nc, p_nec, Gender::MALE, (baseAgeYears+9) * DAYSPERYEAR, "Location", "URBAN" ) );

            p_nec->Add( CreateHuman( p_nc, p_nec, Gender::MALE, (baseAgeYears+9) * DAYSPERYEAR, "Location", "RURAL" ) ); // filtered out by IP-URBAN

            p_nec->Add( CreateHuman( p_nc, p_nec, Gender::MALE,   (baseAgeYears+15) * DAYSPERYEAR, "Location", "URBAN" ) ); // filtered out by age
            p_nec->Add( CreateHuman( p_nc, p_nec, Gender::MALE,   (baseAgeYears+20) * DAYSPERYEAR, "Location", "URBAN" ) ); // filtered out by age
            p_nec->Add( CreateHuman( p_nc, p_nec, Gender::MALE,   (baseAgeYears+25) * DAYSPERYEAR, "Location", "URBAN" ) ); // filtered out by age
            p_nec->Add( CreateHuman( p_nc, p_nec, Gender::FEMALE, (baseAgeYears+55) * DAYSPERYEAR, "Location", "URBAN" ) ); // filtered out by age

            return p_nc;
        }
    };

    class ActionEventListener : public IIndividualEventObserver
    {
    public:
        IMPLEMENT_NO_REFERENCE_COUNTING()
        virtual QueryResult QueryInterface( iid_t iid, void** pinstance ) { return QueryResult::e_NOINTERFACE; }

        ActionEventListener( const EventTrigger& rListeningForEvent )
            : m_Event( rListeningForEvent )
            , m_NumEventsHeard(0)
        {
        }

        virtual bool notifyOnEvent( IIndividualHumanEventContext *context, const EventTrigger& trigger )
        {
            if( m_Event == trigger )
            {
                ++m_NumEventsHeard;
            }
            return true;
        }

        const EventTrigger& GetListeningForEvent() const { return m_Event; }
        uint32_t GetNumEventsHeard() const { return m_NumEventsHeard; }
        void Reset() { m_NumEventsHeard = 0; }

    private:
        EventTrigger m_Event;
        uint32_t m_NumEventsHeard;
    };


    void BroadcastEvent( IIndividualHumanEventContext* pIHEC, const EventTrigger& rTrigger )
    {
        IIndividualEventBroadcaster* broadcaster = pIHEC->GetNodeEventContext()->GetIndividualEventBroadcaster();
        broadcaster->TriggerObservers( pIHEC, rTrigger );
    }

    void BroadcastEvent_NewClinicalCase( IIndividualHumanEventContext* pIHEC )
    {
        BroadcastEvent( pIHEC, EventTrigger::NewClinicalCase );
    }

    void BroadcastEvent_NewSevereCase( IIndividualHumanEventContext* pIHEC )
    {
        BroadcastEvent( pIHEC, EventTrigger::NewSevereCase );
    }

    TEST_FIXTURE( IecFixture, TestRead )
    {
        // --------------------
        // --- Initialize test
        // --------------------
        unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/IncidenceEventCoordinatorTest/TestRead.json" ) );

        unique_ptr<INodeContext> p_nc_1( CreateNode( 1, 20.0, "Risk:HIGH" ) );
        unique_ptr<INodeContext> p_nc_2( CreateNode( 2, 20.0, "Risk:LOW"  ) ); // filtered out
        unique_ptr<INodeContext> p_nc_3( CreateNode( 3, 24.0, "Risk:HIGH" ) );

        ActionEventListener event_listener_1( m_ActionEvent1 );
        ActionEventListener event_listener_2( m_ActionEvent2 );

        INodeEventContextFake* p_nec_fake_1 = static_cast<INodeEventContextFake*>(p_nc_1->GetEventContext());
        INodeEventContextFake* p_nec_fake_2 = static_cast<INodeEventContextFake*>(p_nc_2->GetEventContext());
        INodeEventContextFake* p_nec_fake_3 = static_cast<INodeEventContextFake*>(p_nc_3->GetEventContext());

        p_nec_fake_1->RegisterObserver( &event_listener_1, event_listener_1.GetListeningForEvent() );
        p_nec_fake_2->RegisterObserver( &event_listener_1, event_listener_1.GetListeningForEvent() );
        p_nec_fake_3->RegisterObserver( &event_listener_1, event_listener_1.GetListeningForEvent() );

        p_nec_fake_1->RegisterObserver( &event_listener_2, event_listener_2.GetListeningForEvent() );
        p_nec_fake_2->RegisterObserver( &event_listener_2, event_listener_2.GetListeningForEvent() );
        p_nec_fake_3->RegisterObserver( &event_listener_2, event_listener_2.GetListeningForEvent() );

        ISimulationEventContextFake sec;
        sec.AddNode( p_nc_1->GetEventContext() );
        sec.AddNode( p_nc_2->GetEventContext() );
        sec.AddNode( p_nc_3->GetEventContext() );

        IncidenceEventCoordinator iec;
        iec.SetContextTo( &sec );

        try
        {
            bool configured = iec.Configure( p_config.get() );
            CHECK( configured );
        }
        catch( DetailedException& re )
        {
            PrintDebug( re.GetMsg() );
            CHECK( false );
        }

        // ----------------------
        // --- Test Adding Nodes
        // ----------------------
        iec.AddNode( p_nc_1->GetSuid() );
        iec.AddNode( p_nc_2->GetSuid() );
        iec.AddNode( p_nc_3->GetSuid() );

        // ---------------------------------------------------------------------
        // --- Test Update() / UpdateNodes() - No events to count or distribute
        // ---------------------------------------------------------------------
        CHECK_EQUAL( 0, event_listener_1.GetNumEventsHeard() );
        CHECK_EQUAL( 0, event_listener_2.GetNumEventsHeard() );

        // ---------------------------------------------------------------------------------
        // --- NOTE: Simulation calls SimulationEventContext::Update() 
        // --- which calls XXXEventCoordinator::Update(), UpdateNodes(), and IsFinished()
        // --- Simulation then loops through the nodes updating them.
        // --- This means the event coordinators are updated at the top of the update loop
        // --- and ICE's last counts will be for the previous time step.
        // ---------------------------------------------------------------------------------

        // -----------------------------------------
        // --- Test the following:
        //      - Able to count events from people in different nodes
        //      - filter on NodeProperties
        //      - filter on IndividualProperties
        //      - filter on Demographics
        //      - Events counted
        //      - Count Event Period && Events distributed
        //      - Num reps & time steps between reps
        // -----------------------------------------

        // -------------
        // --- TIME = 1 - Listen/count events
        // -------------
        //printf("time=1\n");
        sec.SetTime( IdmDateTime( 1.0 ) );
        iec.Update( 1.0 );
        iec.UpdateNodes( 1.0 );

        CHECK( !iec.IsFinished() );
        CHECK_EQUAL( 3, iec.GetNumReps() );
        CHECK_EQUAL( 1, iec.GetNumTimeStepsInRep() );
        CHECK_EQUAL( 0, iec.GetEventCount() );
        CHECK_EQUAL( 0, event_listener_1.GetNumEventsHeard() );
        CHECK_EQUAL( 0, event_listener_2.GetNumEventsHeard() );

        // simulate looping through nodes and updating them
        // Broadcast events that ICE is counting
        p_nc_1->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_NewClinicalCase ); // 10 people
        p_nc_2->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_NewClinicalCase ); // zero from this node because of node property
        p_nc_3->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_NewSevereCase   ); // 7 people

        CHECK_EQUAL( 17, iec.GetEventCount() );

        // -------------
        // --- TIME = 2 - Listen/count events
        // -------------
        //printf("time=2\n");
        sec.SetTime( IdmDateTime( 1.0 ) );
        iec.Update( 1.0 );
        iec.UpdateNodes( 1.0 );

        CHECK( !iec.IsFinished() );
        CHECK_EQUAL( 3, iec.GetNumReps() );
        CHECK_EQUAL( 2, iec.GetNumTimeStepsInRep() );
        CHECK_EQUAL( 17, iec.GetEventCount() );
        CHECK_EQUAL( 0, event_listener_1.GetNumEventsHeard() );
        CHECK_EQUAL( 0, event_listener_2.GetNumEventsHeard() );

        // simulate looping through nodes and updating them
        // Broadcast events that ICE is counting
        p_nc_1->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_NewClinicalCase ); // 10 people
        p_nc_2->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_NewClinicalCase ); // zero from this node because of node property
        p_nc_3->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_NewSevereCase   ); // 7 people

        CHECK_EQUAL( 34, iec.GetEventCount() );

        // -------------
        // --- TIME = 3 - Listen/count events
        // -------------

        //printf("time=3\n");
        sec.SetTime( IdmDateTime( 1.0 ) );
        iec.Update( 1.0 );
        iec.UpdateNodes( 1.0 );

        CHECK( !iec.IsFinished() );
        CHECK_EQUAL( 3, iec.GetNumReps() );
        CHECK_EQUAL( 3, iec.GetNumTimeStepsInRep() );
        CHECK_EQUAL( 34, iec.GetEventCount() );

        CHECK_EQUAL( 0, event_listener_1.GetNumEventsHeard() );
        CHECK_EQUAL( 0, event_listener_2.GetNumEventsHeard() );

        // simulate looping through nodes and updating them
        p_nc_1->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_NewClinicalCase ); // 10 people
        p_nc_2->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_NewClinicalCase ); // zero from this node because of node property
        p_nc_3->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_NewSevereCase   ); // 7 people

        CHECK_EQUAL( 51, iec.GetEventCount() );

        // -------------
        // --- TIME = 4 - !!!!! Stop counting, distribute actions/events !!!!!
        // -------------

        // -----------------------------------------------------------
        // --- Three periods will have been completed so the counting
        // --- should stop and any events broadcasted
        // -----------------------------------------------------------

        //printf("time=4\n");
        sec.SetTime( IdmDateTime( 1.0 ) );
        iec.Update( 1.0 );
        iec.UpdateNodes( 1.0 );

        CHECK( !iec.IsFinished() );
        CHECK_EQUAL( 3, iec.GetNumReps() );
        CHECK_EQUAL( 4, iec.GetNumTimeStepsInRep() );
        CHECK_EQUAL( 51, iec.GetEventCount() );
        CHECK_EQUAL(  0, event_listener_1.GetNumEventsHeard() );
        CHECK_EQUAL( 60, event_listener_2.GetNumEventsHeard() ); // 51 > 50 so Action_Event_2

        CHECK_EQUAL( 17, iec.GetQualifyingCount() );

        // simulate looping through nodes and updating them
        p_nc_1->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_NewClinicalCase ); // 10 people
        p_nc_2->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_NewClinicalCase ); // zero from this node because of node property
        p_nc_3->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_NewSevereCase   ); // 7 people

        // no events were added because ICE is done counting for this period.
        CHECK_EQUAL( 51, iec.GetEventCount() );

        // -------------
        // --- TIME = 5 - Wait for rep to finish, no counting
        // -------------
        event_listener_2.Reset();
        CHECK_EQUAL( 0, event_listener_1.GetNumEventsHeard() );
        CHECK_EQUAL( 0, event_listener_2.GetNumEventsHeard() );

        //printf("time=5\n");
        sec.SetTime( IdmDateTime( 1.0 ) );
        iec.Update( 1.0 );
        iec.UpdateNodes( 1.0 );

        CHECK( !iec.IsFinished() );
        CHECK_EQUAL( 3, iec.GetNumReps() );
        CHECK_EQUAL( 5, iec.GetNumTimeStepsInRep() );
        CHECK_EQUAL( 51, iec.GetEventCount() );
        CHECK_EQUAL( 0, event_listener_1.GetNumEventsHeard() );
        CHECK_EQUAL( 0, event_listener_2.GetNumEventsHeard() );

        // simulate looping through nodes and updating them
        p_nc_1->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_NewClinicalCase ); // 10 people
        p_nc_2->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_NewClinicalCase ); // zero from this node because of node property
        p_nc_3->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_NewSevereCase   ); // 7 people

        // no events were added because ICE is done counting for this period.
        CHECK_EQUAL( 51, iec.GetEventCount() );

        // -------------
        // --- TIME = 6 - Wait for rep to finish, no counting
        // -------------
        //printf("time=6\n");
        sec.SetTime( IdmDateTime( 1.0 ) );
        iec.Update( 1.0 );
        iec.UpdateNodes( 1.0 );

        CHECK( !iec.IsFinished() );
        CHECK_EQUAL( 3, iec.GetNumReps() );
        CHECK_EQUAL( 6, iec.GetNumTimeStepsInRep() );
        CHECK_EQUAL( 51, iec.GetEventCount() );
        CHECK_EQUAL( 0, event_listener_1.GetNumEventsHeard() );
        CHECK_EQUAL( 0, event_listener_2.GetNumEventsHeard() );

        // simulate looping through nodes and updating them
        p_nc_1->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_NewClinicalCase ); // 10 people
        p_nc_2->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_NewClinicalCase ); // zero from this node because of node property
        p_nc_3->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_NewSevereCase   ); // 7 people

        // no events were added because ICE is done counting for this period.
        CHECK_EQUAL( 51, iec.GetEventCount() );

        // -------------
        // --- TIME = 7 - >>>>> Start new rep, listen/count events <<<<<
        // -------------

        //printf("time=7\n");
        sec.SetTime( IdmDateTime( 1.0 ) );
        iec.Update( 1.0 );
        iec.UpdateNodes( 1.0 );

        CHECK( !iec.IsFinished() );
        CHECK_EQUAL( 2, iec.GetNumReps() );
        CHECK_EQUAL( 1, iec.GetNumTimeStepsInRep() );
        CHECK_EQUAL( 0, iec.GetEventCount() );
        CHECK_EQUAL( 0, event_listener_1.GetNumEventsHeard() );
        CHECK_EQUAL( 0, event_listener_2.GetNumEventsHeard() );

        // simulate looping through nodes and updating them
        p_nc_1->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_NewClinicalCase ); // 10 people
        p_nc_2->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_NewClinicalCase ); // zero from this node because of node property
        p_nc_3->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_NewSevereCase   ); // 7 people

        CHECK_EQUAL( 17, iec.GetEventCount() );

        // -------------
        // --- TIME = 8 - Listen/count events
        // -------------
        //printf("time=8\n");
        sec.SetTime( IdmDateTime( 1.0 ) );
        iec.Update( 1.0 );
        iec.UpdateNodes( 1.0 );

        CHECK( !iec.IsFinished() );
        CHECK_EQUAL( 2, iec.GetNumReps() );
        CHECK_EQUAL( 2, iec.GetNumTimeStepsInRep() );
        CHECK_EQUAL( 17, iec.GetEventCount() );
        CHECK_EQUAL( 0, event_listener_1.GetNumEventsHeard() );
        CHECK_EQUAL( 0, event_listener_2.GetNumEventsHeard() );

        // Skip events so we get less

        CHECK_EQUAL( 17, iec.GetEventCount() );

        // -------------
        // --- TIME = 9 - Listen/count events
        // -------------
        //printf("time=9\n");
        sec.SetTime( IdmDateTime( 1.0 ) );
        iec.Update( 1.0 );
        iec.UpdateNodes( 1.0 );

        CHECK( !iec.IsFinished() );
        CHECK_EQUAL( 2, iec.GetNumReps() );
        CHECK_EQUAL( 3, iec.GetNumTimeStepsInRep() );
        CHECK_EQUAL( 17, iec.GetEventCount() );
        CHECK_EQUAL( 0, event_listener_1.GetNumEventsHeard() );
        CHECK_EQUAL( 0, event_listener_2.GetNumEventsHeard() );

        // simulate looping through nodes and updating them
        p_nc_1->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_NewClinicalCase ); // 10 people
        p_nc_2->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_NewClinicalCase ); // zero from this node because of node property
        p_nc_3->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_NewSevereCase   ); // 7 people

        CHECK_EQUAL( 34, iec.GetEventCount() );

        // -------------
        // --- TIME = 10 - !!!!! Stop counting, distribute actions/events !!!!!
        // -------------
        //printf("time=10\n");
        sec.SetTime( IdmDateTime( 1.0 ) );
        iec.Update( 1.0 );
        iec.UpdateNodes( 1.0 );

        CHECK( !iec.IsFinished() );
        CHECK_EQUAL( 2, iec.GetNumReps() );
        CHECK_EQUAL( 4, iec.GetNumTimeStepsInRep() );
        CHECK_EQUAL( 34, iec.GetEventCount() );
        CHECK_EQUAL( 60, event_listener_1.GetNumEventsHeard() ); // 50 > 34 > 20, so Action_Event_1
        CHECK_EQUAL( 0, event_listener_2.GetNumEventsHeard() );

        CHECK_EQUAL( 17, iec.GetQualifyingCount() );

        // simulate looping through nodes and updating them
        p_nc_1->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_NewClinicalCase ); // 10 people
        p_nc_2->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_NewClinicalCase ); // zero from this node because of node property
        p_nc_3->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_NewSevereCase ); // 7 people

        // Done counting so no new events added
        CHECK_EQUAL( 34, iec.GetEventCount() );

        // -------------
        // --- TIME = 11 - Wait for rep to finish, no counting
        // -------------
        event_listener_1.Reset();
        CHECK_EQUAL( 0, event_listener_1.GetNumEventsHeard() );
        CHECK_EQUAL( 0, event_listener_2.GetNumEventsHeard() );

        //printf("time=11\n");
        sec.SetTime( IdmDateTime( 1.0 ) );
        iec.Update( 1.0 );
        iec.UpdateNodes( 1.0 );

        CHECK( !iec.IsFinished() );
        CHECK_EQUAL( 2, iec.GetNumReps() );
        CHECK_EQUAL( 5, iec.GetNumTimeStepsInRep() );
        CHECK_EQUAL( 34, iec.GetEventCount() );
        CHECK_EQUAL( 0, event_listener_1.GetNumEventsHeard() );
        CHECK_EQUAL( 0, event_listener_2.GetNumEventsHeard() );

        // simulate looping through nodes and updating them
        p_nc_1->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_NewClinicalCase ); // 10 people
        p_nc_2->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_NewClinicalCase ); // zero from this node because of node property
        p_nc_3->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_NewSevereCase   ); // 7 people

        // Done counting so no new events added
        CHECK_EQUAL( 34, iec.GetEventCount() );

        // -------------
        // --- TIME = 12 - Wait for rep to finish, no counting
        // -------------
        //printf("time=12\n");
        sec.SetTime( IdmDateTime( 1.0 ) );
        iec.Update( 1.0 );
        iec.UpdateNodes( 1.0 );

        CHECK( !iec.IsFinished() );
        CHECK_EQUAL( 2, iec.GetNumReps() );
        CHECK_EQUAL( 6, iec.GetNumTimeStepsInRep() );
        CHECK_EQUAL( 34, iec.GetEventCount() );
        CHECK_EQUAL( 0, event_listener_1.GetNumEventsHeard() );
        CHECK_EQUAL( 0, event_listener_2.GetNumEventsHeard() );

        // simulate looping through nodes and updating them
        p_nc_1->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_NewClinicalCase ); // 10 people
        p_nc_2->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_NewClinicalCase ); // zero from this node because of node property
        p_nc_3->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_NewSevereCase ); // 7 people

        // Done counting so new events added
        CHECK_EQUAL( 34, iec.GetEventCount() );

        // -------------
        // --- TIME = 13 - >>>>> Start new rep, listen/count events <<<<<
        // -------------
        //printf("time=13\n");
        sec.SetTime( IdmDateTime( 1.0 ) );
        iec.Update( 1.0 );
        iec.UpdateNodes( 1.0 );

        CHECK( !iec.IsFinished() );
        CHECK_EQUAL( 1, iec.GetNumReps() );
        CHECK_EQUAL( 1, iec.GetNumTimeStepsInRep() );
        CHECK_EQUAL( 0, iec.GetEventCount() );
        CHECK_EQUAL( 0, event_listener_1.GetNumEventsHeard() );
        CHECK_EQUAL( 0, event_listener_2.GetNumEventsHeard() );

        // Skip events

        CHECK_EQUAL( 0, iec.GetEventCount() );

        // -------------
        // --- TIME = 14 - Listen/count events
        // -------------
        //printf("time=14\n");
        sec.SetTime( IdmDateTime( 1.0 ) );
        iec.Update( 1.0 );
        iec.UpdateNodes( 1.0 );

        CHECK( !iec.IsFinished() );
        CHECK_EQUAL( 1, iec.GetNumReps() );
        CHECK_EQUAL( 2, iec.GetNumTimeStepsInRep() );
        CHECK_EQUAL( 0, iec.GetEventCount() );
        CHECK_EQUAL( 0, event_listener_1.GetNumEventsHeard() );
        CHECK_EQUAL( 0, event_listener_2.GetNumEventsHeard() );

        // simulate looping through nodes and updating them
        p_nc_1->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_NewClinicalCase ); // 10 people
        p_nc_2->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_NewClinicalCase ); // zero from this node because of node property
        p_nc_3->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_NewSevereCase ); // 7 people

        CHECK_EQUAL( 17, iec.GetEventCount() );

        // -------------
        // --- TIME = 15 - Listen/count events
        // -------------
        //printf("time=15\n");
        sec.SetTime( IdmDateTime( 1.0 ) );
        iec.Update( 1.0 );
        iec.UpdateNodes( 1.0 );

        CHECK( !iec.IsFinished() );
        CHECK_EQUAL( 1, iec.GetNumReps() );
        CHECK_EQUAL( 3, iec.GetNumTimeStepsInRep() );
        CHECK_EQUAL( 17, iec.GetEventCount() );
        CHECK_EQUAL( 0, event_listener_1.GetNumEventsHeard() );
        CHECK_EQUAL( 0, event_listener_2.GetNumEventsHeard() );

        // Skip events so we end up with less than 20

        CHECK_EQUAL( 17, iec.GetEventCount() );

        // -------------
        // --- TIME = 16 - !!!!! Stop counting, distribute actions/events !!!!!
        // -------------
        //printf("time=16\n");
        sec.SetTime( IdmDateTime( 1.0 ) );
        iec.Update( 1.0 );
        iec.UpdateNodes( 1.0 );

        CHECK( !iec.IsFinished() );
        CHECK_EQUAL( 1, iec.GetNumReps() );
        CHECK_EQUAL( 4, iec.GetNumTimeStepsInRep() );
        CHECK_EQUAL( 17, iec.GetEventCount() );

        // No events distributed because less than smallest threshold
        CHECK_EQUAL( 0, event_listener_1.GetNumEventsHeard() );
        CHECK_EQUAL( 0, event_listener_2.GetNumEventsHeard() );

        CHECK_EQUAL( 17, iec.GetQualifyingCount() );


        // simulate looping through nodes and updating them
        p_nc_1->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_NewClinicalCase ); // 10 people
        p_nc_2->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_NewClinicalCase ); // zero from this node because of node property
        p_nc_3->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_NewSevereCase ); // 7 people

        // Done Counting
        CHECK_EQUAL( 17, iec.GetEventCount() );

        // -------------
        // --- TIME = 17 - Wait for rep to finish, no counting
        // -------------
        //printf("time=17\n");
        sec.SetTime( IdmDateTime( 1.0 ) );
        iec.Update( 1.0 );
        iec.UpdateNodes( 1.0 );

        CHECK( !iec.IsFinished() );
        CHECK_EQUAL( 1, iec.GetNumReps() );
        CHECK_EQUAL( 5, iec.GetNumTimeStepsInRep() );
        CHECK_EQUAL( 17, iec.GetEventCount() );

        // No events distributed because 17 < 20
        CHECK_EQUAL( 0, event_listener_1.GetNumEventsHeard() );
        CHECK_EQUAL( 0, event_listener_2.GetNumEventsHeard() );

        // simulate looping through nodes and updating them
        p_nc_1->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_NewClinicalCase ); // 10 people
        p_nc_2->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_NewClinicalCase ); // zero from this node because of node property
        p_nc_3->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_NewSevereCase   ); // 7 people

        // Done Counting
        CHECK_EQUAL( 17, iec.GetEventCount() );

        // -------------
        // --- TIME = 18 - Wait for rep to finish, no counting
        // -------------
        //printf("time=18\n");
        sec.SetTime( IdmDateTime( 1.0 ) );
        iec.Update( 1.0 );
        iec.UpdateNodes( 1.0 );

        CHECK( !iec.IsFinished() );
        CHECK_EQUAL( 1, iec.GetNumReps() );
        CHECK_EQUAL( 6, iec.GetNumTimeStepsInRep() );
        CHECK_EQUAL( 17, iec.GetEventCount() );
        CHECK_EQUAL( 0, event_listener_1.GetNumEventsHeard() );
        CHECK_EQUAL( 0, event_listener_2.GetNumEventsHeard() );

        // simulate looping through nodes and updating them
        p_nc_1->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_NewClinicalCase ); // 10 people
        p_nc_2->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_NewClinicalCase ); // zero from this node because of node property
        p_nc_3->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_NewSevereCase   ); // 7 people

        // No new events & Done Counting
        CHECK_EQUAL( 17, iec.GetEventCount() );

        // -------------
        // --- TIME = 19 - <<<<<<< Expire >>>>>>>
        // -------------
        //printf("time=19\n");
        sec.SetTime( IdmDateTime( 1.0 ) );
        iec.Update( 1.0 );
        iec.UpdateNodes( 1.0 );

        CHECK( iec.IsFinished() );

        CHECK_EQUAL( 0, iec.GetNumReps() );
        CHECK_EQUAL( 1, iec.GetNumTimeStepsInRep() );
        CHECK_EQUAL( 0, iec.GetEventCount() );
        CHECK_EQUAL( 0, event_listener_1.GetNumEventsHeard() );
        CHECK_EQUAL( 0, event_listener_2.GetNumEventsHeard() );

        // No new events & Done Counting
        CHECK_EQUAL( 0, iec.GetEventCount() );
    }

    void TestHelper_Exception( int lineNumber, const std::string& rFilename, const std::string& rExpMsg )
    {
        try
        {
            unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( rFilename ) );

            IncidenceEventCoordinator iec;

            iec.Configure( p_config.get() );

            CHECK_LN( false, lineNumber ); // should not get here
        }
        catch( DetailedException& re )
        {
            std::string msg = re.GetMsg();
            if( msg.find( rExpMsg ) == string::npos )
            {
                PrintDebug( msg );
                CHECK_LN( msg.find( rExpMsg ) != string::npos, lineNumber );
            }
        }
    }

    TEST_FIXTURE( IecFixture, TestNoEventDefined )
    {
        TestHelper_Exception( __LINE__, "testdata/IncidenceEventCoordinatorTest/TestNoEventDefined.json",
                              "You must define Event_To_Broadcast" );
    }

    TEST_FIXTURE( IecFixture, TestEqualThresholds )
    {
        TestHelper_Exception( __LINE__, "testdata/IncidenceEventCoordinatorTest/TestEqualThresholds.json",
                              "More than one action has a Threshold equal to 20.  Threshold values must be unique." );
    }

    TEST_FIXTURE( IecFixture, TestNoActions )
    {
        TestHelper_Exception( __LINE__, "testdata/IncidenceEventCoordinatorTest/TestNoActions.json",
                              "At least one action must be defined for the IncidenceEventCoordinator." );
    }

    TEST_FIXTURE( IecFixture, TestBadDurations )
    {
        TestHelper_Exception( __LINE__, "testdata/IncidenceEventCoordinatorTest/TestBadDurations.json",
                              "Variable or parameter 'Timesteps_Between_Repetitions' with value 6 is incompatible with variable or parameter 'Count_Events_For_Num_Timesteps' with value 77. 'Timesteps_Between_Repetitions' must be >= 'Count_Events_For_Num_Timesteps'" );
    }
}