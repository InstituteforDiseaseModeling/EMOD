/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "UnitTest++.h"
#include "ProgramOptions.h"
#include "Exceptions.h"

using namespace std; 

SUITE(ProgramOptionsTest)
{
    TEST(TestNoArgumentOption)
    {
        ProgramOptions po( "test description" );
        po.AddOption( "noArgLongName1", "description for first option" );
        po.AddOption( "noArgLongName2", "t", "'t' stands for two" );

        CHECK( !po.CommandLineHas( "noArgLongName1" ) );
        CHECK( !po.CommandLineHas( "noArgLongName2" ) );

        // ----------------------------------------------------------
        // --- Test parsing the command line when no arguments given.
        // ----------------------------------------------------------
        int t1_argc = 1 ;
        char* t1_argv[] =
        {
            "Eradication.exe"
        };

        string t1_errmsg = po.ParseCommandLine( t1_argc, t1_argv );
        CHECK( t1_errmsg.empty() );

        CHECK( !po.CommandLineHas( "noArgLongName1" ) );
        CHECK( !po.CommandLineHas( "noArgLongName2" ) );

        // -----------------------------------------------------
        // --- Test parsing when one of the arguments are given
        // -----------------------------------------------------
        int t2_argc = 2 ;
        char* t2_argv[] =
        {
            "Eradication.exe",
            "--noArgLongName2"
        };

        string t2_errmsg = po.ParseCommandLine( t2_argc, t2_argv );
        CHECK( t2_errmsg.empty() );

        CHECK( !po.CommandLineHas( "noArgLongName1" ) );
        CHECK(  po.CommandLineHas( "noArgLongName2" ) );

        // -----------------------------------------------------
        // --- Test parsing when both of the arguments are given
        // -----------------------------------------------------
        int t3_argc = 3 ;
        char* t3_argv[] =
        {
            "Eradication.exe",
            "--noArgLongName2",
            "--noArgLongName1"
        };

        string t3_errmsg = po.ParseCommandLine( t3_argc, t3_argv );
        CHECK( t3_errmsg.empty() );

        CHECK(  po.CommandLineHas( "noArgLongName1" ) );
        CHECK(  po.CommandLineHas( "noArgLongName2" ) );

        // -----------------------------------------------------
        // --- Test parsing when one of the arguments is unknown
        // -----------------------------------------------------
        int t4_argc = 3 ;
        char* t4_argv[] =
        {
            "Eradication.exe",
            "--noArgLongName2",
            "--XXXXX"
        };

        string t4_errmsg = po.ParseCommandLine( t4_argc, t4_argv );
        CHECK( !t4_errmsg.empty() );
        CHECK_EQUAL( "Error parsing command line: unrecognised option '--XXXXX'", t4_errmsg );

        // ----------------------------------
        // --- Test parsing using short name
        // ----------------------------------
        int t5_argc = 3 ;
        char* t5_argv[] =
        {
            "Eradication.exe",
            "-t",
            "--noArgLongName1"
        };

        string t5_errmsg = po.ParseCommandLine( t5_argc, t5_argv );
        CHECK( t5_errmsg.empty() );

        CHECK(  po.CommandLineHas( "noArgLongName1" ) );
        CHECK(  po.CommandLineHas( "noArgLongName2" ) );

        // ------------------------------------
        // --- Test printing of these arguments
        // ------------------------------------
        string exp ;
        exp += "test description:\n" ;
        exp += "  --noArgLongName1                      description for first option\n" ;
        exp += "  -t [ --noArgLongName2 ]               't' stands for two\n" ;

        std::ostringstream got_oss ;
        po.Print( got_oss );
        string got = got_oss.str() ;

        CHECK_EQUAL( exp, got );
    }

    TEST(TestOptionWithValueInt)
    {
        ProgramOptions po( "test description" );
        po.AddOptionWithValue( "intLongName1", 777, "description for first option" );
        po.AddOptionWithValue( "intLongName2", 888, "description for second option" );

        CHECK( po.CommandLineHas( "intLongName1" ) );
        CHECK( po.CommandLineHas( "intLongName2" ) );
        CHECK_EQUAL( 777, po.GetCommandLineValueInt( "intLongName1" ) );
        CHECK_EQUAL( 888, po.GetCommandLineValueInt( "intLongName2" ) );

        // -----------------------------------------------------------------
        // --- Test parsing when one of the arguments are given with a space
        // -----------------------------------------------------------------
        int t1_argc = 3 ;
        char* t1_argv[] =
        {
            "Eradication.exe",
            "--intLongName1",
            "999"
        };

        string t1_errmsg = po.ParseCommandLine( t1_argc, t1_argv );
        CHECK( t1_errmsg.empty() );

        CHECK( po.CommandLineHas( "intLongName1" ) );
        CHECK( po.CommandLineHas( "intLongName2" ) );
        CHECK_EQUAL( 999, po.GetCommandLineValueInt( "intLongName1" ) );
        CHECK_EQUAL( 888, po.GetCommandLineValueInt( "intLongName2" ) );

        // -----------------------------------------------------------------
        // --- Test parsing when both of the arguments are given:
        // --- one with a space and one with a '='
        // -----------------------------------------------------------------
        int t2_argc = 4 ;
        char* t2_argv[] =
        {
            "Eradication.exe",
            "--intLongName2=444",
            "--intLongName1",
            "999"
        };

        string t2_errmsg = po.ParseCommandLine( t2_argc, t2_argv );
        CHECK( t2_errmsg.empty() );

        CHECK( po.CommandLineHas( "intLongName1" ) );
        CHECK( po.CommandLineHas( "intLongName2" ) );
        CHECK_EQUAL( 999, po.GetCommandLineValueInt( "intLongName1" ) );
        CHECK_EQUAL( 444, po.GetCommandLineValueInt( "intLongName2" ) );

        // --------------------------------------------------------------------
        // --- Test parsing when no value given and not implicit - with a space
        // --------------------------------------------------------------------
        int t3_argc = 2 ;
        char* t3_argv[] =
        {
            "Eradication.exe",
            "--intLongName1"
        };

        string t3_exp = "Error parsing command line: missing value for option 'intLongName1'" ;
        string t3_got = po.ParseCommandLine( t3_argc, t3_argv );
        CHECK( !t3_got.empty() );
        CHECK_EQUAL( t3_exp, t3_got );

        // ------------------------------------------------------------------
        // --- Test parsing when no value given and not implicit - with a '="
        // ------------------------------------------------------------------
        int t4_argc = 2 ;
        char* t4_argv[] =
        {
            "Eradication.exe",
            "--intLongName1="
        };

        string t4_exp = "Error parsing command line: missing value for option 'intLongName1'" ;
        string t4_got = po.ParseCommandLine( t4_argc, t4_argv );
        CHECK( !t4_got.empty() );
        CHECK_EQUAL( t4_exp, t4_got );

        // ------------------------------------------------------------------
        // --- Test parsing when illegal value given 
        // ------------------------------------------------------------------
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // !!! this test will cause two 'First-chance exception' statements to appear in output
        // !!! It is ok.  It is just std::stoi() choking on trying to conver the string.
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        int t5_argc = 3 ;
        char* t5_argv[] =
        {
            "Eradication.exe",
            "--intLongName1",
            "bad-value"
        };

        string t5_exp = "Error parsing command line: illegal value(bad-value) for option 'intLongName1'" ;
        string t5_got = po.ParseCommandLine( t5_argc, t5_argv );
        CHECK( !t5_got.empty() );
        CHECK_EQUAL( t5_exp, t5_got );

        // ------------------------------------
        // --- Test printing of these arguments
        // ------------------------------------
        string exp ;
        exp += "test description:\n" ;
        exp += "  --intLongName1 arg (=777)             description for first option\n" ;
        exp += "  --intLongName2 arg (=888)             description for second option\n" ;

        std::ostringstream got_oss ;
        po.Print( got_oss );
        string got = got_oss.str() ;

        CHECK_EQUAL( exp, got );
    }

    TEST(TestOptionWithValueIntImplicit)
    {
        ProgramOptions po( "test description" );
        po.AddOptionWithValue( "intLongName1", 777, 222, "description for first option" );
        po.AddOptionWithValue( "intLongName2", 888, 333, "description for second option" );
        po.AddOptionWithValue( "intLongName3", 999,      "description for third option" );

        CHECK( po.CommandLineHas( "intLongName1" ) );
        CHECK( po.CommandLineHas( "intLongName2" ) );
        CHECK( po.CommandLineHas( "intLongName3" ) );
        CHECK_EQUAL( 777, po.GetCommandLineValueInt( "intLongName1" ) );
        CHECK_EQUAL( 888, po.GetCommandLineValueInt( "intLongName2" ) );
        CHECK_EQUAL( 999, po.GetCommandLineValueInt( "intLongName3" ) );

        // -----------------------------------------------------------------
        // --- Test parsing when one of the arguments are given with a space
        // -----------------------------------------------------------------
        int t1_argc = 3 ;
        char* t1_argv[] =
        {
            "Eradication.exe",
            "--intLongName1",
            "111"
        };

        string t1_errmsg = po.ParseCommandLine( t1_argc, t1_argv );
        CHECK( t1_errmsg.empty() );

        CHECK( po.CommandLineHas( "intLongName1" ) );
        CHECK( po.CommandLineHas( "intLongName2" ) );
        CHECK( po.CommandLineHas( "intLongName3" ) );
        CHECK_EQUAL( 111, po.GetCommandLineValueInt( "intLongName1" ) );
        CHECK_EQUAL( 888, po.GetCommandLineValueInt( "intLongName2" ) );
        CHECK_EQUAL( 999, po.GetCommandLineValueInt( "intLongName3" ) );

        // -----------------------------------------------------------------
        // --- Test parsing when one of the arguments is given with no value (implicit)
        // -----------------------------------------------------------------
        int t2_argc = 2 ;
        char* t2_argv[] =
        {
            "Eradication.exe",
            "--intLongName1"
        };

        string t2_errmsg = po.ParseCommandLine( t2_argc, t2_argv );
        CHECK( t2_errmsg.empty() );

        CHECK( po.CommandLineHas( "intLongName1" ) );
        CHECK( po.CommandLineHas( "intLongName2" ) );
        CHECK( po.CommandLineHas( "intLongName3" ) );
        CHECK_EQUAL( 222, po.GetCommandLineValueInt( "intLongName1" ) );
        CHECK_EQUAL( 888, po.GetCommandLineValueInt( "intLongName2" ) );
        CHECK_EQUAL( 999, po.GetCommandLineValueInt( "intLongName3" ) );

        // --------------------------------------------------------------------------------
        // --- Test parsing when the option that does not have an implicit value should cause failure
        // -------------------------------------------------------------------------------
        int t3_argc = 3 ;
        char* t3_argv[] =
        {
            "Eradication.exe",
            "--intLongName1",
            "--intLongName3"
        };

        string t3_errmsg = po.ParseCommandLine( t3_argc, t3_argv );
        CHECK( !t3_errmsg.empty() );
        CHECK_EQUAL( "Error parsing command line: missing value for option 'intLongName3'", t3_errmsg ) ;

        // ------------------------------------
        // --- Test printing of these arguments
        // ------------------------------------
        string exp ;
        exp += "test description:\n" ;
        exp += "  --intLongName1 [=arg(=222)] (=777)    description for first option\n" ;
        exp += "  --intLongName2 [=arg(=333)] (=888)    description for second option\n" ;
        exp += "  --intLongName3 arg (=999)             description for third option\n" ;

        std::ostringstream got_oss ;
        po.Print( got_oss );
        string got = got_oss.str() ;

        CHECK_EQUAL( exp, got );
    }

    TEST(TestOptionWithValueString)
    {
        ProgramOptions po( "test description" );
        po.AddOptionWithValue( "strLongName1", "dv_1",         "description for first option"  );
        po.AddOptionWithValue( "strLongName2", "dv_2",         "description for second option" );
        po.AddOptionWithValue( "strLongName3", "",             "description for third option"  );
        po.AddOptionWithValue( "strLongName4", "F",    "dv_4", "F is for 'four'"               );

        CHECK( po.CommandLineHas( "strLongName1" ) );
        CHECK( po.CommandLineHas( "strLongName2" ) );
        CHECK( po.CommandLineHas( "strLongName3" ) );
        CHECK( po.CommandLineHas( "strLongName4" ) );
        CHECK_EQUAL( "dv_1", po.GetCommandLineValueString( "strLongName1" ) );
        CHECK_EQUAL( "dv_2", po.GetCommandLineValueString( "strLongName2" ) );
        CHECK_EQUAL( "",     po.GetCommandLineValueString( "strLongName3" ) );
        CHECK_EQUAL( "dv_4", po.GetCommandLineValueString( "strLongName4" ) );

        // -----------------------------------------------------------------
        // --- Test parsing when one of the arguments are given with a space
        // -----------------------------------------------------------------
        int t1_argc = 3 ;
        char* t1_argv[] =
        {
            "Eradication.exe",
            "--strLongName2",
            "abcd"
        };

        string t1_errmsg = po.ParseCommandLine( t1_argc, t1_argv );
        CHECK( t1_errmsg.empty() );

        CHECK( po.CommandLineHas( "strLongName1" ) );
        CHECK( po.CommandLineHas( "strLongName2" ) );
        CHECK( po.CommandLineHas( "strLongName3" ) );
        CHECK( po.CommandLineHas( "strLongName4" ) );
        CHECK_EQUAL( "dv_1", po.GetCommandLineValueString( "strLongName1" ) );
        CHECK_EQUAL( "abcd", po.GetCommandLineValueString( "strLongName2" ) );
        CHECK_EQUAL( "",     po.GetCommandLineValueString( "strLongName3" ) );
        CHECK_EQUAL( "dv_4", po.GetCommandLineValueString( "strLongName4" ) );

        CHECK_EQUAL( "dv_1", po.GetCommandLineValueDefaultString( "strLongName1" ) );
        // -----------------------------------------------------------------
        // --- Test parsing when one of the arguments are given with a space
        // --- and the other with an equal
        // -----------------------------------------------------------------
        int t2_argc = 4 ;
        char* t2_argv[] =
        {
            "Eradication.exe",
            "--strLongName2",
            "abcd",
            "--strLongName3=wxyz"
        };

        string t2_errmsg = po.ParseCommandLine( t2_argc, t2_argv );
        CHECK( t2_errmsg.empty() );

        CHECK( po.CommandLineHas( "strLongName1" ) );
        CHECK( po.CommandLineHas( "strLongName2" ) );
        CHECK( po.CommandLineHas( "strLongName3" ) );
        CHECK( po.CommandLineHas( "strLongName4" ) );
        CHECK_EQUAL( "dv_1", po.GetCommandLineValueString( "strLongName1" ) );
        CHECK_EQUAL( "abcd", po.GetCommandLineValueString( "strLongName2" ) );
        CHECK_EQUAL( "wxyz", po.GetCommandLineValueString( "strLongName3" ) );
        CHECK_EQUAL( "dv_4", po.GetCommandLineValueString( "strLongName4" ) );

        // -----------------------------------------------------------------
        // --- Test parsing when one of the arguments are given with a space
        // --- and the other with an equal and a third using short name
        // -----------------------------------------------------------------
        int t3_argc = 5 ;
        char* t3_argv[] =
        {
            "Eradication.exe",
            "--strLongName2",
            "abcd",
            "--strLongName3=wxyz",
            "-F=qwer" 
        };

        string t3_errmsg = po.ParseCommandLine( t3_argc, t3_argv );
        CHECK( t3_errmsg.empty() );

        CHECK( po.CommandLineHas( "strLongName1" ) );
        CHECK( po.CommandLineHas( "strLongName2" ) );
        CHECK( po.CommandLineHas( "strLongName3" ) );
        CHECK( po.CommandLineHas( "strLongName4" ) );
        CHECK_EQUAL( "dv_1", po.GetCommandLineValueString( "strLongName1" ) );
        CHECK_EQUAL( "abcd", po.GetCommandLineValueString( "strLongName2" ) );
        CHECK_EQUAL( "wxyz", po.GetCommandLineValueString( "strLongName3" ) );
        CHECK_EQUAL( "qwer", po.GetCommandLineValueString( "strLongName4" ) );

        // -----------------------------------------------------------------
        // --- Test parsing when one of the arguments are given with a space
        // --- and the other with an equal and a third using short name
        // --- but with space this time
        // -----------------------------------------------------------------
        int t4_argc = 6 ;
        char* t4_argv[] =
        {
            "Eradication.exe",
            "--strLongName2",
            "abcd",
            "--strLongName3=wxyz",
            "-F" ,
            "qwer" 
        };

        string t4_errmsg = po.ParseCommandLine( t4_argc, t4_argv );
        CHECK( t4_errmsg.empty() );

        CHECK( po.CommandLineHas( "strLongName1" ) );
        CHECK( po.CommandLineHas( "strLongName2" ) );
        CHECK( po.CommandLineHas( "strLongName3" ) );
        CHECK( po.CommandLineHas( "strLongName4" ) );
        CHECK_EQUAL( "dv_1", po.GetCommandLineValueString( "strLongName1" ) );
        CHECK_EQUAL( "abcd", po.GetCommandLineValueString( "strLongName2" ) );
        CHECK_EQUAL( "wxyz", po.GetCommandLineValueString( "strLongName3" ) );
        CHECK_EQUAL( "qwer", po.GetCommandLineValueString( "strLongName4" ) );

        // ------------------------------------
        // --- Test printing of these arguments
        // ------------------------------------
        string exp ;
        exp += "test description:\n" ;
        exp += "  --strLongName1 arg (=dv_1)            description for first option\n" ;
        exp += "  --strLongName2 arg (=dv_2)            description for second option\n" ;
        exp += "  --strLongName3 arg                    description for third option\n" ;
        exp += "  -F [ --strLongName4 ] arg (=dv_4)     F is for 'four'\n" ;

        std::ostringstream got_oss ;
        po.Print( got_oss );
        string got = got_oss.str() ;

        CHECK_EQUAL( exp, got );
    }    
    
    TEST(TestLastArgumentNoValue)
    {
        ProgramOptions po("test description");
        po.AddOptionWithValue("strLongName1", "dv_1", "description for first option");
        po.AddOptionWithValue("strLongName2", "T", "dv_2", "T is for 'two'");

        CHECK(po.CommandLineHas("strLongName1"));
        CHECK(po.CommandLineHas("strLongName2"));
        CHECK_EQUAL("dv_1", po.GetCommandLineValueString("strLongName1"));
        CHECK_EQUAL("dv_2", po.GetCommandLineValueString("strLongName2"));

        // --------------------------------------------------------------
        // --- Test parsing when one of the arguments has no string value
        // --------------------------------------------------------------
        int t1_argc = 4;
        char* t1_argv[] =
        {
            "Eradication.exe",
            "--strLongName1",
            "abcd",
            "--strLongName2"
        };

        string t1_errmsg = po.ParseCommandLine(t1_argc, t1_argv);
        CHECK( !t1_errmsg.empty() );
        CHECK_EQUAL("Error parsing command line: missing value for option 'strLongName2'", t1_errmsg);
    }

    TEST(TestOptionWithValueStringList)
    {
        vector<string> pv_1 ;
        pv_1.push_back( "oneA" );
        pv_1.push_back( "oneB" );
        pv_1.push_back( "oneC" );

        vector<string> pv_2 ;
        pv_2.push_back( "twoA" );
        pv_2.push_back( "twoB" );

        ProgramOptions po( "test description" );
        po.AddOptionWithValue( "listLongName1", pv_1, "description for first option" );
        po.AddOptionWithValue( "listLongName2", pv_2, "description for second option" );

        CHECK( po.CommandLineHas( "listLongName1" ) );
        CHECK( po.CommandLineHas( "listLongName1" ) );
        CHECK_EQUAL( "oneA", po.GetCommandLineValueString( "listLongName1" ) );
        CHECK_EQUAL( "twoA", po.GetCommandLineValueString( "listLongName2" ) );

        // -----------------------------------------------------------------
        // --- Test parsing when one of the arguments are given with a space
        // -----------------------------------------------------------------
        int t1_argc = 3 ;
        char* t1_argv[] =
        {
            "Eradication.exe",
            "--listLongName1",
            "oneC"
        };

        string t1_errmsg = po.ParseCommandLine( t1_argc, t1_argv );
        CHECK( t1_errmsg.empty() );

        CHECK( po.CommandLineHas( "listLongName1" ) );
        CHECK( po.CommandLineHas( "listLongName2" ) );
        CHECK_EQUAL( "oneC", po.GetCommandLineValueString( "listLongName1" ) );
        CHECK_EQUAL( "twoA", po.GetCommandLineValueString( "listLongName2" ) );

        // -----------------------------------------------------------------
        // --- Test parsing when both of the arguments are given
        // --- one with a space and one with a '='
        // -----------------------------------------------------------------
        int t2_argc = 4 ;
        char* t2_argv[] =
        {
            "Eradication.exe",
            "--listLongName1",
            "oneC",
            "--listLongName2=twoB"
        };

        string t2_errmsg = po.ParseCommandLine( t2_argc, t2_argv );
        CHECK( t2_errmsg.empty() );

        CHECK( po.CommandLineHas( "listLongName1" ) );
        CHECK( po.CommandLineHas( "listLongName2" ) );
        CHECK_EQUAL( "oneC", po.GetCommandLineValueString( "listLongName1" ) );
        CHECK_EQUAL( "twoB", po.GetCommandLineValueString( "listLongName2" ) );

        // -----------------------------------------------------------------
        // --- Test parsing when one of the arguments has an invalid value
        // -----------------------------------------------------------------
        int t3_argc = 2 ;
        char* t3_argv[] =
        {
            "Eradication.exe",
            "--listLongName2=XXX"
        };

        string t3_errmsg = po.ParseCommandLine( t3_argc, t3_argv );
        CHECK( !t3_errmsg.empty() );
        CHECK_EQUAL( "Error parsing command line: illegal value(XXX) for option 'listLongName2'", t3_errmsg );

        // ------------------------------------
        // --- Test printing of these arguments
        // ------------------------------------
        string exp ;
        exp += "test description:\n" ;
        exp += "  --listLongName1 arg (=oneA)           description for first option\n" ;
        exp += "  --listLongName2 arg (=twoA)           description for second option\n" ;

        std::ostringstream got_oss ;
        po.Print( got_oss );
        string got = got_oss.str() ;

        CHECK_EQUAL( exp, got );
    }

    TEST(TestEradicationOptions)
    {
        ProgramOptions po("Recognized options");

        po.AddOption( "help",         "Show this help message." );
        po.AddOption( "version", "v", "Get version info." );
        po.AddOption("get-schema",     "Request the kernel to write all its input definition schema json to the current working directory and exit." );
        po.AddOptionWithValue( "schema-path", "stdout",                     "Path to write schema(s) to instead of writing to stdout." );
        po.AddOptionWithValue( "config",       "C",          "config.json", "Name of config.json file to use" );
        po.AddOptionWithValue( "input-path",   "I",          ".",           "Relative or absolute path to location of model input files" );       
        po.AddOptionWithValue( "output-path",  "O",          "output",      "Relative or absolute path for output files" );
        po.AddOptionWithValue( "dll-path",     "D",          "",            "Relative (to the executable) or absolute path for dlls" );
        po.AddOptionWithValue( "monitor_host",               "none",        "IP of commissioning/monitoring host" );
        po.AddOptionWithValue( "monitor_port",               0,             "port of commissioning/monitoring host" );
        po.AddOptionWithValue( "sim_id",                    "none",         "Unique id of this simulation, formerly sim_guid. Needed for self-identification to UDP host" );
        po.AddOption( "progress",        "Send updates on the progress of the simulation to the HPC job scheduler." );

        CHECK( !po.CommandLineHas( "help"         ) );
        CHECK( !po.CommandLineHas( "version"      ) );
        CHECK( !po.CommandLineHas( "get-schema"   ) );
        CHECK(  po.CommandLineHas( "schema-path"  ) );
        CHECK(  po.CommandLineHas( "config"       ) );
        CHECK(  po.CommandLineHas( "input-path"   ) );
        CHECK(  po.CommandLineHas( "output-path"  ) );
        CHECK(  po.CommandLineHas( "dll-path"     ) );
        CHECK(  po.CommandLineHas( "monitor_host" ) );
        CHECK(  po.CommandLineHas( "monitor_port" ) );
        CHECK(  po.CommandLineHas( "sim_id"       ) );
        CHECK( !po.CommandLineHas( "progress"     ) );
        CHECK_EQUAL( "stdout",      po.GetCommandLineValueString( "schema-path"  ) );
        CHECK_EQUAL( "config.json", po.GetCommandLineValueString( "config"       ) );
        CHECK_EQUAL( ".",           po.GetCommandLineValueString( "input-path"   ) );
        CHECK_EQUAL( "output",      po.GetCommandLineValueString( "output-path"  ) );
        CHECK_EQUAL( "",            po.GetCommandLineValueString( "dll-path"     ) );
        CHECK_EQUAL( "none",        po.GetCommandLineValueString( "monitor_host" ) );
        CHECK_EQUAL( 0,             po.GetCommandLineValueInt(    "monitor_port" ) );
        CHECK_EQUAL( "none",        po.GetCommandLineValueString( "sim_id"       ) );

        // -----------------------------------------------------------------
        // --- Test parsing with default values in Visual Studio
        // -----------------------------------------------------------------
        int t1_argc = 7 ;
        char* t1_argv[] =
        {
            "Eradication.exe",
            "--config",
            "config.json",
            "--input-path",
            ".",
            "--output-path",
            "testing"
        };

        string t1_errmsg = po.ParseCommandLine( t1_argc, t1_argv );
        CHECK( t1_errmsg.empty() );

        CHECK( !po.CommandLineHas( "help"         ) );
        CHECK( !po.CommandLineHas( "version"      ) );
        CHECK( !po.CommandLineHas( "get-schema"   ) );
        CHECK(  po.CommandLineHas( "schema-path"  ) );
        CHECK(  po.CommandLineHas( "config"       ) );
        CHECK(  po.CommandLineHas( "input-path"   ) );
        CHECK(  po.CommandLineHas( "output-path"  ) );
        CHECK(  po.CommandLineHas( "dll-path"     ) );
        CHECK(  po.CommandLineHas( "monitor_host" ) );
        CHECK(  po.CommandLineHas( "monitor_port" ) );
        CHECK(  po.CommandLineHas( "sim_id"       ) );
        CHECK( !po.CommandLineHas( "progress"     ) );
        CHECK_EQUAL( "stdout",      po.GetCommandLineValueString( "schema-path"  ) );
        CHECK_EQUAL( "config.json", po.GetCommandLineValueString( "config"       ) );
        CHECK_EQUAL( ".",           po.GetCommandLineValueString( "input-path"   ) );
        CHECK_EQUAL( "testing",     po.GetCommandLineValueString( "output-path"  ) );
        CHECK_EQUAL( "",            po.GetCommandLineValueString( "dll-path"     ) );
        CHECK_EQUAL( "none",        po.GetCommandLineValueString( "monitor_host" ) );
        CHECK_EQUAL( 0,             po.GetCommandLineValueInt(    "monitor_port" ) );
        CHECK_EQUAL( "none",        po.GetCommandLineValueString( "sim_id"       ) );

        // -----------------------------------------------------------------
        // --- Test parsing help
        // -----------------------------------------------------------------
        po.Reset();

        int t2_argc = 2 ;
        char* t2_argv[] =
        {
            "Eradication.exe",
            "--help"
        };

        string t2_errmsg = po.ParseCommandLine( t2_argc, t2_argv );
        CHECK( t2_errmsg.empty() );

        CHECK(  po.CommandLineHas( "help"         ) );
        CHECK( !po.CommandLineHas( "version"      ) );
        CHECK( !po.CommandLineHas( "get-schema"   ) );
        CHECK(  po.CommandLineHas( "schema-path"  ) );
        CHECK(  po.CommandLineHas( "config"       ) );
        CHECK(  po.CommandLineHas( "input-path"   ) );
        CHECK(  po.CommandLineHas( "output-path"  ) );
        CHECK(  po.CommandLineHas( "dll-path"     ) );
        CHECK(  po.CommandLineHas( "monitor_host" ) );
        CHECK(  po.CommandLineHas( "monitor_port" ) );
        CHECK(  po.CommandLineHas( "sim_id"       ) );
        CHECK( !po.CommandLineHas( "progress"     ) );
        CHECK_EQUAL( "stdout",      po.GetCommandLineValueString( "schema-path"  ) );
        CHECK_EQUAL( "config.json", po.GetCommandLineValueString( "config"       ) );
        CHECK_EQUAL( ".",           po.GetCommandLineValueString( "input-path"   ) );
        CHECK_EQUAL( "output",      po.GetCommandLineValueString( "output-path"  ) );
        CHECK_EQUAL( "",            po.GetCommandLineValueString( "dll-path"     ) );
        CHECK_EQUAL( "none",        po.GetCommandLineValueString( "monitor_host" ) );
        CHECK_EQUAL( 0,             po.GetCommandLineValueInt(    "monitor_port" ) );
        CHECK_EQUAL( "none",        po.GetCommandLineValueString( "sim_id"       ) );

        // ------------------------------------
        // --- Test printing of these arguments
        // ------------------------------------
        string exp ;
        exp += "Recognized options:\n";
        exp += "  --help                                Show this help message.\n";
        exp += "  -v [ --version ]                      Get version info.\n";
        exp += "  --get-schema                          Request the kernel to write all its \n";
        exp += "                                        input definition schema json to the \n";
        exp += "                                        current working directory and exit.\n";
        exp += "  --schema-path arg (=stdout)           Path to write schema(s) to instead of \n";
        exp += "                                        writing to stdout.\n";
        exp += "  -C [ --config ] arg (=config.json)    Name of config.json file to use\n";
        exp += "  -I [ --input-path ] arg (=.)          Relative or absolute path to location \n";
        exp += "                                        of model input files\n";
        exp += "  -O [ --output-path ] arg (=output)    Relative or absolute path for output \n";
        exp += "                                        files\n";
        exp += "  -D [ --dll-path ] arg                 Relative (to the executable) or \n";
        exp += "                                        absolute path for dlls\n";
        exp += "  --monitor_host arg (=none)            IP of commissioning/monitoring host\n";
        exp += "  --monitor_port arg (=0)               port of commissioning/monitoring host\n";
        exp += "  --sim_id arg (=none)                  Unique id of this simulation, formerly \n";
        exp += "                                        sim_guid. Needed for \n";
        exp += "                                        self-identification to UDP host\n";
        exp += "  --progress                            Send updates on the progress of the \n";
        exp += "                                        simulation to the HPC job scheduler.\n";

        std::ostringstream got_oss ;
        po.Print( got_oss );
        string got = got_oss.str() ;

        CHECK_EQUAL( exp, got );
    }
}