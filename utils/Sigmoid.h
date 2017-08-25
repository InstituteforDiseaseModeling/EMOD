/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <math.h>
#include "Configure.h"

namespace Kernel
{
    struct IArchive;

    struct Sigmoid : public JsonConfigurable
    {
    public:
        // -------------------
        // --- Static methods
        // -------------------
        inline static double basic_sigmoid ( double threshold = 100.0, double variable = 0.0 )
        {
            return (variable > 0) ? (variable / (threshold + variable)) : 0.0;
        }

        inline static float variableWidthSigmoid( float variable, float threshold, float invwidth )
        {
            if( threshold == 0.0 )
            {
                throw OutOfRangeException( __FILE__, __LINE__, __FUNCTION__, "threshold", threshold, 0 );
            }
            if ( invwidth > 0 )
            {
                return 1.0f / ( 1.0f + exp( (threshold-variable) / (threshold/invwidth) ) );
            }
            else
            {
                throw OutOfRangeException( __FILE__, __LINE__, __FUNCTION__, "invwidth", invwidth, 0 );
            }
        }

        inline static float variableWidthAndHeightSigmoid( float variable, float center, float rate, float min_val, float max_val)
        {
            // max_val must be >= min_val, however rate can be negative.
            // A positive (negative) rate creates a sigmoid that increases (decreases) with variable
            if ( max_val - min_val >= 0 )
            {
                return min_val + (max_val-min_val) / ( 1 + exp(-rate * (variable-center)) );
            }
            else
            {
                throw ConfigurationRangeException( __FILE__, __LINE__, __FUNCTION__, "max_val - min_val", max_val - min_val, 0);
            }
        }

        // ---------------------
        // --- Instance methods
        // ---------------------
        Sigmoid();
        Sigmoid( float min, float max, float mid, float rate );
        ~Sigmoid();

        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) { return e_NOINTERFACE; }

        float variableWidthAndHeightSigmoid( float variable ) const;

        void SetMin( float min );
        void SetMax( float max );
        void SetMid( float mid );
        void SetRate( float rate );

        static void serialize( IArchive& ar, Sigmoid& obj );

    private:
        float m_Min;
        float m_Max;
        float m_Mid;
        float m_Rate;
    };
}
