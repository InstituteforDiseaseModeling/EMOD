/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "NodeProperties.h"
#include "BasePropertiesTemplates.h"
#include "PropertiesString.h"
#include "Log.h"
#include "FileSystem.h"
#include "NoCrtWarnings.h"
#include "Types.h"
#include "Debug.h"
#include "Common.h"
#include "RANDOM.h"

static const char* _module = "NodeProperties";

const char* NP_KEY        = "NodeProperties";
const char* NP_NAME_KEY   = "Property";
const char* NP_VALUES_KEY = "Values";
const char* NP_INIT_KEY   = "Initial_Distribution";


namespace Kernel
{
    // ------------------------------------------------------------------------
    // --- NPKey
    // ------------------------------------------------------------------------

    const char* NPKey::GetConstrainedStringConstraintKey()
    {
        return "<demographics>::NodeProperties.*.Property";
    }

    const char* NPKey::GetConstrainedStringConstraintValue()
    {
        return "<demographics>::NodeProperties.*.Values";
    }

    const char* NPKey::GetConstrainedStringConstraintKeyValue()
    {
        return "'<demographics>::NodeProperties.*.Property':'<demographics>::NodeProperties.*.Values'";
    }

    const char* NPKey::GetConstrainedStringDescriptionKey()
    {
        return "Node Property Key from demographics file.";
    }

    const char* NPKey::GetConstrainedStringDescriptionValue()
    {
        return "Node Property Value from demographics file.";
    }

    NPKey::NPKey()
        : BaseKey()
    {
    }

    NPKey::NPKey( const BaseProperty* pnp )
        : BaseKey( pnp )
    {
    }

    NPKey::NPKey( const std::string& rKeyStr )
        : BaseKey()
    {
        m_pIP = NPFactory::GetInstance()->GetNP( rKeyStr );
    }

    NPKey::~NPKey()
    {
    }

    NPKey& NPKey::operator=( const std::string& rKeyStr )
    {
        m_pIP = NPFactory::GetInstance()->GetNP( rKeyStr, m_ParameterName );
        return *this;
    }

    bool NPKey::operator==( const NPKey& rThat ) const
    {
        return BaseKey::operator==( rThat );
    }

    bool NPKey::operator!=( const NPKey& rThat ) const
    {
        return !operator==( rThat ); 
    }

    static void key_assign_func( BaseKey* pkv, const std::string& rKeyStr )
    {
        NPKey* p_key = static_cast<NPKey*>(pkv);
        *p_key = rKeyStr;
    }

    void NPKey::serialize( IArchive& ar, NPKey& key )
    {
        BaseKey::serialize( ar, key, key_assign_func );
    }

    // ------------------------------------------------------------------------
    // --- NPKeyValue
    // ------------------------------------------------------------------------

    NPKeyValue::NPKeyValue()
        : BaseKeyValue()
    {
    }

    NPKeyValue::NPKeyValue( const std::string& rKeyValueStr )
        : BaseKeyValue()
    {
        m_pInternal = NPFactory::GetInstance()->GetKeyValue<NPKeyValueContainer>( NP_KEY, rKeyValueStr );
    }

    NPKeyValue::NPKeyValue( const std::string& rKeyStr, const std::string& rValueStr )
        : BaseKeyValue()
    {
        std::string kv_str = NPFactory::CreateKeyValueString( rKeyStr, rValueStr );
        m_pInternal = NPFactory::GetInstance()->GetKeyValue<NPKeyValueContainer>( NP_KEY, kv_str );
    }

    NPKeyValue::NPKeyValue( KeyValueInternal* pkvi )
        : BaseKeyValue( pkvi )
    {
    }

    NPKeyValue::~NPKeyValue()
    {
    }

    NPKeyValue& NPKeyValue::operator=( const std::string& rKeyValueStr )
    {
        m_pInternal = NPFactory::GetInstance()->GetKeyValue<NPKeyValueContainer>( NP_KEY, rKeyValueStr, m_ParameterName );
        return *this;
    }

    bool NPKeyValue::operator==( const NPKeyValue& rThat ) const 
    {
        return BaseKeyValue::operator==( rThat );
    }

    bool NPKeyValue::operator!=( const NPKeyValue& rThat ) const
    {
        return !operator==( rThat );
    }

    double NPKeyValue::GetInitialDistribution() const
    {
        if( m_pInternal == nullptr )
        {
            throw NullPointerException( __FILE__, __LINE__, __FUNCTION__, "m_pInternal", "NPKeyValueInternal" );
        }
        return m_pInternal->GetInitialDistribution( 0 );
    }

    static void key_value_assign_func( BaseKeyValue* pkva, const std::string& rKvStr )
    {
        NPKeyValue* p_kv = static_cast<NPKeyValue*>(pkva);
        *p_kv = rKvStr;
    }

    void NPKeyValue::serialize( IArchive& ar, NPKeyValue& kv )
    {
        BaseKeyValue::serialize( ar, kv, key_value_assign_func );
    }

    // ------------------------------------------------------------------------
    // --- NPKeyValueIterator
    // ------------------------------------------------------------------------
    // See BasePropertiesTempaltes.h for template classes/methods

    NPKeyValueIterator::NPKeyValueIterator( std::vector<KeyValueInternal*>::const_iterator it )
        : BaseIterator<NPKeyValue>( it )
    {
    }

    NPKeyValueIterator::~NPKeyValueIterator()
    {
    }

    bool NPKeyValueIterator::operator==( const NPKeyValueIterator& rThat ) const
    {
        return BaseIterator<NPKeyValue>::operator==( rThat );
    }

    bool NPKeyValueIterator::operator!=( const NPKeyValueIterator& rThat ) const
    {
        return !operator==( rThat );
    }

    NPKeyValueIterator& NPKeyValueIterator::operator++()
    {
        BaseIterator<NPKeyValue>::operator++();
        return *this;
    }

    // ------------------------------------------------------------------------
    // --- NPKeyValueContainer
    // ------------------------------------------------------------------------
    // See BasePropertiesTempaltes.h for template classes/methods

    NPKeyValueContainer::NPKeyValueContainer()
        : BaseKeyValueContainer<NPKey, NPKeyValue, NPKeyValueIterator>()
    {
    }

    NPKeyValueContainer::NPKeyValueContainer( const std::vector<KeyValueInternal*>& rInternalList )
        : BaseKeyValueContainer<NPKey, NPKeyValue, NPKeyValueIterator>( rInternalList )
    {
    }

    NPKeyValueContainer::NPKeyValueContainer( const NPKeyValueContainer& rThat )
        : BaseKeyValueContainer<NPKey, NPKeyValue, NPKeyValueIterator>( rThat )
    {
    }

    NPKeyValueContainer::~NPKeyValueContainer()
    {
    }

    NPKeyValueContainer& NPKeyValueContainer::operator=( const NPKeyValueContainer& rThat )
    {
        BaseKeyValueContainer<NPKey, NPKeyValue, NPKeyValueIterator>::operator=( rThat );
        return *this;
    }


    bool NPKeyValueContainer::operator==( const NPKeyValueContainer& rThat ) const
    {
        return BaseKeyValueContainer<NPKey, NPKeyValue, NPKeyValueIterator>::operator==( rThat );
    }

    bool NPKeyValueContainer::operator!=( const NPKeyValueContainer& rThat ) const
    {
        return !operator==( rThat );
    }

    void NPKeyValueContainer::Add( const NPKeyValue& rKeyValue )
    {
        BaseKeyValueContainer<NPKey, NPKeyValue, NPKeyValueIterator>::Add( rKeyValue.m_pInternal );
    }

    void NPKeyValueContainer::Remove( const NPKeyValue& rKeyValue )
    {
        BaseKeyValueContainer<NPKey, NPKeyValue, NPKeyValueIterator>::Remove( rKeyValue.m_pInternal );
    }

    void NPKeyValueContainer::Set( const NPKeyValue& rNewKeyValue )
    {
        BaseKeyValueContainer<NPKey, NPKeyValue, NPKeyValueIterator>::Set( rNewKeyValue.m_pInternal );
    }

    NPKeyValue NPKeyValueContainer::FindFirst( const NPKeyValueContainer& rContainer ) const
    {
        return NPKeyValue( BaseKeyValueContainer::FindFirst( rContainer ) );
    }

    // ------------------------------------------------------------------------
    // --- NodeProperty
    // ------------------------------------------------------------------------

    NodeProperty::NodeProperty()
        : BaseProperty()
    {
    }

    NodeProperty::NodeProperty( const std::string& rKeyStr, const std::map<std::string,float>& rValues )
        : BaseProperty( NPFactory::GetInstance(), 0, rKeyStr, rValues ) // Put data in "node" zero InitialDistributions
    {
    }

    NodeProperty::~NodeProperty()
    {
    }

    KeyValueInternal* NodeProperty::get_kvi_func( BaseFactory* pFact, const char* ip_key_str, const std::string& rKvStr )
    {
        NPFactory* p_np_fact = static_cast<NPFactory*>(pFact);
        KeyValueInternal* p_kvi = p_np_fact->GetKeyValue<NPKeyValueContainer>( ip_key_str, rKvStr );
        return nullptr;
    }

    void NodeProperty::ReadProperty( int idx, uint32_t externalNodeId, const JsonObjectDemog& rDemog, bool isNotFirstNode )
    {
        BaseProperty::ReadProperty( NP_KEY,
                                    NP_VALUES_KEY,
                                    NP_INIT_KEY,
                                    NPFactory::GetInstance(),
                                    idx,
                                    0,  // Put data in "node" zero InitialDistributions
                                    rDemog,
                                    isNotFirstNode,
                                    get_kvi_func );
    }

    // ------------------------------------------------------------------------
    // --- NPFactory
    // ------------------------------------------------------------------------
    const char* NPFactory::NODE_PROPERTIES_JSON_KEY = NP_KEY;

    NPFactory::NPFactory()
        : BaseFactory()
    {
        m_KeyWhiteList.erase( "Age_Bin" );
    }

    NPFactory::~NPFactory()
    {
    }

    void NPFactory::ParseKeyValueString( const std::string& rKeyValueStr, std::string& rKeyStr, std::string& rValueStr )
    {
        return BaseFactory::ParseKeyValueString( NP_KEY, rKeyValueStr, rKeyStr, rValueStr );
    }

    void NPFactory::CreateFactory()
    {
        if( Environment::getNPFactory() != nullptr )
        {
            //throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, "NPFactory has already been created." );
            // Not sure we have to do something drastic here. In PyMod we might call this twice and we just want to carry on.
            return;
        }
        Environment::setNPFactory( new NPFactory() );
    }

    void NPFactory::DeleteFactory()
    {
        NPFactory* p_factory = (NPFactory*)Environment::getNPFactory();
        delete p_factory;
        Environment::setNPFactory( nullptr );
    }

    NPFactory* NPFactory::GetInstance()
    {
        NPFactory* p_factory = (NPFactory*)Environment::getNPFactory();
        // TBD: Removing exception in regular IP and Node for PyMod for now. Need to explore if we don't need to do this.
        /*if( p_factory == nullptr )
        {
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, "NPFactory has not been created." );
        }*/
        return p_factory;
    }

    void  NPFactory::Initialize( const JsonObjectDemog& rDemog, bool isWhitelistEnabled )
    {
        if( !rDemog.IsNull() )
        {
            read_function_t read_fn =
                []( int idx, uint32_t externNodeId, const JsonObjectDemog& rDemog, bool isNotFirstNode )
            {
                NodeProperty* p_ip = new NodeProperty();
                p_ip->Read( idx, 0, rDemog, isNotFirstNode );
                return (BaseProperty*)p_ip;
            };

            JsonObjectDemog np_json( JsonObjectDemog::JsonObjectType::JSON_OBJECT_OBJECT );
            np_json.Add( NP_KEY, rDemog );

            BaseFactory::Initialize( NP_KEY, NP_NAME_KEY, read_fn, 0, np_json, isWhitelistEnabled );
        }
    }

    BaseProperty* NPFactory::construct_np( uint32_t externalNodeId,
                                               const std::string& rKeyStr,
                                               const std::map<std::string, float>& rValues )
    {
        return new NodeProperty( rKeyStr, rValues );
    }

    void NPFactory::AddNP( const std::string& rKeyStr, const std::map<std::string,float>& rValues )
    {
        BaseFactory::AddIP( 0, rKeyStr, rValues, NPFactory::construct_np );
    }

    NodeProperty* NPFactory::GetNP( const std::string& rKey, const std::string& rParameterName, bool throwOnNotFound )
    {
        return static_cast<NodeProperty*>( BaseFactory::GetIP( rKey, rParameterName, throwOnNotFound ) );
    }

    std::vector<NodeProperty*> NPFactory::GetNPList() const
    {
        std::vector<NodeProperty*> np_list;
        for( BaseProperty* ip : m_IPList )
        {
            np_list.push_back( static_cast<NodeProperty*>(ip) );
        }
        return np_list;
    }

    NPKeyValueContainer NPFactory::ReadPropertyValues( const JsonObjectDemog& rNodeJson )
    {
        NPKeyValueContainer property_values;

        if( rNodeJson.Contains( "NodeAttributes" ) )
        {
            if( rNodeJson["NodeAttributes"].Contains( "NodePropertyValues" ) )
            {
                JsonObjectDemog np_values = rNodeJson[ "NodeAttributes" ][ "NodePropertyValues" ];
                release_assert( np_values.IsArray() );

                for( int i = 0 ; i < np_values.size() ; ++i )
                {
                    std::string kv_str = np_values[ i ].AsString();

                    // ------------------------------------------------------
                    // --- The following will remove any white space and make
                    // --- sure the read in value is in the correct format.
                    // ------------------------------------------------------
                    std::string key_str;
                    std::string value_str;
                    NPFactory::ParseKeyValueString( kv_str, key_str, value_str );
                    kv_str = NPFactory::CreateKeyValueString( key_str, value_str );

                    NPKeyValue kv( GetKeyValue<NPKeyValueContainer>( NP_KEY, kv_str ) );
                    property_values.Add( kv );
                }
            }
        }

        return property_values;
    }

    NPKeyValueContainer NPFactory::GetInitialValues( RANDOMBASE* pRNG, const JsonObjectDemog& rNodeJson ) 
    {
        NPKeyValueContainer properties;

        // ---------------------------------------------------------------------------------------------
        // --- We are getting all of the initial values even if the user has set the value for the node.
        // --- This should cause the random number stream to be the same whether or not the user selects
        // --- fixed values via the NodePropertyValues option.
        // ---------------------------------------------------------------------------------------------
        for( BaseProperty* p_pa : m_IPList )
        {
            NodeProperty* pnp = static_cast<NodeProperty*>(p_pa);

            // For NodeProperties, there is only one stored location and it is "node" zero
            NPKeyValue init_val = pnp->GetInitialValue<NPKeyValue>( 0, pRNG );
            properties.Add( init_val );
        }

        // ------------------------------------------------------------------
        // --- Override the values selected from the distribution with those
        // --- that the user said are fixed.
        // ------------------------------------------------------------------
        NPKeyValueContainer user_selected_np_values = ReadPropertyValues( rNodeJson );
        for( auto kv : user_selected_np_values )
        {
            properties.Set( kv );
        }
        return properties;
    }

    // -------------------------------------------------------------------------------
    // --- This defines the implementations for these templetes with these parameters.
    // --- If you comment these out, you will get unresolved externals when linking.
    // -------------------------------------------------------------------------------
    template IDMAPI NPKey BaseKeyValue::GetKey<NPKey>() const;
    template IDMAPI NPKey BaseProperty::GetKey<NPKey>() const;
    template IDMAPI NPKeyValueContainer BaseProperty::GetValues<NPKeyValueContainer>() const;
    template IDMAPI std::vector<std::string> BaseFactory::GetAllPossibleKeyValueCombinations<NPKeyValueContainer>() const;
    template IDMAPI KeyValueInternal* BaseFactory::GetKeyValue<NPKeyValueContainer>( const char* ip_key_str, const std::string& rKeyValueString, const std::string& rParameterName );

    template IDMAPI class BaseIterator<NPKeyValue>;
    template IDMAPI class BaseKeyValueContainer<NPKey, NPKeyValue, NPKeyValueIterator>;
}
