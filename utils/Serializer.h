/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <string>
#include <list>
#include <set>
#include <vector>
#include <set>
#include <map>

#include "JsonObject.h"

using namespace std;

namespace Kernel
{
    // Helper class that bridges between actual clientèle class that implements IJsonSerializable
    // and the JSon string manipulation (creation, mapping, finding, reading and writing operation)
    // It operates using the JSON adapter interface (JsonObject.h)
    class JSerializer
    {
    public:
        JSerializer();
        virtual ~JSerializer();

        // Handle vectors 
        void JSerialize(const vector<int>& vectorInt, IJsonObjectAdapter* jsArrayRoot);
        void JSerialize(const vector<int64_t>& vectorInt64, IJsonObjectAdapter* jsArrayRoot);
        void JSerialize(const vector<uint32_t>& vectorUInt32, IJsonObjectAdapter* jsArrayRoot);
        void JSerialize(const vector<float>& vectorFloat, IJsonObjectAdapter* jsArrayRoot);
        void JSerialize(const vector<double>& vectorDouble, IJsonObjectAdapter* jsArrayRoot);
        void JSerialize(const vector<vector<double>>& vectorVDouble, IJsonObjectAdapter* jsArrayRoot);
		void JSerialize(const vector<vector<vector<double>>>& vectorVDouble, IJsonObjectAdapter* jsArrayRoot);
        void JSerialize(const vector<string>& vectorString, IJsonObjectAdapter* jsArrayRoot);

        // Handle lists
        void JSerialize(const list<uint32_t>& listInt, IJsonObjectAdapter* jsArrayRoot);

        // Handle arrays
        void JSerialize(const int64_t* arrayInt64, int size, IJsonObjectAdapter* jsArrayRoot);

        // Handle maps and sets
        void JSerialize(const map<string, float>& table, IJsonObjectAdapter* jsArrayRoot);
        void JSerialize(const map<string, string>& sTable, IJsonObjectAdapter* jsArrayRoot);
        void JDeserialize(map<string, string>& map_string_string, IJsonObjectAdapter* root, bool delete_root = true);
        void JSerialize(const set<string>& objects, IJsonObjectAdapter* jsArrayRoot);

        void GetFormattedOutput(const IJsonObjectAdapter* jsObject, const char*& jsStr);
        void GetPrettyFormattedOutput(const IJsonObjectAdapter* jsObject, char*& jsStr);
    };
}
