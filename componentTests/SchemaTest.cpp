/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "UnitTest++.h"
#include "common.h"

#include "FileSystem.h"
#include "SimulationConfig.h"
#include "IdmMpi.h"
#include "Properties.h"
#include "Schema.h"

#include <string>
#include <climits>
#include <vector>
#include <iostream>
#include <memory> // unique_ptr


using namespace Kernel;
using namespace std;



SUITE(SchemaTest)
{
    struct SchemaFixture
    {
        IdmMpi::MessageInterface* m_pMpi;

        SchemaFixture()
        {
            JsonConfigurable::ClearMissingParameters();

            m_pMpi = IdmMpi::MessageInterface::CreateNull();

            Environment::Finalize();
            Environment::setLogger( new SimpleLogger( Logger::tLevel::WARNING ) );

            json::Object fakeConfigJson;
            Environment::getInstance()->Config = Configuration::CopyFromElement( fakeConfigJson );

            IPFactory::DeleteFactory();
            IPFactory::CreateFactory();

            int argc      = 1;
            char* exeName = "componentTests.exe";
            char** argv   = &exeName;
            string configFilename("");
            string inputPath(".");
            string outputPath(".");
            string statePath(".");
            string dllPath(".");
            Environment::Initialize( m_pMpi, nullptr, configFilename, inputPath, outputPath, /*statePath, */dllPath, true);
            JsonConfigurable::_dryrun = false;
        }

        ~SchemaFixture()
        {
            IPFactory::DeleteFactory();
            Environment::Finalize();
            JsonConfigurable::_dryrun = false;
        }
    };



#if 1
    TEST_FIXTURE(SchemaFixture, WriteSchema)
    {
        std::string exp_filename = "testdata/SchemaTest.WriteSchema.json" ;
        std::string act_filename = "testdata/SchemaTest.WriteSchema-actual.json" ;

        // clean up before testing begnins
        CHECK( FileSystem::FileExists( exp_filename ) );

        if( FileSystem::FileExists( act_filename ) )
        {
            FileSystem::RemoveFile( act_filename );
        }

        // create schema file and compare to expected result
        writeInputSchemas( "", act_filename.c_str() );

        unique_ptr<string> p_act_contents( FileSystem::ReadFile( act_filename.c_str() ) );
        unique_ptr<string> p_exp_contents( FileSystem::ReadFile( exp_filename.c_str() ) );

        // remove the Version element since it contains date and SVN version information
        int act_pos = p_act_contents->find_first_of("}");
        int exp_pos = p_exp_contents->find_first_of("}");
        std::string act_contents = p_act_contents->substr( act_pos, p_act_contents->length() );
        std::string exp_contents = p_exp_contents->substr( exp_pos, p_exp_contents->length() );

        // replace carriage return with space to reduce differences with Linux.
        std::replace( act_contents.begin(), act_contents.end(), char(13), ' ' );
        std::replace( exp_contents.begin(), exp_contents.end(), char(13), ' ' );

        CHECK( exp_contents == act_contents );

        // clean up
        if( FileSystem::FileExists( act_filename ) )
        {
            FileSystem::RemoveFile( act_filename );
        }
    }
#endif
}
