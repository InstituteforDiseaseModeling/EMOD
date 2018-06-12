/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "JsonFullReader.h"
#include "Configure.h"

namespace Kernel
{
    JsonFullReader::JsonFullReader(const char* data)
        : m_document(new rapidjson::Document())
        , m_json(m_document)
        , m_index(0)
        , isObject(true)
        , label()
        , m_index_stack()
        , m_value_stack()
    {
        m_document->Parse<0>(data);
        // TODO - set isObject here based on current m_json?
    }

    IArchive& JsonFullReader::startClass(std::string& class_name)
    {
        startObject();
        auto& name = (*m_json)["__class__"];
        class_name.assign(name.GetString(), name.GetStringLength());
        return *this;
    }

    IArchive& JsonFullReader::endClass()
    {
        return endObject();
    }

    IArchive& JsonFullReader::startObject()
    {
        if (label.size() > 0)
        {
            m_value_stack.push(m_json);

            if (isObject)
            {
                m_json = &(*m_json)[label.c_str()];
            }
            else
            {
                m_json = &(*m_json)[m_index++];
                isObject = true;
            }
        }

        if( !m_json->IsObject() )
        {
            throw SerializationException( __FILE__, __LINE__, __FUNCTION__, "Expected to starting reading an object and it is not an object." );
        }

        return *this;
    }

    IArchive& JsonFullReader::endObject()
    {
        if (m_value_stack.size() > 0)
        {
            m_json = m_value_stack.top();
            m_value_stack.pop();
            isObject = (*m_json).IsObject();
        }

        return *this;
    }

    IArchive& JsonFullReader::startArray(size_t& count)
    {
        m_value_stack.push(m_json);
        if (isObject)
        {
            m_json = &(*m_json)[label.c_str()];
            isObject = false;
        }
        else
        {
            m_json = &(*m_json)[m_index++];
        }
        m_index_stack.push(m_index);
        m_index = 0;
        count = (*m_json).Size();
        return *this;
    }

    IArchive& JsonFullReader::endArray()
    {
        m_index = m_index_stack.top();
        m_index_stack.pop();
        m_json = m_value_stack.top();
        m_value_stack.pop();
        isObject = (*m_json).IsObject();
        return *this;
    }

    IArchive& JsonFullReader::labelElement(const char* key)
    {
        // TODO - consider checking to see if current m_json is an object
        label = key;
        return *this;
    }
    
    rapidjson::GenericValue<rapidjson::UTF8<>>& JsonFullReader::GetElement()
    {
        if( isObject )
        {
            release_assert( label.size() > 0 );

            if( (*m_json).HasMember( label.c_str() ) )
            {
                return (*m_json)[label.c_str()];
            }
            else
            {
                std::stringstream ss;
                ss << "The '" << label << "' element is not in this object." ;
                throw SerializationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }
        }
        else
        {
            if( (*m_json).IsArray() )
            {
                if( (*m_json).Size() > m_index )
                {
                    return (*m_json)[m_index++];
                }
                else
                {
                    std::stringstream ss;
                    ss << "Tried to get the " << m_index << " element when the array only has " << (*m_json).Size() << " elements.";
                    throw SerializationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
                }
            }
            else
            {
                throw SerializationException( __FILE__, __LINE__, __FUNCTION__, "The element is expected to be an array and is not." );
            }
        }
    }

    rapidjson::GenericValue<rapidjson::UTF8<>>& JsonFullReader::GetElementForNumber()
    {
        rapidjson::GenericValue<rapidjson::UTF8<>>& element = GetElement();
        if( !element.IsNumber() )
        {
            throw SerializationException( __FILE__, __LINE__, __FUNCTION__, "Trying to get a number value from something that is not a number." );
        }
        return element;
    }

    IArchive& JsonFullReader::operator&(bool& b)
    {
        b = GetElement().GetBool();
        return *this;
    }

    IArchive& JsonFullReader::operator&(unsigned char& uc)
    {
        uc = GetElement().GetUint();
        return *this;
    }

    IArchive& JsonFullReader::operator&(int32_t& i32)
    {
        i32 = GetElementForNumber().GetInt();
        return *this;
    }

    IArchive& JsonFullReader::operator&(int64_t& i64)
    {
        i64 = GetElementForNumber().GetInt64();
        return *this;
    }

    IArchive& JsonFullReader::operator&(uint32_t& u32)
    {
        u32 = GetElementForNumber().GetUint();
        return *this;
    }

    IArchive& JsonFullReader::operator&(uint64_t& u64)
    {
        u64 = GetElementForNumber().GetUint64();
        return *this;
    }

    IArchive& JsonFullReader::operator&(float& f)
    {
        f = float(GetElementForNumber().GetDouble());
        return *this;
    }

    IArchive& JsonFullReader::operator&(double& d)
    {
        d = GetElementForNumber().GetDouble();
        return *this;
    }

    IArchive& JsonFullReader::operator&(std::string& s)
    {
        auto& element = GetElement();
        s.assign(element.GetString(), element.GetStringLength());
        return *this;
    }

    IArchive& JsonFullReader::operator&( jsonConfigurable::ConstrainedString& cs )
    {
        auto& element = GetElement();
        std::string tmp;
        tmp.assign(element.GetString(), element.GetStringLength());
        cs = tmp;
        return *this;
    }

    bool JsonFullReader::HasError() { return m_document->HasParseError(); }

    bool JsonFullReader::IsWriter() { return false; }

    size_t JsonFullReader::GetBufferSize()
    {
        throw "JsonFullReader doesn't implement GetBufferSize().";
    }

    const char* JsonFullReader::GetBuffer()
    {
        throw "JsonFullReader doesn't implement GetBuffer().";
    }

    JsonFullReader::~JsonFullReader()
    {
        delete m_document;
        m_json = m_document = reinterpret_cast<rapidjson::Document*>(0xDEADBEEFLL);
    }
}
