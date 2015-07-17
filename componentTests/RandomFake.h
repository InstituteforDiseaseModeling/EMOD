/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include "Random.h"

class RandomFake : public RANDOMBASE
{
public:
    RandomFake()
        : RANDOMBASE(0)
        , m_UL(0)
    {
    }

    virtual __ULONG ul() { return m_UL; };   // Returns a random 32 bit number.

    void SetUL( unsigned int ul )
    {
        m_UL = ul ;
    }

private:
    __ULONG m_UL ;
};
