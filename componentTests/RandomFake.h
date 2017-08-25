/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "RANDOM.h"

class RandomFake : public RANDOMBASE
{
public:
    RandomFake()
        : RANDOMBASE(0, 8)
        , m_UL(0)
    {
    }

    void SetUL( uint32_t ul )
    {
        m_UL = ul ;
        fill_bits();
        bits_to_float();
        index = 0;
    }

protected:
    virtual void fill_bits() override { for (size_t i = 0; i < cache_count; ++i) random_bits[i] = m_UL; }

private:
    uint32_t m_UL ;
};
