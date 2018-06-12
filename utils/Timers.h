/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once
#include "IArchive.h"
#include "ISerializable.h"
#include "Types.h"
#include <functional>

namespace Kernel { 

    class CountdownTimer : public NonNegativeFloat, public ISerializable
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject);
        public:
            CountdownTimer();
            CountdownTimer( float initValue );
            void Decrement( float dt ); 
            CountdownTimer& operator=( float val );
            bool IsDead() const { return dead; } 

            std::function< void(float) > handle;
            static void serialize(IArchive& ar, CountdownTimer & ct);
        protected:
            bool expired() const;
        private:
            bool dead;
    };
}

