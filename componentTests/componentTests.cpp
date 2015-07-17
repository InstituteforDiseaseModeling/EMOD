/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"
#include "UnitTest++.h"
#include "IdmInitialize.h"

using namespace std;

int main(int argc, char* argv[])
{
    // ----------------------------------------------------------------------------------
    // --- IdmInitialize() - This is the method that connects the unit tests
    // --- to Eradication.exe.  It is called once at the beginning.
    // --- I have not verified this, but this should mean that all static code/variables
    // --- are created/run once for the unit tests.
    // ----------------------------------------------------------------------------------
    IdmInitialize();

    return UnitTest::RunAllTests();
}
