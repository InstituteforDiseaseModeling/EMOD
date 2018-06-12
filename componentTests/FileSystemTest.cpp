/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include <iostream>
#include <fstream>
#include "UnitTest++.h"
#include "FileSystem.h"
#include "Exceptions.h"

using namespace std; 

SUITE(FileSystemTest)
{
    TEST(TestRemoveTrailingChars)
    {
        string str_space        = "Ends in a space ";
        string str_line_feed    = "Ends in a line feed\n" ;
        string str_return       = "Ends in a return\r" ;
        string str_backslash    = "Ends in a backslash\\" ;
        string str_forwardslash = "Ends in a forward slash/" ;
        string str_all          = "Ends in all the characters to replace \n\r\t\\/" ;
        string str_all_spaces   = "     " ;

        string exp_space        = "Ends in a space";
        string exp_line_feed    = "Ends in a line feed" ;
        string exp_return       = "Ends in a return" ;
        string exp_backslash    = "Ends in a backslash" ;
        string exp_forwardslash = "Ends in a forward slash" ;
        string exp_all          = "Ends in all the characters to replace" ;
        string exp_empty        = "" ;

        string act_space        = FileSystem::RemoveTrailingChars( str_space        );
        string act_line_feed    = FileSystem::RemoveTrailingChars( str_line_feed    );
        string act_return       = FileSystem::RemoveTrailingChars( str_return       );
        string act_backslash    = FileSystem::RemoveTrailingChars( str_backslash    );
        string act_forwardslash = FileSystem::RemoveTrailingChars( str_forwardslash );
        string act_all          = FileSystem::RemoveTrailingChars( str_all          );
        string act_empty        = FileSystem::RemoveTrailingChars( str_all_spaces   );

        CHECK_EQUAL( exp_space,        act_space        );
        CHECK_EQUAL( exp_line_feed,    act_line_feed    );
        CHECK_EQUAL( exp_return,       act_return       );
        CHECK_EQUAL( exp_backslash,    act_backslash    );
        CHECK_EQUAL( exp_forwardslash, act_forwardslash );
        CHECK_EQUAL( exp_all,          act_all          );
        CHECK_EQUAL( exp_empty,        act_empty        );

        wstring w_str_all ;
        wstring w_exp_all ;
        w_str_all.assign( str_all.begin(), str_all.end() );
        w_exp_all.assign( exp_all.begin(), exp_all.end() );

        wstring w_act_all = FileSystem::RemoveTrailingChars( w_str_all );

        CHECK( w_exp_all == w_act_all );
    }

    TEST(TestConcat)
    {
        // ------------------------------------------------------
        // --- Test simple directory and filename concatination
        // ------------------------------------------------------
        string dir = "/work/dev" ;
        string fn  = "tmp.txt" ;
        string exp = "/work/dev/tmp.txt" ;

        string act = FileSystem::Concat( dir, fn );

        CHECK_EQUAL( exp, act );

        // ---------------------------------------------------------------------------
        // --- Test where we have windows type file separators and that the routine
        // --- handles the extra separators.
        // ---------------------------------------------------------------------------
        string dir_win_sep = "\\work\\dev\\" ;
        string fn_start_with_sep = "\\tmp.txt" ;

        act = FileSystem::Concat( dir_win_sep, fn_start_with_sep );

        CHECK_EQUAL( exp, act );

        // ------------------------------------------
        // --- Same thing but with default separator
        // ------------------------------------------
        string dir_relative = "work/dev/" ;
        string fn_dir = "/tmp/" ;
        exp = "work/dev/tmp" ;

        act = FileSystem::Concat( dir_relative, fn_dir );

        CHECK_EQUAL( exp, act );
    }

    TEST(TestFileExistsAndRemoveFile)
    {
        // --------------------------------------------------------------
        // --- Test that an empty string for the file name returns false.
        // --------------------------------------------------------------
        string fn = "" ;
        bool exists = FileSystem::FileExists( fn );

        CHECK( !exists );

        // -----------------------------------------
        // --- Test that the testdata file is there
        // -----------------------------------------
        fn = "testdata/FileSystemTest/FileExists.txt" ;
        exists = FileSystem::FileExists( fn );

        CHECK( exists );

        // --------------------------------------------
        // --- Test for a file that does not exist yet.
        // --------------------------------------------
        fn = "testdata/FileSystemTest/file_to_create_and_remove.txt" ;
        exists = FileSystem::FileExists( fn );

        CHECK( !exists );

        // -------------------------------------------------------------------
        // --- show that it failes trying to remove a file that does not exist
        // -------------------------------------------------------------------
        bool success = FileSystem::RemoveFile( fn );

        CHECK( !success );

        // ----------------------------------------------------
        // --- Create this file that has not previously existed
        // ----------------------------------------------------
        std::ofstream file_stream;
        file_stream.open(fn, ios::app);
        file_stream << "file is supposed to be removed during the test." << std::endl;
        file_stream.close();

        // ----------------------------
        // --- Show that it now exists
        // ----------------------------
        exists = FileSystem::FileExists( fn );

        CHECK( exists );

        // -------------------------------
        // --- Show that we can remove it
        // -------------------------------
        success = FileSystem::RemoveFile( fn );

        CHECK( success );

        // -----------------------------------------------------------
        // --- Show that after removing the file, it no longer exists
        // -----------------------------------------------------------
        exists = FileSystem::FileExists( fn );

        CHECK( !exists );
    }

    TEST(TestDirectoryExistsAndMakeDirectory)
    {
        // --------------------------------------------------
        // --- Tests that our test data directory is present
        // --------------------------------------------------
        string base_test_dir = "testdata/FileSystemTest" ;

        bool exists = FileSystem::DirectoryExists( base_test_dir );

        CHECK( exists );

        // -------------------------------------------------
        // --- Show that a new directory does not exist yet
        // -------------------------------------------------
        string new_dir = FileSystem::Concat( base_test_dir, std::string("tmp") );

        exists = FileSystem::DirectoryExists( new_dir );

        CHECK( !exists );

        // -------------------------------------------------------------
        // --- show that we can create the directory and that it exists
        // -------------------------------------------------------------
        bool success = FileSystem::MakeDirectory( new_dir );

        CHECK( success );

        exists = FileSystem::DirectoryExists( new_dir );

        CHECK( exists );

        // ---------------------------------------------------------------
        // --- show that we cannot create a directory that already exists
        // ---------------------------------------------------------------
        success = FileSystem::MakeDirectory( new_dir );

        CHECK( !success );

        // -----------------------------------------------------------
        // --- remove the directory and show that it no longer exists
        // -----------------------------------------------------------
        success = FileSystem::RemoveDirectory( new_dir );

        CHECK( success );

        exists = FileSystem::DirectoryExists( new_dir );

        CHECK( !exists );

        // -----------------------------------------------------------------------
        // --- show that we fail trying to remove a directory that does not exist
        // -----------------------------------------------------------------------
        success = FileSystem::RemoveDirectory( new_dir );

        CHECK( !success );
    }
}