/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

// put all contexts in one place to reduce clutter in includes
#pragma once

#include <string>
#include <vector>

const std::vector<std::string> getSimTypeList();

void writeInputSchemas( const char* dll_path, const char* output_path );

