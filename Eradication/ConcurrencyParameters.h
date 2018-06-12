/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "Configure.h"
#include "IConcurrency.h"

namespace Kernel
{
    ENUM_DEFINE(ExtraRelationalFlagType,
        ENUM_VALUE_SPEC(Independent , 0)
        ENUM_VALUE_SPEC(Correlated  , 1)
        ENUM_VALUE_SPEC(COUNT       , 2))

    class IDMAPI ConcurrencyByProperty : public JsonConfigurable
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()
    public:
        explicit ConcurrencyByProperty( const std::string& propertyKeyValue );
        virtual ~ConcurrencyByProperty();

        virtual bool Configure( const ::Configuration *json ) override;

        float GetProbExtraRelsMale()          const { return m_ProbExtraRelsMale;          }
        float GetProbExtraRelsFemale()        const { return m_ProbExtraRelsFemale;        }
        float GetMaxSimultaneiousRelsMale()   const { return m_MaxSimultaneiousRelsMale;   }
        float GetMaxSimultaneiousRelsFemale() const { return m_MaxSimultaneiousRelsFemale; }

    private:
#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        std::string m_PropertyValue;

        float m_ProbExtraRelsMale;
        float m_ProbExtraRelsFemale;
        float m_MaxSimultaneiousRelsMale;
        float m_MaxSimultaneiousRelsFemale;
#pragma warning( pop )
    };

    class IDMAPI ConcurrencyParameters : public JsonConfigurable
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()
    public:
        ConcurrencyParameters();
        virtual ~ConcurrencyParameters();

        void ConcurrencyParameters::Initialize( const std::string& rRelTypeName,
                                                const std::string& rConcurrencyProperty, 
                                                const ::Configuration *json );

        float GetProbExtra( const char* prop_value, Gender::Enum gender ) const;
        float GetMaxRels( const char* prop_value, Gender::Enum gender ) const;

    private:
        //virtual bool Configure( const ::Configuration *json ) override;

#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        std::map<std::string,ConcurrencyByProperty*> m_PropertyValueToConcurrencyMap;
#pragma warning( pop )
    };

    class IDMAPI ConcurrencyConfigurationByProperty : public JsonConfigurable
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()
    public:
        explicit ConcurrencyConfigurationByProperty( const std::string& propertyKeyValue );
        virtual ~ConcurrencyConfigurationByProperty();

        virtual bool Configure( const ::Configuration *json ) override;

        ExtraRelationalFlagType::Enum              GetExtraRelationshipFlag() const { return m_ExtraRelFlag; }
        const std::vector<RelationshipType::Enum>& GetRelationshipTypeOrder() const { return m_RelTypeOrder; }

    private:
#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        std::string m_PropretyValue;
        ExtraRelationalFlagType::Enum m_ExtraRelFlag;
        std::vector<RelationshipType::Enum> m_RelTypeOrder;
#pragma warning( pop )
    };

    class IDMAPI ConcurrencyConfiguration : public JsonConfigurable, public IConcurrency
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()
    public:
        ConcurrencyConfiguration();
        virtual ~ConcurrencyConfiguration();

        virtual void Initialize( const ::Configuration *json );

        // ------------------------
        // --- IConcurrency Methods
        // ------------------------
        virtual const std::string& GetPropertyKey() const override;

        virtual float GetProbSuperSpreader() const override;

        virtual bool IsConcurrencyProperty( const char* prop ) const override;

        virtual const char* GetConcurrencyPropertyValue( const tProperties* the_individuals_properties, 
                                                         const char* prop, 
                                                         const char* prop_value ) const override;

        virtual unsigned char GetProbExtraRelationalBitMask( const char* prop, 
                                                             const char* prop_value, 
                                                             Gender::Enum gender,
                                                             bool isSuperSpreader ) const override;

        virtual int GetMaxAllowableRelationships( const char* prop, 
                                                  const char* prop_value, 
                                                  Gender::Enum gender,
                                                  RelationshipType::Enum rel_type ) const override;

        // -----------------
        // --- Other Methods
        // -----------------
        void AddParameters( RelationshipType::Enum relType, ConcurrencyParameters* pCP );

    private:
#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        std::string m_PropertyKey;
        std::map<std::string,ConcurrencyConfigurationByProperty*> m_PropertyValueToConfig;
        std::map<RelationshipType::Enum,ConcurrencyParameters*> m_RelTypeToParametersMap;
        float m_ProbSuperSpreader;
#pragma warning( pop )
    };
}
