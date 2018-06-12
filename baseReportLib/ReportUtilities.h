/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <string>
#include <list>
#include <vector>

namespace Kernel
{
    class IndividualHuman;
    struct IJsonObjectAdapter;
    class JSerializer;
}


namespace ReportUtilities
{
    int GetBinIndex( float val, std::vector<float>& rValues );
    int GetAgeBin( float age, std::vector<float>& rAges );

    void SendData( const std::string& rToSend );
    void GetData( int fromRank, std::vector<char>& rReceive );

    void SerializeVector( Kernel::IJsonObjectAdapter& root, Kernel::JSerializer& helper, const char* pName, std::vector<std::string>& rData );
    void SerializeVector( Kernel::IJsonObjectAdapter& root, Kernel::JSerializer& helper, const char* pName, std::vector<float>& rData );

    void SerializeVector( Kernel::IJsonObjectAdapter& root, Kernel::JSerializer& helper, const char* pName, std::vector<double>& rData );
    void SerializeVector( Kernel::IJsonObjectAdapter& root, Kernel::JSerializer& helper, const char* pName, std::vector<std::vector<double>>& rData );
    void SerializeVector( Kernel::IJsonObjectAdapter& root, Kernel::JSerializer& helper, const char* pName, std::vector<std::vector<std::vector<double>>>& rData );
    void SerializeVector( Kernel::IJsonObjectAdapter& root, Kernel::JSerializer& helper, const char* pName, std::vector<std::vector<std::vector<std::vector<double>>>>& rData);

    void DeserializeVector( Kernel::IJsonObjectAdapter& root, bool isSettingValuesInVector, const char* pName, std::vector<std::string>& rData );
    void DeserializeVector( Kernel::IJsonObjectAdapter& root, bool isSettingValuesInVector, const char* pName, std::vector<double>& rData );
    void DeserializeVector( Kernel::IJsonObjectAdapter& root, bool isSettingValuesInVector, const char* pName, std::vector<std::vector<double>>& rData);
    void DeserializeVector( Kernel::IJsonObjectAdapter& root, bool isSettingValuesInVector, const char* pName, std::vector<std::vector<std::vector<double>>>& rData );
    void DeserializeVector( Kernel::IJsonObjectAdapter& root, bool isSettingValuesInVector, const char* pName, std::vector<std::vector<std::vector<std::vector<double>>>>& rData );

    void AddVector( std::vector<double>& rThis,                                        const std::vector<double>& rThat );
    void AddVector( std::vector<std::vector<double>>& rThis,                           const std::vector<std::vector<double>>& rThat );
    void AddVector( std::vector<std::vector<std::vector<double>>>& rThis,              const std::vector<std::vector<std::vector<double>>>& rThat );
    void AddVector( std::vector<std::vector<std::vector<std::vector<double>>>>& rThis, const std::vector<std::vector<std::vector<std::vector<double>>>>& rThat );
};