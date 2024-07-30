
#pragma once

#include "BaseProperties.h"

// Individual Properties (IP) allow the DTK user to assign attributes to individuals that are not
// built in.  IP applies the BaseProperties feature to individuals.  See BaseProperties.h for more info.
namespace Kernel
{
    class IndividualProperty;

    // An IPKey is the key or name of an Individual Property (i.e. Risk).
    class IDMAPI IPKey : public BaseKey
    {
    public:
        static const char* GetConstrainedStringConstraintKey();
        static const char* GetConstrainedStringConstraintValue();
        static const char* GetConstrainedStringConstraintKeyValue();
        static const char* GetConstrainedStringDescriptionKey();
        static const char* GetConstrainedStringDescriptionValue();

        // Create a holder for a key.  If one attempts to use the object
        // before it is assigned key data, it will throw an exception
        IPKey();

        ~IPKey();

        // Create a key from the given string and validate it
        explicit IPKey( const std::string& rKeyStr );

        // Set this object to one that has this key and validate the key / name / property exists
        IPKey& operator=( const std::string& rKeyStr );

        bool operator==( const IPKey& rThat ) const;
        bool operator!=( const IPKey& rThat ) const;

        static void serialize( IArchive& ar, IPKey& obj );

    protected:
        friend class BaseKeyValue;

        explicit IPKey( const BaseProperty* pip );
    };

    // An IPKeyParameter is used to get configuration data from the user.
    // JsonConfigurable will use the SetParameterName() when configuring.  This
    // will then be used in the error message when verifying the input value is valid.
    // This class was created so that IPKey would not have the m_ParameterName
    // variable and could stay super light.
    class IPKeyParameter : public IPKey
    {
    public:
        IPKeyParameter();
        ~IPKeyParameter();

        // Set this object equal to the given one
        IPKeyParameter& operator=( const std::string& rKeyStr );

        // Return the external name associated with this key-value pair
        const std::string& GetParameterName() const;

        // Set the parameter name associated with this key-value pair.  The name is used in error reporting.
        void SetParameterName( const std::string& rParameterName );

    protected:
        std::string m_ParameterName;
    };

    // An IPKeyValue represents an Individual Property key and value 
    // where the value is one of the possible values for the given key.
    class IDMAPI IPKeyValue : public BaseKeyValue
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

        // Set this object equal to the given one
        IPKeyValue& operator=( const std::string& rKeyValueStr );

        bool operator==( const IPKeyValue& rThat ) const;
        bool operator!=( const IPKeyValue& rThat ) const;

        // Return the initial distribution of individuals that should 
        // receive this property value for this property key
        double GetInitialDistribution( uint32_t externalNodeId ) const;

        static void serialize( IArchive& ar, IPKeyValue& obj );

    protected:
        friend class BaseProperty;
        friend class IndividualProperty;
        friend class IPKeyValueContainer;
        template<class KeyValue> friend class BaseIterator;
        template<class Key, class KeyValue, class Iterator_t> friend class BaseKeyValueContainer;

        IPKeyValue( KeyValueInternal* pkvi );
    };

    // An IPKeyValueParameter is used to get configuration data from the user.
    // JsonConfigurable will use the SetParameterName() when configuring.  This
    // will then be used in the error message when verifying the input value is valid.
    // This class was created so that IPKeyValue would not have the m_ParameterName
    // variable and could stay super light.
    class IDMAPI IPKeyValueParameter : public IPKeyValue
    {
    public:
        IPKeyValueParameter();
        ~IPKeyValueParameter();

        // Set this object equal to the given one
        IPKeyValueParameter& operator=( const std::string& rKeyValueStr );

        // Return the external name associated with this key-value pair
        const std::string& GetParameterName() const;

        // Set the parameter name associated with this key-value pair.  The name is used in error reporting.
        void SetParameterName( const std::string& rParameterName );

    protected:
        std::string m_ParameterName;
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
    };

    // An iterator class used for traversing the elements in IPKeyValueContainer
    class IDMAPI IPKeyValueIterator : public BaseIterator<IPKeyValue>
    {
    public:
        friend class IPKeyValueContainer;
        template<class Key, class KeyValue, class Iterator_t> friend class BaseKeyValueContainer;

        ~IPKeyValueIterator();
        bool operator==( const IPKeyValueIterator& rThat ) const;
        bool operator!=( const IPKeyValueIterator& rThat ) const;
        IPKeyValueIterator& operator++(); //prefix

    private:
        IPKeyValueIterator( std::vector<KeyValueInternal*>::const_iterator it );
    };

    // An IPKeyValueContainer is used to manage a collection of IPKeyValue objects.
    // For example, an Individual has an IPKeyValueContainer to manage the properties 
    // specific to that individual.
    class IDMAPI IPKeyValueContainer : public BaseKeyValueContainer<IPKey,IPKeyValue, IPKeyValueIterator>
    {
    public:
        IPKeyValueContainer();
        IPKeyValueContainer( const IPKeyValueContainer& rThat );
        ~IPKeyValueContainer();

        IPKeyValueContainer& operator=( const IPKeyValueContainer& rThat );

        bool operator==( const IPKeyValueContainer& rThat ) const;
        bool operator!=( const IPKeyValueContainer& rThat ) const;

        virtual void Set( const IPKeyValue& rKeyValue );
        virtual void Add( const IPKeyValue& rKeyValue );
        virtual void Remove( const IPKeyValue& rKeyValue );

        IPKeyValue FindFirst( const IPKeyValueContainer& rContainer ) const;

        static void serialize( IArchive& ar, IPKeyValueContainer& obj );

    protected:
        friend class BaseProperty;
        IPKeyValueContainer( const std::vector<KeyValueInternal*>& rInternalList );
    };

    // The "Full" version is meant to be used by humans versus interventions.
    // It contains a translation to the old-style string map and only humans
    // need that.  This allows the interventions to have a lighter weight version.
    class IDMAPI IPKeyValueContainerFull : public IPKeyValueContainer
    {
    public:
        IPKeyValueContainerFull();
        IPKeyValueContainerFull( const IPKeyValueContainerFull& rThat );
        IPKeyValueContainerFull( const IPKeyValueContainer& rThat );
        ~IPKeyValueContainerFull();

        IPKeyValueContainerFull& operator=( const IPKeyValueContainerFull& rThat );
        IPKeyValueContainerFull& operator=( const IPKeyValueContainer& rThat );

        virtual void Set( const IPKeyValue& rKeyValue ) override;
        virtual void Add( const IPKeyValue& rKeyValue ) override;
        virtual void Remove( const IPKeyValue& rKeyValue ) override;

        std::string ToString() const;

        static void serialize( IArchive& ar, IPKeyValueContainerFull& obj );

    protected:
        friend class BaseProperty;

        void UpdateCache();

        IPKeyValueContainerFull( const std::vector<KeyValueInternal*>& rInternalList );

        std::string m_ToStringCache;
    };

    // Used to in HINT to define how disease is transmitted between people
    // with same/different properties.
    class IDMAPI IPIntraNodeTransmission
    {
    public:
        IPIntraNodeTransmission();
        ~IPIntraNodeTransmission();

        void Read( const std::string& rKeyStr, const JsonObjectDemog& rDemog, int numValues );
        void ReadTxMatrix( const std::string& rKeyStr, const JsonObjectDemog& rDemog, int numValues );

        const std::string& GetRouteName() const;
        bool HasMatrix() const;
        const std::vector<std::vector<float>>& GetMatrix() const;
        const std::map< std::string, std::vector<std::vector<float>>>& GetRouteToMatrixMap() const;

    private:
        std::string m_RouteName;
        std::vector<std::vector<float>> m_Matrix;
        std::map< std::string, std::vector<std::vector<float>>> m_RouteToMatrixMap;
    };

    // An IndividualProperty object represents a single Individual Property.  It contains everything 
    // that relates to a single property: name, values, transitions, and transmissions.
    class IDMAPI IndividualProperty : public BaseProperty
    {
    public:
        IndividualProperty();
        virtual ~IndividualProperty();

        // Read the one Individual Property from demographics
        virtual void Read( int idx, uint32_t externalNodeId, const JsonObjectDemog& rDemog, bool isNotFirstNode );

        const IPIntraNodeTransmission& GetIntraNodeTransmission( uint32_t externalNodeId ) const;

    protected:
        friend class IPFactory;

        IndividualProperty( uint32_t externalNodeId, const std::string& rKeyStr, const std::map<std::string,float>& rValues );

        std::vector<JsonObjectDemog> ConvertTransitions();

        static KeyValueInternal* get_kvi_func( BaseFactory* pFact, const char* ip_key_str, const std::string& rKvStr );

        virtual void ReadProperty( int idx, 
                                   uint32_t externalNodeId, 
                                   const JsonObjectDemog& rDemog, 
                                   bool isNotFirstNode ) override;

        void ReadPropertyAgeBin( int idx, 
                                 uint32_t externalNodeId, 
                                 const JsonObjectDemog& rDemog, 
                                 bool isNotFirstNode );

        void CreateAgeBinTransitions();

        std::vector<IPTransition*> m_Transitions;
        std::map<uint32_t,IPIntraNodeTransmission*> m_IntraNodeTransmissionMap;
    };


    // The IPFactory is a singleton that manages all of the Individual Properties 
    // in the simulation.  It is responsable for ensuring that there is only one
    // of each.  The instance of IPFactory is contained in Environment so that
    // DLL's will have access to this singleton.
    // This design assumes that there is only one valid set of Individual Properties
    // for all Nodes.
    class IDMAPI IPFactory : public BaseFactory
    {
    public:
        static const char* IPFactory::transitions_dot_json_filename;

        // Helper function for parsing the key-value string into its individual pieces
        static void ParseKeyValueString( const std::string& rKeyValueStr, std::string& rKeyStr, std::string& rValueStr );

        // Must be called before any Individual Property related classes can be used
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
        void Initialize( uint32_t externNodeId, const JsonObjectDemog& rDemog );

        // This method will write the transition data into a campaign.json formatted file
        // that is read in later by the application's initialization.  Only one process needs
        // to call this method.
        void WriteTransitionsFile();

        // This method returns the list of Individual Properties defined in the simulation.
        std::vector<IndividualProperty*> GetIPList() const;

        // Return true if the factory has at least one Individual Property
        bool HasIPs() const;

        // Return the Individual Property whose key/name is given
        IndividualProperty* GetIP( const std::string& rKey,
                                   const std::string& rParameterName = std::string(""),
                                   bool throwOnNotFound = true );

        // Add a new IP using the key and set of values
        IndividualProperty* AddIP( uint32_t externalNodeId,
                                   const std::string& rKey,
                                   const std::map<std::string,float>& rValues,
                                   bool haveFactoryMaintain = true );

        // Return an initial set of values for a human.  There should be one value for each key/property.
        // The values are determined by the initial distribution parameters set in the demographics data.
        // The values will be set randomly.
        IPKeyValueContainer GetInitialValues( uint32_t externNodeId, RANDOMBASE* pRNG ) const;

        // I'm really sure these must have local versions
        // This is where I want this function but something weird happens when I do this. Have to push another
        // version for now but not willing to give up completely on this yet.
        //typedef std::map< std::string, std::string > tKeyValuePair; 
        //typedef std::set< tKeyValuePair > tPermutations;
        //static void GenerateAllPermutationsOnce( std::set< std::string > &keys, tKeyValuePair perm, tPermutations &perms );

    protected:
        friend class IPKeyValueParameter;
        friend class IPKeyValue;
        friend class IndividualProperty;

        IPFactory();
        ~IPFactory();

        static BaseProperty* construct_ip( uint32_t externalNodeId,
                                           const std::string& rKeyStr,
                                           const std::map<std::string, float>& rValues );

    };
}
