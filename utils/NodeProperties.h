/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "BaseProperties.h"

// Node Properties (NP) allow the DTK user to assign attributes to nodes that are not
// built in.  NP applies the BaseProperties feature to nodes.  See BaseProperties.h for more info.
namespace Kernel
{
    class NodeProperty;

    // An NPKey is the key or name of an Node Property (i.e. Risk).
    class IDMAPI NPKey : public BaseKey
    {
    public:
        static const char* GetConstrainedStringConstraintKey();
        static const char* GetConstrainedStringConstraintValue();
        static const char* GetConstrainedStringConstraintKeyValue();
        static const char* GetConstrainedStringDescriptionKey();
        static const char* GetConstrainedStringDescriptionValue();

        // Create a holder for a key.  If one attempts to use the object
        // before it is assigned key data, it will throw an exception
        NPKey();

        ~NPKey();

        // Create a key from the given string and validate it
        explicit NPKey( const std::string& rKeyStr );

        // Set this object to one that has this key and validate the key / name / property exists
        NPKey& operator=( const std::string& rKeyStr );

        bool operator==( const NPKey& rThat ) const;
        bool operator!=( const NPKey& rThat ) const;

        static void serialize( IArchive& ar, NPKey& obj );

    protected:
        friend class NPKeyValueContainer;
        friend class BaseKeyValue;

        explicit NPKey( const BaseProperty* pnp );
    };

    // An NPKeyValue represents a Node Property key and value 
    // where the value is one of the possible values for the given key.
    class IDMAPI NPKeyValue : public BaseKeyValue
    {
    public:
        // Create a holder for a key-value.  If one attempts to use the object
        // before it is assigned key-value data, it will throw an exception
        NPKeyValue();

        ~NPKeyValue();

        // Create a new key-value instance and validate that this is
        // a known key-value pair.  This format is "key:value".
        explicit NPKeyValue( const std::string& rKeyValueStr );

        NPKeyValue( const std::string& rKeyStr, const std::string& rValueStr );

        // Set this object equal to the given one
        NPKeyValue& operator=( const std::string& rKeyValueStr );

        bool operator==( const NPKeyValue& rThat ) const;
        bool operator!=( const NPKeyValue& rThat ) const;

        // Return the initial distribution of individuals that should 
        // receive this property value for this property key
        double GetInitialDistribution() const;

        static void serialize( IArchive& ar, NPKeyValue& obj );

    protected:
        friend class BaseProperty;
        friend class NPFactory;
        friend class NodeProperty;
        friend class NPKeyValueContainer;
        template<class KeyValue> friend class BaseIterator;
        template<class Key, class KeyValue, class Iterator_t> friend class BaseKeyValueContainer;

        NPKeyValue( KeyValueInternal* pkvi );
    };

    // An iterator class used for traversing the elements in NPKeyValueContainer
    class IDMAPI NPKeyValueIterator : public BaseIterator<NPKeyValue>
    {
    public:
        friend class NPKeyValueContainer;
        template<class Key, class KeyValue, class Iterator_t> friend class BaseKeyValueContainer;

        ~NPKeyValueIterator();
        bool operator==( const NPKeyValueIterator& rThat ) const;
        bool operator!=( const NPKeyValueIterator& rThat ) const;
        NPKeyValueIterator& operator++(); //prefix

    private:
        NPKeyValueIterator( std::vector<KeyValueInternal*>::const_iterator it );
    };

    // An NPKeyValueContainer is used to manage a collection of NPKeyValue objects.
    // For example, a Node has an NPKeyValueContainer to manage the properties 
    // specific to that node.
    class IDMAPI NPKeyValueContainer : public BaseKeyValueContainer<NPKey, NPKeyValue, NPKeyValueIterator>
    {
    public:
        NPKeyValueContainer();
        NPKeyValueContainer( const NPKeyValueContainer& rThat );
        ~NPKeyValueContainer();

        NPKeyValueContainer& operator=( const NPKeyValueContainer& rThat );

        bool operator==( const NPKeyValueContainer& rThat ) const;
        bool operator!=( const NPKeyValueContainer& rThat ) const;

        void Set( const NPKeyValue& rKeyValue );

        void Add( const NPKeyValue& rKeyValue );
        void Remove( const NPKeyValue& rKeyValue );

        NPKeyValue FindFirst( const NPKeyValueContainer& rContainer ) const;

    protected:
        friend class BaseProperty;

        NPKeyValueContainer( const std::vector<KeyValueInternal*>& rInternalList );
    };

    // An NodeProperty object represents a single Node Property.  It contains everything 
    // that relates to a single property: name and values.
    class IDMAPI NodeProperty : public BaseProperty
    {
    public:
        NodeProperty();
        virtual ~NodeProperty();

    protected:
        friend class NPFactory;

        virtual void ReadProperty( int idx, 
                                   uint32_t externalNodeId, 
                                   const JsonObjectDemog& rDemog, 
                                   bool isNotFirstNode );

        static KeyValueInternal* get_kvi_func( BaseFactory* pFact, const char* ip_key_str, const std::string& rKvStr );

        NodeProperty( const std::string& rKeyStr, const std::map<std::string,float>& rValues );
    };


    // The NPFactory is a singleton that manages all of the Node Properties 
    // in the simulation.  It is responsable for ensuring that there is only one
    // of each.  The instance of NPFactory is contained in Environment so that
    // DLL's will have access to this singleton.
    // This design assumes that there is only one valid set of Node Properties
    // for all Nodes.
    class IDMAPI NPFactory : public BaseFactory
    {
    public:
        static const char* NODE_PROPERTIES_JSON_KEY;

        // Helper function for parsing the key-value string into its individual pieces
        static void ParseKeyValueString( const std::string& rKeyValueStr, std::string& rKeyStr, std::string& rValueStr );

        // Must be called before any NP related classes can be used
        static void CreateFactory();

        // Mostly used in testing to get rid of the factory
        static void DeleteFactory();

        // Returns the single instance of the factory.  It will throw an exception
        // if CreateFactory() has not been called.
        static NPFactory* GetInstance();

        // Initializes / reads the Node Property information from the demographics.
        void Initialize( const JsonObjectDemog& rDemog, bool isWhitelistEnabled );

        // This method returns the list of Node Properties defined in the simulation.
        std::vector<NodeProperty*> GetNPList() const;

        // Return an initial set of values for a node.  There should be one value for each key/property.
        // The values are determined by the initial distribution parameters set in the demographics data.
        // The values will be set randomly.
        NPKeyValueContainer GetInitialValues( RANDOMBASE* pRNG, const JsonObjectDemog& rNodeJson );

        // Return the Node Property of the whose key/name is given
        NodeProperty* GetNP( const std::string& rKey, const std::string& rParameterName=std::string(""), bool throwOnNotFound=true );

        // Add a new NP using the key and set of values
        void AddNP( const std::string& rKey, const std::map<std::string,float>& rValues );

    protected:
        friend class NPKeyValue;
        friend class NodeProperty;

        NPFactory();
        ~NPFactory();

        // Read the NodePropertyValues from the json for a specific node
        NPKeyValueContainer ReadPropertyValues( const JsonObjectDemog& rNodeJson );

        static BaseProperty* construct_np( uint32_t externalNodeId,
                                               const std::string& rKeyStr,
                                               const std::map<std::string, float>& rValues );
    };
}
