/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include <stdafx.h>
#include "Timers.h"

namespace Kernel { 

    BEGIN_QUERY_INTERFACE_BODY(CountdownTimer) 
    END_QUERY_INTERFACE_BODY(CountdownTimer)

    CountdownTimer::CountdownTimer()
    : NonNegativeFloat( 0 ) // need diff base class (RangedFloat?) if we want to init with -1
    , dead( false )
    {
    }

    CountdownTimer::CountdownTimer( float initValue )
    : NonNegativeFloat( initValue )
    , dead( false )
    {
    }

    CountdownTimer& CountdownTimer::operator=( float val )
    {
        // ----------------------------------------------------------------
        // --- We override this operator to ensure that a copy constructor
        // --- isn't called and wipe out the "handle" / callback
        // ----------------------------------------------------------------
        NonNegativeFloat::operator=( val );
        return *this;
    }
            
    void CountdownTimer::Decrement( float dt )
    {
        if( expired() )
        {
            if( !dead ) // only handle once
            {
                handle( dt );
            }
            dead = true;
        }
        else
        {
            _value -= dt;
        }

    }

    bool CountdownTimer::expired() const
    {
        return( _value <= 0 );
    }

    //REGISTER_SERIALIZABLE(CountdownTimer);
    void CountdownTimer::serialize(IArchive& ar, CountdownTimer& obj)
    {
        NonNegativeFloat::serialize( ar, obj );
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // !!! Owner of this class is responsible for updating "handle"
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    }
}

