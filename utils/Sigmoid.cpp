/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include <stdafx.h>

#include "Sigmoid.h"
#include "IArchive.h"

SETUP_LOGGING( "Sigmoid" )

namespace Kernel 
{
    Sigmoid::Sigmoid()
        : JsonConfigurable()
        , m_Min(0.0)
        , m_Max(1.0)
        , m_Mid(0.0)
        , m_Rate(1.0)
    {
        initConfigTypeMap( "Min",  &m_Min,  Sigmoid_Min_DESC_TEXT,     0.0,     1.0,    1.0 );
        initConfigTypeMap( "Max",  &m_Max,  Sigmoid_Max_DESC_TEXT,     0.0,     1.0,    1.0 );
        initConfigTypeMap( "Mid",  &m_Mid,  Sigmoid_Mid_DESC_TEXT,     0.0, FLT_MAX, 2000.0 );
        initConfigTypeMap( "Rate", &m_Rate, Sigmoid_Rate_DESC_TEXT, -100.0,   100.0,    1.0 );
    }

    Sigmoid::Sigmoid( float min, float max, float mid, float rate )
        : JsonConfigurable()
        , m_Min(min)
        , m_Max(max)
        , m_Mid(mid)
        , m_Rate(rate)
    {
    }

    Sigmoid::~Sigmoid()
    {
    }

    float Sigmoid::variableWidthAndHeightSigmoid( float variable ) const
    {
        return variableWidthAndHeightSigmoid( variable, m_Mid, m_Rate, m_Min, m_Max );
    }

    void Sigmoid::SetMin( float min )
    {
        m_Min = min;
    }

    void Sigmoid::SetMax( float max )
    {
        m_Max = max;
    }

    void Sigmoid::SetMid( float mid )
    {
        m_Mid = mid;
    }

    void Sigmoid::SetRate( float rate )
    {
        m_Rate = rate;
    }

    void Sigmoid::serialize( Kernel::IArchive& ar, Sigmoid& obj )
    {
        ar.startObject();
            ar.labelElement("m_Min" ) & obj.m_Min;
            ar.labelElement("m_Max" ) & obj.m_Max;
            ar.labelElement("m_Mid" ) & obj.m_Mid;
            ar.labelElement("m_Rate") & obj.m_Rate;
        ar.endObject();
    }
}
