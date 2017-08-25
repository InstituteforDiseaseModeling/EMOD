/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include <memory> // unique_ptr
#include "UnitTest++.h"
#include "common.h"
#include "Environment.h"
#include "SimulationConfig.h"
#include "JsonFullReader.h"
#include "JsonFullWriter.h"
#include "BinaryArchiveReader.h"
#include "BinaryArchiveWriter.h"
#include "JsonObjectDemog.h"
#include "Properties.h"

#include "FileSystem.h"
#include "IndividualTB.h"

#include "Diagnostics.h"
#include "RandomFake.h"

using namespace Kernel; 

SUITE(SerializationTest)
{
    static int m_NextId = 1 ;

    struct SerializationFixture
    {
        SimulationConfig* m_pSimulationConfig ;
        RandomFake fake_rng ;

        SerializationFixture()
        :  m_pSimulationConfig( new SimulationConfig() )
        {
            Environment::Finalize();
            Environment::setLogger( new SimpleLogger() );
            Environment::setSimulationConfig( m_pSimulationConfig );
            const_cast<Environment*>(Environment::getInstance())->RNG = &fake_rng;

            IPFactory::DeleteFactory();
            IPFactory::CreateFactory();

            std::map<std::string,float> ip_values ;
            ip_values.insert( std::make_pair( "LOW",  0.9f ) );
            ip_values.insert( std::make_pair( "HIGH", 0.1f ) );

            IPFactory::GetInstance()->AddIP( 1, "Risk", ip_values );
        }

        ~SerializationFixture()
        {
            delete m_pSimulationConfig;
            m_pSimulationConfig = nullptr;
            IPFactory::DeleteFactory();
            Environment::Finalize();
        }
    };

    TEST_FIXTURE(SerializationFixture, TestSerializeDeserialize)
    {
        // --------------------
        // --- Initialize test
        // --------------------
        EventTriggerFactory::DeleteInstance();

        json::Object fakeConfigJson;
        Configuration * fakeConfigValid = Environment::CopyFromElement( fakeConfigJson );
        EventTriggerFactory::GetInstance()->Configure( fakeConfigValid );

        try
        {
            JsonObjectDemog json ;
            json.ParseFile( "testdata/SerializationTest/SerializationTest.json" );
            CHECK( json.Contains( "Listed_Events" ) );
            CHECK( json["Listed_Events"].IsArray() );

            for( int i = 0 ; i < json["Listed_Events"].size() ; i++ )
            {
                EventTriggerFactory::GetInstance()->CreateUserEventTrigger( json["Listed_Events"][i].AsString() );
            }

            std::map<std::string, float> ip_values_state ;
            CHECK( json.Contains( "Valid_Intervention_Statuses" ) );
            CHECK( json["Valid_Intervention_Statuses"].IsArray() );
            for( int i = 0 ; i < json["Valid_Intervention_Statuses"].size() ; i++ )
            {
                float percent_dist = 0.0;
                if( (i + 1) == json[ "Valid_Intervention_Statuses" ].size() )
                {
                    percent_dist = 1.0;
                }
                ip_values_state.insert( std::make_pair( json[ "Valid_Intervention_Statuses" ][ i ].AsString(), percent_dist ) );
            }
            IPFactory::GetInstance()->AddIP( 1, "InterventionStatus", ip_values_state );

            CHECK( json.Contains( "Objects" ) );
            CHECK( json["Objects"].IsArray() );

            for( int i = 0 ; i < json["Objects"].size() ; i++ )
            {
                string class_name = json["Objects"][i]["Test"]["__class__"].AsString();
                printf("testing  %s\n",class_name.c_str());

                string expected_json_str = json["Objects"][i].ToString();

                ISerializable* p_read_obj = nullptr;
                JsonFullReader json_reader( expected_json_str.c_str() );
                IArchive* p_json_reader = &json_reader;
                p_json_reader->labelElement("Test") & p_read_obj;

                BinaryArchiveWriter binary_writer;
                IArchive* p_binary_writer = &binary_writer;
                p_binary_writer->labelElement("Test") & p_read_obj;

                ISerializable* p_read_obj_2 = nullptr;
                BinaryArchiveReader binary_reader( p_binary_writer->GetBuffer(), p_binary_writer->GetBufferSize() );
                IArchive* p_binary_reader = &binary_reader;
                p_binary_reader->labelElement("Test") & p_read_obj_2;

                JsonFullWriter json_writer;
                IArchive* p_json_writer = &json_writer;
                p_json_writer->labelElement("Test") & p_read_obj_2;

                std::string actual_json_str = p_json_writer->GetBuffer();

                CHECK_EQUAL( expected_json_str, actual_json_str );
            }
        }
        catch( DetailedException& re )
        {
            PrintDebug( re.GetMsg() );
            PrintDebug( re.GetStackTrace() );
            CHECK( false );
        }
    }

    TEST_FIXTURE(SerializationFixture, TestGenericSerialization)
    {
        // Open and read JSON
        std::string* json = FileSystem::ReadFile( "testdata/SerializationTest/genericIndividual.json" );

        // Instantiate from JSON
        IndividualHuman* individual = nullptr;
        ISerializable* source = (ISerializable*)individual;
        IArchive* reader = dynamic_cast<IArchive*>(new JsonFullReader( json->c_str() ));
        (*reader).labelElement("individual") & source;

        // Serialize to binary
        IArchive* binary_writer = dynamic_cast<IArchive*>(new BinaryArchiveWriter());
        (*binary_writer) & source;

        // Deserialize from binary
        const char* buffer = binary_writer->GetBuffer();
        size_t count = binary_writer->GetBufferSize();
        IArchive* binary_reader = dynamic_cast<IArchive*>(new BinaryArchiveReader( buffer, count ));
        ISerializable* destination = nullptr;
        (*binary_reader) & destination;
        IndividualHuman* compare = (IndividualHuman*)destination;

        // Compare
        IArchive* writer = dynamic_cast<IArchive*>(new BinaryArchiveWriter());
        (*writer) & compare;

        const char* actual = writer->GetBuffer();
        size_t actual_count = writer->GetBufferSize();

        CHECK_EQUAL( count, actual_count );
        CHECK_ARRAY_EQUAL( buffer, actual, count );

        delete writer;

        compare = nullptr;
        delete destination;
        delete binary_reader;
        delete source;
        individual = nullptr;
        delete binary_writer;
        delete reader;
        delete json;
    }

    TEST_FIXTURE(SerializationFixture, TestAirborneSerialization)
    {
        try
        {
            // Open and read JSON
            std::string* json = FileSystem::ReadFile( "testdata/SerializationTest/airborneIndividual.json" );

            // Instantiate from JSON
            IndividualHumanAirborne* individual = nullptr;
            ISerializable* source = (ISerializable*)individual;
            IArchive* reader = dynamic_cast<IArchive*>(new JsonFullReader( json->c_str() ));
            (*reader).labelElement("individual") & source;

            // Serialize to binary
            IArchive* binary_writer = dynamic_cast<IArchive*>(new BinaryArchiveWriter());
            (*binary_writer) & source;

            // Deserialize from binary
            const char* buffer = binary_writer->GetBuffer();
            size_t count = binary_writer->GetBufferSize();
            IArchive* binary_reader = dynamic_cast<IArchive*>(new BinaryArchiveReader( buffer, count ));
            ISerializable* destination = nullptr;
            (*binary_reader) & destination;
            IndividualHumanAirborne* compare = (IndividualHumanAirborne*)destination;

            // Compare
            IArchive* writer = dynamic_cast<IArchive*>(new BinaryArchiveWriter());
            (*writer) & compare;

            const char* actual = writer->GetBuffer();
            size_t actual_count = writer->GetBufferSize();

            CHECK_EQUAL( count, actual_count );
            CHECK_ARRAY_EQUAL( buffer, actual, count );

            delete writer;

            compare = nullptr;
            delete destination;
            delete binary_reader;
            delete source;
            individual = nullptr;
            delete binary_writer;
            delete reader;
            delete json;
        }
        catch( DetailedException& re )
        {
            PrintDebug( re.GetMsg() );
            PrintDebug( re.GetStackTrace() );
            CHECK( false );
        }
    }

    TEST_FIXTURE(SerializationFixture, TestTbSerialization)
    {
        try
        {
            // Open and read JSON
            std::string* json = FileSystem::ReadFile( "testdata/SerializationTest/tbIndividual.json" );

            // Instantiate from JSON
            IndividualHumanTB* individual = nullptr;
            ISerializable* source = (ISerializable*)individual;
            IArchive* reader = dynamic_cast<IArchive*>(new JsonFullReader( json->c_str() ));
            (*reader).labelElement("individual") & source;

            // Serialize to binary
            IArchive* binary_writer = dynamic_cast<IArchive*>(new BinaryArchiveWriter());
            (*binary_writer) & source;

            // Deserialize from binary
            const char* buffer = binary_writer->GetBuffer();
            size_t count = binary_writer->GetBufferSize();
            IArchive* binary_reader = dynamic_cast<IArchive*>(new BinaryArchiveReader( buffer, count ));
            ISerializable* destination = nullptr;
            (*binary_reader) & destination;
            IndividualHumanTB* compare = (IndividualHumanTB*)destination;

            // Compare
            IArchive* writer = dynamic_cast<IArchive*>(new BinaryArchiveWriter());
            (*writer) & compare;

            const char* actual = writer->GetBuffer();
            size_t actual_count = writer->GetBufferSize();

            CHECK_EQUAL( count, actual_count );
            CHECK_ARRAY_EQUAL( buffer, actual, count );

            delete writer;

            compare = nullptr;
            delete destination;
            delete binary_reader;
            delete source;
            individual = nullptr;
            delete binary_writer;
            delete reader;
            delete json;
        }
        catch( DetailedException& re )
        {
            PrintDebug( re.GetMsg() );
            PrintDebug( re.GetStackTrace() );
            CHECK( false );
        }
    }
}
