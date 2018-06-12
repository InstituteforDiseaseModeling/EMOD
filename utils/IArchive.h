/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once
#include <string>
#include <vector>
#include <map>
#include <set>
#include "suids.hpp"
#include "IdmApi.h"

#include "ISerializable.h"

namespace Kernel
{

    namespace jsonConfigurable
    {
        class ConstrainedString;
    }

    struct IDMAPI IArchive
    {
        virtual ~IArchive() {}

        virtual IArchive& startClass(std::string&) = 0;
        virtual IArchive& endClass() = 0;

        virtual IArchive& startObject() = 0;
        virtual IArchive& endObject() = 0;

        virtual IArchive& startArray(size_t&) = 0;
        virtual IArchive& endArray() = 0;

        virtual IArchive& labelElement(const char*) = 0;

        virtual IArchive& operator & (bool&) = 0;
        virtual IArchive& operator & (unsigned char&) = 0;
        virtual IArchive& operator & (int32_t&) = 0;
        virtual IArchive& operator & (int64_t&) = 0;
        virtual IArchive& operator & (uint32_t&) = 0;
        virtual IArchive& operator & (uint64_t&) = 0;
        virtual IArchive& operator & (float&) = 0;
        virtual IArchive& operator & (double&) = 0;
        virtual IArchive& operator & (std::string&) = 0;
        virtual IArchive& operator & (jsonConfigurable::ConstrainedString&) = 0;

        IArchive& operator & (ISerializable*&);

        template<typename I>
        void operator & (I*& ptr)
        {
            if (this->IsWriter())
            {
                ISerializable* serializable = dynamic_cast<ISerializable*>(ptr);
                (*this) & serializable;
            }
            else
            {
                ISerializable* serializable;
                (*this) & serializable;
                ptr = dynamic_cast<I*>(serializable);
            }
        }

        template<typename T>
        void operator & (T& obj)
        {
            T::serialize(*this, obj);
        }

        template <typename I>
        void operator & (std::list<I*>& list)
        {
            size_t count = this->IsWriter() ? list.size() : -1;

            this->startArray(count);
            if (this->IsWriter())
            {
                for (auto& entry : list)
                {
                    ISerializable* serializable = dynamic_cast<ISerializable*>(entry);
                    (*this) & serializable;
                }
            }
            else
            {
                for (size_t i = 0; i < count; ++i)
                {
                    ISerializable* serializable;
                    (*this) & serializable;
                    list.push_back(dynamic_cast<I*>(serializable));
                }
            }
            this->endArray();
        }

        template <typename T>
        void operator & (std::list<T>& list)
        {
            size_t count = this->IsWriter() ? list.size() : -1;

            this->startArray(count);
            if (this->IsWriter())
            {
                for (auto& entry : list)
                {
                    (*this) & entry;
                }
            }
            else
            {
                for (size_t i = 0; i < count; ++i)
                {
                    T entry;
                    (*this) & entry;
                    list.push_back(entry);
                }
            }
            this->endArray();
        }

        template <typename T>
        void operator & (std::set<T>& st)
        {
            size_t count = this->IsWriter() ? st.size() : -1;

            this->startArray(count);
            if (this->IsWriter())
            {
                for (auto& entry : st)
                {
                    (*this) & entry;
                }
            }
            else
            {
                for (size_t i = 0; i < count; ++i)
                {
                    T entry;
                    (*this) & entry;
                    st.insert( entry );
                }
            }
            this->endArray();
        }

        template <typename T>
        void operator & (std::vector<T>& vec)
        {
            size_t count = this->IsWriter() ? vec.size() : -1;

            this->startArray(count);
            if (!this->IsWriter()) {
                vec.resize(count);
            }
            for (auto& entry : vec)
            {
                (*this) & entry;
            }
            this->endArray();
        }

        template <typename T, typename U>
        IArchive& operator & (std::map<T, U>& mapping)
        {
            size_t count = this->IsWriter() ? mapping.size() : -1;

            this->startArray(count);
            if (this->IsWriter())
            {
                for (auto& entry : mapping)
                {
                    T key   = entry.first;
                    U value = entry.second;
                    startObject();
                        labelElement("key"  ) & key;
                        labelElement("value") & value;
                    endObject();
                }
            }
            else
            {
                for (size_t i = 0; i < count; ++i)
                {
                    T key;
                    U value;
                    startObject();
                        labelElement("key"  ) & key;
                        labelElement("value") & value;
                    endObject();
                    mapping[key] = value;
                }
            }
            this->endArray();

            return *this;
        }

        // IDM specific types
        /* virtual */ IArchive& operator & (std::vector<Kernel::suids::suid>&);

        template <typename A>
        void serialize(A array[], size_t count)
        {
            startArray(count);
            for (size_t i = 0; i < count; ++i)
            {
                (*this) & array[i];
            }
            endArray();
        }

        virtual bool HasError() = 0;
        virtual bool IsWriter() = 0;
        bool IsReader() { return !IsWriter(); }
        virtual size_t GetBufferSize() = 0;
        virtual const char* GetBuffer() = 0;
    };
}