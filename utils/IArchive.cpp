
#include "stdafx.h"
#include "IArchive.h"

namespace Kernel
{
    IArchive& IArchive::operator&(ISerializable*& obj)
    {
        ISerializable::serialize(*this, obj);

        return *this;
    }

    IArchive& IArchive::operator & (std::vector<Kernel::suids::suid>& vec)
    {
        size_t count = this->IsWriter() ? vec.size() : -1;

        this->startArray(count);
        if (this->IsWriter())
        {
            for (auto& entry : vec)
            {
                *this & entry;
            }
        }
        else
        {
            vec.resize(count);
            for (size_t i = 0; i < count; ++i)
            {
                *this & vec[i];
            }
        }
        this->endArray();

        return *this;
    }
}
