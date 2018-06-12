/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "BinaryArchiveReader.h"
#include "Configure.h"

namespace Kernel
{
    BinaryArchiveReader::BinaryArchiveReader(const char* data, size_t size)
        : buffer(data)
        , capacity(size)
        , count(0)
        , error(false)
    {
    }

    IArchive& BinaryArchiveReader::startClass(std::string& class_name)
    {
        return (*this) & class_name;
    }

    IArchive& BinaryArchiveReader::endClass() { return *this; }

    IArchive& BinaryArchiveReader::startObject() { return *this; }
    IArchive& BinaryArchiveReader::endObject()   { return *this; }

    IArchive& BinaryArchiveReader::startArray(size_t& count)
    {
        // Revisit this if we need more than 4,294,967,296 entries in an array.
        uint32_t entries;
        pop(entries);
        count = size_t(entries);
        return *this;
    }

    IArchive& BinaryArchiveReader::endArray() { return *this; }
    IArchive& BinaryArchiveReader::labelElement(const char*) { return *this; }

    IArchive& BinaryArchiveReader::operator&(bool& b)
    {
        // Use a uint32_t so we stay on a 4-byte boundary.
        uint32_t value;
        pop(value);
        b = (value != 0);

        return *this;
    }

    IArchive& BinaryArchiveReader::operator&(unsigned char& uc)
    {
        // Use a uint32_t so we stay on a 4-byte boundary.
        uint32_t value;
        pop(value);
        uc = value;

        return *this;
    }

    IArchive& BinaryArchiveReader::operator&(int32_t& i32)
    {
        pop(i32);
        return *this;
    }

    IArchive& BinaryArchiveReader::operator&(int64_t& i64)
    {
        pop(i64);
        return *this;
    }

    IArchive& BinaryArchiveReader::operator&(uint32_t& u32)
    {
        pop(u32);
        return *this;
    }

    IArchive& BinaryArchiveReader::operator&(uint64_t& u64)
    {
        pop(u64);
        return *this;
    }

    IArchive& BinaryArchiveReader::operator&(float& f)
    {
        pop(f);
        return *this;
    }

    IArchive& BinaryArchiveReader::operator&(double& d)
    {
        pop(d);
        return *this;
    }

    IArchive& BinaryArchiveReader::operator&(std::string& s)
    {
        uint32_t length;
        pop(length);
        std::vector<char> buffer(length);
        for (auto& c : buffer)
        {
            pop(c);
        }
        // Pop chars until we're back on a 4-byte boundary.
        while ((count & 3) != 0)
        {
            char c;
            pop(c);
        }

        buffer.push_back('\0');
        s = buffer.data();

        return *this;
    }

    IArchive& BinaryArchiveReader::operator&( jsonConfigurable::ConstrainedString& cs )
    {
        std::string tmp;
        this->operator&( tmp );
        cs = tmp;
        return *this;
    }

    bool BinaryArchiveReader::HasError() { return error; }
    bool BinaryArchiveReader::IsWriter() { return false; }

    size_t BinaryArchiveReader::GetBufferSize()
    {
        throw "BinaryArchiveReader doesn't implement GetBufferSize().";
    }

    const char* BinaryArchiveReader::GetBuffer()
    {
        throw "BinaryArchiveReader doesn't implement GetBuffer().";
    }

    BinaryArchiveReader::~BinaryArchiveReader()
    {
    }
}
