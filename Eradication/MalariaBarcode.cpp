/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "MalariaBarcode.h"

#include <bitset>

#include "Environment.h"
#include "Log.h"
#include "RANDOM.h"

static const char * _module = "MalariaBarcode";

namespace Kernel 
{
    MalariaBarcode * MalariaBarcode::_instance = nullptr;

    std::string MalariaBarcode::toBits(uint32_t id)
    {
        std::bitset<BARCODE_BITS> bits(id);
        return bits.to_string();
    }

    uint32_t MalariaBarcode::randomBarcode()
    {
        auto rng = Environment::getInstance()->RNG;
        return rng->ul() & ((1 << BARCODE_BITS)-1);
    }

    uint32_t MalariaBarcode::fromOutcrossing(uint32_t id1, uint32_t id2)
    {
        uint32_t mask = randomBarcode();
        uint32_t child_strain =  (id1 & mask) | (id2 & ~mask);
        LOG_DEBUG_F("%d + %d with mask=%d --> %d\n", id1, id2, mask, child_strain);
        return child_strain;
    }
}
