
#include "stdafx.h"
#include <memory> // unique_ptr
#include "UnitTest++.h"
#include "CommunityHealthWorkerEventCoordinator.h"
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
#include "componentTests.h"

using namespace std; 
using namespace Kernel; 

SUITE(CommunityHealthWorkerEventCoordinatorTest)
{
    static int m_NextId = 1;

    struct ChwFixture
    {
        std::vector< IndividualHumanInterventionsContextFake* > m_hic_list ;
        std::vector< IndividualHumanContextFake*              > m_human_list ;
        IdmMpi::MessageInterface* m_pMpi;
        SimulationConfig* m_pSimulationConfig ;
        EventTrigger m_ListenForEvent;
        EventTrigger m_EventFromIntervention;

        ChwFixture()
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
            int argc      = 1;
            char* exeName = "componentTests.exe";
            char** argv   = &exeName;
            string configFilename("testdata/CommunityHealthWorkerEventCoordinatorTest/config.json");
            string inputPath("testdata/CommunityHealthWorkerEventCoordinatorTest");
            string outputPath("testdata/CommunityHealthWorkerEventCoordinatorTest");
            string statePath("testdata/CommunityHealthWorkerEventCoordinatorTest");
            string dllPath("");
            Environment::Initialize( m_pMpi, configFilename, inputPath, outputPath, /*statePath, */dllPath, false);

            Environment::setSimulationConfig( m_pSimulationConfig );

            EventTriggerFactory::DeleteInstance();
            EventTriggerFactory::GetInstance()->Configure( EnvPtr->Config );
            m_ListenForEvent        = EventTriggerFactory::GetInstance()->CreateUserEventTrigger( "ListenForEvent"        );
            m_EventFromIntervention = EventTriggerFactory::GetInstance()->CreateUserEventTrigger( "EventFromIntervention" );

            IPFactory::DeleteFactory();
            IPFactory::CreateFactory();

            std::map<std::string, float> ip_values ;
            ip_values.insert( std::make_pair( "URBAN", 0.5f ) );
            ip_values.insert( std::make_pair( "RURAL", 0.5f ) );

            IPFactory::GetInstance()->AddIP( 1, "Location", ip_values );
        }

        ~ChwFixture()
        {
            for( auto hic : m_hic_list )
            {
                delete hic ;
            }
            m_hic_list.clear();

            for( auto human : m_human_list )
            {
                delete human ;
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

            return (IIndividualHumanContext*)p_human ;
        }

        INodeContext* CreateNode( int id )
        {
            suids::suid s_id ;
            s_id.data = id;

            INodeEventContextFake* p_nec = new INodeEventContextFake();
            p_nec->Initialize();
            INodeContextFake* p_nc = new INodeContextFake( s_id, p_nec );

            p_nec->Add( CreateHuman( p_nc, p_nec, Gender::FEMALE, 15*DAYSPERYEAR, "Location", "URBAN" ) ); // filtered out by age

            p_nec->Add( CreateHuman( p_nc, p_nec, Gender::FEMALE, 21*DAYSPERYEAR, "Location", "URBAN" ) ); // filtered out by gender
            p_nec->Add( CreateHuman( p_nc, p_nec, Gender::FEMALE, 22*DAYSPERYEAR, "Location", "URBAN" ) ); // filtered out by gender

            p_nec->Add( CreateHuman( p_nc, p_nec, Gender::MALE,   21*DAYSPERYEAR, "Location", "RURAL" ) ); // filtered out by IP-URBAN
            p_nec->Add( CreateHuman( p_nc, p_nec, Gender::MALE,   22*DAYSPERYEAR, "Location", "RURAL" ) ); // filtered out by IP-URBAN

            p_nec->Add( CreateHuman( p_nc, p_nec, Gender::MALE,   20*DAYSPERYEAR, "Location", "URBAN" ) );
            p_nec->Add( CreateHuman( p_nc, p_nec, Gender::MALE,   21*DAYSPERYEAR, "Location", "URBAN" ) );
            p_nec->Add( CreateHuman( p_nc, p_nec, Gender::MALE,   22*DAYSPERYEAR, "Location", "URBAN" ) );
            p_nec->Add( CreateHuman( p_nc, p_nec, Gender::MALE,   23*DAYSPERYEAR, "Location", "URBAN" ) );
            p_nec->Add( CreateHuman( p_nc, p_nec, Gender::MALE,   24*DAYSPERYEAR, "Location", "URBAN" ) );
            p_nec->Add( CreateHuman( p_nc, p_nec, Gender::MALE,   25*DAYSPERYEAR, "Location", "URBAN" ) );
            p_nec->Add( CreateHuman( p_nc, p_nec, Gender::MALE,   26*DAYSPERYEAR, "Location", "URBAN" ) );
            p_nec->Add( CreateHuman( p_nc, p_nec, Gender::MALE,   27*DAYSPERYEAR, "Location", "URBAN" ) );
            p_nec->Add( CreateHuman( p_nc, p_nec, Gender::MALE,   28*DAYSPERYEAR, "Location", "URBAN" ) );
            p_nec->Add( CreateHuman( p_nc, p_nec, Gender::MALE,   29*DAYSPERYEAR, "Location", "URBAN" ) );

            p_nec->Add( CreateHuman( p_nc, p_nec, Gender::MALE,   29*DAYSPERYEAR, "Location", "RURAL" ) ); // filtered out by IP-URBAN

            p_nec->Add( CreateHuman( p_nc, p_nec, Gender::MALE,   35*DAYSPERYEAR, "Location", "URBAN" ) ); // filtered out by age
            p_nec->Add( CreateHuman( p_nc, p_nec, Gender::MALE,   40*DAYSPERYEAR, "Location", "URBAN" ) ); // filtered out by age
            p_nec->Add( CreateHuman( p_nc, p_nec, Gender::MALE,   45*DAYSPERYEAR, "Location", "URBAN" ) ); // filtered out by age
            p_nec->Add( CreateHuman( p_nc, p_nec, Gender::FEMALE, 75*DAYSPERYEAR, "Location", "URBAN" ) ); // filtered out by age

            return p_nc;
        }

    };

    void BroadcastEvent( IIndividualHumanEventContext* pIHEC, const EventTrigger& rTrigger )
    {
        IIndividualEventBroadcaster* broadcaster = pIHEC->GetNodeEventContext()->GetIndividualEventBroadcaster();
        broadcaster->TriggerObservers( pIHEC, rTrigger );
    }

    //typedef std::function<void (/*suids::suid, */IIndividualHumanEventContext*)> individual_visit_function_t;
    void BroadcastEvent_ListenForEvent( IIndividualHumanEventContext* pIHEC )
    {
        BroadcastEvent( pIHEC, EventTrigger("ListenForEvent") );
    }

#if 1
    TEST_FIXTURE(ChwFixture, TestHappyPathIndividuals)
    {
        // --------------------
        // --- Initialize test
        // --------------------
        unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/CommunityHealthWorkerEventCoordinatorTest/TestHappyPathIndividuals.json" ) );

        unique_ptr<INodeContext> p_nc_1( CreateNode( 1 ) );
        unique_ptr<INodeContext> p_nc_2( CreateNode( 2 ) );
        unique_ptr<INodeContext> p_nc_3( CreateNode( 3 ) );

        ISimulationEventContextFake sec;
        sec.AddNode( p_nc_1->GetEventContext() );
        sec.AddNode( p_nc_2->GetEventContext() );
        sec.AddNode( p_nc_3->GetEventContext() );

        CommunityHealthWorkerEventCoordinator chw;

        try
        {
            bool configured = chw.Configure( p_config.get() );
            CHECK( configured );

            // ----------------------
            // --- Test Adding Nodes
            // ----------------------
            chw.SetContextTo( &sec );
            chw.AddNode( p_nc_1->GetSuid() );
            chw.AddNode( p_nc_2->GetSuid() );
            chw.AddNode( p_nc_3->GetSuid() );

            CHECK_EQUAL(   7, chw.GetCurrentStock()       );
            CHECK_EQUAL(   1, chw.GetDaysToNextShipment() );
            CHECK_EQUAL(   0, chw.GetNumEntitiesInQueue() );
            CHECK_EQUAL( 100, chw.GetDaysRemaining()      );
        }
        catch( DetailedException& re )
        {
            PrintDebug( re.GetMsg() );
            CHECK( false );
        }

        // -----------------------------------
        // --- Test Update() & Updating stock
        // -----------------------------------
        //printf("time=1\n");
        sec.SetTime( IdmDateTime( 1.0 ) );
        chw.Update( 1.0 );

        CHECK_EQUAL(   7, chw.GetCurrentStock()       );
        CHECK_EQUAL(   0, chw.GetDaysToNextShipment() );
        CHECK_EQUAL(   0, chw.GetNumEntitiesInQueue() );
        CHECK_EQUAL(  99, chw.GetDaysRemaining()      );

        //printf("time=2\n");
        sec.SetTime( IdmDateTime( 2.0 ) );
        chw.Update( 1.0 );

        CHECK_EQUAL( 107, chw.GetCurrentStock()       ); // updated stock with new shipment
        CHECK_EQUAL(  20, chw.GetDaysToNextShipment() );
        CHECK_EQUAL(   0, chw.GetNumEntitiesInQueue() );
        CHECK_EQUAL(  98, chw.GetDaysRemaining()      );

        // ----------------------------------------------------------------------------
        // --- Add individuals to the queue by broadcasting event we are listening for
        // ----------------------------------------------------------------------------
        p_nc_1->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_ListenForEvent );
        CHECK_EQUAL( 20, chw.GetNumEntitiesInQueue() );

        p_nc_2->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_ListenForEvent );
        CHECK_EQUAL( 40, chw.GetNumEntitiesInQueue() );

        p_nc_3->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_ListenForEvent );
        CHECK_EQUAL( 60, chw.GetNumEntitiesInQueue() );

        // ------------------------------------------
        // --- Test removal of people from the queue
        // ------------------------------------------
        INodeEventContextFake* p_nec_1 = dynamic_cast<INodeEventContextFake*>( p_nc_1->GetEventContext() );
        INodeEventContextFake* p_nec_2 = dynamic_cast<INodeEventContextFake*>( p_nc_2->GetEventContext() );
        INodeEventContextFake* p_nec_3 = dynamic_cast<INodeEventContextFake*>( p_nc_3->GetEventContext() );

        BroadcastEvent( p_nec_1->GetIndividualById(  1 )->GetEventContext(), EventTrigger::Emigrating );
        BroadcastEvent( p_nec_1->GetIndividualById(  2 )->GetEventContext(), EventTrigger::Emigrating );
        BroadcastEvent( p_nec_1->GetIndividualById( 20 )->GetEventContext(), EventTrigger::Emigrating );
        CHECK_EQUAL( 57, chw.GetNumEntitiesInQueue() );

        BroadcastEvent( p_nec_2->GetIndividualById( 23 )->GetEventContext(), EventTrigger::Emigrating );
        BroadcastEvent( p_nec_2->GetIndividualById( 24 )->GetEventContext(), EventTrigger::Emigrating );
        BroadcastEvent( p_nec_2->GetIndividualById( 39 )->GetEventContext(), EventTrigger::Emigrating );
        CHECK_EQUAL( 54, chw.GetNumEntitiesInQueue() );

        BroadcastEvent( p_nec_3->GetIndividualById( 45 )->GetEventContext(), EventTrigger::Emigrating );
        BroadcastEvent( p_nec_3->GetIndividualById( 57 )->GetEventContext(), EventTrigger::Emigrating );
        BroadcastEvent( p_nec_3->GetIndividualById( 58 )->GetEventContext(), EventTrigger::Emigrating );
        BroadcastEvent( p_nec_3->GetIndividualById( 59 )->GetEventContext(), EventTrigger::Emigrating );
        CHECK_EQUAL( 50, chw.GetNumEntitiesInQueue() );

        // ----------------------
        // --- Test UpdateNodes()
        // ----------------------
        chw.UpdateNodes( 1.0 );

        CHECK_EQUAL( 107, chw.GetCurrentStock()       );
        CHECK_EQUAL(  20, chw.GetDaysToNextShipment() );
        CHECK_EQUAL(  50, chw.GetNumEntitiesInQueue() ); // entities add this time step so not processed
        CHECK_EQUAL(  98, chw.GetDaysRemaining()      );

        //printf("time=3\n");
        sec.SetTime( IdmDateTime( 3.0 ) );
        chw.Update( 1.0 );

        CHECK_EQUAL( 107, chw.GetCurrentStock()       );
        CHECK_EQUAL(  19, chw.GetDaysToNextShipment() );
        CHECK_EQUAL(  50, chw.GetNumEntitiesInQueue() ); // no one timed-out
        CHECK_EQUAL(  97, chw.GetDaysRemaining()      );

        chw.UpdateNodes( 1.0 );

        CHECK_EQUAL( 102, chw.GetCurrentStock()       );
        CHECK_EQUAL(  19, chw.GetDaysToNextShipment() );
        CHECK_EQUAL(  42, chw.GetNumEntitiesInQueue() ); // 3 did not qualify + 5 distributed
        CHECK_EQUAL(  97, chw.GetDaysRemaining()      );

        //printf("time=4\n");
        sec.SetTime( IdmDateTime( 4.0 ) );
        chw.Update( 1.0 );

        CHECK_EQUAL( 102, chw.GetCurrentStock()       );
        CHECK_EQUAL(  18, chw.GetDaysToNextShipment() );
        CHECK_EQUAL(  42, chw.GetNumEntitiesInQueue() ); // no one timed-out
        CHECK_EQUAL(  96, chw.GetDaysRemaining()      );

        chw.UpdateNodes( 1.0 );

        CHECK_EQUAL(  97, chw.GetCurrentStock()       );
        CHECK_EQUAL(  18, chw.GetDaysToNextShipment() );
        CHECK_EQUAL(  37, chw.GetNumEntitiesInQueue() ); // 5 distributed
        CHECK_EQUAL(  96, chw.GetDaysRemaining()      );

        //printf("time=5\n");
        sec.SetTime( IdmDateTime( 5.0 ) );
        chw.Update( 1.0 );

        CHECK_EQUAL(  97, chw.GetCurrentStock()       );
        CHECK_EQUAL(  17, chw.GetDaysToNextShipment() );
        CHECK_EQUAL(  37, chw.GetNumEntitiesInQueue() ); // no one timed-out
        CHECK_EQUAL(  95, chw.GetDaysRemaining()      );

        chw.UpdateNodes( 1.0 );

        CHECK_EQUAL(  92, chw.GetCurrentStock()       );
        CHECK_EQUAL(  17, chw.GetDaysToNextShipment() );
        CHECK_EQUAL(  25, chw.GetNumEntitiesInQueue() ); // 7 did not qualify + 5 distributed
        CHECK_EQUAL(  95, chw.GetDaysRemaining()      );

        //printf("time=6\n");
        sec.SetTime( IdmDateTime( 6.0 ) );

        p_nc_2->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_ListenForEvent );

        chw.Update( 1.0 );

        CHECK_EQUAL(  92, chw.GetCurrentStock()       );
        CHECK_EQUAL(  16, chw.GetDaysToNextShipment() );
        CHECK_EQUAL(  45, chw.GetNumEntitiesInQueue() ); // no one timed-out and 20 added to queue
        CHECK_EQUAL(  94, chw.GetDaysRemaining()      );

        chw.UpdateNodes( 1.0 );

        CHECK_EQUAL(  87, chw.GetCurrentStock()       );
        CHECK_EQUAL(  16, chw.GetDaysToNextShipment() );
        CHECK_EQUAL(  40, chw.GetNumEntitiesInQueue() ); // 5 distributed
        CHECK_EQUAL(  94, chw.GetDaysRemaining()      );

        //printf("time=7\n");
        sec.SetTime( IdmDateTime( 7.0 ) );
        chw.Update( 1.0 );

        CHECK_EQUAL(  87, chw.GetCurrentStock()       );
        CHECK_EQUAL(  15, chw.GetDaysToNextShipment() );
        CHECK_EQUAL(  40, chw.GetNumEntitiesInQueue() );
        CHECK_EQUAL(  93, chw.GetDaysRemaining()      );

        chw.UpdateNodes( 1.0 );

        CHECK_EQUAL(  82, chw.GetCurrentStock()       );
        CHECK_EQUAL(  15, chw.GetDaysToNextShipment() );
        CHECK_EQUAL(  27, chw.GetNumEntitiesInQueue() ); // 8 did not qualify + 5 distributed
        CHECK_EQUAL(  93, chw.GetDaysRemaining()      );

        //printf("time=8\n");
        sec.SetTime( IdmDateTime( 8.0 ) );
        chw.Update( 1.0 );

        CHECK_EQUAL(  82, chw.GetCurrentStock()       );
        CHECK_EQUAL(  14, chw.GetDaysToNextShipment() );
        CHECK_EQUAL(  20, chw.GetNumEntitiesInQueue() ); // 7 timed-out
        CHECK_EQUAL(  92, chw.GetDaysRemaining()      );

        chw.UpdateNodes( 1.0 );

        CHECK_EQUAL(  77, chw.GetCurrentStock()       );
        CHECK_EQUAL(  14, chw.GetDaysToNextShipment() );
        CHECK_EQUAL(  10, chw.GetNumEntitiesInQueue() ); // 5 not qualified + 5 distributed
        CHECK_EQUAL(  92, chw.GetDaysRemaining()      );

        //printf("time=9\n");
        sec.SetTime( IdmDateTime( 9.0 ) );
        chw.Update( 1.0 );

        CHECK_EQUAL(  77, chw.GetCurrentStock()       );
        CHECK_EQUAL(  13, chw.GetDaysToNextShipment() );
        CHECK_EQUAL(  10, chw.GetNumEntitiesInQueue() ); // no one timed-out
        CHECK_EQUAL(  91, chw.GetDaysRemaining()      );

        chw.UpdateNodes( 1.0 );

        CHECK_EQUAL(  72, chw.GetCurrentStock()       );
        CHECK_EQUAL(  13, chw.GetDaysToNextShipment() );
        CHECK_EQUAL(   5, chw.GetNumEntitiesInQueue() ); // 5 distributed
        CHECK_EQUAL(  91, chw.GetDaysRemaining()      );
    }

    TEST_FIXTURE(ChwFixture, TestHappyPathNodes)
    {
        // --------------------
        // --- Initialize test
        // --------------------
        unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/CommunityHealthWorkerEventCoordinatorTest/TestHappyPathNodes.json" ) );

        unique_ptr<INodeContext> p_nc_1( CreateNode( 1 ) );
        unique_ptr<INodeContext> p_nc_2( CreateNode( 2 ) );
        unique_ptr<INodeContext> p_nc_3( CreateNode( 3 ) );

        ISimulationEventContextFake sec;
        sec.AddNode( p_nc_1->GetEventContext() );
        sec.AddNode( p_nc_2->GetEventContext() );
        sec.AddNode( p_nc_3->GetEventContext() );

        CommunityHealthWorkerEventCoordinator chw;

        try
        {
            bool configured = chw.Configure( p_config.get() );
            CHECK( configured );

            // ----------------------
            // --- Test Adding Nodes
            // ----------------------
            chw.SetContextTo( &sec );
            chw.AddNode( p_nc_1->GetSuid() );
            chw.AddNode( p_nc_2->GetSuid() );
            chw.AddNode( p_nc_3->GetSuid() );

            CHECK_EQUAL(   6, chw.GetCurrentStock()       );
            CHECK_EQUAL(   6, chw.GetDaysToNextShipment() );
            CHECK_EQUAL(   0, chw.GetNumEntitiesInQueue() );
            CHECK_EQUAL( 100, chw.GetDaysRemaining()      );
        }
        catch( DetailedException& re )
        {
            PrintDebug( re.GetMsg() );
            CHECK( false );
        }

        // -----------------------------------
        // --- Test Update() & UpdateNodes()
        // -----------------------------------
        //printf("time=1\n");
        sec.SetTime( IdmDateTime( 1.0 ) );
        chw.Update( 1.0 );

        CHECK_EQUAL(   6, chw.GetCurrentStock()       );
        CHECK_EQUAL(   5, chw.GetDaysToNextShipment() );
        CHECK_EQUAL(   0, chw.GetNumEntitiesInQueue() );
        CHECK_EQUAL(  99, chw.GetDaysRemaining()      );

        chw.UpdateNodes( 1.0 );

        CHECK_EQUAL(   6, chw.GetCurrentStock()       );
        CHECK_EQUAL(   5, chw.GetDaysToNextShipment() );
        CHECK_EQUAL(   0, chw.GetNumEntitiesInQueue() );
        CHECK_EQUAL(  99, chw.GetDaysRemaining()      );

        //printf("time=2\n");
        sec.SetTime( IdmDateTime( 2.0 ) );
        chw.Update( 1.0 );

        CHECK_EQUAL(   6, chw.GetCurrentStock()       );
        CHECK_EQUAL(   4, chw.GetDaysToNextShipment() );
        CHECK_EQUAL(   0, chw.GetNumEntitiesInQueue() );
        CHECK_EQUAL(  98, chw.GetDaysRemaining()      );

        // ----------------------------------------------------------------------------
        // --- Add ndoes to the queue by broadcasting event we are listening for
        // --- A node can only enter the queue once per time step
        // ----------------------------------------------------------------------------
        p_nc_1->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_ListenForEvent );
        CHECK_EQUAL( 1, chw.GetNumEntitiesInQueue() );

        p_nc_2->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_ListenForEvent );
        CHECK_EQUAL( 2, chw.GetNumEntitiesInQueue() );

        p_nc_3->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_ListenForEvent );
        CHECK_EQUAL( 3, chw.GetNumEntitiesInQueue() );

        chw.UpdateNodes( 1.0 );

        CHECK_EQUAL(   6, chw.GetCurrentStock()       );
        CHECK_EQUAL(   4, chw.GetDaysToNextShipment() );
        CHECK_EQUAL(   3, chw.GetNumEntitiesInQueue() ); // Added 3 entities this time step, so not processed
        CHECK_EQUAL(  98, chw.GetDaysRemaining()      );

        //printf("time=3\n");
        sec.SetTime( IdmDateTime( 3.0 ) );
        chw.Update( 1.0 );

        CHECK_EQUAL(   6, chw.GetCurrentStock()       );
        CHECK_EQUAL(   3, chw.GetDaysToNextShipment() );
        CHECK_EQUAL(   3, chw.GetNumEntitiesInQueue() );
        CHECK_EQUAL(  97, chw.GetDaysRemaining()      );

        chw.UpdateNodes( 1.0 );

        CHECK_EQUAL(   5, chw.GetCurrentStock()       );
        CHECK_EQUAL(   3, chw.GetDaysToNextShipment() );
        CHECK_EQUAL(   2, chw.GetNumEntitiesInQueue() ); // 1 distributed
        CHECK_EQUAL(  97, chw.GetDaysRemaining()      );

        //printf("time=4\n");
        sec.SetTime( IdmDateTime( 4.0 ) );
        chw.Update( 1.0 );

        CHECK_EQUAL(   5, chw.GetCurrentStock()       );
        CHECK_EQUAL(   2, chw.GetDaysToNextShipment() );
        CHECK_EQUAL(   2, chw.GetNumEntitiesInQueue() );
        CHECK_EQUAL(  96, chw.GetDaysRemaining()      );

        chw.UpdateNodes( 1.0 );

        CHECK_EQUAL(   4, chw.GetCurrentStock()       );
        CHECK_EQUAL(   2, chw.GetDaysToNextShipment() );
        CHECK_EQUAL(   1, chw.GetNumEntitiesInQueue() ); // 1 distributed
        CHECK_EQUAL(  96, chw.GetDaysRemaining()      );

        //printf("time=5\n");
        sec.SetTime( IdmDateTime( 5.0 ) );
        chw.Update( 1.0 );

        CHECK_EQUAL(   4, chw.GetCurrentStock()       );
        CHECK_EQUAL(   1, chw.GetDaysToNextShipment() );
        CHECK_EQUAL(   1, chw.GetNumEntitiesInQueue() );
        CHECK_EQUAL(  95, chw.GetDaysRemaining()      );

        chw.UpdateNodes( 1.0 );

        CHECK_EQUAL(   3, chw.GetCurrentStock()       );
        CHECK_EQUAL(   1, chw.GetDaysToNextShipment() );
        CHECK_EQUAL(   0, chw.GetNumEntitiesInQueue() ); // 1 distributed
        CHECK_EQUAL(  95, chw.GetDaysRemaining()      );

        //printf("time=6\n");
        sec.SetTime( IdmDateTime( 6.0 ) );
        chw.Update( 1.0 );

        CHECK_EQUAL(   3, chw.GetCurrentStock()       );
        CHECK_EQUAL(   0, chw.GetDaysToNextShipment() );
        CHECK_EQUAL(   0, chw.GetNumEntitiesInQueue() );
        CHECK_EQUAL(  94, chw.GetDaysRemaining()      );

        chw.UpdateNodes( 1.0 );

        CHECK_EQUAL(   3, chw.GetCurrentStock()       );
        CHECK_EQUAL(   0, chw.GetDaysToNextShipment() );
        CHECK_EQUAL(   0, chw.GetNumEntitiesInQueue() );
        CHECK_EQUAL(  94, chw.GetDaysRemaining()      );

        //printf("time=7\n");
        sec.SetTime( IdmDateTime( 7.0 ) );
        chw.Update( 1.0 );

        CHECK_EQUAL(  12, chw.GetCurrentStock()       ); // new shipment - max stock is 12
        CHECK_EQUAL(  10, chw.GetDaysToNextShipment() );
        CHECK_EQUAL(   0, chw.GetNumEntitiesInQueue() ); // 1 timed-out
        CHECK_EQUAL(  93, chw.GetDaysRemaining()      );

        chw.UpdateNodes( 1.0 );

        CHECK_EQUAL(  12, chw.GetCurrentStock()       );
        CHECK_EQUAL(  10, chw.GetDaysToNextShipment() );
        CHECK_EQUAL(   0, chw.GetNumEntitiesInQueue() ); // none distributed
        CHECK_EQUAL(  93, chw.GetDaysRemaining()      );

        //printf("time=8\n");
        sec.SetTime( IdmDateTime( 8.0 ) );
        chw.Update( 1.0 );

        CHECK_EQUAL(  12, chw.GetCurrentStock()       );
        CHECK_EQUAL(   9, chw.GetDaysToNextShipment() );
        CHECK_EQUAL(   0, chw.GetNumEntitiesInQueue() );
        CHECK_EQUAL(  92, chw.GetDaysRemaining()      );

        p_nc_1->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_ListenForEvent );
        CHECK_EQUAL(   1, chw.GetNumEntitiesInQueue() );

        chw.UpdateNodes( 1.0 );

        CHECK_EQUAL(  12, chw.GetCurrentStock()       );
        CHECK_EQUAL(   9, chw.GetDaysToNextShipment() );
        CHECK_EQUAL(   1, chw.GetNumEntitiesInQueue() ); // none distributed due to entering queue this time step
        CHECK_EQUAL(  92, chw.GetDaysRemaining()      );

        //printf("time=9\n");
        sec.SetTime( IdmDateTime( 9.0 ) );
        chw.Update( 1.0 );

        CHECK_EQUAL(  12, chw.GetCurrentStock()       );
        CHECK_EQUAL(   8, chw.GetDaysToNextShipment() );
        CHECK_EQUAL(   1, chw.GetNumEntitiesInQueue() );
        CHECK_EQUAL(  91, chw.GetDaysRemaining()      );

        p_nc_3->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_ListenForEvent );
        CHECK_EQUAL(   2, chw.GetNumEntitiesInQueue() );

        chw.UpdateNodes( 1.0 );

        CHECK_EQUAL(  11, chw.GetCurrentStock()       );
        CHECK_EQUAL(   8, chw.GetDaysToNextShipment() );
        CHECK_EQUAL(   1, chw.GetNumEntitiesInQueue() ); // 1 distributed
        CHECK_EQUAL(  91, chw.GetDaysRemaining()      );

        //printf("time=10\n");
        sec.SetTime( IdmDateTime( 10.0 ) );
        chw.Update( 1.0 );

        CHECK_EQUAL(  11, chw.GetCurrentStock()       );
        CHECK_EQUAL(   7, chw.GetDaysToNextShipment() );
        CHECK_EQUAL(   1, chw.GetNumEntitiesInQueue() );
        CHECK_EQUAL(  90, chw.GetDaysRemaining()      );

        chw.UpdateNodes( 1.0 );

        CHECK_EQUAL(  10, chw.GetCurrentStock()       );
        CHECK_EQUAL(   7, chw.GetDaysToNextShipment() );
        CHECK_EQUAL(   0, chw.GetNumEntitiesInQueue() ); // 1 distributed
        CHECK_EQUAL(  90, chw.GetDaysRemaining()      );
    }
#endif

    TEST_FIXTURE(ChwFixture, TestDt5)
    {
        // --------------------
        // --- Initialize test
        // --------------------
        float dt = 5.0;
        unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/CommunityHealthWorkerEventCoordinatorTest/TestDt5.json" ) );

        unique_ptr<INodeContext> p_nc_1( CreateNode( 1 ) );
        unique_ptr<INodeContext> p_nc_2( CreateNode( 2 ) );
        unique_ptr<INodeContext> p_nc_3( CreateNode( 3 ) );
        unique_ptr<INodeContext> p_nc_4( CreateNode( 4 ) );
        unique_ptr<INodeContext> p_nc_5( CreateNode( 5 ) );
        unique_ptr<INodeContext> p_nc_6( CreateNode( 6 ) );
        unique_ptr<INodeContext> p_nc_7( CreateNode( 7 ) );
        unique_ptr<INodeContext> p_nc_8( CreateNode( 8 ) );
        unique_ptr<INodeContext> p_nc_9( CreateNode( 9 ) );

        ISimulationEventContextFake sec;
        sec.AddNode( p_nc_1->GetEventContext() );
        sec.AddNode( p_nc_2->GetEventContext() );
        sec.AddNode( p_nc_3->GetEventContext() );
        sec.AddNode( p_nc_4->GetEventContext() );
        sec.AddNode( p_nc_5->GetEventContext() );
        sec.AddNode( p_nc_6->GetEventContext() );
        sec.AddNode( p_nc_7->GetEventContext() );
        sec.AddNode( p_nc_8->GetEventContext() );
        sec.AddNode( p_nc_9->GetEventContext() );

        CommunityHealthWorkerEventCoordinator chw;

        try
        {
            bool configured = chw.Configure( p_config.get() );
            CHECK( configured );

            // ----------------------
            // --- Test Adding Nodes
            // ----------------------
            chw.SetContextTo( &sec );
            chw.AddNode( p_nc_1->GetSuid() );
            chw.AddNode( p_nc_2->GetSuid() );
            chw.AddNode( p_nc_3->GetSuid() );
            chw.AddNode( p_nc_4->GetSuid() );
            chw.AddNode( p_nc_5->GetSuid() );
            chw.AddNode( p_nc_6->GetSuid() );
            chw.AddNode( p_nc_7->GetSuid() );
            chw.AddNode( p_nc_8->GetSuid() );
            chw.AddNode( p_nc_9->GetSuid() );

            CHECK_EQUAL(  10, chw.GetCurrentStock()       );
            CHECK_EQUAL(  10, chw.GetDaysToNextShipment() );
            CHECK_EQUAL(   0, chw.GetNumEntitiesInQueue() );
            CHECK_EQUAL(  65, chw.GetDaysRemaining()      );
        }
        catch( DetailedException& re )
        {
            PrintDebug( re.GetMsg() );
            CHECK( false );
        }

        // -----------------------------------
        // --- Test Update() & UpdateNodes()
        // -----------------------------------
        //printf("time=5\n");
        sec.SetTime( IdmDateTime( 5.0 ) );
        chw.Update( dt );

        CHECK_EQUAL(  10, chw.GetCurrentStock()       );
        CHECK_EQUAL(   5, chw.GetDaysToNextShipment() );
        CHECK_EQUAL(   0, chw.GetNumEntitiesInQueue() );
        CHECK_EQUAL(  60, chw.GetDaysRemaining()      );

        chw.UpdateNodes( dt );

        CHECK_EQUAL(  10, chw.GetCurrentStock()       );
        CHECK_EQUAL(   5, chw.GetDaysToNextShipment() );
        CHECK_EQUAL(   0, chw.GetNumEntitiesInQueue() );
        CHECK_EQUAL(  60, chw.GetDaysRemaining()      );

        //printf("time=10\n");
        sec.SetTime( IdmDateTime( 10.0 ) );
        chw.Update( dt );

        CHECK_EQUAL(  10, chw.GetCurrentStock()       );
        CHECK_EQUAL(   0, chw.GetDaysToNextShipment() );
        CHECK_EQUAL(   0, chw.GetNumEntitiesInQueue() );
        CHECK_EQUAL(  55, chw.GetDaysRemaining()      );

        // events occurred via another event coordinator
        p_nc_1->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_ListenForEvent );
        p_nc_2->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_ListenForEvent );
        p_nc_3->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_ListenForEvent );
        p_nc_4->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_ListenForEvent );
        p_nc_5->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_ListenForEvent );
        p_nc_6->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_ListenForEvent );
        p_nc_7->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_ListenForEvent );
        p_nc_8->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_ListenForEvent );
        p_nc_9->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_ListenForEvent );
        CHECK_EQUAL(   9, chw.GetNumEntitiesInQueue() );

        chw.UpdateNodes( dt );

        CHECK_EQUAL(  10, chw.GetCurrentStock()       );
        CHECK_EQUAL(   0, chw.GetDaysToNextShipment() );
        CHECK_EQUAL(   9, chw.GetNumEntitiesInQueue() ); // none distributed because entered the queue this time step
        CHECK_EQUAL(  55, chw.GetDaysRemaining()      );

        //printf("time=15\n");
        sec.SetTime( IdmDateTime( 15.0 ) );
        chw.Update( dt );

        CHECK_EQUAL(  12, chw.GetCurrentStock()       ); // new shipment - max stock is 12
        CHECK_EQUAL(  30, chw.GetDaysToNextShipment() );
        CHECK_EQUAL(   9, chw.GetNumEntitiesInQueue() );
        CHECK_EQUAL(  50, chw.GetDaysRemaining()      );

        chw.UpdateNodes( dt );

        CHECK_EQUAL(   7, chw.GetCurrentStock()       );
        CHECK_EQUAL(  30, chw.GetDaysToNextShipment() );
        CHECK_EQUAL(   4, chw.GetNumEntitiesInQueue() ); // 5 distributed
        CHECK_EQUAL(  50, chw.GetDaysRemaining()      );

        //printf("time=20\n");
        sec.SetTime( IdmDateTime( 20.0 ) );
        chw.Update( dt );

        CHECK_EQUAL(   7, chw.GetCurrentStock()       );
        CHECK_EQUAL(  25, chw.GetDaysToNextShipment() );
        CHECK_EQUAL(   4, chw.GetNumEntitiesInQueue() );
        CHECK_EQUAL(  45, chw.GetDaysRemaining()      );

        p_nc_1->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_ListenForEvent );
        p_nc_2->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_ListenForEvent );
        p_nc_3->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_ListenForEvent );
        CHECK_EQUAL(  7, chw.GetNumEntitiesInQueue() );

        chw.UpdateNodes( dt );

        CHECK_EQUAL(   3, chw.GetCurrentStock()       );
        CHECK_EQUAL(  25, chw.GetDaysToNextShipment() );
        CHECK_EQUAL(   3, chw.GetNumEntitiesInQueue() ); // 4 distributed, did not distribute to the 3 just added
        CHECK_EQUAL(  45, chw.GetDaysRemaining()      );

        //printf("time=25\n");
        sec.SetTime( IdmDateTime( 25.0 ) );
        chw.Update( dt );

        CHECK_EQUAL(   3, chw.GetCurrentStock()       );
        CHECK_EQUAL(  20, chw.GetDaysToNextShipment() );
        CHECK_EQUAL(   3, chw.GetNumEntitiesInQueue() );
        CHECK_EQUAL(  40, chw.GetDaysRemaining()      );

        p_nc_4->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_ListenForEvent );
        p_nc_5->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_ListenForEvent );
        p_nc_6->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_ListenForEvent );
        CHECK_EQUAL( 6, chw.GetNumEntitiesInQueue() );

        chw.UpdateNodes( dt );

        CHECK_EQUAL(   0, chw.GetCurrentStock()       );
        CHECK_EQUAL(  20, chw.GetDaysToNextShipment() );
        CHECK_EQUAL(   3, chw.GetNumEntitiesInQueue() ); // 3 distributed - only three in stock and the other three were just added
        CHECK_EQUAL(  40, chw.GetDaysRemaining()      );

        //printf("time=30\n");
        sec.SetTime( IdmDateTime( 30.0 ) );
        chw.Update( dt );

        CHECK_EQUAL(   0, chw.GetCurrentStock()       );
        CHECK_EQUAL(  15, chw.GetDaysToNextShipment() );
        CHECK_EQUAL(   3, chw.GetNumEntitiesInQueue() );
        CHECK_EQUAL(  35, chw.GetDaysRemaining()      );

        p_nc_7->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_ListenForEvent );
        p_nc_8->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_ListenForEvent );
        p_nc_9->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_ListenForEvent );
        CHECK_EQUAL( 6, chw.GetNumEntitiesInQueue() );

        chw.UpdateNodes( dt );

        CHECK_EQUAL(   0, chw.GetCurrentStock()       );
        CHECK_EQUAL(  15, chw.GetDaysToNextShipment() );
        CHECK_EQUAL(   6, chw.GetNumEntitiesInQueue() ); // 0 distributed
        CHECK_EQUAL(  35, chw.GetDaysRemaining()      );

        //printf("time=35\n");
        sec.SetTime( IdmDateTime( 35.0 ) );
        chw.Update( dt );

        CHECK_EQUAL(   0, chw.GetCurrentStock()       );
        CHECK_EQUAL(  10, chw.GetDaysToNextShipment() );
        CHECK_EQUAL(   6, chw.GetNumEntitiesInQueue() );
        CHECK_EQUAL(  30, chw.GetDaysRemaining()      );

        p_nc_1->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_ListenForEvent );
        p_nc_2->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_ListenForEvent );
        p_nc_3->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_ListenForEvent );
        CHECK_EQUAL(  9, chw.GetNumEntitiesInQueue() );

        chw.UpdateNodes( dt );

        CHECK_EQUAL(   0, chw.GetCurrentStock()       );
        CHECK_EQUAL(  10, chw.GetDaysToNextShipment() );
        CHECK_EQUAL(   9, chw.GetNumEntitiesInQueue() ); // 0 distributed
        CHECK_EQUAL(  30, chw.GetDaysRemaining()      );

        //printf("time=40\n");
        sec.SetTime( IdmDateTime( 40.0 ) );
        chw.Update( dt );

        CHECK_EQUAL(   0, chw.GetCurrentStock()       );
        CHECK_EQUAL(   5, chw.GetDaysToNextShipment() );
        CHECK_EQUAL(   9, chw.GetNumEntitiesInQueue() );
        CHECK_EQUAL(  25, chw.GetDaysRemaining()      );

        p_nc_4->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_ListenForEvent );
        p_nc_5->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_ListenForEvent );
        p_nc_6->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_ListenForEvent );
        CHECK_EQUAL( 12, chw.GetNumEntitiesInQueue() );

        chw.UpdateNodes( dt );

        CHECK_EQUAL(   0, chw.GetCurrentStock()       );
        CHECK_EQUAL(   5, chw.GetDaysToNextShipment() );
        CHECK_EQUAL(  12, chw.GetNumEntitiesInQueue() ); // 2 distributed
        CHECK_EQUAL(  25, chw.GetDaysRemaining()      );

        //printf("time=45\n");
        sec.SetTime( IdmDateTime( 45.0 ) );
        chw.Update( dt );

        CHECK_EQUAL(   0, chw.GetCurrentStock()       );
        CHECK_EQUAL(   0, chw.GetDaysToNextShipment() );
        CHECK_EQUAL(  12, chw.GetNumEntitiesInQueue() );
        CHECK_EQUAL(  20, chw.GetDaysRemaining()      );

        p_nc_7->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_ListenForEvent );
        p_nc_8->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_ListenForEvent );
        p_nc_9->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_ListenForEvent );
        CHECK_EQUAL( 15, chw.GetNumEntitiesInQueue() );

        chw.UpdateNodes( dt );

        CHECK_EQUAL(   0, chw.GetCurrentStock()       );
        CHECK_EQUAL(   0, chw.GetDaysToNextShipment() );
        CHECK_EQUAL(  15, chw.GetNumEntitiesInQueue() ); // 5 distributed
        CHECK_EQUAL(  20, chw.GetDaysRemaining()      );

        //printf("time=50\n");
        sec.SetTime( IdmDateTime( 50.0 ) );
        chw.Update( dt );

        CHECK_EQUAL(  10, chw.GetCurrentStock()       );
        CHECK_EQUAL(  30, chw.GetDaysToNextShipment() );
        CHECK_EQUAL(  12, chw.GetNumEntitiesInQueue() ); // 3 timed-out
        CHECK_EQUAL(  15, chw.GetDaysRemaining()      );

        p_nc_1->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_ListenForEvent );
        p_nc_2->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_ListenForEvent );
        p_nc_3->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_ListenForEvent );
        CHECK_EQUAL( 15, chw.GetNumEntitiesInQueue() );

        chw.UpdateNodes( dt );

        CHECK_EQUAL(   5, chw.GetCurrentStock()       );
        CHECK_EQUAL(  30, chw.GetDaysToNextShipment() );
        CHECK_EQUAL(  10, chw.GetNumEntitiesInQueue() ); // 5 distributed
        CHECK_EQUAL(  15, chw.GetDaysRemaining()      );

        //printf("time=55\n");
        sec.SetTime( IdmDateTime( 55.0 ) );
        chw.Update( dt );

        CHECK_EQUAL(   5, chw.GetCurrentStock()       );
        CHECK_EQUAL(  25, chw.GetDaysToNextShipment() );
        CHECK_EQUAL(  10, chw.GetNumEntitiesInQueue() );
        CHECK_EQUAL(  10, chw.GetDaysRemaining()      );

        p_nc_4->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_ListenForEvent );
        p_nc_5->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_ListenForEvent );
        p_nc_6->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_ListenForEvent );
        CHECK_EQUAL( 13, chw.GetNumEntitiesInQueue() );

        chw.UpdateNodes( dt );

        CHECK_EQUAL(   0, chw.GetCurrentStock()       );
        CHECK_EQUAL(  25, chw.GetDaysToNextShipment() );
        CHECK_EQUAL(   8, chw.GetNumEntitiesInQueue() ); // 5 distributed
        CHECK_EQUAL(  10, chw.GetDaysRemaining()      );

        //printf("time=60\n");
        sec.SetTime( IdmDateTime( 60.0 ) );
        chw.Update( dt );

        CHECK_EQUAL(   0, chw.GetCurrentStock()       );
        CHECK_EQUAL(  20, chw.GetDaysToNextShipment() );
        CHECK_EQUAL(   8, chw.GetNumEntitiesInQueue() );
        CHECK_EQUAL(   5, chw.GetDaysRemaining()      );

        p_nc_7->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_ListenForEvent );
        p_nc_8->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_ListenForEvent );
        p_nc_9->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_ListenForEvent );
        CHECK_EQUAL( 11, chw.GetNumEntitiesInQueue() );

        chw.UpdateNodes( dt );

        CHECK_EQUAL(   0, chw.GetCurrentStock()       );
        CHECK_EQUAL(  20, chw.GetDaysToNextShipment() );
        CHECK_EQUAL(  11, chw.GetNumEntitiesInQueue() ); // 0 distributed - no stock
        CHECK_EQUAL(   5, chw.GetDaysRemaining()      );

        //printf("time=65\n");
        sec.SetTime( IdmDateTime( 65.0 ) );
        chw.Update( dt );

        CHECK_EQUAL(   0, chw.GetCurrentStock()       );
        CHECK_EQUAL(  15, chw.GetDaysToNextShipment() );
        CHECK_EQUAL(  11, chw.GetNumEntitiesInQueue() );
        CHECK_EQUAL(   0, chw.GetDaysRemaining()      );

        p_nc_1->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_ListenForEvent );
        p_nc_2->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_ListenForEvent );
        p_nc_3->GetEventContext()->VisitIndividuals( (INodeEventContext::individual_visit_function_t)BroadcastEvent_ListenForEvent );
        CHECK_EQUAL( 14, chw.GetNumEntitiesInQueue() );

        chw.UpdateNodes( dt );

        CHECK_EQUAL(   0, chw.GetCurrentStock()       );
        CHECK_EQUAL(  15, chw.GetDaysToNextShipment() );
        CHECK_EQUAL(  14, chw.GetNumEntitiesInQueue() ); // 2 distributed
        CHECK_EQUAL(   0, chw.GetDaysRemaining()      );

        CHECK( chw.IsFinished() );
    }
}