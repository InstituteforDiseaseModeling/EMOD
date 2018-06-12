/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once
#include "IArchive.h"

#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

namespace Kernel
{
    class JsonRawWriter : public IArchive
    {
    public:
        explicit JsonRawWriter();
        virtual ~JsonRawWriter();

    private:

        virtual IArchive& startClass(std::string&) override;
        virtual IArchive& endClass() override;
        virtual IArchive& startObject() override;
        virtual IArchive& endObject() override;
        virtual IArchive& startArray(size_t&) override;
        virtual IArchive& endArray() override;
        virtual IArchive& labelElement(const char*) override;

        virtual IArchive& operator&(bool&) override;
        virtual IArchive& operator&(unsigned char&) override;
        virtual IArchive& operator&(int32_t&) override;
        virtual IArchive& operator&(int64_t&) override;
        virtual IArchive& operator&(uint32_t&) override;
        virtual IArchive& operator&(uint64_t&) override;
        virtual IArchive& operator&(float&) override;
        virtual IArchive& operator&(double&) override;
        virtual IArchive& operator&(std::string&) override;
        virtual IArchive& operator & (jsonConfigurable::ConstrainedString&) override;

        virtual bool HasError() override;
        virtual bool IsWriter() override;
        virtual size_t GetBufferSize() override;
        virtual const char* GetBuffer() override;

        rapidjson::StringBuffer* m_buffer;
        rapidjson::Writer<rapidjson::StringBuffer>* m_writer;
        bool m_closed;
    };
}
