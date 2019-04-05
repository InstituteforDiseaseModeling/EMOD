/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <map>
#include <list>
#include <set>
#include <string>
#include <vector>
#include "JsonObjectDemog.h"
#include "IdmApi.h"
#include "ISerializable.h"
#include "IArchive.h"
#include "Types.h"

#define KEY_VALUE_SEPARATOR_CHAR (':')
#define KEY_VALUE_SEPARATOR (":")
#define PROP_SEPARATOR (",")

// Properties allow the DTK user to assign attributes to individuals/nodes that are not
// built in.  These properties are "sprayed on" so that the user can change them depending
// on their needs.  Properties are defined in the demographics data and can be
// targeted in campaign events.
//
// BaseProperties is the foundation of the implementation for Individual and Node Properties.
// To keep things small, this design avoided virtual methods in the key, value, and container
// and instead made use of templates.
//
// The implementation for the templates can befound in BasePropertiesTemplates.h.  They were
// put in that header so that the could be included in Properties.cpp and NodeProperties.cpp.
namespace Kernel
{
    class RANDOMBASE;
    class BaseProperty;
    class BaseFactory;

    // ------------------------------------------------------------------------
    // --- KeyValueInternal
    // ------------------------------------------------------------------------

    class KeyValueInternal
    {
    public:
        inline const std::string& GetKeyValueString() const
        {
            return m_KeyValueString;
        }

        inline const std::string& GetValueAsString() const
        {
            return m_Value;
        }

        inline float GetInitialDistribution( uint32_t externalNodeId ) const
        {
            return m_InitialDistributions.at( externalNodeId );
        }

        inline void SetInitialDistribution( uint32_t externalNodeId, float value )
        {
            m_InitialDistributions[ externalNodeId ] = value;
        }

        inline const BaseProperty* GetProperty() const
        {
            return m_pIP;
        }

    protected:
        friend class BaseProperty;
        friend class IndividualProperty;
        friend class NodeProperty;

        KeyValueInternal( BaseProperty* pip,
                          const std::string& rValue,
                          uint32_t externalNodeId,
                          const ProbabilityNumber& rInitialDist );

        BaseProperty* m_pIP;
        std::string m_KeyValueString;
        std::string m_Value;
        std::map<uint32_t,float> m_InitialDistributions;
    };

    // An BaseKey is the key or name of the Property (i.e. Risk).
    // Instances of BaseKey are ensured to be valid keys/names to Properties
    // or not valid/null because they have not been set to a valid entry.
    // If the validity of the variable is in question, one should use the IsValid() method.
    class IDMAPI BaseKey
    {
    public:
        // Return the key/name of the property as a string
        const std::string& ToString() const;

        // Return true if this is a valid key
        bool IsValid() const;

        // Return the external name associated with this key
        const std::string& GetParameterName() const;

        // Set the parameter name associated with this key.  The name is used in error reporting.
        void SetParameterName( const std::string& rParameterName );

    protected:
        template<class Key, class KeyValue, class Iterator> friend class BaseKeyValueContainer;

        // Create a holder for a key.  If one attempts to use the object
        // before it is assigned key data, it will throw an exception
        BaseKey();

        ~BaseKey();

        explicit BaseKey( const BaseProperty* pip );

        bool operator==( const BaseKey& rThat ) const;
        bool operator!=( const BaseKey& rThat ) const;

        typedef std::function<void( BaseKey* pkv, const std::string& rKeyStr )> k_assignment_function_t;
        static void serialize( IArchive& ar, BaseKey& obj, k_assignment_function_t assign_func );

#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        const BaseProperty* m_pIP;
        std::string m_ParameterName;
#pragma warning( pop )
    };

    // An BaseKeyValue represents a Property key and value pair
    // where the value is one of the possible values for the given key.
    // Instances of BaseKeyValue are ensured to be valid entry or are not valid
    // because the have not been set.  One uses the IsValid() method to check
    // if the instance has been set to a valid entry.
    class IDMAPI BaseKeyValue
    {
    public:
        // Return the "key:value" as a string
        const std::string& ToString() const;

        // Return true if this is a valid key-value object
        bool IsValid() const;

        // Return the external name associated with this key-value pair
        const std::string& GetParameterName() const;

        // Set the parameter name associated with this key-value pair.  The name is used in error reporting.
        void SetParameterName( const std::string& rParameterName );

        // Return the key associated with this key-value pair.
        template<class Key>
        Key GetKey() const;

        // Return the property name or key as a string
        const std::string& GetKeyAsString() const;

        // Return the value associated with this key-value pair
        const std::string& GetValueAsString() const;

        void UpdateInitialDistribution( uint32_t externalNodeId, double value );

    protected:
        template<class Key, class KeyValue, class Iterator> friend class BaseKeyValueContainer;

        typedef std::function< void ( BaseKeyValue* pkv, const std::string& rKvStr) > kv_assignment_function_t;
        static void serialize( IArchive& ar, BaseKeyValue& obj, kv_assignment_function_t assign_func );

        // Create a holder for a key-value.  If one attempts to use the object
        // before it is assigned key-value data, it will throw an exception
        BaseKeyValue();

        ~BaseKeyValue();

        BaseKeyValue( KeyValueInternal* pkvi );

        bool operator==( const BaseKeyValue& rThat ) const;
        bool operator!=( const BaseKeyValue& rThat ) const;

#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        KeyValueInternal* m_pInternal;
        std::string m_ParameterName;
#pragma warning( pop )
    };

    // An iterator class used for traversing the elements in BaseKeyValueContainer.
    template<class KeyValue>
    class IDMAPI BaseIterator
    {
    public:
        const KeyValue& operator*();

    protected:
        template<class Key, class KeyValue2, class Iterator> friend class BaseKeyValueContainer;

        BaseIterator( std::vector<KeyValueInternal*>::const_iterator it );
        ~BaseIterator();

        bool operator==( const BaseIterator<KeyValue>& rThat ) const;
        bool operator!=( const BaseIterator<KeyValue>& rThat ) const;
        BaseIterator& operator++(); //prefix

#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        std::vector<KeyValueInternal*>::const_iterator m_Iterator;
        KeyValue m_KeyValue; //reused to reduce lots of object creation
#pragma warning( pop )
    };


    // An BaseKeyValueContainer is used to manage a collection of BaseKeyValue objects.
    // For example, an Individual has an IPKeyValueContainer to manage the properties 
    // specific to that individual.
    template<class Key, class KeyValue,class Iterator_t>
    class IDMAPI BaseKeyValueContainer
    {
    public:
        // Return an iterator to the first element in the list
        Iterator_t begin() const;

        // Return an terator to the last element in the list
        Iterator_t end() const;

        KeyValue Get( const Key& rKey ) const;

        KeyValue Get( const std::string& rKeyValueString ) const;

        // Returns true if the container contains this key-value pair
        bool Contains( const std::string& rKeyValueString ) const;

        // Returns true if the container contains this key-value pair
        bool Contains( const KeyValue& rKeyValue ) const;

        // Returns true if the container contains a key-value pair with this key
        bool Contains( const Key& rKey ) const;

        // Returns the number of key-value pairs in the container
        size_t Size() const { return m_Vector.size(); }

        // Returns a string with list of the values in the container.  Used in reporting configuration errors.
        std::string GetValuesToString() const;

        // Returs a set of strings with the values in the container.  Used in reporting configuration errors.
        std::set<std::string> GetValuesToStringSet() const;

        // Returns a list of strings with the values in the container.  Used in IntraNodeTransmission
        std::list<std::string> GetValuesToList() const;

        // Returns a string with the list of key-value pairs in the container.  Used in reporting configuration errors.
        std::string ToString() const;

        // Delete all of the elements in the container
        void Clear();

        static void serialize( IArchive& ar, BaseKeyValueContainer<Key, KeyValue, Iterator_t>& obj );

    protected:

        BaseKeyValueContainer();
        BaseKeyValueContainer( const BaseKeyValueContainer& rThat );
        ~BaseKeyValueContainer();

        BaseKeyValueContainer( const std::vector<KeyValueInternal*>& rInternalList );

        BaseKeyValueContainer& operator=( const BaseKeyValueContainer& rThat );

        bool operator==( const BaseKeyValueContainer& rThat ) const;
        bool operator!=( const BaseKeyValueContainer& rThat ) const;

        void Set( KeyValueInternal* pInternal );

        void Add( KeyValueInternal* pInternal );
        void Remove( const KeyValueInternal* pInternal );

        KeyValueInternal* FindFirst( const BaseKeyValueContainer& rContainer ) const;

#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        std::vector<KeyValueInternal*> m_Vector;
#pragma warning( pop )
    };

    // The BaseProperty object represents a single Property.  It contains everything 
    // that relates to a single property: name, values, transitions, and transmissions.
    class IDMAPI BaseProperty
    {
    public:
        BaseProperty();
        virtual ~BaseProperty();

        // Read the one Property from demographics
        virtual void Read( int idx,
                           uint32_t externalNodeId,
                           const JsonObjectDemog& rDemog,
                           bool isNotFirstNode );

        // Return the key / name of this property
        template<class Key>
        Key GetKey() const;

        //Return the key / name as a string
        inline const std::string& GetKeyAsString() const { return m_Key; }

        // Return a randomly selected value based on the initial distribution values
        template<class KeyValue>
        KeyValue GetInitialValue( uint32_t externalNodeId, RANDOMBASE* pRNG );

        // Return a container that has all of the values
        template<class Container>
        Container GetValues() const;

    protected:
        friend class BaseFactory;
        friend class BaseKey;
        friend class KeyValueInternal;

        static bool Compare( BaseProperty* pLeft, 
                             BaseProperty* pRight );

        BaseProperty( BaseFactory* pFactory, 
                          uint32_t externalNodeId, 
                          const std::string& rKeyStr, 
                          const std::map<std::string,float>& rValues );

    protected:
        virtual void ReadProperty( int idx, 
                                   uint32_t externalNodeId, 
                                   const JsonObjectDemog& rDemog, 
                                   bool isNotFirstNode ) = 0;

        typedef std::function< KeyValueInternal* ( BaseFactory* pFact,
                                                   const char* ip_key_str,
                                                   const std::string& rKvStr ) > get_kvi_function_t;

        void ReadProperty( const char* ip_key_str,
                           const char* ip_values_key_str,
                           const char* ip_init_key_str,
                           BaseFactory* ip_factory,
                           int idx,
                           uint32_t externalNodeId,
                           const JsonObjectDemog& rDemog,
                           bool isNotFirstNode,
                           get_kvi_function_t get_kvi_func );

        static bool Contains( const std::vector<KeyValueInternal*>& rValueList,
                              const std::string& rKey,
                              const std::string& rNewValue );

#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        std::string  m_Key;
        std::vector<KeyValueInternal*> m_Values;
#pragma warning( pop )
    };


    // The BaseFactory is a singleton that manages all of the Properties 
    // in the simulation.  It is responsable for ensuring that there is only one
    // of each.  The instance of IPFactory/NPFactory is contained in Environment so that
    // DLL's will have access to the singleton.
    // This design assumes that there is only one valid set of Properties
    // for all Nodes.
    class IDMAPI BaseFactory
    {
    public:

        // Helper function for creating key-value strings
        static std::string CreateKeyValueString( const std::string& rKeyStr, const std::string& rValueStr );

        // Returns a set of the possible keys.  Used with ConstrainedString in places 
        // where a key or other value (i.e. NONE) is allowed.
        std::set< std::string > GetKeysAsStringSet() const;

        // Returns a set of possible keys in a delimited string.  Used in error messages.
        std::string GetKeysAsString() const;

        // Given the set of keys and their values return a list of strings containing all of the
        // possible combinations where all keys must be present.  Used in PropertyReport.
        template<class Container>
        std::vector<std::string> GetAllPossibleKeyValueCombinations() const;

    protected:
        friend class KeyValueInternal;
        friend class BaseKey;
        friend class BaseKeyValue;
        friend class BaseProperty;

        static void ParseKeyValueString( const char* ip_key_str,
                                         const std::string& rKeyValueStr,
                                         std::string& rKeyStr,
                                         std::string& rValueStr );

        BaseFactory();
       ~BaseFactory();

        typedef std::function< BaseProperty* ( int idx, 
                                               uint32_t externalNodeId,
                                               const JsonObjectDemog& rDemog,
                                               bool isNotFirstNode )> read_function_t;

        void Initialize( const char* ip_key_str,
                         const char* ip_name_key_str,
                         read_function_t read_func,
                         uint32_t node_id,
                         const JsonObjectDemog& rDemog, 
                         bool isWhitelistEnabled );

        // If the White List is enabled, this will throw an exception if the given key
        // is not one of the approved keys/properties.
        void CheckIpKeyInWhitelist( const char* ip_key, const std::string& rKey, int numValues );

        // Add this new/unique Key-Value pair to the possible set
        // It throws an exception if the "key:value" already exists.
        void AddKeyValue( KeyValueInternal* pkvi );

        // Return the internal representation of the "key:value" pair.
        // It throws an exception if not found
        template<class Container>
        KeyValueInternal* GetKeyValue( const char* ip_key_str,
                                       const std::string& rKeyValueString, 
                                       const std::string& rParameterName=std::string("") );

        // Return the Property of the whose key/name is given
        BaseProperty* GetIP( const std::string& rKey, 
                             const std::string& rParameterName = std::string( "" ),
                             bool throwOnNotFound = true );

        typedef std::function< BaseProperty* ( uint32_t externalNodeId,
                                               const std::string& rKeyStr, 
                                               const std::map<std::string, float>& rValues ) > ip_construct_function_t;

        // Add a new Property using the key and set of values
        void AddIP( uint32_t externalNodeId, 
                    const std::string& rKey, 
                    const std::map<std::string, float>& rValues,
                    ip_construct_function_t create_ip );

        void CheckForDuplicateKey( const std::string& rKeyStr );

        uint32_t m_ExternalNodeIdOfFirst;
        bool m_WhiteListEnabled;
        std::vector<BaseProperty*> m_IPList;
        std::map<std::string,KeyValueInternal*> m_KeyValueMap; // should contain all of the KeyValueInternal objects for properties
        std::set<std::string> m_KeyWhiteList;
    };
}
