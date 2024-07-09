
// put all contexts in one place to reduce clutter in includes
#pragma once

#include <string>
#include <vector>

const std::vector<std::string> getSimTypeList();

void writeInputSchemas( const char* dll_path, const char* output_path );

