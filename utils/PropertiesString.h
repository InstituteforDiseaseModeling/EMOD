
#pragma once

#include <map>
#include <string>

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!! OLD - want to remove
typedef std::map< std::string, std::string > tProperties;
std::string PropertiesToString( const tProperties& properties );
std::string PropertiesToStringCsvFriendly( const tProperties& properties );
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
