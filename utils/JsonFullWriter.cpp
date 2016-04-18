/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "JsonFullWriter.h"
#include "Configure.h"

namespace Kernel
{
    JsonFullWriter::JsonFullWriter()
        : m_buffer(new rapidjson::StringBuffer())
        , m_writer(new rapidjson::Writer<rapidjson::StringBuffer>(*m_buffer,false))
        , m_closed(false)
    {
        m_writer->StartObject();
    }

    IArchive& JsonFullWriter::startClass(std::string& class_name)
    {
        m_writer->StartObject();
        m_writer->String("__class__");
        m_writer->String(class_name.c_str());
        return *this;
    }

    IArchive& JsonFullWriter::endClass()
    {
        m_writer->EndObject();
        return *this;
    }

    IArchive& JsonFullWriter::startObject()
    {
        m_writer->StartObject();
        return *this;
    }

    IArchive& JsonFullWriter::endObject()
    {
        m_writer->EndObject();
        return *this;
    }

    IArchive& JsonFullWriter::startArray(size_t& count)
    {
        m_writer->StartArray();
        return *this;
    }

    IArchive& JsonFullWriter::endArray()
    {
        m_writer->EndArray();
        return *this;
    }

    IArchive& JsonFullWriter::labelElement(const char* label)
    {
        m_writer->String(label);
        return *this;
    }

    IArchive& JsonFullWriter::operator&(bool& b)
    {
        m_writer->Bool(b);
        return *this;
    }

    IArchive& JsonFullWriter::operator&(unsigned char& uc)
    {
        m_writer->Uint(uc);
        return *this;
    }

    IArchive& JsonFullWriter::operator&(int32_t& i32)
    {
        m_writer->Int(i32);
        return *this;
    }

    IArchive& JsonFullWriter::operator&(int64_t& i64)
    {
        m_writer->Int64(i64);
        return *this;
    }

    IArchive& JsonFullWriter::operator&(uint32_t& u32)
    {
        m_writer->Uint(u32);
        return *this;
    }

    IArchive& JsonFullWriter::operator&(uint64_t& u64)
    {
#if defined(WIN32)
        m_writer->Uint64(u64);
#endif
        return *this;
    }

    IArchive& JsonFullWriter::operator&(float& f)
    {
        m_writer->Double(double(f));
        return *this;
    }

    IArchive& JsonFullWriter::operator&(double& d)
    {
        m_writer->Double(d);
        return *this;
    }

    IArchive& JsonFullWriter::operator&(std::string& s)
    {
        m_writer->String(s.c_str(), rapidjson::SizeType(s.size()), true);
        return *this;
    }

    IArchive& JsonFullWriter::operator&( jsonConfigurable::ConstrainedString& cs )
    {
#if defined(WIN32)
        this->operator&( (std::string)cs );
#endif
        return *this;
    }

    bool JsonFullWriter::HasError() { return false; }

    bool JsonFullWriter::IsWriter() { return true; }

    uint32_t JsonFullWriter::GetBufferSize()
    {
        GetBuffer(); // RapidJson will ensure that there's a terminating null ('\0')
        return uint32_t(m_buffer->Size());
    }

    const char* JsonFullWriter::GetBuffer()
    {
        if (!m_closed)
        {
            m_writer->EndObject();
            m_closed = true;
        }

        return m_buffer->GetString();
    }

    JsonFullWriter::~JsonFullWriter()
    {
        delete m_buffer;
        delete m_writer;
    }
}
