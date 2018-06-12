/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "BinaryArchiveWriter.h"
#include "Configure.h"

namespace Kernel
{
    BinaryArchiveWriter::BinaryArchiveWriter()
        : capacity(1024)
        , buffer((char*)malloc(capacity))
        , count(0)
    {
    }

    IArchive& BinaryArchiveWriter::startClass(std::string& class_name)
    {
        return (*this) & class_name;
    }

    IArchive& BinaryArchiveWriter::endClass() { return *this; }

    IArchive& BinaryArchiveWriter::startObject() { return *this; }
    IArchive& BinaryArchiveWriter::endObject()   { return *this; }

    IArchive& BinaryArchiveWriter::startArray(size_t& count)
    {
        // Revisit this if we need more than 4,294,967,296 entries in an array.
        uint32_t entries = uint32_t(count);
        push(entries);
        return *this;
    }

    IArchive& BinaryArchiveWriter::endArray() { return *this; }
    IArchive& BinaryArchiveWriter::labelElement(const char*) { return *this; }

    IArchive& BinaryArchiveWriter::operator&(bool& b)
    {
        // Use a uint32_t so we stay on a 4-byte boundary.
        uint32_t value = (b ? 1 : 0);
        push(value);

        return *this;
    }

    IArchive& BinaryArchiveWriter::operator&(unsigned char& uc)
    {
        // Use a uint32_t so we stay on a 4-byte boundary.
        uint32_t value = uc;
        push(value);

        return *this;
    }

    IArchive& BinaryArchiveWriter::operator&(int32_t& i32)
    {
        push(i32);
        return *this;
    }

    IArchive& BinaryArchiveWriter::operator&(int64_t& i64)
    {
        push(i64);
        return *this;
    }

    IArchive& BinaryArchiveWriter::operator&(uint32_t& u32)
    {
        push(u32);
        return *this;
    }

    IArchive& BinaryArchiveWriter::operator&(uint64_t& u64)
    {
        push(u64);
        return *this;
    }

    IArchive& BinaryArchiveWriter::operator&(float& f)
    {
        push(f);
        return *this;
    }

    IArchive& BinaryArchiveWriter::operator&(double& d)
    {
        push(d);
        return *this;
    }

    IArchive& BinaryArchiveWriter::operator&(std::string& s)
    {
        uint32_t length = s.size();
        push(length);
        for (auto c : s)
        {
            push(c);
        }
        // Push chars until we're back on a 4-byte boundary.
        while ((count & 3) != 0)
        {
            char c = 0;
            push(c);
        }

        return *this;
    }

    IArchive& BinaryArchiveWriter::operator&( jsonConfigurable::ConstrainedString& cs )
    {
#if defined(WIN32)
        this->operator&( (std::string)cs );
#endif
        return *this;
    }

    bool BinaryArchiveWriter::HasError() { return false; }

    bool BinaryArchiveWriter::IsWriter() { return true; }

    size_t BinaryArchiveWriter::GetBufferSize()
    {
        return count;
    }

    const char* BinaryArchiveWriter::GetBuffer()
    {
        return reinterpret_cast<char*>(buffer);
    }

    BinaryArchiveWriter::~BinaryArchiveWriter()
    {
        free(buffer);
    }
}
