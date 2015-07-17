/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include "Configure.h"
#include "IdmApi.h"

namespace Kernel
{

    class IDMAPI InterpolatedValueMap : public JsonConfigurable, public JsonConfigurable::tFloatFloatMapConfigType /* really just a map */
    {
        friend class ::boost::serialization::access;
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) { return e_NOINTERFACE; }
        public:
            InterpolatedValueMap() {}
            float getValuePiecewiseConstant( float year, float default_value = 0) const;
            float getValueLinearInterpolation( float year, float default_value = 0) const;
            virtual void ConfigureFromJsonAndKey( const Configuration* inputJson, const std::string& key );
            virtual json::QuickBuilder GetSchema();
    };
}
