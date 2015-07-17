/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"
#include "UnitTest++.h"
#include "BaseEventReport.h"
#include "NodeEventContext.h"
#include "NodeEventContextHost.h"
#include "SimulationConfig.h"

using namespace Kernel; 
using namespace std; 

class TestReport : public Kernel::BaseEventReport
{
public:
    TestReport()
        : BaseEventReport( "TestReport" )
        , m_ReportValue(0)
        , m_NumBirthEvents(0)
        , m_NumDeathEvents(0)
        , m_NumOtherEvents(0)
    {};

    virtual ~TestReport() {};

    virtual bool Configure( const Configuration* inputJson )
    {
        initConfigTypeMap("Report_Value", &m_ReportValue, "Test getting subclass value from config", 0, 10, 1 );

        bool ret = BaseEventReport::Configure( inputJson );
        return ret;
    }

    virtual bool notifyOnEvent( Kernel::IIndividualHumanEventContext *context, 
                                const std::string& StateChange)
    {
        if( HaveUnregisteredAllEvents() )
        {
            return false ;
        }
        else if( StateChange == "Births" )
        {
            m_NumBirthEvents++ ;
        }
        else if( StateChange == "NonDiseaseDeaths" )
        {
            m_NumDeathEvents++ ;
        }
        else
        {
            m_NumOtherEvents++ ;
        }
        return true ;
    }

    int GetReportValue()    const { return m_ReportValue    ; }
    int GetNumBirthEvents() const { return m_NumBirthEvents ; }
    int GetNumDeathEvents() const { return m_NumDeathEvents ; }
    int GetNumOtherEvents() const { return m_NumOtherEvents ; }
private:
    int m_ReportValue ;
    int m_NumBirthEvents ;
    int m_NumDeathEvents ;
    int m_NumOtherEvents ;
};

class MyIntervention : public Kernel::INodeDistributableIntervention
{
public:
    MyIntervention( INodeTriggeredInterventionConsumer* pNTIC ) 
        : INodeDistributableIntervention()
        , m_pNTIC( pNTIC )
    {};

    virtual void Update(float dt)
    {
        m_pNTIC->TriggerNodeEventObservers( nullptr, IndividualEventTriggerType::NonDiseaseDeaths );
    };

    virtual bool Distribute(INodeEventContext *context, IEventCoordinator2* pEC = NULL ) { assert( false ) ; return false ; }; 
    virtual void SetContextTo(INodeEventContext *context) {};
    virtual void ValidateSimType( const std::string& simTypeStr ) {};
    virtual QueryResult QueryInterface(iid_t iid, void** pinstance) { return Kernel::e_NOINTERFACE; };
    virtual int32_t AddRef() { return 0 ;};
    virtual int32_t Release() { return 0 ;};
#if USE_JSON_SERIALIZATION
    // For JSON serialization
    virtual void JSerialize( IJsonObjectAdapter* root, JSerializer* helper ) const {} ;
    virtual void JDeserialize( IJsonObjectAdapter* root, JSerializer* helper ) {};
#endif
private:
    INodeTriggeredInterventionConsumer* m_pNTIC ;
};


SUITE(BaseEventReportTest)
{
    struct ReportFixture
    {
        static bool environmentInitialized;
        static boost::mpi::environment* env;
        static boost::mpi::communicator* world;
        SimulationConfig* m_pSimulationConfig ;

        ReportFixture()
        {
            JsonConfigurable::ClearMissingParameters();
            m_pSimulationConfig = new SimulationConfig();
            m_pSimulationConfig->sim_type = SimType::HIV_SIM ;

            if (!environmentInitialized)
            {
                Environment::setLogger(new SimpleLogger());
                int argc      = 1;
                char* exeName = "componentTests.exe";
                char** argv   = &exeName;
                env           = new boost::mpi::environment(argc, argv);
                world         = new boost::mpi::communicator;
                string configFilename("testdata/BaseEventReportTest/TestReport.config.json");
                string inputPath("testdata/BaseEventReportTest");
                string outputPath("testdata/BaseEventReportTest");
                string statePath("testdata/BaseEventReportTest");
                string dllPath("");
                Environment::Initialize(env, world, configFilename, inputPath, outputPath, /*statePath, */dllPath, false);
                environmentInitialized = true;
            }

            Environment::setSimulationConfig( m_pSimulationConfig );
            m_pSimulationConfig->listed_events.insert("Births"          );
            m_pSimulationConfig->listed_events.insert("NonDiseaseDeaths");
        }

        ~ReportFixture()
        {
            delete m_pSimulationConfig;
            Environment::setSimulationConfig( nullptr );
        }
    };

    bool                      ReportFixture::environmentInitialized = false ;
    boost::mpi::environment*  ReportFixture::env        = nullptr ;
    boost::mpi::communicator* ReportFixture::world      = nullptr ;

    TEST_FIXTURE(ReportFixture, TestConfigure)
    {
        TestReport report ;

        CHECK_EQUAL( 0.0f, report.GetStartDay() );
        CHECK_EQUAL( 0.0f, report.GetDurationDays() );
        CHECK_EQUAL(    0, report.GetReportValue() );

        json::Array report_data = Environment::getInstance()->Config->operator[]( std::string("Reports") ).As<json::Array>() ;

        CHECK_EQUAL( 1, report_data.Size() );

        unique_ptr<Configuration> p_cfg( Environment::CopyFromElement( report_data[0] ) );
        std::string class_name = (string)((*p_cfg)["class"].As<json::String>());

        CHECK_EQUAL( std::string("TestReport"), class_name );

        report.Configure( p_cfg.get() );

        CHECK_EQUAL( 123.0f, report.GetStartDay() );
        CHECK_EQUAL(   4.0f, report.GetDurationDays() );
        CHECK_EQUAL(      5, report.GetReportValue() );

        CHECK_EQUAL( 2, report.GetEventTriggerList().size() );
        CHECK_EQUAL( "Births",           report.GetEventTriggerList()[0] );
        CHECK_EQUAL( "NonDiseaseDeaths", report.GetEventTriggerList()[1] );
    }

    TEST_FIXTURE(ReportFixture, TestEvents)
    {
        json::Array report_data = Environment::getInstance()->Config->operator[]( std::string("Reports") ).As<json::Array>() ;
        unique_ptr<Configuration> p_cfg( Environment::CopyFromElement( report_data[0] ) );

        TestReport report ;
        report.Configure( p_cfg.get() );

        NodeEventContextHost nec ;
        MyIntervention* p_mi = new MyIntervention( &nec );
        nec.GiveIntervention( p_mi ); // NEC takes ownership
        std::vector<INodeEventContext*> nec_list ;
        nec_list.push_back( &nec );

        // -----------------------------------------
        // --- Show the initial state of the report
        // -----------------------------------------
        CHECK( !report.HaveRegisteredAllEvents() );
        CHECK( !report.HaveUnregisteredAllEvents() );
        CHECK_EQUAL( 0, report.GetNumBirthEvents() );
        CHECK_EQUAL( 0, report.GetNumDeathEvents() );
        CHECK_EQUAL( 0, report.GetNumOtherEvents() ); // should always be zero

        // -------------------------------------------------------------------------
        // --- Show that an update before the start day does not update the report
        // --- NOTE: the calls to TriggerNodeEventObservers() in this method simulate
        // ---       calls to these methods from within the Node::Update() method
        // ---       which happens after updating the interventions.
        // -------------------------------------------------------------------------
        report.UpdateEventRegistration( 122.0, 1.0, nec_list );
        nec.UpdateInterventions( 1.0 );
        nec.TriggerNodeEventObservers( nullptr, IndividualEventTriggerType::Births );
        nec.TriggerNodeEventObservers( nullptr, IndividualEventTriggerType::NonDiseaseDeaths );
        nec.TriggerNodeEventObservers( nullptr, IndividualEventTriggerType::EveryUpdate ); // not listening for

        CHECK( !report.HaveRegisteredAllEvents() );
        CHECK( !report.HaveUnregisteredAllEvents() );
        CHECK_EQUAL( 0, report.GetNumBirthEvents() );
        CHECK_EQUAL( 0, report.GetNumDeathEvents() );
        CHECK_EQUAL( 0, report.GetNumOtherEvents() );

        // --------------------------------------------------------------------------------------
        // --- START_DAY - Show that we register for two events and get updates on those events.
        // --------------------------------------------------------------------------------------
        report.UpdateEventRegistration( 123.0, 1.0, nec_list );
        nec.UpdateInterventions( 1.0 ); // a NonDiseaseDeaths from MyIntervention
        nec.TriggerNodeEventObservers( nullptr, IndividualEventTriggerType::Births );
        nec.TriggerNodeEventObservers( nullptr, IndividualEventTriggerType::NonDiseaseDeaths );
        nec.TriggerNodeEventObservers( nullptr, IndividualEventTriggerType::EveryUpdate ); // not listening for

        CHECK(  report.HaveRegisteredAllEvents() );
        CHECK( !report.HaveUnregisteredAllEvents() );
        CHECK_EQUAL( 1, report.GetNumBirthEvents() );
        CHECK_EQUAL( 2, report.GetNumDeathEvents() ); 
        CHECK_EQUAL( 0, report.GetNumOtherEvents() );

        // ------------------------------------------
        // --- Simple update but with an extra birth
        // ------------------------------------------
        report.UpdateEventRegistration( 124.0, 1.0, nec_list );
        nec.UpdateInterventions( 1.0 ); // a NonDiseaseDeaths from MyIntervention
        nec.TriggerNodeEventObservers( nullptr, IndividualEventTriggerType::Births );
        nec.TriggerNodeEventObservers( nullptr, IndividualEventTriggerType::Births );
        nec.TriggerNodeEventObservers( nullptr, IndividualEventTriggerType::NonDiseaseDeaths );
        nec.TriggerNodeEventObservers( nullptr, IndividualEventTriggerType::EveryUpdate ); // not listening for

        CHECK(  report.HaveRegisteredAllEvents() );
        CHECK( !report.HaveUnregisteredAllEvents() );
        CHECK_EQUAL( 3, report.GetNumBirthEvents() );
        CHECK_EQUAL( 4, report.GetNumDeathEvents() );
        CHECK_EQUAL( 0, report.GetNumOtherEvents() );

        // ------------------
        // --- Simple Update
        // ------------------
        report.UpdateEventRegistration( 125.0, 1.0, nec_list );
        nec.UpdateInterventions( 1.0 ); // a NonDiseaseDeaths from MyIntervention
        nec.TriggerNodeEventObservers( nullptr, IndividualEventTriggerType::Births );
        nec.TriggerNodeEventObservers( nullptr, IndividualEventTriggerType::NonDiseaseDeaths );
        nec.TriggerNodeEventObservers( nullptr, IndividualEventTriggerType::EveryUpdate ); // not listening for

        CHECK(  report.HaveRegisteredAllEvents() );
        CHECK( !report.HaveUnregisteredAllEvents() );
        CHECK_EQUAL( 4, report.GetNumBirthEvents() );
        CHECK_EQUAL( 6, report.GetNumDeathEvents() );
        CHECK_EQUAL( 0, report.GetNumOtherEvents() );

        // ----------------------------------------------------------
        // --- LAST UPDATE - Duration_Days = 4 => 123, 124, 125, 126
        // ----------------------------------------------------------
        report.UpdateEventRegistration( 126.0, 1.0, nec_list );
        nec.UpdateInterventions( 1.0 ); // a NonDiseaseDeaths from MyIntervention
        nec.TriggerNodeEventObservers( nullptr, IndividualEventTriggerType::Births );
        nec.TriggerNodeEventObservers( nullptr, IndividualEventTriggerType::NonDiseaseDeaths );
        nec.TriggerNodeEventObservers( nullptr, IndividualEventTriggerType::EveryUpdate ); // not listening for

        CHECK(  report.HaveRegisteredAllEvents() );
        CHECK( !report.HaveUnregisteredAllEvents() );
        CHECK_EQUAL( 5, report.GetNumBirthEvents() );
        CHECK_EQUAL( 8, report.GetNumDeathEvents() );
        CHECK_EQUAL( 0, report.GetNumOtherEvents() );

        // --------------------------------------------------------------------------------------------
        // --- UN-REGISTER - The events are unregistered in UpdateEventRegistration(), but not really
        // --- removed until after the interventions have had a chance to be update.  This means to
        // --- stop getting events from UpdateInterventions(), the onNotifyEvent() method must have
        // --- a check to see if report has unregistered the events so it will stop processing them.
        // --------------------------------------------------------------------------------------------
        report.UpdateEventRegistration( 127.0, 1.0, nec_list );
        nec.UpdateInterventions( 1.0 ); // see note above
        nec.TriggerNodeEventObservers( nullptr, IndividualEventTriggerType::Births );
        nec.TriggerNodeEventObservers( nullptr, IndividualEventTriggerType::NonDiseaseDeaths );
        nec.TriggerNodeEventObservers( nullptr, IndividualEventTriggerType::EveryUpdate ); // not listening for

        // --------------------------------------------------
        // --- Notice that the numbers have not been updated
        // --------------------------------------------------
        CHECK(  report.HaveRegisteredAllEvents() );
        CHECK(  report.HaveUnregisteredAllEvents() );
        CHECK_EQUAL( 5, report.GetNumBirthEvents() ); 
        CHECK_EQUAL( 8, report.GetNumDeathEvents() );
        CHECK_EQUAL( 0, report.GetNumOtherEvents() );

        // -------------------------------------------------------
        // --- Simple Update but we are not listening for events.
        // -------------------------------------------------------
        report.UpdateEventRegistration( 128.0, 1.0, nec_list );
        nec.UpdateInterventions( 1.0 );
        nec.TriggerNodeEventObservers( nullptr, IndividualEventTriggerType::Births );
        nec.TriggerNodeEventObservers( nullptr, IndividualEventTriggerType::NonDiseaseDeaths );
        nec.TriggerNodeEventObservers( nullptr, IndividualEventTriggerType::EveryUpdate ); // not listening for

        CHECK(  report.HaveRegisteredAllEvents() );
        CHECK(  report.HaveUnregisteredAllEvents() );
        CHECK_EQUAL( 5, report.GetNumBirthEvents() );
        CHECK_EQUAL( 8, report.GetNumDeathEvents() );
        CHECK_EQUAL( 0, report.GetNumOtherEvents() );
    }
}
