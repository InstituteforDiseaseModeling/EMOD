/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"

#include "Configuration.h"
#include "Serializer.h"
#include "Log.h"

//////////////////////////////////////////////////////////////////////////
// Implementation of basic JsonSerializer class that bridges between
// clientèle serialization and the underlying JSON
static const char* _module = "JsonSerializer";

using namespace std;

namespace Kernel
{
    JSerializer::JSerializer()
    {
    }

    JSerializer::~JSerializer()
    {
    }

    void JSerializer::JSerialize(const vector<int>& vectorInt, IJsonObjectAdapter* jsArrayRoot)
    {
        jsArrayRoot->BeginArray();
        for (int entry : vectorInt)
        {
            jsArrayRoot->Add(entry);
        }
        jsArrayRoot->EndArray();
    }

    void JSerializer::JSerialize(const vector<int64_t>& vectorInt64, IJsonObjectAdapter* jsArrayRoot)
    {
        jsArrayRoot->BeginArray();
        for (int64_t entry : vectorInt64)
        {
            jsArrayRoot->Add(entry);
        }
        jsArrayRoot->EndArray();
    }

    void JSerializer::JSerialize(const vector<float>& vectorFloat, IJsonObjectAdapter* jsArrayRoot)
    {
        jsArrayRoot->BeginArray();
        for (float entry : vectorFloat)
        {
            jsArrayRoot->Add(entry);
        }
        jsArrayRoot->EndArray();
    }

    void JSerializer::JSerialize(const vector<double>& vectorDouble, IJsonObjectAdapter* jsArrayRoot)
    {
        jsArrayRoot->BeginArray();
        for (double entry : vectorDouble)
        {
            jsArrayRoot->Add(entry);
        }
        jsArrayRoot->EndArray();
    }

    void JSerializer::JSerialize(const vector<vector<double>>& vectorVDouble, IJsonObjectAdapter* jsArrayRoot)
    {
        jsArrayRoot->BeginArray();
        for (auto& inner : vectorVDouble)
        {
            JSerialize(inner, jsArrayRoot);
        }
        jsArrayRoot->EndArray();
    }

    void JSerializer::JSerialize(const vector<string>& vectorString, IJsonObjectAdapter* jsArrayRoot)
    {
        jsArrayRoot->BeginArray();
        for (auto& entry : vectorString)
        {
            jsArrayRoot->Add(entry.c_str());
        }
        jsArrayRoot->EndArray();
    }

    void JSerializer::JSerialize(const list<uint32_t>& listInt, IJsonObjectAdapter* jsArrayRoot)
    {
        jsArrayRoot->BeginArray();
        for (uint32_t entry : listInt)
        {
            jsArrayRoot->Add(entry);
        }
        jsArrayRoot->EndArray();
    }

    void JSerializer::JSerialize(const int64_t* arrayInt64, int size, IJsonObjectAdapter* jsArrayRoot)
    {
        jsArrayRoot->BeginArray();
        for (int i = 0; i < size; i++)
        { 
            jsArrayRoot->Add(arrayInt64[i]);
        }
        jsArrayRoot->EndArray();
    }

    void JSerializer::JSerialize(const map<string, float>& table, IJsonObjectAdapter* jsArrayRoot)
    {
        jsArrayRoot->BeginArray();
        for (auto& entry : table)
        {
            jsArrayRoot->BeginArray();
            jsArrayRoot->Add(entry.first.c_str());
            jsArrayRoot->Add(entry.second);
            jsArrayRoot->EndArray();
        }
        jsArrayRoot->EndArray();
    }

    void JSerializer::JSerialize(const map<string, string>& sTable, IJsonObjectAdapter* jsArrayRoot)
    {
        jsArrayRoot->BeginArray();
        for (auto& entry : sTable)
        { 
            jsArrayRoot->BeginArray();
            jsArrayRoot->Add(entry.first.c_str());
            jsArrayRoot->Add(entry.second.c_str());
            jsArrayRoot->EndArray();

        }
        jsArrayRoot->EndArray();
    }

    void JSerializer::JSerialize(const set<string>& objects, IJsonObjectAdapter* jsArrayRoot)
    {
        jsArrayRoot->BeginArray();
        for (auto& entry : objects)
        {
            jsArrayRoot->Add(entry.c_str());
        }
        jsArrayRoot->EndArray();
    }

    void JSerializer::JSerialize( const char* key, const IJsonSerializable* object, IJsonObjectAdapter* root )
    {
        root->Insert(key);
        object->JSerialize(root, this);
    }

#if 0
    void JSerializer::JSerializeToString(IJsonSerializable* clientObj, string& jsStr)
    {
        IJsonObjectAdapter* jsRoot = CreateJsonObjAdapter();
        if (!clientObj || !jsRoot)
        {
            LOG_INFO("Nothing to serialize because input serializable clientObj or underlying Json object is NULL\n"); 
            return;
        }
        clientObj->JSerialize( jsRoot, this );
        jsStr = jsRoot->ToString();
        delete jsRoot;
    }

    void JSerializer::JDeserializeFromString(IJsonSerializable* clientObj, string& jsStr)
    {
        IJsonObjectAdapter* jsRoot = CreateJsonObjAdapter();
        if (!clientObj)
        {
            LOG_INFO("Nothing to deserialize because input serializable clientObj or the underlying Json object is NULL\n"); 
            return;
        }
        jsRoot->Parse(jsStr);
        clientObj->JDeserialize( jsRoot, this );
        delete jsRoot;
    }
#endif

    void JSerializer::GetFormattedOutput(const IJsonObjectAdapter* jsObject, const char*& jsStr)
    {
        jsStr = jsObject->ToString();
    }

    void JSerializer::GetPrettyFormattedOutput(const IJsonObjectAdapter* jsObject, char*& jsStr)
    {
        jsStr = jsObject->ToPrettyString();
    }

    void JSerializer::JDeserialize( IJsonSerializable* object, IJsonObjectAdapter* root, bool delete_root )
    {
        object->JDeserialize(root, this);
        if (delete_root) delete root;
    }

    void JSerializer::JDeserialize( map<string, string>& map_string_string, IJsonObjectAdapter* root, bool delete_root )
    {
        for (unsigned int i = 0; i < root->GetSize(); i++)
        {
            IJsonObjectAdapter* entry = (*root)[i];
            IJsonObjectAdapter* key   = (*entry)[(uint32_t)0];
            IJsonObjectAdapter* value = (*entry)[1];
            map_string_string[key->AsString()] = value->AsString();
            delete value;
            delete key;
            delete entry;
        }
        if (delete_root) delete root;
    }
}
