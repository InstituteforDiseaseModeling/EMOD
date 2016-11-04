/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

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

class RANDOMBASE;

// Individual Properties allow the DTK user to assign attributes to individuals that are not
// built in.  These properties are "sprayed on" so that the user can change them depending
// on their needs.  Individual Properties are defined in the demographics data and can be
// targeted in campaign events.
namespace Kernel
{
    class IndividualProperty;
    class IPKeyValueInternal;

    // An IPKey is the key or name of an Individual Property (i.e. Risk).
    // Instances of IPKey are ensured to be valid keys/names to Individual Properties
    // or not valid/null because they have not been set to a valid entry.
    // If the validity of the variable is in question, one should use the IsValid() method.
    class IDMAPI IPKey
    {
    public:
        // Create a holder for a key.  If one attempts to use the object
        // before it is assigned key data, it will throw an exception
        IPKey();

        ~IPKey();

        // Create a key from the given string and validate it
        explicit IPKey( const std::string& rKeyStr );

        // Return the key/name of the property as a string
        const std::string& ToString() const;

        // Set this object to one that has this key and validate the key / name / property exists
        IPKey& operator=( const std::string& rKeyStr );

        // Return true if this is a valid key
        bool IsValid() const;

        bool operator==( const IPKey& rThat ) const;
        bool operator!=( const IPKey& rThat ) const;

        // Return the external name associated with this key
        const std::string& GetParameterName() const;

        // Set the parameter name associated with this key.  The name is used in error reporting.
        void SetParameterName( const std::string& rParameterName );

    protected:
        friend class IPKeyValue;
        explicit IPKey( IndividualProperty* pip );

    private:
#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        IndividualProperty* m_pIP;
        std::string m_ParameterName;
#pragma warning( pop )
    };

    // An IPKeyValue represents an Individual Property key and value 
    // where the value is one of the possible values for the given key.
    // Instances of IPKeyValue are ensured to be valid entry or are not valid
    // because the have not been set.  One uses the IsValid() method to check
    // if the instance has been set to a valid entry.
    class IDMAPI IPKeyValue
    {
    public:
        // Create a holder for a key-value.  If one attempts to use the object
        // before it is assigned key-value data, it will throw an exception
        IPKeyValue();

        ~IPKeyValue();

        // Create a new key-value instance and validate that this is
        // a known key-value pair.  This format is "key:value".
        explicit IPKeyValue( const std::string& rKeyValueStr );

        IPKeyValue( const std::string& rKeyStr, const std::string& rValueStr );

        // Return the "key:value" as a string
        const std::string& ToString() const;

        // Set this object equal to the given one
        IPKeyValue& operator=( const std::string& rKeyValueStr );

        bool operator==( const IPKeyValue& rThat ) const;
        bool operator!=( const IPKeyValue& rThat ) const;

        // Return true if this is a valid key-value object
        bool IsValid() const;

        // Return the external name associated with this key-value pair
        const std::string& GetParameterName() const;

        // Set the parameter name associated with this key-value pair.  The name is used in error reporting.
        void SetParameterName( const std::string& rParameterName );

        // Return the key associated with this key-value pair.
        IPKey GetKey() const;

        // Return the value associated with this key-value pair
        std::string GetValueAsString() const;

        // Return the initial distribution of individuals that should 
        // receive this property value for this property key
        double GetInitialDistribution( uint32_t externalNodeId ) const;

        void UpdateInitialDistribution( uint32_t externalNodeId, double value );

    protected:
        friend class IndividualProperty;
        IPKeyValue( IPKeyValueInternal* pkvi );

    private:
#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        IPKeyValueInternal* m_pInternal;
        std::string m_ParameterName;
#pragma warning( pop )
    };

    // An IPTransition is an event where individuals can have the value of
    // their Individual Property changed.  An IPTransition is converted to
    // a campaign event and added to the simulation's other events.
    class IDMAPI IPTransition
    {
    public:
        IPTransition();
        IPTransition( const IPKeyValue& rFrom,
                      const IPKeyValue& rTo,
                      const std::string& rType,
                      float coverage,
                      float startDay,
                      float durationDays,
                      float probability,
                      float revision,
                      float ageYears,
                      bool hasAgeConstraint,
                      float minAgeYears,
                      float maxAgeYears );

        ~IPTransition();

        // Read/extract an Individual Property Transition from the demographics
        void Read( int idx, int itran, const JsonObjectDemog& rDemog, const std::string& rKeyStr );

        // Return a json object in CampaignEvent format
        std::vector<JsonObjectDemog> ConvertToCampaignEvent( const IPKey& rKey );

        void Validate( const JsonObjectDemog& rDemog );

    private:
#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        IPKeyValue  m_From;
        IPKeyValue  m_To;
        std::string m_Type;
        float       m_Coverage;
        float       m_Start;
        float       m_Duration;
        float       m_Probability;
        float       m_Revision;
        bool        m_HasAgeConstraint;
        float       m_MinAgeYears;
        float       m_MaxAgeYears;
        float       m_AgeYears;
#pragma warning( pop )
    };

    // An IPKeyValueContainer is used to manage a collection of IPKeyValue objects.
    // For example, an Individual has an IPKeyValueContainer to manage the properties 
    // specific to that individual.
    class IDMAPI IPKeyValueContainer
    {
    public:
        IPKeyValueContainer();
        ~IPKeyValueContainer();

        // An iterator class used for traversing the elements in this container
        class Iterator
        {
        public:
            friend class IPKeyValueContainer;

            ~Iterator();
            bool operator==( const Iterator& rThat ) const;
            bool operator!=( const Iterator& rThat ) const;
            Iterator& operator++(); //prefix
            IPKeyValue operator*();
        private:
            // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            // !!! Ideally, we would just use the map for the container, but to keep from changing the results,
            // !!! we also use a vector to maintain the order of the values like in the old version.
            // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            //Iterator( std::map<std::string,IPKeyValue>::const_iterator it );
            //std::map<std::string,IPKeyValue>::const_iterator m_Iterator;

            Iterator( std::vector<IPKeyValue>::const_iterator it );
            std::vector<IPKeyValue>::const_iterator m_Iterator;

        };
        bool operator==( const IPKeyValueContainer& rThat ) const;
        bool operator!=( const IPKeyValueContainer& rThat ) const;

        // Return an iterator to the first element in the list
        Iterator begin() const;

        // Return an terator to the last element in the list
        Iterator end() const;

        //IPKeyValue Get( const std::string& rKeyStr ) const;
        IPKeyValue Get( const IPKey& rKey ) const;
        IPKeyValue Get( const std::string& rKeyValueString ) const;

        void Set( const IPKeyValue& rKeyValue );

        void Add( const IPKeyValue& rKeyValue );
        void Remove( const IPKeyValue& rKeyValue );

        // Returns true if the container contains this key-value pair
        bool Contains( const std::string& rKeyValueString ) const;
        bool Contains( const IPKeyValue& rKeyValue ) const;

        // Returns true if the container contains a key-value pair with this key
        bool Contains( const IPKey& rKey ) const;

        // Returns the number of key-value pairs in the container
        size_t Size() const { return m_Map.size(); }

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
    private:
#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        std::map<std::string,IPKeyValue> m_Map;
        std::vector<IPKeyValue> m_Vector; // !!! TEMPORARY.  Only needed to keep current order and match current results!!!
#pragma warning( pop )
    };

    class IDMAPI IPIntraNodeTransmissions
    {
    public:
        IPIntraNodeTransmissions();
        ~IPIntraNodeTransmissions();

        void Read( const std::string& rKeyStr, const JsonObjectDemog& rDemog, int numValues );

        const std::string& GetRouteName() const;
        bool HasMatrix() const;
        const std::vector<std::vector<float>>& GetMatrix() const;
    private:
#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        std::string m_RouteName;
        std::vector<std::vector<float>> m_Matrix;
#pragma warning( pop )
    };

    // An IP object represents a single Individual Property.  It contains everything 
    // that relates to a single property: name, values, transitions, and transmissions.
    class IDMAPI IndividualProperty
    {
    public:
        IndividualProperty();
        ~IndividualProperty();

        // Read the one Individual Property from demographics
        void Read( int idx, uint32_t externalNodeId, const JsonObjectDemog& rDemog, bool isNotFirstNode );

        // Return the key / name of this property
        IPKey GetKey() const;

        // Return the container that has all of the values
        const IPKeyValueContainer& GetValues() const;

        // Return a randomly selected value based on the initial distribution values
        IPKeyValue GetInitialValue( uint32_t externalNodeId, RANDOMBASE* pRNG );

        const IPIntraNodeTransmissions& GetIntraNodeTransmissions( uint32_t externalNodeId ) const;

    protected:
        friend class IPFactory;
        friend class IPKey;
        friend class IPKeyValueInternal;

        static bool Compare( IndividualProperty* pLeft, IndividualProperty* pRight );

        IndividualProperty( uint32_t externalNodeId, const std::string& rKeyStr, const std::map<std::string,float>& rValues );

        const std::string& GetKeyAsString() const { return m_Key; }

        std::vector<JsonObjectDemog> ConvertTransitions();

    private:
        void ReadProperty( int idx, uint32_t externalNodeId, const JsonObjectDemog& rDemog, bool isNotFirstNode );
        void ReadPropertyAgeBin( int idx, uint32_t externalNodeId, const JsonObjectDemog& rDemog, bool isNotFirstNode );
        void CreateAgeBinTransitions();

#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        std::string  m_Key;
        IPKeyValueContainer  m_Values;
        std::vector<IPTransition*> m_Transitions;
        std::map<uint32_t,IPIntraNodeTransmissions*> m_IntraNodeTransmissionsMap;
#pragma warning( pop )
    };


    // The IPFactory is a singleton that manages all of the Individual Properties 
    // in the simulation.  It is responsable for ensuring that there is only one
    // of each.  The instance of IPFactory is contained in Environment so that
    // DLL's will have access to this singleton.
    // This design assumes that there is only one valid set of Individual Properties
    // for all Nodes.
    class IDMAPI IPFactory
    {
    public:
        static const char* IPFactory::transitions_dot_json_filename;

        // Helper function for creating key-value strings
        static std::string CreateKeyValueString( const std::string& rKeyStr, const std::string& rValueStr );

        // Helper function for parsing the key-value string into its individual pieces
        static void ParseKeyValueString( const std::string& rKeyValueStr, std::string& rKeyStr, std::string& rValueStr );

        // Must be called before any IP related classes can be used
        static void CreateFactory();

        // Mostly used in testing to get rid of the factory
        static void DeleteFactory();

        // Returns the single instance of the factory.  It will throw an exception
        // if CreateFactory() has not been called.
        static IPFactory* GetInstance();

        // Initializes / reads the Individual Property information from the demographics.
        // It is assumed that each node has the same Individual Property values in its demographics.
        // Hence, it will only read the demographics when it has no individual properties and
        // assume anything new / different is an error.
        void Initialize( uint32_t externNodeId, const JsonObjectDemog& rDemog, bool isWhitelistEnabled );

        // This method will write the transition data into a campaign.json formatted file
        // that is read in later by the application's initialization.  Only one process needs
        // to call this method.
        void WriteTransitionsFile();

        // This method returns the list of Individual Properties defined in the simulation.
        const std::vector<IndividualProperty*>& GetIPList() const { return m_IPList; }

        // Return an initial set of values for a human.  There should be one value for each key/property.
        // The values are determined by the initial distribution parameters set in the demographics data.
        // The values will be set randomly.
        IPKeyValueContainer GetInitialValues( uint32_t externNodeId, RANDOMBASE* pRNG ) const;

        // Return the Individual Property of the whose key/name is given
        IndividualProperty* GetIP( const std::string& rKey, const std::string& rParameterName=std::string(""), bool throwOnNotFound=true );

        // Given the set of keys and their values return a list of strings containing all of the
        // possible combinations where all keys must be present.  Used in PropertyReport.
        std::vector<std::string> GetAllPossibleKeyValueCombinations() const;

        // Add a new IP using the key and set of values
        void AddIP( uint32_t externalNodeId, const std::string& rKey, const std::map<std::string,float>& rValues );

        // Returns a set of the possible IP keys.  Used with ConstrainedString in places 
        // where a key or other value (i.e. NONE) is allowed.
        std::set< std::string > GetKeysAsStringSet() const;

        // Returs a set of possible IP keys in a delimited string.  Used in error messages.
        std::string GetKeysAsString() const;
    protected:
        friend class IndividualProperty;
        friend class IPKey;
        friend class IPKeyValue;
        friend class IPKeyValueInternal;
        friend class IPTransition;

        IPFactory();
        ~IPFactory();

        // If the White List is enabled, this will throw an exception if the given key
        // is not one of the approved keys/properties.
        void CheckIpKeyInWhitelist( const std::string& rKey, int numValues );

        // Add this new/unique Key-Value pair to the possible set
        // It throws an exception if the "key:value" already exists.
        void AddKeyValue( IPKeyValueInternal* pkvi );

        // Return the internal representation of the "key:value" pair.
        // It throws an exception if not found
        IPKeyValueInternal* GetKeyValue( const std::string& rKeyValueString, const std::string& rParameterName=std::string("") );

    private:
#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        uint32_t m_ExternalNodeIdOfFirst;
        bool m_WhiteListEnabled;
        std::vector<IndividualProperty*> m_IPList;
        std::map<std::string,IPKeyValueInternal*> m_KeyValueMap;
        std::set<std::string> m_KeyWhiteList;
#pragma warning( pop )
    };
}
