/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/
#pragma once
#include <string>

namespace Kernel {

// TODO: data-member initialized first time based on log2(number_substrains)
const unsigned int BARCODE_BITS = 24;

class MalariaBarcode
{
    public:
        static MalariaBarcode * getInstance()
        {
            if( _instance == NULL )
            {
                _instance = new MalariaBarcode();
            }
            return _instance;
        }

        std::string toBits(uint32_t id);
        uint32_t randomBarcode();
        uint32_t fromOutcrossing(uint32_t id1, uint32_t id2);

    private:

        MalariaBarcode()
        {
        }

        static MalariaBarcode * _instance;
};
}
