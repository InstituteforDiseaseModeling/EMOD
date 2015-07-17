/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

//////////////////////////////////////////////////////////////////////////////
//
// The wrapper class for underlying json libraries
//
#pragma once

#include "rapidjson/prettywriter.h" // for stringify JSON
#include "rapidjson/filestream.h"   // wrapper of C stream for prettywriter as output
#include "rapidjson/document.h"
#include <cstdio>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <time.h>

#include "JsonObject.h"

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
        virtual void CreateNewWriter(bool bCacheWriter = false);
        virtual void FinishWriter();
        virtual void BeginObject();
        virtual void EndObject();
        virtual void BeginArray();
        virtual void EndArray();
        virtual const char* ToString() const;
        virtual char* ToPrettyString() const;
        virtual void Parse(const char* jsBuffer);

        virtual void DeleteUnderlyingJsonObjs();

        // IJsonObjectAdapter interface
        virtual void Insert( const char* key);
        virtual void Insert( const char* key, const char* val );
        virtual void Insert( const char* key, const int32_t val );
        virtual void Insert( const char* key, const uint32_t val );
        virtual void Insert( const char* key, const int64_t val );
        virtual void Insert( const char* key, const uint64_t val );
        virtual void Insert( const char* key, const float val );
        virtual void Insert( const char* key, const double val );
        virtual void Insert( const char* key, const bool val );
        virtual void Insert( const char* key, const IJsonObjectAdapter* val );

        virtual void Add( const char* val );
        virtual void Add( const int32_t val );
        virtual void Add( const uint32_t val );
        virtual void Add( const int64_t val );
        virtual void Add( const uint64_t val );
        virtual void Add( const float val );
        virtual void Add( const double val );
        virtual void Add( const bool val );
        virtual void Add( const IJsonObjectAdapter* val );

        virtual IJsonObjectAdapter* operator[](const char* key) const;
        virtual IJsonObjectAdapter* GetObject(const char* key) const;
        virtual IJsonObjectAdapter* GetArray(const char* key) const;
        virtual const char* GetString(const char* key) const;
        virtual int32_t GetInt(const char* key) const;
        virtual uint32_t GetUint(const char* key) const;
        virtual int64_t GetInt64(const char* key) const;
        virtual uint64_t GetUInt64(const char* key) const;
        virtual float GetFloat(const char* key) const;
        virtual double GetDouble(const char* key) const;
        virtual bool GetBool(const char* key) const;

        virtual IJsonObjectAdapter* operator[](IndexType index) const;

        virtual const char* AsString() const;
        virtual int32_t AsInt() const;
        virtual uint32_t AsUint() const;
        virtual int64_t AsInt64() const;
        virtual uint64_t AsUint64() const;
        virtual float AsFloat() const;
        virtual double AsDouble() const;
        virtual bool AsBool() const;

/*
        operator const char*() { return AsString(); }
        operator int32_t()     { return AsInt(); }
        operator uint32_t()    { return AsUint(); }
        operator int64_t()     { return AsInt64(); }
        operator uint64_t()    { return AsUint64(); }
        operator float()       { return AsFloat(); }
        operator double()      { return AsDouble(); }
        operator bool()        { return AsBool(); }
*/

        virtual operator const char*() const;
        virtual operator int32_t() const;
        virtual operator float() const;
        virtual operator bool() const;

        virtual unsigned int GetSize() const;

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
