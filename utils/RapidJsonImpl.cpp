/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/
#include <stdafx.h>
#include <cstdio>
#include <string>
#include <vector>
#include <assert.h>

#include <sstream>

#include "Log.h"
#include "RapidJsonImpl.h"

#pragma warning(disable : 4996)

SETUP_LOGGING( "RapidJsonObj" )

namespace Kernel {

    RapidJsonObj::RapidJsonObj()
        : m_buffer(nullptr)
        , m_writer(nullptr)
        , m_writerQueue()
        , m_bufferQueue()
        , m_document(nullptr)
        , m_bCacheWriter(false)
    {
    }

    RapidJsonObj::~RapidJsonObj()
    {
        for (auto buffer : m_bufferQueue)
        {
            delete buffer;
        }

        for (auto writer : m_writerQueue)
        {
            delete writer;
        }
    }

    void RapidJsonObj::CreateNewWriter(bool bCacheWriter)
    {
        m_buffer = _new_ StringBuffer();
        m_writer = _new_ Writer<StringBuffer>(*m_buffer);

        m_bCacheWriter = bCacheWriter;
        if (m_bCacheWriter)
        {
            m_bufferQueue.push_back(m_buffer);
            m_writerQueue.push_back(m_writer);
        }
    }

    void RapidJsonObj::FinishWriter()
    {
        if (!m_bCacheWriter)
        {
            if (m_buffer) delete m_buffer;
            if (m_writer) delete m_writer;
        }
    }

    void RapidJsonObj::BeginObject() { m_writer->StartObject(); }
    void RapidJsonObj::EndObject()   { m_writer->EndObject(); }
    void RapidJsonObj::BeginArray()  { m_writer->StartArray(); }
    void RapidJsonObj::EndArray()    { m_writer->EndArray(); }

    const char* RapidJsonObj::ToString() const
    {
        return m_buffer->GetString();
    }

    struct OutputStringStream : public std::ostringstream 
    {
        typedef char Ch;

        void Put(char c) 
        {
            put(c);
        }
    };

    char* RapidJsonObj::ToPrettyString() const
    {
        char* prettyString = nullptr;
        Document doc;
        if (doc.Parse<0>(m_buffer->GetString()).HasParseError())
        {
            const char* err = doc.GetParseError();
            LOG_ERR_F("Parse json error: %s\n", err);
            return prettyString;
        }

        OutputStringStream os;
        PrettyWriter<OutputStringStream> writer(os);
        doc.Accept(writer);

        std::size_t len = os.str().size() + 1;
        prettyString = static_cast<char*>(malloc( len ));
        if( prettyString != nullptr )
        {
            strcpy(prettyString, os.str().c_str());
        }

        //LOG_INFO_F("string length=%d and string=%s\n", len, prettyString);
        return prettyString;
    }

    // This is for UTF-8 only (not for wchar or UNICODE)
    void RapidJsonObj::Parse(const char* jsBuffer)
    {
        if (m_document)
        {
            delete m_document;
        }
        m_document = new Document();
        m_document->Parse<0>(jsBuffer);
    }

    void RapidJsonObj::DeleteUnderlyingJsonObjs()
    {
        if (m_document)
        {
            delete m_document;
        }
    }

    void RapidJsonObj::Insert( const char* key)
    {
        m_writer->String(key);
    }

    void RapidJsonObj::Insert( const char* key, const char* val )
    {
        m_writer->String(key);
        m_writer->String(val);
    }
        
    void RapidJsonObj::Insert( const char* key, const int32_t val )
    {
        m_writer->String(key);
        m_writer->Int(val);
    }

    void RapidJsonObj::Insert( const char* key, const uint32_t val )
    {
        m_writer->String(key);
        m_writer->Uint(val);
    }

    void RapidJsonObj::Insert( const char* key, const int64_t val )
    {
        m_writer->String(key);
        m_writer->Int64(val);
    }

    void RapidJsonObj::Insert( const char* key, const uint64_t val )
    {
        m_writer->String(key);
        m_writer->Uint64(val);
    }

    void RapidJsonObj::Insert( const char* key, const float val )
    {
        m_writer->String(key);
        m_writer->Double(val);
    }

    void RapidJsonObj::Insert( const char* key, const double val )
    {
        m_writer->String(key);
        m_writer->Double(val);
    }

    void RapidJsonObj::Insert( const char* key, const bool val )
    {
        m_writer->String(key);
        m_writer->Bool(val);
    }

    void RapidJsonObj::Insert( const char* key, const IJsonObjectAdapter* val )
    {
        m_writer->String(key);
        m_writer->String(val->ToString());
    }

    void RapidJsonObj::Add( const char* val )    { m_writer->String(val); }
    void RapidJsonObj::Add( const int32_t val )  { m_writer->Int(val); }
    void RapidJsonObj::Add( const uint32_t val ) { m_writer->Uint(val); }
    void RapidJsonObj::Add( const float val )    { m_writer->Double(val); }
    void RapidJsonObj::Add( const double val )   { m_writer->Double(val); }
    void RapidJsonObj::Add( const int64_t val )  { m_writer->Int64(val); }
    void RapidJsonObj::Add( const uint64_t val ) { m_writer->Uint64(val); }
    void RapidJsonObj::Add( const bool val )     { m_writer->Bool(val); }
    void RapidJsonObj::Add( const IJsonObjectAdapter* val ) { m_writer->String(val->ToString()); }

    bool RapidJsonObj::Contains(const char* key) const { return m_document->HasMember(key); }

    IJsonObjectAdapter* RapidJsonObj::operator[](const char* key) const { return GetJsonObject(key); }

    IJsonObjectAdapter* RapidJsonObj::GetJsonObject(const char* key) const
    {
        RapidJsonObj* rjObj = (RapidJsonObj*) CreateJsonObjAdapter();
        if (!rjObj)
        {
            LOG_ERR_F("Failed to clone IJsonObjectAdapter object for key=%s\n", key);
            return nullptr;
        }

        assert(m_document);
        Document& document = *m_document;

        // Make sure it is JSON object
        assert(document[key].IsObject());

        rjObj->m_document = (Document*) &document[key];

        return rjObj;
    }

    IJsonObjectAdapter* RapidJsonObj::GetJsonArray(const char* key) const
    {
        RapidJsonObj* rjObj = (RapidJsonObj*) CreateJsonObjAdapter();
        if (!rjObj)
        {
            LOG_ERR_F("Failed to clone IJsonObjectAdapter object for key=%s\n", key);
            return nullptr;
        }

        assert(m_document);
        Document& document = *m_document;

        // Make sure it is JSON object
        assert(document[key].IsArray());

        rjObj->m_document = (Document*) &document[key];

        return rjObj;
    }

    const char* RapidJsonObj::GetString(const char* key) const
    {
        assert(m_document);
        Document& document = *m_document;
        assert(document[key].IsString());
        return document[key].GetString();
    }

    int32_t RapidJsonObj::GetInt(const char* key) const
    {
        assert(m_document);
        Document& document = *m_document;
        assert(document[key].IsNumber());
        return document[key].GetInt();
    }

    uint32_t RapidJsonObj::GetUint(const char* key) const
    {
        assert(m_document);
        Document& document = *m_document;
        assert(document[key].IsNumber());
        return document[key].GetUint();
    }

    int64_t RapidJsonObj::GetInt64(const char* key) const
    {
        assert(m_document);
        Document& document = *m_document;
        assert(document[key].IsNumber());
        return document[key].GetInt64();
    }

    uint64_t RapidJsonObj::GetUInt64(const char* key) const
    {
        assert(m_document);
        Document& document = *m_document;
        assert(document[key].IsNumber());
        return document[key].GetUint64();
    }

    float RapidJsonObj::GetFloat(const char* key) const
    {
        assert(m_document);
        Document& document = *m_document;
        assert(document[key].IsNumber());
        return document[key].GetDouble();
    }

    double RapidJsonObj::GetDouble(const char* key) const
    {
        assert(m_document);
        Document& document = *m_document;
        assert(document[key].IsNumber());
        return document[key].GetDouble();
    }

    bool RapidJsonObj::GetBool(const char* key) const
    {
        assert(m_document);
        Document& document = *m_document;
        assert(document[key].IsBool());
        return document[key].GetBool();
    }

    IJsonObjectAdapter* RapidJsonObj::operator[](IndexType index) const
    {
        RapidJsonObj* rjObj = (RapidJsonObj*) CreateJsonObjAdapter();
        if (!rjObj)
        {
            LOG_ERR_F("Failed to clone IJsonObjectAdapter object for index=%d\n", index);
            return nullptr;
        }

        assert(m_document);
        Document& document = *m_document;
        assert(document.IsArray());
        rjObj->m_document = (Document*) &document[SizeType(index)];

        return rjObj;
    }

    const char* RapidJsonObj::AsString() const { assert(m_document->IsNumber()); return m_document->GetString(); }
    int32_t     RapidJsonObj::AsInt()    const { assert(m_document->IsNumber()); return m_document->GetInt(); }
    uint32_t    RapidJsonObj::AsUint()   const { assert(m_document->IsNumber()); return m_document->GetUint(); }
    int64_t     RapidJsonObj::AsInt64()  const { assert(m_document->IsNumber()); return m_document->GetInt64(); }
    uint64_t    RapidJsonObj::AsUint64() const { assert(m_document->IsNumber()); return m_document->GetUint64(); }
    float       RapidJsonObj::AsFloat()  const { assert(m_document->IsNumber()); return float(m_document->GetDouble()); }
    double      RapidJsonObj::AsDouble() const { assert(m_document->IsNumber()); return m_document->GetDouble(); }
    bool        RapidJsonObj::AsBool()   const { assert(m_document->IsNumber()); return m_document->GetBool(); }

    RapidJsonObj::operator const char*() const { return AsString(); }
    RapidJsonObj::operator int32_t()     const { return AsInt(); }
    RapidJsonObj::operator float()       const { return AsFloat(); }
    RapidJsonObj::operator  bool()       const { return AsBool(); }

    unsigned int RapidJsonObj::GetSize() const
    {
        assert(m_document->IsArray());
        return m_document->Size();
    }
}