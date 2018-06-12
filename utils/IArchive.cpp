/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

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
