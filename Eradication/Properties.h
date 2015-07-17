/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include <map>
#include <string>

extern const char* IP_PROBABILITY_KEY;
extern const char* IP_REVERSION_KEY;
extern const char* IP_INIT_KEY;
extern const char* IP_AGE_KEY;
extern const char* IP_WHEN_KEY;
extern const char* IP_AGE_BIN_KEY;

extern const char* IP_KEY;
extern const char* IP_NAME_KEY;
extern const char* IP_VALUES_KEY;
extern const char* TRANSMISSION_MATRIX_KEY;
extern const char* ROUTE_KEY;
extern const char* TRANSMISSION_DATA_KEY;

// property bag of key:value pairs
typedef std::map< std::string, std::string > tProperties;

std::string PropertiesToString( const tProperties& properties );
std::string PropertiesToStringCsvFriendly( const tProperties& properties );
