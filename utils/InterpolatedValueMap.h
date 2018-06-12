/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "Configure.h"
#include "IdmApi.h"

namespace Kernel
{
#pragma warning(push)
#pragma warning(disable: 4251)
    class IDMAPI InterpolatedValueMap : public JsonConfigurable,
                                        public IComplexJsonConfigurable, 
                                        public JsonConfigurable::tFloatFloatMapConfigType /* really just a map */
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) { return e_NOINTERFACE; }
        public:
            InterpolatedValueMap( float min_time = 0.0f,
                                  float max_time = 999999.0f,
                                  float min_value = 0.0f,
                                  float max_value = FLT_MAX );

            float getValuePiecewiseConstant( float year, float default_value = 0) const;
            float getValueLinearInterpolation( float year, float default_value = 0) const;
            bool isAtEnd( float currentYear ) const;
            virtual void ConfigureFromJsonAndKey( const Configuration* inputJson, const std::string& key ) override;
            virtual json::QuickBuilder GetSchema() override;
            virtual bool  HasValidDefault() const override { return false; }

            static void serialize( IArchive& ar, InterpolatedValueMap& map );
        private:
            float m_MinTime;
            float m_MaxTime;
            float m_MinValue;
            float m_MaxValue;
    };
#pragma warning( pop )
}
