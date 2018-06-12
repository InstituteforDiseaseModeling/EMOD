/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "BaseProperties.h"
#include "PropertiesString.h"
#include "Log.h"
#include "IdmString.h"
#include "FileSystem.h"
#include "NoCrtWarnings.h"
#include "Types.h"
#include "Debug.h"
#include "Common.h"
#include "RANDOM.h"

SETUP_LOGGING( "PropertiesAbstract" )

extern const char* IP_AGE_BIN_PROPERTY;

const char* IP_NAME_KEY               = "Property";
const char* IP_VALUES_KEY             = "Values";
const char* IP_INIT_KEY               = "Initial_Distribution";


std::string PropertiesToString( const tProperties& properties, 
                                const char propValSeparator, 
                                const char propSeparator )
{
    std::string propertyString;
    for (const auto& entry : properties)
    {
        const std::string& key   = entry.first;
        const std::string& value = entry.second;
        propertyString += key + propValSeparator + value + propSeparator;
    }

    if( !propertyString.empty() )
    {
#ifdef WIN32
        propertyString.pop_back();
#else
        propertyString.resize(propertyString.size() - 1);
#endif
    }

    return propertyString;
}

std::string PropertiesToString( const tProperties& properties )
{
    return PropertiesToString( properties, ':', ',' );
}

std::string PropertiesToStringCsvFriendly( const tProperties& properties )
{
    return PropertiesToString( properties, '-', ';' );
}

namespace Kernel
{
    template <typename T, std::size_t N>
    inline std::size_t sizeof_array( T (&)[N] ) { return N; }

    static const char* KEY_WHITE_LIST_TMP[] = { "Age_Bin", 
                                                "Accessibility", 
                                                "Geographic",
                                                "Place",
                                                "Risk",
                                                "QualityOfCare",
                                                "HasActiveTB",
                                                "InterventionStatus"  };

    // ------------------------------------------------------------------------
    // --- KeyValueInternal
    // ------------------------------------------------------------------------

    KeyValueInternal::KeyValueInternal( BaseProperty* pip, 
                                        const std::string& rValue,
                                        uint32_t externalNodeId,
                                        const ProbabilityNumber& rInitialDist )
        : m_pIP( pip )
        , m_KeyValueString()
        , m_Value( rValue )
        , m_InitialDistributions()
    {
        m_KeyValueString = BaseFactory::CreateKeyValueString( m_pIP->GetKeyAsString(), m_Value );
        m_InitialDistributions[ externalNodeId ] = rInitialDist;
    }

    // ------------------------------------------------------------------------
    // --- BaseKey
    // ------------------------------------------------------------------------

    BaseKey::BaseKey()
        : m_pIP( nullptr )
        , m_ParameterName()
    {
    }

    BaseKey::BaseKey( const BaseProperty* pip )
        : m_pIP( pip )
        , m_ParameterName()
    {
        release_assert( m_pIP );
    }

    BaseKey::~BaseKey()
    {
    }

    const std::string& BaseKey::ToString() const
    {
        if( m_pIP == nullptr )
        {
            throw NullPointerException( __FILE__, __LINE__, __FUNCTION__, "m_pIP", "BaseProperty" );
        }
        return m_pIP->GetKeyAsString();
    }

    bool BaseKey::operator==( const BaseKey& rThat ) const
    {
        // ------------------------------------------------------------
        // --- Don't check the parameter name.  The user wants to know
        // --- if this is the same key, not the same parameter.
        // ------------------------------------------------------------
        return ( this->m_pIP == rThat.m_pIP );
    }

    bool BaseKey::operator!=( const BaseKey& rThat ) const
    {
        return !operator==( rThat ); 
    }

    bool BaseKey::IsValid() const
    {
        return (m_pIP != nullptr);
    }

    const std::string& BaseKey::GetParameterName() const
    {
        return m_ParameterName;
    }

    void BaseKey::SetParameterName( const std::string& rParameterName )
    {
        m_ParameterName = rParameterName;
    }

    void BaseKey::serialize( IArchive& ar, BaseKey& key, k_assignment_function_t assign_func )
    {
        std::string key_str;
        if( ar.IsWriter() )
        {
            key_str = key.ToString();
        }

        ar & key_str;

        if( ar.IsReader() )
        {
            assign_func( &key, key_str );
        }
    }

    // ------------------------------------------------------------------------
    // --- BaseKeyValue
    // ------------------------------------------------------------------------

    BaseKeyValue::BaseKeyValue()
        : m_pInternal( nullptr )
        , m_ParameterName()
    {
    }

    BaseKeyValue::BaseKeyValue( KeyValueInternal* pkvi )
        : m_pInternal( pkvi )
        , m_ParameterName()
    {
    }

    BaseKeyValue::~BaseKeyValue()
    {
        // we don't own so don't delete
        m_pInternal = nullptr;
    }

    bool BaseKeyValue::IsValid() const
    {
        return (m_pInternal != nullptr);
    }

    const std::string& BaseKeyValue::ToString() const
    {
        if( m_pInternal == nullptr )
        {
            throw NullPointerException( __FILE__, __LINE__, __FUNCTION__, "m_pInternal", "KeyValueInternal" );
        }
        return m_pInternal->GetKeyValueString();
    }

    bool BaseKeyValue::operator==( const BaseKeyValue& rThat ) const
    {
        // ------------------------------------------------------------
        // --- Don't check the parameter name.  The user wants to know
        // --- if this is the same key-value, not the same parameter.
        // ------------------------------------------------------------
        //if( this->m_ParameterName != rThat.m_ParameterName ) return false;

        if( this->m_pInternal != rThat.m_pInternal ) return false;

        return true;
    }

    bool BaseKeyValue::operator!=( const BaseKeyValue& rThat ) const
    {
        return !operator==( rThat );
    }

    const std::string& BaseKeyValue::GetParameterName() const
    {
        return m_ParameterName;
    }

    void BaseKeyValue::SetParameterName( const std::string& rParameterName )
    {
        m_ParameterName = rParameterName;
    }

    // See BasePropertiesTempaltes.h for template methods

    const std::string& BaseKeyValue::GetKeyAsString() const
    {
        if( m_pInternal == nullptr )
        {
            throw NullPointerException( __FILE__, __LINE__, __FUNCTION__, "m_pInternal", "KeyValueInternal" );
        }
        return m_pInternal->GetProperty()->GetKeyAsString();
    }

    const std::string& BaseKeyValue::GetValueAsString() const
    {
        if( m_pInternal == nullptr )
        {
            throw NullPointerException( __FILE__, __LINE__, __FUNCTION__, "m_pInternal", "KeyValueInternal" );
        }
        return m_pInternal->GetValueAsString();
    }

    void BaseKeyValue::UpdateInitialDistribution( uint32_t externalNodeId, double value )
    {
        if( m_pInternal == nullptr )
        {
            throw NullPointerException( __FILE__, __LINE__, __FUNCTION__, "m_pInternal", "KeyValueInternal" );
        }
        m_pInternal->SetInitialDistribution( externalNodeId, value );
    }

    void BaseKeyValue::serialize( IArchive& ar, BaseKeyValue& kv, kv_assignment_function_t assign_func )
    {
        std::string kv_str;
        if( ar.IsWriter() && kv.IsValid() )
        {
            kv_str = kv.ToString();
        }

        ar & kv_str;

        if( ar.IsReader() && !kv_str.empty() )
        {
            assign_func( &kv, kv_str );
        }
    }

    // ------------------------------------------------------------------------
    // --- BaseIterator
    // ------------------------------------------------------------------------

    // See BasePropertiesTempaltes.h for template classes/methods

    // ------------------------------------------------------------------------
    // --- BaseKeyValueContainer
    // ------------------------------------------------------------------------

    // See BasePropertiesTempaltes.h for template classes/methods

    // ------------------------------------------------------------------------
    // --- BaseProperty
    // ------------------------------------------------------------------------

    BaseProperty::BaseProperty()
        : m_Key()
        , m_Values()
    {
    }

    BaseProperty::BaseProperty( BaseFactory* pFactory, 
                                uint32_t externalNodeId, 
                                const std::string& rKeyStr, 
                                const std::map<std::string,float>& rValues )
        : m_Key(rKeyStr)
        , m_Values()
    {
        float total_prob = 0.0;
        for( auto entry : rValues )
        {
            KeyValueInternal* pkvi = new KeyValueInternal( this, entry.first, externalNodeId, entry.second );
            pFactory->AddKeyValue( pkvi );
            m_Values.push_back( pkvi );
            total_prob += entry.second;
        }
        if( (total_prob < 0.99999) || (1.000001 < total_prob) )
        {
            std::ostringstream msg;
            msg << "Bin probabilities in "
                << IP_INIT_KEY
                << " section for property "
                << m_Key
                << " must add up to 1.0. Instead came to "
                << total_prob
                << "."
                << std::endl;
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }
    }

    BaseProperty::~BaseProperty()
    {
    }

    void BaseProperty::Read( int idx, uint32_t externalNodeId, const JsonObjectDemog& rDemog, bool isNotFirstNode )
    {
        if( isNotFirstNode )
        {
            release_assert( m_Key == rDemog[ IP_NAME_KEY ].AsString() );
        }
        else
        {
            m_Key = rDemog[ IP_NAME_KEY ].AsString();
        }

        if( m_Key != IP_AGE_BIN_PROPERTY )
        {
            ReadProperty( idx, externalNodeId, rDemog, isNotFirstNode );
        }
    }

    bool BaseProperty::Contains( const std::vector<KeyValueInternal*>& rValueList, const std::string& rKey, const std::string& rNewValue )
    {
        std::string kv_str = BaseFactory::CreateKeyValueString( rKey, rNewValue );

        for( auto pkvi : rValueList )
        {
            if( pkvi->GetKeyValueString() == kv_str )
            {
                return true;
            }
        }
        return false;
    }

    void BaseProperty::ReadProperty( const char* ip_key_str,
                                     const char* ip_values_key_str,
                                     const char* ip_init_key_str,
                                     BaseFactory* ip_factory,
                                     int idx, 
                                     uint32_t externalNodeId, 
                                     const JsonObjectDemog& rDemog,
                                     bool isNotFirstNode,
                                     get_kvi_function_t get_kvi_func )
    {
        if( !rDemog.Contains( ip_values_key_str ) )
        {
            std::ostringstream badMap;
            badMap << "demographics[" << ip_key_str << "][" << idx << "]";
            throw BadMapKeyException( __FILE__, __LINE__, __FUNCTION__, badMap.str().c_str(), ip_values_key_str );
        }
        if( !rDemog.Contains( ip_init_key_str ) )
        {
            std::ostringstream badMap;
            badMap << "demographics[" << ip_key_str << "][" << idx << "]";
            throw BadMapKeyException( __FILE__, __LINE__, __FUNCTION__, badMap.str().c_str(), ip_init_key_str );
        }

        auto num_values = rDemog[ ip_values_key_str ].size();
        auto num_probs  = rDemog[ ip_init_key_str   ].size();

        if( num_values != num_probs )
        {
            std::ostringstream msg;
            msg << "Number of Values in " << ip_values_key_str << " ("
                << num_values
                << ") needs to be the same as number of values in "
                << ip_init_key_str
                << " ("
                << num_probs
                << ")."
                << std::endl;
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }
        else if( num_values == 0 )
        {
            std::ostringstream msg;
            msg << "demographics[" << ip_key_str << "][" << idx << "][" << ip_values_key_str << "] (property=" << m_Key << ") cannot have zero values.";
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }
        else if( isNotFirstNode && (num_values != m_Values.size()) )
        {
            std::stringstream ss;
            ss << "demographics[" << ip_key_str << "][" << idx << "][" << ip_values_key_str << "] for key=" << m_Key << " and nodeId=" << externalNodeId << " has " << num_values << " values.\n";
            ss << "The previous node(s) had " << m_Values.size() << " values.  All nodes must have the same keys and values.";
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }

        ip_factory->CheckIpKeyInWhitelist( ip_key_str, m_Key, num_values );

        float total_prob = 0.0;
        for( int val_idx = 0; val_idx < num_values; val_idx++ )
        {
            std::string       value        = rDemog[ ip_values_key_str ][val_idx].AsString();
            ProbabilityNumber initial_dist = rDemog[ ip_init_key_str   ][val_idx].AsDouble();
            std::string kv_str = BaseFactory::CreateKeyValueString( m_Key, value );
            bool contains_kv = BaseProperty::Contains( m_Values, m_Key, value );
            if( isNotFirstNode )
            {
                if( !contains_kv )
                {
                    std::ostringstream ss;
                    ss << "demographics[" << ip_key_str << "][" << idx << "] with property=" << m_Key << " for NodeId=" << externalNodeId << " has value=" << value << ".\n";
                    ss << "Previous node(s) do not have this value.  All nodes must have the same keys and values.";
                    throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
                }
                KeyValueInternal* p_kvi = get_kvi_func( ip_factory, ip_key_str, kv_str );
                p_kvi->m_InitialDistributions[ externalNodeId ] = initial_dist;
            }
            else
            {
                if( contains_kv )
                {
                    std::ostringstream ss;
                    ss << "demographics[" << ip_key_str << "][" << idx << "] with property=" << m_Key << " has a duplicate value = " << value ;
                    throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
                }
                KeyValueInternal* pkvi = new KeyValueInternal( this, value, externalNodeId, initial_dist );
                ip_factory->AddKeyValue( pkvi );
                m_Values.push_back( pkvi );
            }
            total_prob += initial_dist;
        }

        if( (total_prob < 0.99999) || (1.000001 < total_prob) )
        {
            std::ostringstream ss;
            ss << "The values in demographics[" << ip_key_str << "][" << idx << "][" << ip_init_key_str << "] (property=" << m_Key << ") add up to " << total_prob << ".  They must add up to 1.0" ;
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
    }

    // See BasePropertiesTempaltes.h for template classes/methods

    bool BaseProperty::Compare( BaseProperty* pLeft, BaseProperty* pRight )
    {
        return (pLeft->GetKeyAsString() < pRight->GetKeyAsString());
    }

    // ------------------------------------------------------------------------
    // --- BaseFactory
    // ------------------------------------------------------------------------

    BaseFactory::BaseFactory()
        : m_ExternalNodeIdOfFirst( UINT32_MAX )
        , m_WhiteListEnabled( true )
        , m_IPList()
        , m_KeyValueMap()
        , m_KeyWhiteList()
    {
        m_KeyWhiteList = std::set< std::string> ( KEY_WHITE_LIST_TMP, KEY_WHITE_LIST_TMP+sizeof_array(KEY_WHITE_LIST_TMP) );
    }

    BaseFactory::~BaseFactory()
    {
        for( auto pip : m_IPList )
        {
            delete pip;
        }
        m_IPList.clear();

        for( auto entry : m_KeyValueMap )
        {
            delete entry.second;
        }
        m_KeyValueMap.clear();
    }

    std::string BaseFactory::CreateKeyValueString( const std::string& rKeyStr, const std::string& rValueStr )
    {
        return rKeyStr + std::string(KEY_VALUE_SEPARATOR) + rValueStr;
    }

    std::string trim( const std::string& str,
                      const std::string& whitespace = " \t" )
    {
        const auto strBegin = str.find_first_not_of( whitespace );
        if( strBegin == std::string::npos )
        {
            return ""; // no content
        }

        const auto strEnd = str.find_last_not_of( whitespace );
        const auto strRange = strEnd - strBegin + 1;

        return str.substr( strBegin, strRange );
    }

    void BaseFactory::ParseKeyValueString( const char* ip_key_str, const std::string& rKeyValueStr, std::string& rKeyStr, std::string& rValueStr )
    {
        IdmString kv_str = rKeyValueStr;
        auto fragments = kv_str.split(KEY_VALUE_SEPARATOR_CHAR);
        if( (fragments.size() != 2) || fragments[0].empty() || fragments[1].empty() )
        {
            std::ostringstream msg;
            msg << "Invalid " << ip_key_str << " Key-Value string = '" << rKeyValueStr << "'.  Format is 'key:value'.";
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }
        rKeyStr   = trim( fragments[0] );
        rValueStr = trim( fragments[1] );
    }

    void BaseFactory::Initialize( const char* ip_key_str,
                                        const char* ip_name_key_str,
                                        BaseFactory::read_function_t read_func,
                                        uint32_t externalNodeId,
                                        const JsonObjectDemog& rDemog,
                                        bool isWhitelistEnabled )
    {
        m_WhiteListEnabled = isWhitelistEnabled;

        if( externalNodeId == UINT32_MAX )
        {
            std::stringstream ss;
            ss << "You are not allowed to use an external Node ID of " << UINT32_MAX << ".";
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }

        bool first_time = (m_ExternalNodeIdOfFirst == UINT32_MAX);
        if( first_time )
        {
            m_ExternalNodeIdOfFirst = externalNodeId;
        }

        if( !rDemog.Contains( ip_key_str ) )
        {
            return;
        }

        // Check that we're not using more than 3 axes in whitelist mode
        if( rDemog[ ip_key_str ].size() > 3 && isWhitelistEnabled )
        {
            std::ostringstream msg;
            msg << "Too many " << ip_key_str << " (" 
                << rDemog[ ip_key_str ].size()
                << "). Max is 3."
                << std::endl;
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }

        if( first_time )
        {
            for( int idx = 0; idx < rDemog[ ip_key_str ].size(); idx++ )
            {
                std::string key_name = rDemog[ ip_key_str ][ idx ][ ip_name_key_str ].AsString();
                CheckForDuplicateKey( key_name );

                BaseProperty* p_ip = read_func( idx, externalNodeId, rDemog[ ip_key_str ][ idx ], false );
                m_IPList.push_back( p_ip );
            }

            // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            // !!! Needed only to stay in sync with previous version
            // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            std::sort( m_IPList.begin(), m_IPList.end(), BaseProperty::Compare );
        }
        else if( m_ExternalNodeIdOfFirst != externalNodeId )
        {
            if( rDemog[ ip_key_str ].size() != m_IPList.size() )
            {
                std::stringstream ss;
                ss << ip_key_str << " were first intialized for nodeID=" << m_ExternalNodeIdOfFirst << " and it had " << m_IPList.size() << " properties.\n";
                ss << "nodeID=" << externalNodeId << " has " << rDemog[ ip_key_str ].size() << " properties.  All nodes must have the same keys and values.";
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }
            for( int idx = 0; idx < rDemog[ ip_key_str ].size(); idx++ )
            {
                BaseProperty* p_ip = GetIP( rDemog[ ip_key_str ][ idx ][ IP_NAME_KEY ].AsString(), "", false );
                if( p_ip == nullptr )
                {
                    std::stringstream ss;
                    ss << ip_key_str  << " were first initialized for node " << m_ExternalNodeIdOfFirst << ".\n";
                    ss << "nodeID=" << externalNodeId << " has '" << rDemog[ ip_key_str ][ idx ][ ip_name_key_str ].AsString() << "' which is not in the first node.\n";
                    ss << "All nodes must have the same keys and values (and in the same order).";
                    throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
                }
                else
                {
                    p_ip->Read( idx, externalNodeId, rDemog[ ip_key_str ][ idx ], true );
                }
            }
        }
    }

    void BaseFactory::CheckIpKeyInWhitelist( const char* ip_key_str, const std::string& rKey, int numValues )
    {
        if( m_WhiteListEnabled )
        {
            if( m_KeyWhiteList.count( rKey ) == 0 )
            {
                std::ostringstream msg;
                msg << "Invalid " << ip_key_str << " key '" << rKey << "' found in demographics file. Use one of: ";
                for (auto& key : m_KeyWhiteList)
                {
                    msg << "'" << key<< "', " ;
                }
                std::string msg_str = msg.str();
                msg_str = msg_str.substr( 0, msg_str.length()-2 );
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
            }

            if (((numValues > 5) && (rKey != "Geographic") && (rKey != "InterventionStatus")) || (numValues > 125))
            {
                std::ostringstream msg;
                msg << "Too many values for Individual Property key " << rKey
                    << ".  This key has " << numValues << " and the limit is 5, except for Geographic & InterventionStatus, which is 125." << std::endl;
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
            }
        }
    }

    void BaseFactory::CheckForDuplicateKey( const std::string& rKeyStr )
    {
        bool found = false;
        std::string known_keys;
        for( auto p_ip : m_IPList )
        {
            known_keys += p_ip->GetKeyAsString() + ", ";
            if( p_ip->GetKeyAsString() == rKeyStr )
            {
                found = true;
            }
        }
        if( found )
        {
            known_keys = known_keys.substr( 0, known_keys.length() - 2 );
            std::ostringstream msg;
            msg << "Found existing Property key = '" << rKeyStr << "'.  Can't create duplicate key.  ";
            msg << "Known keys are: " << known_keys;
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }
    }

    void BaseFactory::AddIP( uint32_t externalNodeId,
                             const std::string& rKeyStr, 
                             const std::map<std::string,float>& rValues,
                             ip_construct_function_t create_ip )
    {
        CheckForDuplicateKey( rKeyStr );

        BaseProperty* p_ip = create_ip( externalNodeId, rKeyStr, rValues );
        m_IPList.push_back( p_ip );
    }

    BaseProperty* BaseFactory::GetIP( const std::string& rKey, const std::string& rParameterName, bool throwOnNotFound )
    {
        for( auto p_ip : m_IPList )
        {
            if( p_ip->GetKeyAsString() == rKey )
            {
                return p_ip;
            }
        }
        if( throwOnNotFound )
        {
            std::ostringstream msg;
            msg << "Could not find the IndividualProperty key = '" << rKey;
            if( !rParameterName.empty() )
            {
                msg << "' for parameter '" << rParameterName;
            }
            msg << "'.  ";
            msg << "Known keys are: " << GetKeysAsString();
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }
        return nullptr;
    }

    // See BasePropertiesTempaltes.h for template classes/methods

    void BaseFactory::AddKeyValue( KeyValueInternal* pkvi )
    {
        if( m_KeyValueMap.count( pkvi->GetKeyValueString() ) != 0 )
        {
            std::ostringstream msg;
            msg << "The IndividualProperty key-value = " << pkvi->GetKeyValueString() << " already exists.  ";
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }
        m_KeyValueMap[ pkvi->GetKeyValueString() ] = pkvi;
    }

    std::set< std::string > BaseFactory::GetKeysAsStringSet() const
    {
        std::set<std::string> keys;
        for( auto pIP : m_IPList )
        {
            keys.insert( pIP->m_Key );
        }
        return keys;
    }

    std::string BaseFactory::GetKeysAsString() const
    {
        std::string keys_str;
        if( m_IPList.size() > 0 )
        {
            keys_str = m_IPList[0]->GetKeyAsString();
            for( int i = 1 ; i < m_IPList.size() ; i++ )
            {
                keys_str += std::string(", ") + m_IPList[i]->GetKeyAsString();
            }
        }
        return keys_str;
    }
}
