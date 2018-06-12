/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

//////////////////////////////////////////////////////////////////////////////
//
// The wrapper class for underlying json libraries
//
#pragma once

#include "JsonObject.h"

#include "rapidjson/prettywriter.h" // for stringify JSON
#include "rapidjson/document.h"
#include <vector>

#include "rapidjson/stringbuffer.h"

using namespace std;
using namespace rapidjson;

namespace Kernel {

    // Actual implementation for specific JSon library
    // Since it is an adapter between the user and the JSon library, it is 
    // necessary to keep as minimium internal states as possible. The main
    // function is to adapt the call from the user to the right method in 
    // the underlying library
    class RapidJsonObj : public IJsonObjectAdapter
    {
    public:

        RapidJsonObj();
        virtual ~RapidJsonObj();

        // IJsonAdapter interface
        virtual void CreateNewWriter(bool bCacheWriter = false) override;
        virtual void FinishWriter() override;
        virtual void BeginObject() override;
        virtual void EndObject() override;
        virtual void BeginArray() override;
        virtual void EndArray() override;
        virtual const char* ToString() const override;
        virtual char* ToPrettyString() const override;
        virtual void Parse(const char* jsBuffer) override;

        virtual void DeleteUnderlyingJsonObjs() override;

        // IJsonObjectAdapter interface
        virtual void Insert( const char* key) override;
        virtual void Insert( const char* key, const char* val ) override;
        virtual void Insert( const char* key, const int32_t val ) override;
        virtual void Insert( const char* key, const uint32_t val ) override;
        virtual void Insert( const char* key, const int64_t val ) override;
        virtual void Insert( const char* key, const uint64_t val ) override;
        virtual void Insert( const char* key, const float val ) override;
        virtual void Insert( const char* key, const double val ) override;
        virtual void Insert( const char* key, const bool val ) override;
        virtual void Insert( const char* key, const IJsonObjectAdapter* val ) override;

        virtual void Add( const char* val ) override;
        virtual void Add( const int32_t val ) override;
        virtual void Add( const uint32_t val ) override;
        virtual void Add( const int64_t val ) override;
        virtual void Add( const uint64_t val ) override;
        virtual void Add( const float val ) override;
        virtual void Add( const double val ) override;
        virtual void Add( const bool val ) override;
        virtual void Add( const IJsonObjectAdapter* val ) override;

        virtual bool Contains(const char* key) const override;

        virtual IJsonObjectAdapter* operator[](const char* key) const override;
        virtual IJsonObjectAdapter* GetJsonObject(const char* key) const override;
        virtual IJsonObjectAdapter* GetJsonArray(const char* key) const override;
        virtual const char* GetString(const char* key) const override;
        virtual int32_t GetInt(const char* key) const override;
        virtual uint32_t GetUint(const char* key) const override;
        virtual int64_t GetInt64(const char* key) const override;
        virtual uint64_t GetUInt64(const char* key) const override;
        virtual float GetFloat(const char* key) const override;
        virtual double GetDouble(const char* key) const override;
        virtual bool GetBool(const char* key) const override;

        virtual IJsonObjectAdapter* operator[](IndexType index) const override;

        virtual const char* AsString() const override;
        virtual int32_t AsInt() const override;
        virtual uint32_t AsUint() const override;
        virtual int64_t AsInt64() const override;
        virtual uint64_t AsUint64() const override;
        virtual float AsFloat() const override;
        virtual double AsDouble() const override;
        virtual bool AsBool() const override;

        virtual operator const char*() const override;
        virtual operator int32_t() const override;
        virtual operator float() const override;
        virtual operator bool() const override;

        virtual unsigned int GetSize() const override;

    private:

        // For serialization
        StringBuffer* m_buffer;
        rapidjson::Writer<StringBuffer>* m_writer;

        typedef rapidjson::Writer<StringBuffer> RJWriter_t;
        std::vector<RJWriter_t*> m_writerQueue;
        std::vector<StringBuffer*> m_bufferQueue;

        // For deserialization
        rapidjson::Document* m_document;

        bool m_bCacheWriter;
    };
}
