
#pragma once
#include "IArchive.h"

namespace Kernel
{
    class IDMAPI BinaryArchiveWriter : public IArchive
    {
    public:
        explicit BinaryArchiveWriter();
        virtual ~BinaryArchiveWriter();

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
        virtual IArchive& operator&(jsonConfigurable::ConstrainedString&) override;

        virtual bool HasError() override;
        virtual bool IsWriter() override;
        virtual size_t GetBufferSize() override;
        virtual const char* GetBuffer() override;

        size_t capacity;
        char* buffer;
        size_t count;

        template<typename T>
        void push(T& element)
        {
            if ( (count + sizeof(T)) > capacity )
            {
                buffer = (char*)realloc( buffer, capacity *= 2 );
            }

            *reinterpret_cast<T*>(buffer + count) = element;
            count += sizeof(T);
        }
    };
}