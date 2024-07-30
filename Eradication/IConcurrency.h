
#pragma once

#include "SimulationEnums.h"
#include "Properties.h"
#include "IRelationship.h"

#define EXTRA_RELATIONAL_ALLOWED(rel)   (1 << (unsigned int)rel)

class Configuration;

namespace Kernel
{
    class RANDOMBASE;

    struct IDMAPI IConcurrency
    {
        virtual ~IConcurrency() {};

        virtual const std::string& GetPropertyKey() const =0;

        virtual float GetProbSuperSpreader() const = 0;

        virtual bool IsConcurrencyProperty( const char* prop ) const = 0;

        virtual const char* GetConcurrencyPropertyValue( const IPKeyValueContainer& rProperties, 
                                                         const char* prop, 
                                                         const char* prop_value ) const = 0;

        virtual unsigned char GetProbExtraRelationalBitMask( RANDOMBASE* pRNG,
                                                             const char* prop, 
                                                             const char* prop_value, 
                                                             Gender::Enum gender,
                                                             bool isSuperSpreader ) const = 0;

        virtual int GetMaxAllowableRelationships( RANDOMBASE* pRNG,
                                                  const char* prop, 
                                                  const char* prop_value, 
                                                  Gender::Enum gender,
                                                  RelationshipType::Enum rel_type ) const = 0;
    };
}
