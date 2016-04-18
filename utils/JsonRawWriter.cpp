/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "JsonRawWriter.h"
#include "Configure.h"

namespace Kernel
{
    JsonRawWriter::JsonRawWriter()
        : m_buffer(new rapidjson::StringBuffer())
        , m_writer(new rapidjson::Writer<rapidjson::StringBuffer>(*m_buffer))
        , m_closed(false)
    {
        m_writer->StartArray();
    }

    IArchive& JsonRawWriter::startClass(std::string& class_name)
    {
        return (*this) & class_name;
    }

    IArchive& JsonRawWriter::endClass() { return *this; }

    IArchive& JsonRawWriter::startObject() { return *this; }
    IArchive& JsonRawWriter::endObject() { return *this; }

    IArchive& JsonRawWriter::startArray(size_t& count)
    {
        m_writer->Uint(uint32_t(count));
        return *this;
    }

    IArchive& JsonRawWriter::endArray() { return *this; }
    IArchive& JsonRawWriter::labelElement(const char*) { return *this; }

    IArchive& JsonRawWriter::operator&(bool& b)
    {
        m_writer->Bool(b);
        return *this;
    }

    IArchive& JsonRawWriter::operator&(unsigned char& uc)
    {
        m_writer->Uint(uc);
        return *this;
    }

    IArchive& JsonRawWriter::operator&(int32_t& i32)
    {
        m_writer->Int(i32);
        return *this;
    }

    IArchive& JsonRawWriter::operator&(int64_t& i64)
    {
        m_writer->Int64(i64);
        return *this;
    }

    IArchive& JsonRawWriter::operator&(uint32_t& u32)
    {
        m_writer->Uint(u32);
        return *this;
    }

    IArchive& JsonRawWriter::operator&(uint64_t& u64)
    {
        m_writer->Uint64(u64);
        return *this;
    }

    IArchive& JsonRawWriter::operator&(float& f)
    {
        m_writer->Double(double(f));
        return *this;
    }

    IArchive& JsonRawWriter::operator&(double& d)
    {
        m_writer->Double(d);
        return *this;
    }

    IArchive& JsonRawWriter::operator&(std::string& s)
    {
        m_writer->String(s.c_str(), rapidjson::SizeType(s.size()), true);
        return *this;
    }

    IArchive& JsonRawWriter::operator&( jsonConfigurable::ConstrainedString& cs )
    {
#if defined(WIN32)
        this->operator&( (std::string)cs );
#endif
        return *this;
    }

    bool JsonRawWriter::HasError() { return false; }

    bool JsonRawWriter::IsWriter() { return true; }

    uint32_t JsonRawWriter::GetBufferSize()
    {
        return strlen(GetBuffer()) + 1;
    }

    const char* JsonRawWriter::GetBuffer()
    {
        if (!m_closed)
        {
            m_writer->EndArray();
            m_closed = true;
        }

        return m_buffer->GetString();
    }

    JsonRawWriter::~JsonRawWriter()
    {
        delete m_buffer;
        delete m_writer;
    }
}
