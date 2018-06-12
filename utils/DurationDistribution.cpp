/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "DurationDistribution.h"
#include "IArchive.h"


SETUP_LOGGING( "DurationDistribution" )

namespace Kernel
{
    DurationDistribution::DurationDistribution( DistributionFunction::Enum defaultType )
    : m_Type(defaultType)
    , m_Param1(0.0f)
    , m_Param2(0.0f)
    , m_Param3(0.0f)
    , m_TypeName()
    , m_TypeDesc()
    , m_SupportedParameterInfoMap()
    {
    }

    DurationDistribution::DurationDistribution( const DurationDistribution& master )
    : m_Type( master.m_Type )
    , m_Param1( master.m_Param1 )
    , m_Param2( master.m_Param2 )
    , m_Param3( master.m_Param3 )
    , m_TypeName() // don't copy - only used during configure
    , m_TypeDesc() // don't copy - only used during configure
    , m_SupportedParameterInfoMap() // don't copy - only used during configure
    {
    }

    DurationDistribution::~DurationDistribution()
    {
    }

    void DurationDistribution::SetParameters(DistributionFunction::Enum distributionFunction, float param1, float param2)
    {
        m_Type = distributionFunction;
        m_Param1 = param1;
        m_Param2 = param2;
    }

    bool DurationDistribution::IncludeParameters( DistributionFunction::Enum typeToInclude )
    {
        bool include = (m_SupportedParameterInfoMap.count( typeToInclude ) > 0) && ((m_Type == typeToInclude) || JsonConfigurable::_dryrun);
        return include;
    }

    void DurationDistribution::SetTypeNameDesc( const char* pTypeName, const char* pTypeDesc )
    {
        release_assert( pTypeName != nullptr );
        release_assert( pTypeDesc != nullptr );

        m_TypeName = pTypeName;
        m_TypeDesc = pTypeDesc;

        release_assert( !m_TypeName.empty() );
        release_assert( !m_TypeDesc.empty() );
    }

    void DurationDistribution::AddSupportedType( DistributionFunction::Enum supportedType,
                                                 const char* pParamName_1, const char* pParamDesc_1,
                                                 const char* pParamName_2, const char* pParamDesc_2,
                                                 const char* pParamName_3, const char* pParamDesc_3 )
    {
        release_assert( pParamName_1 != nullptr );
        release_assert( pParamDesc_1 != nullptr );
        release_assert( pParamName_2 != nullptr );
        release_assert( pParamDesc_2 != nullptr );

        if( supportedType == DistributionFunction::NOT_INITIALIZED )
        {
            std::stringstream ss;
            ss << "supportedType='" << DistributionFunction::pairs::lookup_key( m_Type ) << "' is invalid.";
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }

        ParameterInfo info;
        info.m_ParamName_1 = pParamName_1 ;
        info.m_ParamDesc_1 = pParamDesc_1 ;
        info.m_ParamName_2 = pParamName_2 ;
        info.m_ParamDesc_2 = pParamDesc_2 ;
        if( (pParamName_3 != nullptr) && (pParamDesc_3 != nullptr) )
        {
            info.m_ParamName_3 = pParamName_3;
            info.m_ParamDesc_3 = pParamDesc_3;
        }
        m_SupportedParameterInfoMap.insert( std::make_pair( supportedType, info ) );
    }

    void DurationDistribution::Configure( JsonConfigurable* pParent, const Configuration * inputJson )
    {
        release_assert( !m_TypeName.empty() );
        release_assert( !m_TypeDesc.empty() );

        pParent->initConfig( m_TypeName.c_str(), m_Type, inputJson, MetadataDescriptor::Enum(m_TypeName.c_str(), m_TypeDesc.c_str(), MDD_ENUM_ARGS(DistributionFunction)) );

        if( IncludeParameters( DistributionFunction::FIXED_DURATION ) )
        {
            ParameterInfo& info = m_SupportedParameterInfoMap[ DistributionFunction::FIXED_DURATION ];
            pParent->initConfigTypeMap( info.m_ParamName_1.c_str(), &m_Param1, info.m_ParamDesc_1.c_str(), 0.0f, FLT_MAX, 6.0f, m_TypeName.c_str(), "FIXED_DURATION" );
        }

        if( IncludeParameters( DistributionFunction::UNIFORM_DURATION ) )
        {
            ParameterInfo& info = m_SupportedParameterInfoMap[ DistributionFunction::UNIFORM_DURATION ];
            pParent->initConfigTypeMap( info.m_ParamName_1.c_str(), &m_Param1, info.m_ParamDesc_1.c_str(), 0.0f, FLT_MAX, 0.0f, m_TypeName.c_str(), "UNIFORM_DURATION" ); // min
            pParent->initConfigTypeMap( info.m_ParamName_2.c_str(), &m_Param2, info.m_ParamDesc_2.c_str(), 0.0f, FLT_MAX, 1.0f, m_TypeName.c_str(), "UNIFORM_DURATION" ); // max
        }

        if( IncludeParameters( DistributionFunction::EXPONENTIAL_DURATION ) )
        {
            ParameterInfo& info = m_SupportedParameterInfoMap[ DistributionFunction::EXPONENTIAL_DURATION ];
            pParent->initConfigTypeMap( info.m_ParamName_1.c_str(), &m_Param1, info.m_ParamDesc_1.c_str(), 0.0f, FLT_MAX, 6.0f, m_TypeName.c_str(), "EXPONENTIAL_DURATION" ); // decay length
        }

        if( IncludeParameters( DistributionFunction::GAUSSIAN_DURATION ) )
        {
            ParameterInfo& info = m_SupportedParameterInfoMap[ DistributionFunction::GAUSSIAN_DURATION ];
            pParent->initConfigTypeMap( info.m_ParamName_1.c_str(), &m_Param1, info.m_ParamDesc_1.c_str(), 0.0f, FLT_MAX, 6.0f, m_TypeName.c_str(), "GAUSSIAN_DURATION" ); // mean
            pParent->initConfigTypeMap( info.m_ParamName_2.c_str(), &m_Param2, info.m_ParamDesc_2.c_str(), 0.0f, FLT_MAX, 1.0f, m_TypeName.c_str(), "GAUSSIAN_DURATION" ); // std_dev
        }

        if( IncludeParameters( DistributionFunction::POISSON_DURATION ) )
        {
            ParameterInfo& info = m_SupportedParameterInfoMap[ DistributionFunction::POISSON_DURATION ];
            pParent->initConfigTypeMap( info.m_ParamName_1.c_str(), &m_Param1, info.m_ParamDesc_1.c_str(), 0.0f, FLT_MAX, 6.0f, m_TypeName.c_str(), "POISSON_DURATION" ); // mean
        }

        if( IncludeParameters( DistributionFunction::LOG_NORMAL_DURATION ) )
        {
            ParameterInfo& info = m_SupportedParameterInfoMap[ DistributionFunction::LOG_NORMAL_DURATION ];
            pParent->initConfigTypeMap( info.m_ParamName_1.c_str(), &m_Param1, info.m_ParamDesc_1.c_str(), 0.0f, FLT_MAX, 6.0f, m_TypeName.c_str(), "LOG_NORMAL_DURATION" ); // log-mean
            pParent->initConfigTypeMap( info.m_ParamName_2.c_str(), &m_Param2, info.m_ParamDesc_2.c_str(), 0.0f, FLT_MAX, 1.0f, m_TypeName.c_str(), "LOG_NORMAL_DURATION" ); // log-width
        }

        if( IncludeParameters( DistributionFunction::BIMODAL_DURATION ) )
        {
            ParameterInfo& info = m_SupportedParameterInfoMap[ DistributionFunction::BIMODAL_DURATION ];
            pParent->initConfigTypeMap( info.m_ParamName_1.c_str(), &m_Param1, info.m_ParamDesc_1.c_str(), 0.0f,    1.0f, 1.0f, m_TypeName.c_str(), "BIMODAL_DURATION" ); // probability
            pParent->initConfigTypeMap( info.m_ParamName_2.c_str(), &m_Param2, info.m_ParamDesc_2.c_str(), 0.0f, FLT_MAX, 1.0f, m_TypeName.c_str(), "BIMODAL_DURATION" ); // value
        }

        if( IncludeParameters( DistributionFunction::WEIBULL_DURATION ) )
        {
            ParameterInfo& info = m_SupportedParameterInfoMap[ DistributionFunction::WEIBULL_DURATION ];
            pParent->initConfigTypeMap( info.m_ParamName_1.c_str(), &m_Param1, info.m_ParamDesc_1.c_str(), 0.0f, FLT_MAX, 16.0f, m_TypeName.c_str(), "WEIBULL_DURATION" ); // scale (lambda)
            pParent->initConfigTypeMap( info.m_ParamName_2.c_str(), &m_Param2, info.m_ParamDesc_2.c_str(), 0.0f, FLT_MAX, 20.0f, m_TypeName.c_str(), "WEIBULL_DURATION" ); // shape (kappa)
        }

        if( IncludeParameters( DistributionFunction::DUAL_TIMESCALE_DURATION ) )
        {
            ParameterInfo& info = m_SupportedParameterInfoMap[ DistributionFunction::DUAL_TIMESCALE_DURATION ];
            pParent->initConfigTypeMap( info.m_ParamName_1.c_str(), &m_Param1, info.m_ParamDesc_1.c_str(), 0.0f, FLT_MAX, 6.0f, m_TypeName.c_str(), "DUAL_TIMESCALE_DURATION" ); // decay length time scale #1
            pParent->initConfigTypeMap( info.m_ParamName_2.c_str(), &m_Param2, info.m_ParamDesc_2.c_str(), 0.0f, FLT_MAX, 6.0f, m_TypeName.c_str(), "DUAL_TIMESCALE_DURATION" ); // decay length time scale #2
            pParent->initConfigTypeMap( info.m_ParamName_3.c_str(), &m_Param3, info.m_ParamDesc_3.c_str(), 0.0f,    1.0f, 0.5f, m_TypeName.c_str(), "DUAL_TIMESCALE_DURATION" ); // percentage of distribution that is time scale #1
        }

        if( !JsonConfigurable::_dryrun && (m_SupportedParameterInfoMap.count( m_Type ) == 0) )
        {
            std::stringstream ss;
            ss << "DistributionFuction type='" << DistributionFunction::pairs::lookup_key( m_Type ) << "' is not supported.  Supported values are: ";
            for( auto& entry : m_SupportedParameterInfoMap )
            {
                ss << "'" << DistributionFunction::pairs::lookup_key( entry.first ) << "', ";
            }
            std::string msg = ss.str();
            msg = msg.substr( 0, msg.length()-2 );
            throw NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, msg.c_str() );
        }
    }

    void DurationDistribution::CheckConfiguration()
    {
        if( !JsonConfigurable::_dryrun )
        {
            ParameterInfo& info = m_SupportedParameterInfoMap[ m_Type ];
            const char* type_str = DistributionFunction::pairs::lookup_key( m_Type );

            if( (m_Type == DistributionFunction::UNIFORM_DURATION) && (m_Param1 >= m_Param2) )
            {
                std::stringstream ss1, ss2;
                ss1 << info.m_ParamName_1 << " >= " << info.m_ParamName_2 ;
                ss2 << m_Param1 << " >= " << m_Param2;
                throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, m_TypeName.c_str(), type_str, ss1.str().c_str(), ss2.str().c_str() );
            }
            else if( (m_Type == DistributionFunction::GAUSSIAN_DURATION) && (m_Param2 <= 0.0) )
            {
                throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, m_TypeName.c_str(), type_str, info.m_ParamName_2.c_str(), "0" );
            }
            else if( (m_Type == DistributionFunction::EXPONENTIAL_DURATION) && (m_Param1 <= 0.0) )
            {
                throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, m_TypeName.c_str(), type_str, info.m_ParamName_1.c_str(), "0" );
            }
            else if( (m_Type == DistributionFunction::POISSON_DURATION) && (m_Param1 <= 0.0) )
            {
                throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, m_TypeName.c_str(), type_str, info.m_ParamName_1.c_str(), "0" );
            }
            else if( (m_Type == DistributionFunction::LOG_NORMAL_DURATION) && ((m_Param1 <= 0.0) || (m_Param2 <= 0.0)) )
            {
                if( m_Param1 <= 0.0 )
                    throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, m_TypeName.c_str(), type_str, info.m_ParamName_1.c_str(), "0" );
                else
                    throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, m_TypeName.c_str(), type_str, info.m_ParamName_2.c_str(), "0" );
            }
            else if( (m_Type == DistributionFunction::BIMODAL_DURATION) && ((m_Param1 < 0.0) || (m_Param1 > 1.0) || (m_Param2 < 0.0)) )
            {
                if( m_Param1 < 0.0 )
                    throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, m_TypeName.c_str(), type_str, info.m_ParamName_1.c_str(), "0" );
                else if( m_Param1 > 1.0 )
                    throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, m_TypeName.c_str(), type_str, info.m_ParamName_1.c_str(), "1" );
                else
                    throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, m_TypeName.c_str(), type_str, info.m_ParamName_2.c_str(), "0" );
            }
            else if( (m_Type == DistributionFunction::WEIBULL_DURATION) && ((m_Param1 <= 0.0) || (m_Param2 <= 0.0)) )
            {
                if( m_Param1 <= 0.0 )
                    throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, m_TypeName.c_str(), type_str, info.m_ParamName_1.c_str(), "0" );
                else
                    throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, m_TypeName.c_str(), type_str, info.m_ParamName_2.c_str(), "0" );
            }
            else if( (m_Type == DistributionFunction::DUAL_TIMESCALE_DURATION) && ((m_Param1 <= 0.0) || (m_Param2 <= 0.0) || (m_Param3 < 0.0) || (m_Param3 > 1.0)) )
            {
                if( m_Param1 <= 0.0 )
                    throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, m_TypeName.c_str(), type_str, info.m_ParamName_1.c_str(), "0" );
                else if( m_Param2 <= 0.0 )
                    throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, m_TypeName.c_str(), type_str, info.m_ParamName_2.c_str(), "0" );
                else if( m_Param3 < 0.0 )
                    throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, m_TypeName.c_str(), type_str, info.m_ParamName_3.c_str(), "0" );
                else
                    throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, m_TypeName.c_str(), type_str, info.m_ParamName_3.c_str(), "1" );
            }
        }
        m_SupportedParameterInfoMap.clear();
    }

    float DurationDistribution::CalculateDuration()
    {
        // -----------------------------------------------------------------------
        // --- the following divides are done here and using doubles in an effort
        // --- to ensure that the regression tests pass as is.
        // -----------------------------------------------------------------------
        double p1 = m_Param1;
        double p2 = m_Param2;
        double p3 = m_Param3;
        if( m_Type == DistributionFunction::EXPONENTIAL_DURATION )
        {
            p1 = 1.0/p1;// data entered as period, but need as rate
        }
        else if( m_Type == DistributionFunction::DUAL_TIMESCALE_DURATION )
        {
            p1 = 1.0 / p1;// data entered as period, but need as rate
            p2 = 1.0 / p2;// data entered as period, but need as rate
        }
        float duration = Probability::getInstance()->fromDistribution( m_Type, p1, p2, p3, 0.0 );
        return duration;
    }

    DistributionFunction::Enum DurationDistribution::GetType() const
    {
        return m_Type;
    }

    float DurationDistribution::GetParam1() const
    {
        return m_Param1;
    }

    float DurationDistribution::GetParam2() const
    {
        return m_Param2;
    }

    float DurationDistribution::GetParam3() const
    {
        return m_Param3;
    }

    void DurationDistribution::serialize_map( IArchive& ar, std::map< DistributionFunction::Enum, ParameterInfo >& map )
    {
        size_t count = ar.IsWriter() ? map.size() : -1;

        ar.startArray(count);
        if (ar.IsWriter())
        {
            for (auto& entry : map)
            {
                DistributionFunction::Enum key   = entry.first;
                ParameterInfo              value = entry.second;
                ar.startObject();
                    ar.labelElement("key"  ) & (uint32_t&)key;
                    ar.labelElement("value.m_ParamName_1") & value.m_ParamName_1;
                    //ar.labelElement("value.m_ParamDesc_1") & value.m_ParamDesc_1;
                    ar.labelElement("value.m_ParamName_2") & value.m_ParamName_2;
                    //ar.labelElement("value.m_ParamDesc_2") & value.m_ParamDesc_2;
                    ar.labelElement( "value.m_ParamName_3" ) & value.m_ParamName_3;
                    //ar.labelElement("value.m_ParamDesc_3") & value.m_ParamDesc_3;
                    ar.endObject();
            }
        }
        else
        {
            for (size_t i = 0; i < count; ++i)
            {
                DistributionFunction::Enum key;
                ParameterInfo value;
                ar.startObject();
                    ar.labelElement("key"  ) & (uint32_t&)key;
                    ar.labelElement("value.m_ParamName_1") & value.m_ParamName_1;
                    //ar.labelElement("value.m_ParamDesc_1") & value.m_ParamDesc_1;
                    ar.labelElement("value.m_ParamName_2") & value.m_ParamName_2;
                    //ar.labelElement("value.m_ParamDesc_2") & value.m_ParamDesc_2;
                    ar.labelElement( "value.m_ParamName_3" ) & value.m_ParamName_3;
                    //ar.labelElement("value.m_ParamDesc_3") & value.m_ParamDesc_3;
                    ar.endObject();
                map[key] = value;
            }
        }
        ar.endArray();
    }

    void DurationDistribution::serialize( IArchive& ar, DurationDistribution& dd )
    {
        ar.startObject();
        ar.labelElement("m_Type"    ) & (uint32_t&)dd.m_Type;
        ar.labelElement("m_Param1"  ) & dd.m_Param1;
        ar.labelElement( "m_Param2" ) & dd.m_Param2;
        ar.labelElement( "m_Param3" ) & dd.m_Param3;
        // I don't think we need to serialize the following.
        //ar.labelElement("m_TypeName") & dd.m_TypeName;
        //ar.labelElement("m_TypeDesc") & dd.m_TypeDesc;
        //ar.labelElement("m_SupportedParameterInfoMap"); serialize_map( ar, dd.m_SupportedParameterInfoMap );
        ar.endObject();
    }
}
