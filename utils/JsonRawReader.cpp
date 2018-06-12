/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "JsonRawReader.h"
#include "Configure.h"

namespace Kernel
{
    JsonRawReader::JsonRawReader(const char* data)
        : m_document(new rapidjson::Document())
        , m_json(m_document)
        , m_index(0)
        , m_index_stack()
        , m_value_stack()
    {
        m_document->Parse<0>(data);
    }

    IArchive& JsonRawReader::startClass(std::string& class_name)
    {
        return (*this) & class_name;
    }

    IArchive& JsonRawReader::endClass() { return *this; }

    IArchive& JsonRawReader::startObject() { return *this; }
    IArchive& JsonRawReader::endObject() { return *this; }

    IArchive& JsonRawReader::startArray(size_t& count)
    {
        count = size_t((*m_json)[m_index++].GetUint());
        return *this;
    }

    IArchive& JsonRawReader::endArray() { return *this; }
    IArchive& JsonRawReader::labelElement(const char*) { return *this; }

    IArchive& JsonRawReader::operator&(bool& b)
    {
        b = (*m_json)[m_index++].GetBool();
        return *this;
    }

    IArchive& JsonRawReader::operator&(unsigned char& uc)
    {
        uc = (*m_json)[m_index++].GetUint();
        return *this;
    }

    IArchive& JsonRawReader::operator&(int32_t& i32)
    {
        i32 = (*m_json)[m_index++].GetInt();
        return *this;
    }

    IArchive& JsonRawReader::operator&(int64_t& i64)
    {
        i64 = (*m_json)[m_index++].GetInt64();
        return *this;
    }

    IArchive& JsonRawReader::operator&(uint32_t& u32)
    {
        u32 = (*m_json)[m_index++].GetUint();
        return *this;
    }

    IArchive& JsonRawReader::operator&(uint64_t& u64)
    {
        u64 = (*m_json)[m_index++].GetUint64();
        return *this;
    }

    IArchive& JsonRawReader::operator&(float& f)
    {
        f = float((*m_json)[m_index++].GetDouble());
        return *this;
    }

    IArchive& JsonRawReader::operator&(double& d)
    {
        d = (*m_json)[m_index++].GetDouble();
        return *this;
    }

    IArchive& JsonRawReader::operator&(std::string& s)
    {
        s.assign((*m_json)[m_index].GetString(), (*m_json)[m_index].GetStringLength());
        ++m_index;
        return *this;
    }

    IArchive& JsonRawReader::operator&( jsonConfigurable::ConstrainedString& cs )
    {
        std::string tmp;
        tmp.assign((*m_json)[m_index].GetString(), (*m_json)[m_index].GetStringLength());
        cs = tmp;
        ++m_index;
        return *this;
    }

    bool JsonRawReader::HasError() { return m_document->HasParseError(); }

    bool JsonRawReader::IsWriter() { return false; }

    size_t JsonRawReader::GetBufferSize()
    {
        throw "JsonRawReader doesn't implement GetBufferSize().";
    }

    const char* JsonRawReader::GetBuffer()
    {
        throw "JsonRawReader doesn't implement GetBuffer().";
    }

    JsonRawReader::~JsonRawReader()
    {
        delete m_document;
        m_json = m_document = reinterpret_cast<rapidjson::Document*>(0xDEADBEEFLL);
    }
}
