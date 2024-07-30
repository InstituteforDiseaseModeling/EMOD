
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


        inline static float sigmoid(float x)
        {
            return 0.5f + 0.5f * tanh(x / 2.0f); // instead of 1/(1+exp(x)), prevents exp() from overflowing
        }


        inline static float variableWidthSigmoid( float variable, float threshold, float invwidth )
        {
            if( threshold == 0.0 )
            {
                throw OutOfRangeException( __FILE__, __LINE__, __FUNCTION__, "threshold", threshold, 0 );
            }
            if ( invwidth > 0 )
            {
                return sigmoid(-( threshold - variable ) / ( threshold / invwidth ));
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
                return min_val + ( max_val - min_val ) * sigmoid(rate* ( variable - center ));
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

        virtual bool Configure( const Configuration* config ) override;

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
