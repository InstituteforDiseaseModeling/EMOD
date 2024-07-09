
#include "stdafx.h"
#include <iostream>
#include <memory> // unique_ptr
#include "UnitTest++.h"
#include "PfaFixture.h"
#include "componentTests.h"

#include "InterventionForCurrentPartners.h"

using namespace std;
using namespace Kernel;

SUITE( InterventionForCurrentPartnersTest )
{
    IndividualHumanContextFake* SetupFixture( PfaFixture* pFixture )
    {
        IndividualHumanContextFake* p_partner_1 = pFixture->CreateHuman( Gender::FEMALE, 7301 );
        IndividualHumanContextFake* p_partner_2 = pFixture->CreateHuman( Gender::FEMALE, 7302 );
        IndividualHumanContextFake* p_partner_3 = pFixture->CreateHuman( Gender::FEMALE, 7303 );
        IndividualHumanContextFake* p_partner_4 = pFixture->CreateHuman( Gender::FEMALE, 7304 );
        IndividualHumanContextFake* p_partner_5 = pFixture->CreateHuman( Gender::FEMALE, 7305 );
        IndividualHumanContextFake* p_partner_6 = pFixture->CreateHuman( Gender::FEMALE, 7306 );
        IndividualHumanContextFake* p_partner_7 = pFixture->CreateHuman( Gender::FEMALE, 7307 );
        IndividualHumanContextFake* p_partner_8 = pFixture->CreateHuman( Gender::FEMALE, 7308 );
        IndividualHumanContextFake* p_partner_9 = pFixture->CreateHuman( Gender::FEMALE, 7309 );

        IndividualHumanContextFake* p_self = pFixture->CreateHuman( Gender::MALE, 7300 );
        
        p_self->SetMyRand( new PSEUDO_DES() );

        pFixture->AddRelationship( p_self->GetRng(), p_self, p_partner_1, RelationshipType::MARITAL,     900 );
        pFixture->AddRelationship( p_self->GetRng(), p_self, p_partner_2, RelationshipType::TRANSITORY,  800 );
        pFixture->AddRelationship( p_self->GetRng(), p_self, p_partner_3, RelationshipType::MARITAL,     700 );
        pFixture->AddRelationship( p_self->GetRng(), p_self, p_partner_4, RelationshipType::INFORMAL,    600 );
        pFixture->AddRelationship( p_self->GetRng(), p_self, p_partner_5, RelationshipType::COMMERCIAL,  500 );
        pFixture->AddRelationship( p_self->GetRng(), p_self, p_partner_6, RelationshipType::MARITAL,     400 );
        pFixture->AddRelationship( p_self->GetRng(), p_self, p_partner_7, RelationshipType::INFORMAL,    300 );
        pFixture->AddRelationship( p_self->GetRng(), p_self, p_partner_8, RelationshipType::TRANSITORY,  200 );
        pFixture->AddRelationship( p_self->GetRng(), p_self, p_partner_9, RelationshipType::COMMERCIAL,  100 );

        return p_self;
    }

    // observer so we know who received the event/intervention
    struct EventListener : IIndividualEventObserver
    {
        std::vector<uint32_t> selected_ids;

        EventListener()
            : IIndividualEventObserver()
            , selected_ids()
        {
        }

        virtual bool notifyOnEvent( IIndividualHumanEventContext *context, const EventTrigger& trigger ) override
        {
            selected_ids.push_back( context->GetSuid().data );
            return true;
        }

        virtual Kernel::QueryResult QueryInterface( Kernel::iid_t iid, void **ppvObject ) override { return Kernel::e_NOINTERFACE; }
        virtual int32_t AddRef() { return 1; }
        virtual int32_t Release() { return 1; }
    };

    // Most of the tests will use this method and then just check who received the event/intervention
    void RunBaseTest( PfaFixture* pFixture, const std::string& rFilename, EventListener& rListener )
    {
        unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( rFilename.c_str() ) );

        InterventionForCurrentPartners ifcp;
        try
        {
            ifcp.Configure( p_config.get() );
        }
        catch( DetailedException& de )
        {
            PrintDebug( de.GetMsg() );
            CHECK( false );
        }

        pFixture->Register( &rListener, EventTrigger::NewMalariaInfectionObject );

        IndividualHumanContextFake* p_self = SetupFixture( pFixture );

        ifcp.SetContextTo( p_self );
        ifcp.Update( 1.0 );
        pFixture->GetNodeContext()->PostUpdate();
    }

    TEST_FIXTURE( PfaFixture, TestNoPrioritization )
    {
        std::string filename = "testdata/InterventionForCurrentPartnersTest/TestNoPrioritization.json";

        EventListener listener;
        RunBaseTest( this, filename, listener );

        CHECK_EQUAL( 9, listener.selected_ids.size() );
        CHECK_EQUAL( 1, listener.selected_ids[ 0 ] );
        CHECK_EQUAL( 2, listener.selected_ids[ 1 ] );
        CHECK_EQUAL( 3, listener.selected_ids[ 2 ] );
        CHECK_EQUAL( 4, listener.selected_ids[ 3 ] );
        CHECK_EQUAL( 5, listener.selected_ids[ 4 ] );
        CHECK_EQUAL( 6, listener.selected_ids[ 5 ] );
        CHECK_EQUAL( 7, listener.selected_ids[ 6 ] );
        CHECK_EQUAL( 8, listener.selected_ids[ 7 ] );
        CHECK_EQUAL( 9, listener.selected_ids[ 8 ] );
    }

    TEST_FIXTURE( PfaFixture, TestNoPrioritizationMinDuration365 )
    {
        std::string filename = "testdata/InterventionForCurrentPartnersTest/TestNoPrioritizationMinDuration365.json";

        EventListener listener;
        RunBaseTest( this, filename, listener );

        CHECK_EQUAL( 6, listener.selected_ids.size() );
        CHECK_EQUAL( 1, listener.selected_ids[ 0 ] );
        CHECK_EQUAL( 2, listener.selected_ids[ 1 ] );
        CHECK_EQUAL( 3, listener.selected_ids[ 2 ] );
        CHECK_EQUAL( 4, listener.selected_ids[ 3 ] );
        CHECK_EQUAL( 5, listener.selected_ids[ 4 ] );
        CHECK_EQUAL( 6, listener.selected_ids[ 5 ] );
    }

    TEST_FIXTURE( PfaFixture, TestChosenAtRandom )
    {
        std::string filename = "testdata/InterventionForCurrentPartnersTest/TestChosenAtRandom.json";

        EventListener listener;
        RunBaseTest( this, filename, listener );

        CHECK_EQUAL( 9, listener.selected_ids.size() );
        CHECK_EQUAL( 3, listener.selected_ids[ 0 ] );
        CHECK_EQUAL( 1, listener.selected_ids[ 1 ] );
        CHECK_EQUAL( 5, listener.selected_ids[ 2 ] );
        CHECK_EQUAL( 2, listener.selected_ids[ 3 ] );
        CHECK_EQUAL( 8, listener.selected_ids[ 4 ] );
        CHECK_EQUAL( 7, listener.selected_ids[ 5 ] );
        CHECK_EQUAL( 9, listener.selected_ids[ 6 ] );
        CHECK_EQUAL( 4, listener.selected_ids[ 7 ] );
        CHECK_EQUAL( 6, listener.selected_ids[ 8 ] );
    }

    TEST_FIXTURE( PfaFixture, TestLongerTime )
    {
        std::string filename = "testdata/InterventionForCurrentPartnersTest/TestLongerTime.json";

        EventListener listener;
        RunBaseTest( this, filename, listener );

        CHECK_EQUAL( 9, listener.selected_ids.size() );
        CHECK_EQUAL( 1, listener.selected_ids[ 0 ] );
        CHECK_EQUAL( 2, listener.selected_ids[ 1 ] );
        CHECK_EQUAL( 3, listener.selected_ids[ 2 ] );
        CHECK_EQUAL( 4, listener.selected_ids[ 3 ] );
        CHECK_EQUAL( 5, listener.selected_ids[ 4 ] );
        CHECK_EQUAL( 6, listener.selected_ids[ 5 ] );
        CHECK_EQUAL( 7, listener.selected_ids[ 6 ] );
        CHECK_EQUAL( 8, listener.selected_ids[ 7 ] );
        CHECK_EQUAL( 9, listener.selected_ids[ 8 ] );
    }

    TEST_FIXTURE( PfaFixture, TestShorterTime )
    {
        std::string filename = "testdata/InterventionForCurrentPartnersTest/TestShorterTime.json";

        EventListener listener;
        RunBaseTest( this, filename, listener );

        CHECK_EQUAL( 9, listener.selected_ids.size() );
        CHECK_EQUAL( 9, listener.selected_ids[ 0 ] );
        CHECK_EQUAL( 8, listener.selected_ids[ 1 ] );
        CHECK_EQUAL( 7, listener.selected_ids[ 2 ] );
        CHECK_EQUAL( 6, listener.selected_ids[ 3 ] );
        CHECK_EQUAL( 5, listener.selected_ids[ 4 ] );
        CHECK_EQUAL( 4, listener.selected_ids[ 5 ] );
        CHECK_EQUAL( 3, listener.selected_ids[ 6 ] );
        CHECK_EQUAL( 2, listener.selected_ids[ 7 ] );
        CHECK_EQUAL( 1, listener.selected_ids[ 8 ] );
    }

    TEST_FIXTURE( PfaFixture, TestOlder )
    {
        std::string filename = "testdata/InterventionForCurrentPartnersTest/TestOlder.json";

        EventListener listener;
        RunBaseTest( this, filename, listener );

        CHECK_EQUAL( 9, listener.selected_ids.size() );
        CHECK_EQUAL( 9, listener.selected_ids[ 0 ] );
        CHECK_EQUAL( 8, listener.selected_ids[ 1 ] );
        CHECK_EQUAL( 7, listener.selected_ids[ 2 ] );
        CHECK_EQUAL( 6, listener.selected_ids[ 3 ] );
        CHECK_EQUAL( 5, listener.selected_ids[ 4 ] );
        CHECK_EQUAL( 4, listener.selected_ids[ 5 ] );
        CHECK_EQUAL( 3, listener.selected_ids[ 6 ] );
        CHECK_EQUAL( 2, listener.selected_ids[ 7 ] );
        CHECK_EQUAL( 1, listener.selected_ids[ 8 ] );
    }

    TEST_FIXTURE( PfaFixture, TestYounger )
    {
        std::string filename = "testdata/InterventionForCurrentPartnersTest/TestYounger.json";

        EventListener listener;
        RunBaseTest( this, filename, listener );

        CHECK_EQUAL( 9, listener.selected_ids.size() );
        CHECK_EQUAL( 1, listener.selected_ids[ 0 ] );
        CHECK_EQUAL( 2, listener.selected_ids[ 1 ] );
        CHECK_EQUAL( 3, listener.selected_ids[ 2 ] );
        CHECK_EQUAL( 4, listener.selected_ids[ 3 ] );
        CHECK_EQUAL( 5, listener.selected_ids[ 4 ] );
        CHECK_EQUAL( 6, listener.selected_ids[ 5 ] );
        CHECK_EQUAL( 7, listener.selected_ids[ 6 ] );
        CHECK_EQUAL( 8, listener.selected_ids[ 7 ] );
        CHECK_EQUAL( 9, listener.selected_ids[ 8 ] );
    }

    TEST_FIXTURE( PfaFixture, TestRelationshipType )
    {
        std::string filename = "testdata/InterventionForCurrentPartnersTest/TestRelationshipType.json";

        EventListener listener;
        RunBaseTest( this, filename, listener );

        CHECK_EQUAL( 7, listener.selected_ids.size() );
        CHECK_EQUAL( 6, listener.selected_ids[ 0 ] );
        CHECK_EQUAL( 1, listener.selected_ids[ 1 ] );
        CHECK_EQUAL( 3, listener.selected_ids[ 2 ] );
        CHECK_EQUAL( 4, listener.selected_ids[ 3 ] );
        CHECK_EQUAL( 7, listener.selected_ids[ 4 ] );
        CHECK_EQUAL( 9, listener.selected_ids[ 5 ] );
        CHECK_EQUAL( 5, listener.selected_ids[ 6 ] );
    }

    TEST_FIXTURE( PfaFixture, TestLongerTimeMaxPartners )
    {
        std::string filename = "testdata/InterventionForCurrentPartnersTest/TestLongerTimeMaxPartners.json";

        EventListener listener;
        RunBaseTest( this, filename, listener );

        CHECK_EQUAL( 5, listener.selected_ids.size() );
        CHECK_EQUAL( 1, listener.selected_ids[ 0 ] );
        CHECK_EQUAL( 2, listener.selected_ids[ 1 ] );
        CHECK_EQUAL( 3, listener.selected_ids[ 2 ] );
        CHECK_EQUAL( 4, listener.selected_ids[ 3 ] );
        CHECK_EQUAL( 5, listener.selected_ids[ 4 ] );
    }

    void TestHelper_ConfigureException( int lineNumber, const std::string& rFilename, const std::string& rExpMsg )
    {
        unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( rFilename.c_str() ) );

        InterventionForCurrentPartners ifcp;
        try
        {
            bool ret = ifcp.Configure( p_config.get() );
            CHECK( ret );

            CHECK_LN( false, lineNumber ); // should not get here
        }
        catch( DetailedException& re )
        {
            std::string msg = re.GetMsg();
            if( msg.find( rExpMsg ) == string::npos )
            {
                PrintDebug( rExpMsg );
                PrintDebug( msg );
                CHECK_LN( false, lineNumber );
            }
        }
    }

    TEST_FIXTURE( PfaFixture, TestUndefinedBroadcastEvent )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/InterventionForCurrentPartnersTest/TestUndefinedBroadcastEvent.json",
                                       "If you set 'Event_Or_Config' = 'Event', then you must define 'Broadcast_Event'" );
    }

    TEST_FIXTURE( PfaFixture, TestUndefinedInterventionConfig )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/InterventionForCurrentPartnersTest/TestUndefinedInterventionConfig.json",
                                       "If you set 'Event_Or_Config' = 'Config', then you must define 'Intervention_Config'" );
    }
}