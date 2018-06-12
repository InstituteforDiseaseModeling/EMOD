/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "BaseProperties.h"
#include "Debug.h"
#include "RANDOM.h"

// -------------------------------------------------------------------------------------
// This file contains the all of the template classes/functions for the Base Properties
// class.  It is expected to be included in the implementing class CPP files.
// -------------------------------------------------------------------------------------

namespace Kernel
{
    // ------------------------------------------------------------------------
    // --- BaseKeyValue
    // ------------------------------------------------------------------------

    template<class Key>
    Key BaseKeyValue::GetKey() const
    {
        if( m_pInternal == nullptr )
        {
            throw NullPointerException( __FILE__, __LINE__, __FUNCTION__, "m_pInternal", "KeyValueInternal" );
        }
        Key key( m_pInternal->GetProperty() );
        return key;
    }

    // ------------------------------------------------------------------------
    // --- BaseIterator
    // ------------------------------------------------------------------------

    template<class KeyValue>
    BaseIterator<KeyValue>::BaseIterator( std::vector<KeyValueInternal*>::const_iterator it )
        : m_Iterator( it )
        , m_KeyValue()
    {
    }

    template<class KeyValue>
    BaseIterator<KeyValue>::~BaseIterator()
    {
    }

    template<class KeyValue>
    bool BaseIterator<KeyValue>::operator==( const BaseIterator<KeyValue>& rThat ) const
    {
        return (this->m_Iterator == rThat.m_Iterator);
    }

    template<class KeyValue>
    bool BaseIterator<KeyValue>::operator!=( const BaseIterator& rThat ) const
    {
        return !operator==( rThat );
    }

    template<class KeyValue>
    BaseIterator<KeyValue>& BaseIterator<KeyValue>::operator++()
    {
        m_Iterator++;
        return *this;
    }

    template<class KeyValue>
    const KeyValue& BaseIterator<KeyValue>::operator*()
    {
        m_KeyValue.m_pInternal = *m_Iterator;
        return m_KeyValue;
    }

    // ------------------------------------------------------------------------
    // --- BaseKeyValueContainer
    // ------------------------------------------------------------------------

    template<class Key, class KeyValue, class Iterator_t>
    Iterator_t BaseKeyValueContainer<Key, KeyValue, Iterator_t>::begin() const
    {
        return Iterator_t( m_Vector.begin() );
    }

    template<class Key, class KeyValue, class Iterator_t>
    Iterator_t BaseKeyValueContainer<Key, KeyValue, Iterator_t>::end() const
    {
        return Iterator_t( m_Vector.end() );
    }

    template<class Key, class KeyValue, class Iterator_t>
    BaseKeyValueContainer<Key, KeyValue, Iterator_t>::BaseKeyValueContainer()
        : m_Vector()
    {
    }

    template<class Key, class KeyValue, class Iterator_t>
    BaseKeyValueContainer<Key, KeyValue, Iterator_t>::BaseKeyValueContainer( const std::vector<KeyValueInternal*>& rInternalList )
        : m_Vector()
    {
        for( auto pkvi : rInternalList )
        {
            m_Vector.push_back( pkvi );
        }
    }

    template<class Key, class KeyValue, class Iterator_t>
    BaseKeyValueContainer<Key, KeyValue, Iterator_t>::~BaseKeyValueContainer()
    {
    }

    template<class Key, class KeyValue, class Iterator_t>
    BaseKeyValueContainer<Key, KeyValue, Iterator_t>::BaseKeyValueContainer( const BaseKeyValueContainer<Key, KeyValue, Iterator_t>& rThat )
        : m_Vector( rThat.m_Vector )
    {
    }

    template<class Key, class KeyValue, class Iterator_t>
    BaseKeyValueContainer<Key, KeyValue, Iterator_t>& BaseKeyValueContainer<Key, KeyValue, Iterator_t>::operator=( const BaseKeyValueContainer<Key, KeyValue, Iterator_t>& rThat )
    {
        if( this != &rThat )
        {
            this->m_Vector = rThat.m_Vector;
        }
        return *this;
    }

    template<class Key, class KeyValue, class Iterator_t>
    bool BaseKeyValueContainer<Key, KeyValue, Iterator_t>::operator==( const BaseKeyValueContainer<Key, KeyValue, Iterator_t>& rThat ) const
    {
        if( this->m_Vector.size() != rThat.m_Vector.size() ) return false;

        for( int i = 0 ; i < this->m_Vector.size() ; ++i )
        {
            KeyValueInternal* p_this_kvi = this->m_Vector[ i ];
            KeyValueInternal* p_that_kvi = rThat.m_Vector[ i ];

            if( p_this_kvi != p_that_kvi ) return false;
        }
        return true;
    }

    template<class Key, class KeyValue, class Iterator_t>
    bool BaseKeyValueContainer<Key, KeyValue, Iterator_t>::operator!=( const BaseKeyValueContainer<Key, KeyValue, Iterator_t>& rThat ) const
    {
        return !operator==( rThat );
    }

    template<class Key, class KeyValue, class Iterator_t>
    void BaseKeyValueContainer<Key, KeyValue, Iterator_t>::Add( KeyValueInternal* pInternal )
    {
        m_Vector.push_back( pInternal );
    }

    template<class Key, class KeyValue, class Iterator_t>
    void BaseKeyValueContainer<Key, KeyValue, Iterator_t>::Remove( const KeyValueInternal* pInternal )
    {
        for( auto it = m_Vector.begin(); it != m_Vector.end(); ++it )
        {
            if( pInternal == *it )
            {
                m_Vector.erase( it );
                break;
            }
        }
    }

    template<class Key, class KeyValue, class Iterator_t>
    KeyValue BaseKeyValueContainer<Key, KeyValue, Iterator_t>::Get( const Key& rKey ) const
    {
        KeyValue kv;
        bool found = false;
        for( auto p_kvi : m_Vector )
        {
            if( p_kvi->GetProperty() == rKey.m_pIP )
            {
                if( found )
                {
                    std::ostringstream ss;
                    ss << "Illegal use of KeyValueContainer::Get( const Key& rKey ).  Should not be used on containers that have multiple values for one key.";
                    throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
                }
                else
                {
                    kv = p_kvi;
                    found = true;
                }
            }
        }
        return kv;
    }

    template<class Key, class KeyValue, class Iterator_t>
    void BaseKeyValueContainer<Key, KeyValue, Iterator_t>::Set( KeyValueInternal* pInternal )
    {
        bool found = false;
        for( int i = 0; i < m_Vector.size(); ++i )
        {
            if( m_Vector[ i ]->GetProperty() == pInternal->GetProperty() )
            {
                if( found )
                {
                    std::ostringstream ss;
                    ss << "Illegal use of IPKeyValueContainer::Set( const IPKeyValue& rKeyValue ).  Should not be used on containers that have multiple values for one key.";
                    throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
                }
                else
                {
                    m_Vector[ i ] = pInternal;
                    found = true;
                }
            }
        }
        if( !found )
        {
            m_Vector.push_back( pInternal );
        }
    }

    template<class Key, class KeyValue, class Iterator_t>
    KeyValue BaseKeyValueContainer<Key, KeyValue, Iterator_t>::Get( const std::string& rKeyValueString ) const
    {
        for( auto p_kvi : m_Vector )
        {
            if( p_kvi->GetKeyValueString() == rKeyValueString )
            {
                return KeyValue( p_kvi );
            }
        }
        std::stringstream ss;
        ss << "Cound not find '" << rKeyValueString << "'";
        throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
    }

    template<class Key, class KeyValue, class Iterator_t>
    bool BaseKeyValueContainer<Key, KeyValue, Iterator_t>::Contains( const std::string& rKeyValueString ) const
    {
        for( auto p_kvi : m_Vector )
        {
            if( p_kvi->GetKeyValueString() == rKeyValueString )
            {
                return true;
            }
        }
        return false;
    }

    template<class Key, class KeyValue, class Iterator_t>
    bool BaseKeyValueContainer<Key, KeyValue, Iterator_t>::Contains( const KeyValue& rKeyValue ) const
    {
        return Contains( rKeyValue.m_pInternal->GetKeyValueString() );
    }

    template<class Key, class KeyValue, class Iterator_t>
    bool BaseKeyValueContainer<Key, KeyValue, Iterator_t>::Contains( const Key& rKey ) const
    {
        for( auto p_kvi : m_Vector )
        {
            if( rKey.m_pIP == p_kvi->GetProperty() )
            {
                return true;
            }
        }
        return false;
    }

    template<class Key, class KeyValue, class Iterator_t>
    std::string BaseKeyValueContainer<Key, KeyValue, Iterator_t>::GetValuesToString() const
    {
        std::stringstream ss;
        for( auto p_pa : m_Vector )
        {
            ss << p_pa->GetValueAsString() << ", ";
        }
        std::string ret_str = ss.str();
        ret_str = ret_str.substr( 0, ret_str.length() - 2 );
        return ret_str;
    }

    template<class Key, class KeyValue, class Iterator_t>
    std::set<std::string> BaseKeyValueContainer<Key, KeyValue, Iterator_t>::GetValuesToStringSet() const
    {
        std::set<std::string> str_set;
        for( auto p_pa : m_Vector )
        {
            str_set.insert( p_pa->GetValueAsString() );
        }
        return str_set;
    }

    template<class Key, class KeyValue, class Iterator_t>
    std::list<std::string> BaseKeyValueContainer<Key, KeyValue, Iterator_t>::GetValuesToList() const
    {
        std::list<std::string> list;
        for( auto p_pa : m_Vector )
        {
            list.push_back( p_pa->GetValueAsString() );
        }
        return list;
    }

    template<class Key, class KeyValue, class Iterator_t>
    std::string BaseKeyValueContainer<Key, KeyValue, Iterator_t>::ToString() const
    {
        std::string ret_str;
        for( auto p_pa : m_Vector )
        {
            ret_str += p_pa->GetKeyValueString() + std::string( PROP_SEPARATOR );
        }
        if( !ret_str.empty() )
        {
            ret_str = ret_str.substr( 0, ret_str.length() - 1 );
        }
        return ret_str;
    }

    template<class Key, class KeyValue, class Iterator_t>
    void BaseKeyValueContainer<Key, KeyValue, Iterator_t>::Clear()
    {
        m_Vector.clear();
    }

    template<class Key, class KeyValue, class Iterator_t>
    KeyValueInternal* BaseKeyValueContainer<Key, KeyValue, Iterator_t>::FindFirst( const BaseKeyValueContainer<Key, KeyValue, Iterator_t>& rThatContainer ) const
    {
        for( auto p_that_kvi : rThatContainer.m_Vector )
        {
            auto this_it = m_Vector.begin();
            auto this_end = m_Vector.end();
            while( this_it != this_end )
            {
                if( *this_it == p_that_kvi )
                {
                    return p_that_kvi;
                }
                ++this_it;
            }
        }
        return nullptr; // not found
    }

    template<class Key, class KeyValue, class Iterator_t>
    void BaseKeyValueContainer<Key, KeyValue, Iterator_t>::serialize( IArchive& ar, BaseKeyValueContainer<Key, KeyValue, Iterator_t>& container )
    {
        std::vector<std::string> string_vec;
        if( ar.IsWriter() )
        {
            for( auto kv : container.m_Vector )
            {
                string_vec.push_back( kv->GetKeyValueString() );
            }
        }

        ar & string_vec;

        if( ar.IsReader() )
        {
            for( auto kv_str : string_vec )
            {
                KeyValue kv( kv_str );
                container.Add( kv.m_pInternal );
            }
        }
    }

    // ------------------------------------------------------------------------
    // --- BaseProperty
    // ------------------------------------------------------------------------

    template<class Key>
    Key BaseProperty::GetKey() const
    {
        return Key( m_Key );
    }

    template<class Container>
    Container BaseProperty::GetValues() const
    {
        Container container( m_Values );
        return container;
    }

    template<class KeyValue>
    KeyValue BaseProperty::GetInitialValue( uint32_t externalNodeId, RANDOMBASE* pRNG )
    {
        float ran = -1.0f;
        float prob = 0.0;

        for( auto kvi : m_Values )
        {
            float dis = kvi->m_InitialDistributions[ externalNodeId ];

            // -------------------------------------------------------------
            // --- The following if-else is an attempt to avoid calling RNG
            // --- if the distribution is set to 1.0 for one value.
            // -------------------------------------------------------------
            if( dis == 0.0f )
            {
                continue;
            }
            else if( dis == 1.0f )
            {
                return KeyValue( kvi );
            }
            else if( ran == -1.0f )
            {
                ran = pRNG->e();
            }

            prob += dis;
            if( prob >= ran )
            {
                return KeyValue( kvi );
            }
        }
        std::ostringstream msg;
        msg << "Was not able to select an initial value for Property = " << m_Key;
        throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
    }

    // ------------------------------------------------------------------------
    // --- BaseFactory
    // ------------------------------------------------------------------------

    template<class Container>
    std::vector<std::string> BaseFactory::GetAllPossibleKeyValueCombinations() const
    {
        std::vector<std::string> possible_list;
        if( m_IPList.size() > 0 )
        {
            Container container = m_IPList[ 0 ]->GetValues<Container>();
            for( auto kv : container )
            {
                std::string kv_str = kv.ToString();
                possible_list.push_back( kv_str );
            }
        }
        for( int i = 1; i < m_IPList.size(); i++ )
        {
            std::vector<std::string> new_list;
            Container container = m_IPList[ i ]->GetValues<Container>();
            for( auto kv : container )
            {
                std::string kv_str = kv.ToString();
                for( auto pv : possible_list )
                {
                    std::string new_value = pv + std::string( PROP_SEPARATOR ) + kv_str;
                    new_list.push_back( new_value );
                }
            }
            possible_list = new_list;
        }
        return possible_list;
    }

    template<class Container>
    KeyValueInternal* BaseFactory::GetKeyValue( const char* ip_key_str, const std::string& rKeyValueString, const std::string& rParameterName )
    {
        if( m_KeyValueMap.count( rKeyValueString ) == 0 )
        {
            std::string keyStr, valueStr;
            BaseFactory::ParseKeyValueString( ip_key_str, rKeyValueString, keyStr, valueStr );
            BaseProperty* pip = GetIP( keyStr, rParameterName, false );
            std::ostringstream msg;
            if( !rParameterName.empty() )
            {
                msg << "Parameter '" << rParameterName << "' is invalid.  ";
            }
            if( pip == nullptr )
            {
                std::string keys = BaseFactory::GetKeysAsString();
                msg << "Could not find the key(" << keyStr << ") for the key-value=" << rKeyValueString << ".  Possible keys are: " << keys;
            }
            else
            {
                std::string values = pip->GetValues<Container>().GetValuesToString();
                msg << "Could not find the value(" << valueStr << ") for the key(" << keyStr << ").  Possible values for the key are: " << values;
            }
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }

        return m_KeyValueMap.at( rKeyValueString );
    }
}
