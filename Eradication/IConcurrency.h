/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "SimulationEnums.h"
#include "Properties.h"
#include "IRelationship.h"

#define EXTRA_RELATIONAL_ALLOWED(rel)   (1 << (unsigned int)rel)

class Configuration;

namespace Kernel
{
    struct IDMAPI IConcurrency
    {
        virtual ~IConcurrency() {};

        virtual const std::string& GetPropertyKey() const =0;

        virtual float GetProbSuperSpreader() const = 0;

        virtual bool IsConcurrencyProperty( const char* prop ) const = 0;

        virtual const char* GetConcurrencyPropertyValue( const tProperties* the_individuals_properties, 
                                                         const char* prop, 
                                                         const char* prop_value ) const = 0;

        virtual unsigned char GetProbExtraRelationalBitMask( const char* prop, 
                                                             const char* prop_value, 
                                                             Gender::Enum gender,
                                                             bool isSuperSpreader ) const = 0;

        virtual int GetMaxAllowableRelationships( const char* prop, 
                                                  const char* prop_value, 
                                                  Gender::Enum gender,
                                                  RelationshipType::Enum rel_type ) const = 0;
    };
}
