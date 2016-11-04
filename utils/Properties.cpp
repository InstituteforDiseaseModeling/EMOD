/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "Properties.h"
#include "PropertiesString.h"
#include "Log.h"
#include "IdmString.h"
#include "FileSystem.h"
#include "NoCrtWarnings.h"
#include "Types.h"
#include "Debug.h"
#include "Common.h"
#include "RANDOM.h"

static const char* _module = "Properties";

const char* IP_AGE_BIN_PROPERTY = "Age_Bin";
const char* IP_AGE_BIN_VALUE_0  = "Age_Bin_Property_From_0";
std::string IP_AGE_BIN_VALUE_PREFIX  = "Age_Bin_Property_From_";

const char* IP_KEY                    = "IndividualProperties";
const char* IP_NAME_KEY               = "Property";
const char* IP_VALUES_KEY             = "Values";
const char* IP_INIT_KEY               = "Initial_Distribution";
const char* IP_AGE_BIN_EDGE_KEY       = "Age_Bin_Edges_In_Years";

const char* IP_TRANS_KEY              = "Transitions"; // optional
const char* IP_TRANS_FROM_KEY             = "From"; // required
const char* IP_TRANS_TO_KEY               = "To";// required
const char* IP_TRANS_TYPE_KEY             = "Type";// required
const char* IP_TRANS_COVERAGE_KEY         = "Coverage";// required
const char* IP_TRANS_PROBABILITY_KEY      = "Probability_Per_Timestep";// required
const char* IP_TRANS_REVERSION_KEY        = "Timesteps_Until_Reversion";// optional
const char* IP_TRANS_AGE_KEY              = "Age_In_Years";// optional
const char* IP_TRANS_WHEN_KEY             = "Timestep_Restriction";// required
const char* IP_TRANS_START_KEY                = "Start";// required
const char* IP_TRANS_DURATION_KEY             = "Duration";// optional
const char* IP_TRANS_AGE_RESTRICTION_KEY  = "Age_In_Years_Restriction";// optional
const char* IP_TRANS_AGE_RESTRICTION_MIN_KEY  = "Min";// required
const char* IP_TRANS_AGE_RESTRICTION_MAX_KEY  = "Max";// optional

const char* IP_TRANS_TYPE_VALUE_TIMESTEP = "At_Timestep";
const char* IP_TRANS_TYPE_VALUE_AGE      = "At_Age";

const char* IP_TM_KEY                 = "TransmissionMatrix";
const char* IP_TM_ROUTE_KEY               = "Route";
const char* IP_TM_MATRIX_KEY              = "Matrix";

#define KEY_VALUE_SEPARATOR_CHAR (':')
#define KEY_VALUE_SEPARATOR (":")
#define PROP_SEPARATOR (",")

#ifdef OLD

//const char* IP_KEY;
//const char* IP_NAME_KEY;
//const char* IP_VALUES_KEY;
//const char* IP_INIT_KEY;
//const char* IP_AGE_BIN_KEY;

const char* IP_PROBABILITY_KEY = IP_TRANS_PROBABILITY_KEY;
const char* IP_REVERSION_KEY   = IP_TRANS_REVERSION_KEY;
const char* IP_AGE_KEY         = IP_TRANS_AGE_KEY;
const char* IP_WHEN_KEY        = IP_TRANS_WHEN_KEY;

const char* TRANSMISSION_MATRIX_KEY = IP_TM_KEY;
const char* ROUTE_KEY               = IP_TM_ROUTE_KEY;
const char* TRANSMISSION_DATA_KEY   = IP_TM_MATRIX_KEY;
#endif //OLD


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

static const char* KEY_WHITE_LIST_TMP[] = { "Age_Bin", "Accessibility", "Geographic", "Place", "Risk", "QualityOfCare", "HasActiveTB"  };

const char* IPFactory::transitions_dot_json_filename = "transitions.json";

    // ------------------------------------------------------------------------
    // --- IPKeyValueInternal
    // ------------------------------------------------------------------------

    class IPKeyValueInternal
    {
    public:
        IPKeyValueInternal( IndividualProperty* pip, const std::string& rValue, uint32_t externalNodeId, const ProbabilityNumber& rInitialDist )
            : m_pIP( pip )
            , m_KeyValueString()
            , m_Value( rValue )
            , m_InitialDistributions()
        {
            m_KeyValueString = IPFactory::CreateKeyValueString( m_pIP->GetKeyAsString(), m_Value );
            m_InitialDistributions[ externalNodeId ] = rInitialDist;
        }

    protected:
        friend class IPKeyValue;
        friend class IPFactory;

        IndividualProperty* m_pIP;
        std::string m_KeyValueString;
        std::string m_Value;
        std::map<uint32_t,float> m_InitialDistributions;
    };

    // ------------------------------------------------------------------------
    // --- IPKey
    // ------------------------------------------------------------------------

    IPKey::IPKey()
        : m_pIP( nullptr )
        , m_ParameterName()
    {
    }

    IPKey::IPKey( IndividualProperty* pip )
        : m_pIP( pip )
        , m_ParameterName()
    {
        release_assert( m_pIP );
    }

    IPKey::IPKey( const std::string& rKeyStr )
        : m_pIP( nullptr )
        , m_ParameterName()
    {
        m_pIP = IPFactory::GetInstance()->GetIP( rKeyStr );
    }

    IPKey::~IPKey()
    {
    }

    const std::string& IPKey::ToString() const
    {
        if( m_pIP == nullptr )
        {
            throw NullPointerException( __FILE__, __LINE__, __FUNCTION__, "m_pIP" );
        }
        return m_pIP->GetKeyAsString();
    }

    IPKey& IPKey::operator=( const std::string& rKeyStr )
    {
        m_pIP = IPFactory::GetInstance()->GetIP( rKeyStr, m_ParameterName );
        return *this;
    }

    bool IPKey::operator==( const IPKey& rThat ) const
    {
        // ------------------------------------------------------------
        // --- Don't check the parameter name.  The user wants to know
        // --- if this is the same key, not the same parameter.
        // ------------------------------------------------------------
        return ( this->m_pIP == rThat.m_pIP );
    }

    bool IPKey::operator!=( const IPKey& rThat ) const
    {
        return !operator==( rThat ); 
    }

    bool IPKey::IsValid() const
    {
        return (m_pIP != nullptr);
    }

    const std::string& IPKey::GetParameterName() const
    {
        return m_ParameterName;
    }

    void IPKey::SetParameterName( const std::string& rParameterName )
    {
        m_ParameterName = rParameterName;
    }

    // ------------------------------------------------------------------------
    // --- IPTransition
    // ------------------------------------------------------------------------
#define UNKNOWN ("UNKNOWN")

    IPTransition::IPTransition()
        : m_From()
        , m_To()
        , m_Type(UNKNOWN)
        , m_Coverage(1.0)
        , m_Start(0.0)
        , m_Duration(FLT_MAX)
        , m_Probability(0.0)
        , m_Revision(0.0)
        , m_HasAgeConstraint(false)
        , m_MinAgeYears(0.0)
        , m_MaxAgeYears(FLT_MAX)
        , m_AgeYears(FLT_MAX)
    {
    }

    IPTransition::IPTransition( const IPKeyValue& rFrom,
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
                                float maxAgeYears )
        : m_From(rFrom)
        , m_To(rTo)
        , m_Type(rType)
        , m_Coverage(coverage)
        , m_Start(startDay)
        , m_Duration(durationDays)
        , m_Probability(probability)
        , m_Revision(revision)
        , m_HasAgeConstraint(hasAgeConstraint)
        , m_MinAgeYears(minAgeYears)
        , m_MaxAgeYears(maxAgeYears)
        , m_AgeYears(ageYears)
    {
    }

    IPTransition::~IPTransition()
    {
    }

    void IPTransition::Read( int idx, int itran, const JsonObjectDemog& rDemog, const std::string& rKeyStr )
    {
        Validate( rDemog );
        std::string from_str = rDemog[ IP_TRANS_FROM_KEY ].AsString();
        std::string to_str   = rDemog[ IP_TRANS_TO_KEY   ].AsString();

        if( from_str != "NULL" )
        {
            std::string from_kv_str = IPFactory::CreateKeyValueString( rKeyStr, from_str );
            m_From = IPKeyValue( from_kv_str );
        }
        std::string to_kv_str = IPFactory::CreateKeyValueString( rKeyStr, to_str );
        m_To = IPKeyValue( to_kv_str );

        m_Type         = rDemog[ IP_TRANS_TYPE_KEY        ].AsString();
        m_Coverage     = rDemog[ IP_TRANS_COVERAGE_KEY    ].AsDouble();
        m_Probability  = rDemog[ IP_TRANS_PROBABILITY_KEY ].AsDouble();
        m_Start        = rDemog[ IP_TRANS_WHEN_KEY        ][ IP_TRANS_START_KEY ].AsDouble();
        if( rDemog[ IP_TRANS_WHEN_KEY ].Contains( IP_TRANS_DURATION_KEY ) )
        {
            m_Duration = rDemog[ IP_TRANS_WHEN_KEY ][ IP_TRANS_DURATION_KEY ].AsDouble();
        }
        if( rDemog.Contains( IP_TRANS_REVERSION_KEY ) )
        {
            m_Revision = rDemog[ IP_TRANS_REVERSION_KEY ].AsDouble();
        }

        if( m_Type == IP_TRANS_TYPE_VALUE_AGE )
        {
            m_AgeYears = rDemog[ IP_TRANS_AGE_KEY ].AsDouble();
        }
        else if( m_Type != IP_TRANS_TYPE_VALUE_TIMESTEP )
        {
            std::ostringstream msg;
            msg << "Invalid Individual_Property Transitions value for Type = " << m_Type 
                << ".  Known values are: "  << IP_TRANS_TYPE_VALUE_TIMESTEP << " and " << IP_TRANS_TYPE_VALUE_AGE;
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }

        m_HasAgeConstraint = rDemog.Contains( IP_TRANS_AGE_RESTRICTION_KEY ) 
                          && (  rDemog[ IP_TRANS_AGE_RESTRICTION_KEY ].Contains( IP_TRANS_AGE_RESTRICTION_MIN_KEY )
                             || rDemog[ IP_TRANS_AGE_RESTRICTION_KEY ].Contains( IP_TRANS_AGE_RESTRICTION_MAX_KEY ) );
        if( m_HasAgeConstraint )
        {
            m_MinAgeYears = 0.0;
            if( rDemog[ IP_TRANS_AGE_RESTRICTION_KEY ].Contains( IP_TRANS_AGE_RESTRICTION_MIN_KEY ) )
            {
                m_MinAgeYears = rDemog[ IP_TRANS_AGE_RESTRICTION_KEY ][ IP_TRANS_AGE_RESTRICTION_MIN_KEY ].AsDouble();
            }
            m_MaxAgeYears = MAX_HUMAN_AGE;
            if( rDemog[ IP_TRANS_AGE_RESTRICTION_KEY ].Contains( IP_TRANS_AGE_RESTRICTION_MAX_KEY ) )
            {
                m_MaxAgeYears = rDemog[ IP_TRANS_AGE_RESTRICTION_KEY ][ IP_TRANS_AGE_RESTRICTION_MAX_KEY ].AsDouble();
            }

            // ------------------------------------------------------------------------------------------
            // --- IVCalendar will not distribute the intervention to people who are initially older than 
            // --- m_AgeYears so we want to ensure that the restriction is at most m_AgeYears
            // ------------------------------------------------------------------------------------------
            if( (m_Type == IP_TRANS_TYPE_VALUE_AGE) && (m_MaxAgeYears > m_AgeYears) )
            {
                m_MaxAgeYears = m_AgeYears;
            }

            if( m_MinAgeYears >= m_MaxAgeYears )
            {
                std::ostringstream min_str ;
                min_str << m_MinAgeYears ;
                std::ostringstream max_str ;
                max_str << m_MaxAgeYears ;
                std::ostringstream msg ;
                msg << "In the Demographics for "<< IP_KEY << ":" << IP_NAME_KEY << "=" << rKeyStr << ", Max age must be greater than Min age.";
                throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                        "demographics[Age_In_Years_Restriction][Min]", min_str.str().c_str(),
                                                        "demographics[Age_In_Years_Restriction][Max]", max_str.str().c_str(),
                                                        msg.str().c_str());

            }
        }

        if( (m_Coverage < 0.0) || (1.0 < m_Coverage) )
        {
            float violated_threshold = 0.0;
            if( m_Coverage > 1.0 )
                violated_threshold = 1.0;

            std::ostringstream msg ;
            msg << "Demographics["<< IP_KEY << "][" << IP_NAME_KEY << "=" << rKeyStr << "][" << IP_TRANS_COVERAGE_KEY << "]";
            throw OutOfRangeException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str(), m_Coverage, violated_threshold );
        }
        if( (m_Probability < 0.0) || (1.0 < m_Probability) )
        {
            float violated_threshold = 0.0;
            if( m_Probability > 1.0 )
                violated_threshold = 1.0;

            std::ostringstream msg ;
            msg << "Demographics["<< IP_KEY << "][" << IP_NAME_KEY << "=" << rKeyStr << "][" << IP_TRANS_PROBABILITY_KEY << "]";
            throw OutOfRangeException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str(), m_Probability, violated_threshold );
        }
    }

    std::vector<JsonObjectDemog> IPTransition::ConvertToCampaignEvent( const IPKey& rKey )
    {
        std::vector<JsonObjectDemog> ret_list ;

        JsonObjectDemog intervention_config( JsonObjectDemog::JSON_OBJECT_OBJECT );

        if( m_Type == IP_TRANS_TYPE_VALUE_TIMESTEP )
        {
            intervention_config.Add( "class",                 "PropertyValueChanger" );
            intervention_config.Add( "Dont_Allow_Duplicates", 0.0                    );
            intervention_config.Add( "Target_Property_Key",   rKey.ToString()        );
            intervention_config.Add( "Target_Property_Value", m_To.GetValueAsString());
            intervention_config.Add( "Daily_Probability",     m_Probability          );
            intervention_config.Add( "Maximum_Duration",      m_Duration             );
            intervention_config.Add( "Revert",                m_Revision             );
        }
        else // IP_TRANS_TYPE_VALUE_AGE
        {
            JsonObjectDemog calendar( JsonObjectDemog::JSON_OBJECT_OBJECT );
            calendar.Add( "Age", m_AgeYears*DAYSPERYEAR );
            calendar.Add( "Probability", 1.0 );

            JsonObjectDemog cal_array( JsonObjectDemog::JSON_OBJECT_ARRAY );
            cal_array.PushBack( calendar );

            JsonObjectDemog act_config( JsonObjectDemog::JSON_OBJECT_OBJECT );
            act_config.Add( "class",                 "PropertyValueChanger" );
            act_config.Add( "Dont_Allow_Duplicates", 0.0                    );
            act_config.Add( "Target_Property_Key",   rKey.ToString()        );
            act_config.Add( "Target_Property_Value", m_To.GetValueAsString());
            act_config.Add( "Daily_Probability",     m_Probability          );
            act_config.Add( "Maximum_Duration",      m_Duration             );
            act_config.Add( "Revert",                m_Revision             );

            JsonObjectDemog act_array( JsonObjectDemog::JSON_OBJECT_ARRAY );
            act_array.PushBack( act_config );

            intervention_config.Add( "class",                                 "IVCalendar" );
            intervention_config.Add( "Dont_Allow_Duplicates",                 0.0          );
            intervention_config.Add( "Dropout",                               0.0          );
            intervention_config.Add( "Calendar",                              cal_array    );
            intervention_config.Add( "Actual_IndividualIntervention_Configs", act_array    );
        }

        JsonObjectDemog array_obj( JsonObjectDemog::JSON_OBJECT_ARRAY );

        JsonObjectDemog std_evt_coord( JsonObjectDemog::JSON_OBJECT_OBJECT );
        std_evt_coord.Add( "class",                        "StandardInterventionDistributionEventCoordinator" );
        std_evt_coord.Add( "Number_Distributions",         -1.0 );
        std_evt_coord.Add( "Number_Repetitions",            1.0 );
        std_evt_coord.Add( "Property_Restrictions",         array_obj );
        std_evt_coord.Add( "Target_Demographic",            "Everyone" );
        std_evt_coord.Add( "Timesteps_Between_Repetitions", 0.0 );
        std_evt_coord.Add( "Target_Residents_Only",         0.0 );
        std_evt_coord.Add( "Include_Arrivals",              0.0 );
        std_evt_coord.Add( "Include_Departures",            0.0 );
        std_evt_coord.Add( "Demographic_Coverage",          m_Coverage );
        std_evt_coord.Add( "Intervention_Config",           intervention_config );

        if( m_HasAgeConstraint )
        {
            std_evt_coord.Add( "Target_Demographic", "ExplicitAgeRanges" );
            std_evt_coord.Add( "Target_Age_Min", m_MinAgeYears );
            std_evt_coord.Add( "Target_Age_Max", m_MaxAgeYears );
        }
        else if( m_Type == IP_TRANS_TYPE_VALUE_AGE )
        {
            std_evt_coord.Add( "Target_Demographic", "ExplicitAgeRanges" );
            std_evt_coord.Add( "Target_Age_Min", 0.0 );
            std_evt_coord.Add( "Target_Age_Max", m_AgeYears );
        }

        // Don't use property_restrictions to target age_bin transitions
        // Initial transitions for Age_Bins have no original property
        if( m_From.IsValid() && (rKey.ToString() != IP_AGE_BIN_PROPERTY) )
        {
            JsonObjectDemog prop_res_array( JsonObjectDemog::JSON_OBJECT_ARRAY );
            prop_res_array.PushBack( m_From.ToString() );

            std_evt_coord.Add( "Property_Restrictions", prop_res_array );
        } 

        JsonObjectDemog node_set( JsonObjectDemog::JSON_OBJECT_OBJECT );
        node_set.Add( "class", "NodeSetAll" );

        JsonObjectDemog campaign_event( JsonObjectDemog::JSON_OBJECT_OBJECT );
        campaign_event.Add( "class",                    "CampaignEvent" );
        campaign_event.Add( "Start_Day",                m_Start );
        campaign_event.Add( "Event_Coordinator_Config", std_evt_coord );
        campaign_event.Add( "Nodeset_Config",           node_set );

        ret_list.push_back( campaign_event );

        std::string to_value_str = m_To.GetValueAsString();
        if( (m_Type == IP_TRANS_TYPE_VALUE_AGE) ||
            ((m_Type == IP_TRANS_TYPE_VALUE_TIMESTEP) && (m_Start == 0.0) &&
                                                         (rKey.ToString() == IP_AGE_BIN_PROPERTY) &&
                                                         (to_value_str.find(IP_AGE_BIN_VALUE_0) !=  string::npos) ) )
        {
            JsonObjectDemog birth_intervention_config( JsonObjectDemog::JSON_OBJECT_OBJECT );
            birth_intervention_config.Parse( intervention_config.ToString().c_str() );

            // Add BirthTriggeredIV.
            JsonObjectDemog birth_iv( JsonObjectDemog::JSON_OBJECT_OBJECT );
            birth_iv.Add( "class",                "BirthTriggeredIV" );
            birth_iv.Add( "Dont_Allow_Duplicates", 0.0 );
            birth_iv.Add( "Demographic_Coverage",  1.0 );
            birth_iv.Add( "Duration",             -1.0 );
            birth_iv.Add( "Actual_IndividualIntervention_Config", birth_intervention_config );

            JsonObjectDemog campaign_event_birth( JsonObjectDemog::JSON_OBJECT_OBJECT );

            // ------------------------------------
            // --- Copy the original campaign event
            // ------------------------------------
            campaign_event_birth.Parse( campaign_event.ToString().c_str() );
            campaign_event_birth.Add("Start_Day", 0.0 );

            // --------------------------------------------------
            // --- Get the standard event coordinator and replace
            // --- the intervention with this birth triggered one
            // --------------------------------------------------
            JsonObjectDemog birth_std_evt_coord = campaign_event_birth[ "Event_Coordinator_Config" ];
            birth_std_evt_coord.Add( "Intervention_Config", birth_iv );

            // --------------------------------------------------------------------------
            // --- Since BirthTriggeredIV is a node level intervetion, we need to remove
            // --- any individual restrictions.
            // --------------------------------------------------------------------------
            for( JsonObjectDemog::Iterator it = birth_std_evt_coord.Begin(); it != birth_std_evt_coord.End(); )
            {
                if( (it.GetKey() == "Target_Demographic"   ) ||
                    (it.GetKey() == "Target_Residents_Only" ) ||
                    (it.GetKey() == "Demographic_Coverage" ) ||
                    (it.GetKey() == "Property_Restrictions") ||
                    (it.GetKey() == "Target_Age_Min"       ) ||
                    (it.GetKey() == "Target_Age_Max"       ) )
                {
                    birth_std_evt_coord.Remove( it );
                }
                else
                {
                    ++it;
                }
            }

            ret_list.push_back( campaign_event_birth );
        }

        return ret_list;
    }

    void IPTransition::Validate( const JsonObjectDemog& rDemog )
    {
        std::string key_list[] = { IP_TRANS_FROM_KEY, 
                                   IP_TRANS_TO_KEY, 
                                   IP_TRANS_TYPE_KEY, 
                                   IP_TRANS_COVERAGE_KEY, 
                                   IP_TRANS_PROBABILITY_KEY, 
                                   IP_TRANS_WHEN_KEY
                                 };

        for( auto key : key_list )
        {
            if( !rDemog.Contains( key.c_str() ) )
            {
                throw MissingParameterFromConfigurationException( __FILE__, __LINE__, __FUNCTION__, "demographics file", key.c_str() );
            }
        }

        if( !rDemog[ IP_TRANS_WHEN_KEY ].Contains( IP_TRANS_START_KEY ) )
        {
            throw MissingParameterFromConfigurationException( __FILE__, __LINE__, __FUNCTION__, "demographics file", IP_TRANS_START_KEY );
        }
    }


    // ------------------------------------------------------------------------
    // --- IPKeyValue
    // ------------------------------------------------------------------------
    IPKeyValue::IPKeyValue()
        : m_pInternal( nullptr )
        , m_ParameterName()
    {
    }

    IPKeyValue::IPKeyValue( const std::string& rKeyValueStr )
        : m_pInternal( nullptr )
        , m_ParameterName()
    {
        m_pInternal = IPFactory::GetInstance()->GetKeyValue( rKeyValueStr );
    }

    IPKeyValue::IPKeyValue( const std::string& rKeyStr, const std::string& rValueStr )
        : m_pInternal( nullptr )
        , m_ParameterName()
    {
        std::string kv_str = IPFactory::CreateKeyValueString( rKeyStr, rValueStr );
        m_pInternal = IPFactory::GetInstance()->GetKeyValue( kv_str );
    }

    IPKeyValue::IPKeyValue( IPKeyValueInternal* pkvi )
        : m_pInternal( pkvi )
        , m_ParameterName()
    {
        release_assert( m_pInternal );
    }

    IPKeyValue::~IPKeyValue()
    {
        // we don't own so don't delete
        m_pInternal = nullptr;
    }

    bool IPKeyValue::IsValid() const
    {
        return (m_pInternal != nullptr);
    }

    const std::string& IPKeyValue::ToString() const
    {
        if( m_pInternal == nullptr )
        {
            throw NullPointerException( __FILE__, __LINE__, __FUNCTION__, "m_pInternal" );
        }
        return m_pInternal->m_KeyValueString;
    }

    IPKeyValue& IPKeyValue::operator=( const std::string& rKeyValueStr )
    {
        m_pInternal =IPFactory::GetInstance()->GetKeyValue( rKeyValueStr, m_ParameterName );
        return *this;
    }

    bool IPKeyValue::operator==( const IPKeyValue& rThat ) const 
    {
        // ------------------------------------------------------------
        // --- Don't check the parameter name.  The user wants to know
        // --- if this is the same key-value, not the same parameter.
        // ------------------------------------------------------------
        //if( this->m_ParameterName != rThat.m_ParameterName ) return false;

        if( this->m_pInternal != rThat.m_pInternal ) return false;

        return true;
    }

    bool IPKeyValue::operator!=( const IPKeyValue& rThat ) const
    {
        return !operator==( rThat );
    }

    const std::string& IPKeyValue::GetParameterName() const
    {
        return m_ParameterName;
    }

    void IPKeyValue::SetParameterName( const std::string& rParameterName )
    {
        m_ParameterName = rParameterName;
    }

    IPKey IPKeyValue::GetKey() const
    {
        if( m_pInternal == nullptr )
        {
            throw NullPointerException( __FILE__, __LINE__, __FUNCTION__, "m_pInternal" );
        }
        IPKey key( m_pInternal->m_pIP );
        return key;
    }

    std::string IPKeyValue::GetValueAsString() const
    {
        if( m_pInternal == nullptr )
        {
            throw NullPointerException( __FILE__, __LINE__, __FUNCTION__, "m_pInternal" );
        }
        return m_pInternal->m_Value;
    }

    double IPKeyValue::GetInitialDistribution( uint32_t externalNodeId ) const
    {
        if( m_pInternal == nullptr )
        {
            throw NullPointerException( __FILE__, __LINE__, __FUNCTION__, "m_pInternal" );
        }
        return m_pInternal->m_InitialDistributions[ externalNodeId ];
    }

    void IPKeyValue::UpdateInitialDistribution( uint32_t externalNodeId, double value )
    {
        if( m_pInternal == nullptr )
        {
            throw NullPointerException( __FILE__, __LINE__, __FUNCTION__, "m_pInternal" );
        }
        m_pInternal->m_InitialDistributions[ externalNodeId ] = value;
    }

    // ------------------------------------------------------------------------
    // --- IPKeyValueContainer::Iterator
    // ------------------------------------------------------------------------
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // !!! Ideally, we would just use the map for the container, but to keep from changing the results,
    // !!! we also use a vector to maintain the order of the values like in the old version.
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    //IPKeyValueContainer::Iterator::Iterator( std::map<std::string,IPKeyValue>::const_iterator it )
    //    : m_Iterator( it )
    //{
    //}

    IPKeyValueContainer::Iterator::Iterator( std::vector<IPKeyValue>::const_iterator it )
        : m_Iterator( it )
    {
    }

    IPKeyValueContainer::Iterator::~Iterator()
    {
    }

    bool IPKeyValueContainer::Iterator::operator==( const IPKeyValueContainer::Iterator& rThat ) const
    {
        return (this->m_Iterator == rThat.m_Iterator);
    }

    bool IPKeyValueContainer::Iterator::operator!=( const IPKeyValueContainer::Iterator& rThat ) const
    {
        return !operator==( rThat );
    }

    IPKeyValueContainer::Iterator& IPKeyValueContainer::Iterator::operator++()
    {
        m_Iterator++;
        return *this;
    }

    IPKeyValue IPKeyValueContainer::Iterator::operator*()
    {
        //return m_Iterator->second;
        return *m_Iterator;
    }

    // ------------------------------------------------------------------------
    // --- IPKeyValueContainer
    // ------------------------------------------------------------------------

    IPKeyValueContainer::Iterator IPKeyValueContainer::begin() const
    {
        //return Iterator( m_Map.begin() ); // !!! See comment for interator
        return Iterator( m_Vector.begin() );
    }

    IPKeyValueContainer::Iterator IPKeyValueContainer::end() const 
    {
        //return Iterator( m_Map.end() ); // !!! See comment for interator
        return Iterator( m_Vector.end() );
    }

    IPKeyValueContainer::IPKeyValueContainer()
        : m_Map()
        , m_Vector()
    {
    }

    IPKeyValueContainer::~IPKeyValueContainer()
    {
    }

    bool IPKeyValueContainer::operator==( const IPKeyValueContainer& rThat ) const
    {
        if( this->m_Map.size() != rThat.m_Map.size() ) return false;

        for( auto entry : this->m_Map )
        {
            IPKeyValue this_kv = this->m_Map.at( entry.first );
            IPKeyValue that_kv = rThat.m_Map.at( entry.first );

            if( this_kv != that_kv ) return false;
        }
        return true;
    }

    bool IPKeyValueContainer::operator!=( const IPKeyValueContainer& rThat ) const
    {
        return !operator==( rThat );
    }

    void IPKeyValueContainer::Add( const IPKeyValue& rKeyValue )
    {
        m_Map[ rKeyValue.ToString() ] = rKeyValue;
        m_Vector.push_back( rKeyValue );
        release_assert( m_Map.size() == m_Vector.size() );
    }

    void IPKeyValueContainer::Remove( const IPKeyValue& rKeyValue )
    {
        m_Map.erase( rKeyValue.ToString() );

        for( auto it = m_Vector.begin() ; it != m_Vector.end() ; ++it )
        {
            if( rKeyValue == *it )
            {
                m_Vector.erase( it );
                break;
            }
        }
        release_assert( m_Map.size() == m_Vector.size() );
    }

    IPKeyValue IPKeyValueContainer::Get( const IPKey& rKey ) const
    {
        IPKeyValue kv;
        bool found = false;
        for( auto entry : m_Map )
        {
            if( entry.second.GetKey() == rKey )
            {
                if( found )
                {
                    std::ostringstream ss;
                    ss << "Illegal use of IPKeyValueContainer::Get( const IPKey& rKey ).  Should not be used on containers that have multiple values for one key.";
                    throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
                }
                else
                {
                    kv = entry.second;
                    found = true;
                }
            }
        }
        return kv;
    }

    void IPKeyValueContainer::Set( const IPKeyValue& rNewKeyValue )
    {
        bool found = false;
        for( auto& rEntry : m_Map )
        {
            if( rEntry.second.GetKey() == rNewKeyValue.GetKey() )
            {
                if( found )
                {
                    std::ostringstream ss;
                    ss << "Illegal use of IPKeyValueContainer::Set( const IPKeyValue& rKeyValue ).  Should not be used on containers that have multiple values for one key.";
                    throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
                }
                else
                {
                    rEntry.second = rNewKeyValue;
                    found = true;
                }
            }
        }
        for( IPKeyValue& r_kv : m_Vector )
        {
            if( r_kv.GetKey() == rNewKeyValue.GetKey() )
            {
                r_kv = rNewKeyValue;
                break;
            }
        }
    }

    IPKeyValue IPKeyValueContainer::Get( const std::string& rKeyValueString ) const
    {
        return m_Map.at( rKeyValueString );
    }

    bool IPKeyValueContainer::Contains( const std::string& rKeyValueString ) const
    {
        return (m_Map.count( rKeyValueString ) > 0);
    }

    bool IPKeyValueContainer::Contains( const IPKeyValue& rKeyValue ) const
    {
        return (m_Map.count( rKeyValue.ToString() ) > 0);
    }

    bool IPKeyValueContainer::Contains( const IPKey& rKey ) const
    {
        for( auto entry : m_Map )
        {
            if( rKey == entry.second.GetKey() )
            {
                return true;
            }
        }
        return false;
    }

    std::string IPKeyValueContainer::GetValuesToString() const
    {
        std::stringstream ss;
        for( auto kv : *this )
        {
            ss << kv.GetValueAsString() << ", ";
        }
        std::string ret_str = ss.str();
        ret_str = ret_str.substr(0, ret_str.length()-2 );
        return ret_str;
    }

    std::set<std::string> IPKeyValueContainer::GetValuesToStringSet() const
    {
        std::set<std::string> str_set;
        for( auto kv : *this )
        {
            str_set.insert( kv.GetValueAsString() );
        }
        return str_set;
    }

    std::list<std::string> IPKeyValueContainer::GetValuesToList() const
    {
        std::list<std::string> list;
        for( auto kv : *this )
        {
            list.push_back( kv.GetValueAsString() );
        }
        return list;
    }

    std::string IPKeyValueContainer::ToString() const
    {
        std::string ret_str;
        for( auto kv : *this )
        {
            ret_str += kv.ToString() + std::string(PROP_SEPARATOR);
        }
        if( !ret_str.empty() )
        {
            ret_str = ret_str.substr(0, ret_str.length()-1 );
        }
        return ret_str;
    }

    void IPKeyValueContainer::Clear()
    {
        m_Map.clear();
        m_Vector.clear();
    }


    // ------------------------------------------------------------------------
    // --- IPIntraNodeTransmissions
    // ------------------------------------------------------------------------

    IPIntraNodeTransmissions::IPIntraNodeTransmissions()
        : m_RouteName("contact")
        , m_Matrix()
    {
    }
    IPIntraNodeTransmissions::~IPIntraNodeTransmissions()
    {
    }

    void IPIntraNodeTransmissions::Read( const std::string& rKeyStr, const JsonObjectDemog& rDemog, int numValues )
    {
        if( rDemog.Contains( IP_TM_KEY ) )
        {
            if( !rDemog[ IP_TM_KEY ].Contains( IP_TM_MATRIX_KEY ) )
            {
                return;
            }

            if( rDemog[ IP_TM_KEY ].Contains( IP_TM_ROUTE_KEY ) )
            {
                m_RouteName =  rDemog[ IP_TM_KEY ][ IP_TM_ROUTE_KEY ].AsString();
                std::transform(m_RouteName.begin(), m_RouteName.end(), m_RouteName.begin(), ::tolower);
            }
            else 
            {
                LOG_WARN_F("Missing '%s' in demographics for property '%s'. Will use default route 'contact'.\n",IP_TM_ROUTE_KEY, rKeyStr.c_str());
            }

            if( rDemog[ IP_TM_KEY ][ IP_TM_MATRIX_KEY ].IsArray() &&
                rDemog[ IP_TM_KEY ][ IP_TM_MATRIX_KEY ].size() != numValues )
            {
                std::ostringstream msg;
                msg << "Invalid Transmission Matrix for property '" << rKeyStr << "'.  It has "
                    << rDemog[ IP_TM_KEY ][ IP_TM_MATRIX_KEY ].size() << " rows when it should have " << numValues
                    << ".  It should be square with one row/col per value for the property." ;
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
            }

            for( int i = 0 ; i < rDemog[ IP_TM_KEY ][ IP_TM_MATRIX_KEY ].size() ; i++ )
            {
                if( rDemog[ IP_TM_KEY ][ IP_TM_MATRIX_KEY ][i].size() != numValues )
                {
                    std::ostringstream msg;
                    msg << "Invalid Transmission Matrix for property '" << rKeyStr << "'.  Row " << i << " has "
                        << rDemog[ IP_TM_KEY ][ IP_TM_MATRIX_KEY ][i].size() << " columns when it should have " << numValues
                        << ".  It should be square with one row/col per value for the property." ;
                    throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
                }

                std::vector<float> row ;
                for( int j = 0 ; j < rDemog[ IP_TM_KEY ][ IP_TM_MATRIX_KEY ][i].size() ; j++ )
                {
                    float val = rDemog[ IP_TM_KEY ][ IP_TM_MATRIX_KEY ][i][j].AsDouble();
                    row.push_back( val );
                }
                m_Matrix.push_back( row );
            }
        }
    }

    bool IPIntraNodeTransmissions::HasMatrix() const
    {
        return m_Matrix.size() > 0 ;
    }

    const std::string& IPIntraNodeTransmissions::GetRouteName() const
    {
        return m_RouteName;
    }

    const std::vector<std::vector<float>>& IPIntraNodeTransmissions::GetMatrix() const
    {
        return m_Matrix;
    }

    // ------------------------------------------------------------------------
    // --- IndividualProperty
    // ------------------------------------------------------------------------

    IndividualProperty::IndividualProperty()
        : m_Key()
        , m_Values()
        , m_Transitions()
        , m_IntraNodeTransmissionsMap()
    {
    }

    IndividualProperty::IndividualProperty( uint32_t externalNodeId, const std::string& rKeyStr, const std::map<std::string,float>& rValues )
        : m_Key(rKeyStr)
        , m_Values()
        , m_Transitions()
    {
        float total_prob = 0.0;
        for( auto entry : rValues )
        {
            IPKeyValueInternal* pkvi = new IPKeyValueInternal( this, entry.first, externalNodeId, entry.second );
            IPFactory::GetInstance()->AddKeyValue( pkvi );
            IPKeyValue kv( pkvi );
            m_Values.Add( kv );
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

    IndividualProperty::~IndividualProperty()
    {
        for( auto p_tran : m_Transitions )
        {
            delete p_tran;
        }
        m_Transitions.clear();

        for( auto entry : m_IntraNodeTransmissionsMap )
        {
            delete entry.second;
        }
        m_IntraNodeTransmissionsMap.clear();
    }

    void IndividualProperty::Read( int idx, uint32_t externalNodeId, const JsonObjectDemog& rDemog, bool isNotFirstNode )
    {
        if( isNotFirstNode )
        {
            release_assert( m_Key == rDemog[ IP_NAME_KEY ].AsString() );
        }
        else
        {
            m_Key = rDemog[ IP_NAME_KEY ].AsString();
        }

        if( m_Key == IP_AGE_BIN_PROPERTY )
        {
            ReadPropertyAgeBin( idx, externalNodeId, rDemog, isNotFirstNode );
        }
        else
        {
            ReadProperty( idx, externalNodeId, rDemog, isNotFirstNode );
        }
        IPIntraNodeTransmissions* p_transmission = new IPIntraNodeTransmissions();
        p_transmission->Read( m_Key, rDemog, m_Values.Size() );
        m_IntraNodeTransmissionsMap[ externalNodeId ] = p_transmission;
    }

    void IndividualProperty::ReadProperty( int idx, uint32_t externalNodeId, const JsonObjectDemog& rDemog, bool isNotFirstNode )
    {
        if( !rDemog.Contains( IP_VALUES_KEY ) )
        {
            std::ostringstream badMap;
            badMap << "demographics[" << IP_KEY << "][" << idx << "]";
            throw BadMapKeyException( __FILE__, __LINE__, __FUNCTION__, badMap.str().c_str(), IP_VALUES_KEY );
        }
        if( !rDemog.Contains( IP_INIT_KEY ) )
        {
            std::ostringstream badMap;
            badMap << "demographics[" << IP_KEY << "][" << idx << "]";
            throw BadMapKeyException( __FILE__, __LINE__, __FUNCTION__, badMap.str().c_str(), IP_INIT_KEY );
        }

        auto num_values = rDemog[IP_VALUES_KEY].size();
        auto num_probs  = rDemog[IP_INIT_KEY  ].size();

        if( num_values != num_probs )
        {
            std::ostringstream msg;
            msg << "Number of Values in " << IP_VALUES_KEY << " ("
                << num_values
                << ") needs to be the same as number of values in "
                << IP_INIT_KEY
                << " ("
                << num_probs
                << ")."
                << std::endl;
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }
        else if( num_values == 0 )
        {
            std::ostringstream msg;
            msg << "demographics[" << IP_KEY << "][" << idx << "][" << IP_VALUES_KEY << "] (property=" << m_Key << ") cannot have zero values.";
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }
        else if( isNotFirstNode && (num_values != m_Values.Size()) )
        {
            std::stringstream ss;
            ss << "demographics[" << IP_KEY << "][" << idx << "][" << IP_VALUES_KEY << "] for key=" << m_Key << " and nodeId=" << externalNodeId << " has " << num_values << " values.\n";
            ss << "The previous node(s) had " << m_Values.Size() << " values.  All nodes must have the same keys and values.";
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }

        IPFactory::GetInstance()->CheckIpKeyInWhitelist( m_Key, num_values );

        float total_prob = 0.0;
        for( int val_idx = 0; val_idx < num_values; val_idx++ )
        {
            std::string       value        = rDemog[IP_VALUES_KEY][val_idx].AsString();
            ProbabilityNumber initial_dist = rDemog[IP_INIT_KEY  ][val_idx].AsDouble();
            std::string kv_str = IPFactory::CreateKeyValueString( m_Key, value );
            bool contains_kv = m_Values.Contains( kv_str );
            if( isNotFirstNode )
            {
                if( !contains_kv )
                {
                    std::ostringstream ss;
                    ss << "demographics[" << IP_KEY << "][" << idx << "] with property=" << m_Key << " for NodeId=" << externalNodeId << " has value=" << value << ".\n";
                    ss << "Previous node(s) do not have this value.  All nodes must have the same keys and values.";
                    throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
                }
                IPKeyValue kv = m_Values.Get( kv_str );
                kv.UpdateInitialDistribution( externalNodeId, initial_dist );
            }
            else
            {
                if( contains_kv )
                {
                    std::ostringstream ss;
                    ss << "demographics[" << IP_KEY << "][" << idx << "] with property=" << m_Key << " has a duplicate value = " << value ;
                    throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
                }
                IPKeyValueInternal* pkvi = new IPKeyValueInternal( this, value, externalNodeId, initial_dist );
                IPFactory::GetInstance()->AddKeyValue( pkvi );
                IPKeyValue kv( pkvi );
                m_Values.Add( kv );
            }
            total_prob += initial_dist;
        }

        if( (total_prob < 0.99999) || (1.000001 < total_prob) )
        {
            std::ostringstream ss;
            ss << "The values in demographics[" << IP_KEY << "][" << idx << "][" << IP_INIT_KEY << "] (property=" << m_Key << ") add up to " << total_prob << ".  They must add up to 1.0" ;
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }

        if( !isNotFirstNode )
        {
            if( rDemog.Contains( IP_TRANS_KEY ) )
            {
                for( int itran = 0 ; itran < rDemog[ IP_TRANS_KEY ].size() ; itran++ )
                {
                    IPTransition* p_tran = new IPTransition();
                    p_tran->Read( idx, itran, rDemog[ IP_TRANS_KEY ][ itran ], m_Key );
                    m_Transitions.push_back( p_tran );
                }
            }
        }
    }

    void IndividualProperty::ReadPropertyAgeBin( int idx, uint32_t externalNodeId, const JsonObjectDemog& rDemog, bool isNotFirstNode )
    {
        if( !rDemog.Contains( IP_AGE_BIN_EDGE_KEY ) )
        {
            std::ostringstream badMap;
            badMap << "demographics[" << IP_KEY << "][" << idx << "] (property = " << m_Key <<")";
            throw BadMapKeyException( __FILE__, __LINE__, __FUNCTION__, badMap.str().c_str(), IP_AGE_BIN_EDGE_KEY );
        }
        if( rDemog[ IP_AGE_BIN_EDGE_KEY ].size() <= 1 )
        {
            std::ostringstream msg;
            msg << "demographics[" << IP_KEY << "][" << idx << "][" << IP_AGE_BIN_EDGE_KEY << "] must have at least two values: 0 must be first and -1 is last.";
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }

        for( int age_bin_idx = 0 ; age_bin_idx < rDemog[ IP_AGE_BIN_EDGE_KEY ].size(); age_bin_idx++ )
        {
            float age_edge = rDemog[ IP_AGE_BIN_EDGE_KEY ][ age_bin_idx ].AsDouble();

            // ----------------------------------------------------------------------
            // --- Test edge value to make sure first value is 0 and last value is -1
            // ----------------------------------------------------------------------
            if( age_bin_idx == 0 )
            {
                if( age_edge != 0 )
                {
                    std::ostringstream errMsg;
                    errMsg << "demographics[" << IP_KEY << "][" << idx << "][" << IP_AGE_BIN_EDGE_KEY << "] must have at least two values: 0 must be first and -1 is last.  The first value cannot be " << age_edge ;
                    throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, errMsg.str().c_str() );
                }
                else
                {
                    continue;
                }
            }
            else if( age_bin_idx == (rDemog[ IP_AGE_BIN_EDGE_KEY ].size()-1) )
            {
                if( age_edge != -1 )
                {
                    std::ostringstream errMsg;
                    errMsg << "demographics[" << IP_KEY << "][" << idx << "][" << IP_AGE_BIN_EDGE_KEY << "] must have at least two values: 0 must be first and -1 is last.  The last value cannot be " << age_edge ;
                    throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, errMsg.str().c_str() );
                }
            }

            // ----------------------
            // --- Extract age range
            // ----------------------
            float min_age_years = rDemog[ IP_AGE_BIN_EDGE_KEY ][ age_bin_idx-1 ].AsDouble();
            float max_age_years = age_edge;
            if( max_age_years == -1 )
            {
                max_age_years = MAX_HUMAN_AGE;
            }

            if( min_age_years >= max_age_years )
            {
                std::ostringstream errMsg;
                errMsg << "demographics[" << IP_KEY << "][" << idx << "][" << IP_AGE_BIN_EDGE_KEY << "] must be in increasing order with the last value = -1." ;
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, errMsg.str().c_str() );
            }

            // ------------------------------------
            // --- Create value name from age range
            // ------------------------------------
            std::ostringstream ss;
            ss << IP_AGE_BIN_VALUE_PREFIX << min_age_years << "_To_" << max_age_years;
            std::string value = ss.str();

            // ---------------
            // --- Add value
            // ---------------
            bool contains_kv = m_Values.Contains( IPFactory::CreateKeyValueString( m_Key, value ) );
            if( isNotFirstNode )
            {
                if( !contains_kv )
                {
                    std::stringstream ss;
                    ss << "demographics[" << IP_KEY << "][" << idx << "][" << IP_VALUES_KEY << "] for key=" << m_Key << "  and nodeId" << externalNodeId ;
                    ss << " has value=" << value << " but the previous nodes do not have this value.  All nodes must have the same keys and values.";
                    throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
                }
            }
            else
            {
                if( contains_kv )
                {
                    std::ostringstream ss;
                    ss << "demographics[" << IP_KEY << "][" << idx << "][" << IP_VALUES_KEY << "] for key=" << m_Key << "  and nodeId" << externalNodeId ;
                    ss << ": Duplicate Value found: " << value;
                    throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
                }
                IPKeyValueInternal* pkvi = new IPKeyValueInternal( this, value, externalNodeId, 0.0 );
                IPFactory::GetInstance()->AddKeyValue( pkvi );
                IPKeyValue kv( pkvi );
                m_Values.Add( kv );
            }
        }

        if( !isNotFirstNode )
        {
            IPFactory::GetInstance()->CheckIpKeyInWhitelist( m_Key, m_Values.Size() );

            if( rDemog.Contains( IP_TRANS_KEY ) && (rDemog[ IP_TRANS_KEY ].size() > 0) )
            {
                std::ostringstream msg;
                msg << "demographics[" << IP_KEY << "][" << idx << "][" << IP_TRANS_KEY << "] has more than zero entries.  They are not allowed with property=" << IP_AGE_BIN_PROPERTY;
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
            }

            CreateAgeBinTransitions();
        }
    }

    void ExtractAges( const IPKeyValue& rTo, float* pMinAgeYears, float* pMaxAgeYears )
    {
        std::string val_str = rTo.GetValueAsString().substr( IP_AGE_BIN_VALUE_PREFIX.length() );
        int loc = val_str.find( "_To_" );
        std::string min_age_str = val_str.substr( 0, loc );
        std::string max_age_str = val_str.substr( loc+4 );
        sscanf_s(min_age_str.c_str(),"%f",pMinAgeYears);
        sscanf_s(max_age_str.c_str(),"%f",pMaxAgeYears);
    }

    void IndividualProperty::CreateAgeBinTransitions()
    {
        int index = 0;
        for( auto to_kv : m_Values )
        {
            float min_age_years = 0.0 ;
            float max_age_years = 0.0 ;
            ExtractAges( to_kv, &min_age_years, &max_age_years );

            IPTransition* p_tran = new IPTransition( IPKeyValue(), to_kv, IP_TRANS_TYPE_VALUE_TIMESTEP, 1.0, 0.0, -1.0, 1.0, 0.0, max_age_years, true, min_age_years, max_age_years );
            m_Transitions.push_back( p_tran );

            if( index > 0 )
            {
                // Give people calendars to move to next bucket.
                IPTransition* p_tran2 = new IPTransition( IPKeyValue(), to_kv, IP_TRANS_TYPE_VALUE_AGE, 1.0, 1.0, -1.0, 1.0, 0.0, min_age_years, true, 0.0, min_age_years );
                m_Transitions.push_back( p_tran2 );
            }
            index++;
        }
    }

    IPKey IndividualProperty::GetKey() const
    {
        return IPKey(m_Key);
    }

    const IPKeyValueContainer& IndividualProperty::GetValues() const
    {
        return m_Values;
    }

    IPKeyValue IndividualProperty::GetInitialValue( uint32_t externalNodeId, RANDOMBASE* pRNG )
    {
        float ran = pRNG->e();
        float prob = 0.0;

        for( auto kv : m_Values )
        {
            prob += kv.GetInitialDistribution( externalNodeId );
            if( prob >= ran )
            {
                return kv;
            }
        }
        std::ostringstream msg;
        msg << "Was not able to select an initial value for IP = " << m_Key;
        throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
    }

    std::vector<JsonObjectDemog> IndividualProperty::ConvertTransitions()
    {
        std::vector<JsonObjectDemog> main_list ;
        for( auto p_tran : m_Transitions )
        {
            std::vector<JsonObjectDemog> list = p_tran->ConvertToCampaignEvent( GetKey() );
            for( auto jod : list )
            {
                main_list.push_back( jod );
            }
        }
        return main_list;
    }

    const IPIntraNodeTransmissions& IndividualProperty::GetIntraNodeTransmissions( uint32_t externalNodeId ) const
    {
        return *m_IntraNodeTransmissionsMap.at( externalNodeId );
    }

    bool IndividualProperty::Compare( IndividualProperty* pLeft, IndividualProperty* pRight )
    {
        return (pLeft->GetKeyAsString() < pRight->GetKeyAsString());
    }

    // ------------------------------------------------------------------------
    // --- IPFactor
    // ------------------------------------------------------------------------

    IPFactory::IPFactory()
        : m_ExternalNodeIdOfFirst( UINT32_MAX )
        , m_WhiteListEnabled( true )
        , m_IPList()
        , m_KeyValueMap()
        , m_KeyWhiteList()
    {
        m_KeyWhiteList = std::set< std::string> ( KEY_WHITE_LIST_TMP, KEY_WHITE_LIST_TMP+sizeof_array(KEY_WHITE_LIST_TMP) );
    }

    IPFactory::~IPFactory()
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

    std::string IPFactory::CreateKeyValueString( const std::string& rKeyStr, const std::string& rValueStr )
    {
        return rKeyStr + std::string(KEY_VALUE_SEPARATOR) + rValueStr;
    }

    void IPFactory::ParseKeyValueString( const std::string& rKeyValueStr, std::string& rKeyStr, std::string& rValueStr )
    {
        IdmString kv_str = rKeyValueStr;
        auto fragments = kv_str.split(KEY_VALUE_SEPARATOR_CHAR);
        if( (fragments.size() != 2) || fragments[0].empty() || fragments[1].empty() )
        {
            std::ostringstream msg;
            msg << "Invalid Individual Property Key-Value string = '" << rKeyValueStr << "'.  Format is 'key:value'.";
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }
        rKeyStr   = fragments[0];
        rValueStr = fragments[1];
    }

    void IPFactory::CreateFactory()
    {
        if( Environment::getIPFactory() != nullptr )
        {
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, "IPFactory has already been created." );
        }
        Environment::setIPFactory( new IPFactory() );
    }

    void IPFactory::DeleteFactory()
    {
        IPFactory* p_factory = (IPFactory*)Environment::getIPFactory();
        delete p_factory;
        Environment::setIPFactory( nullptr );
    }

    IPFactory* IPFactory::GetInstance()
    {
        IPFactory* p_factory = (IPFactory*)Environment::getIPFactory();
        if( p_factory == nullptr )
        {
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, "IPFactory has not been created." );
        }
        return p_factory;
    }

    void IPFactory::Initialize( uint32_t externalNodeId, const JsonObjectDemog& rDemog, bool isWhitelistEnabled )
    {
        m_WhiteListEnabled = isWhitelistEnabled;

        if( externalNodeId == UINT32_MAX )
        {
            std::stringstream ss;
            ss << "You are not allowed to use an external Node ID of " << UINT32_MAX << ".";
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }

        if( m_ExternalNodeIdOfFirst == UINT32_MAX )
        {
            m_ExternalNodeIdOfFirst = externalNodeId;
        }

        if( !rDemog.Contains( IP_KEY ) )
        {
            return;
        }

        LOG_INFO_F( "%d Individual_Properties found in demographics for NodeId=%d\n", rDemog[IP_KEY].size(), externalNodeId );

        // Check that we're not using more than 2 axes in whitelist mode
        if( rDemog[IP_KEY].size() > 2 && isWhitelistEnabled )
        {
            std::ostringstream msg;
            msg << "Too many Individual Properties (" 
                << rDemog[IP_KEY].size()
                << "). Max is 2."
                << std::endl;
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }

        if( m_ExternalNodeIdOfFirst == externalNodeId )
        {
            for( int idx = 0; idx < rDemog[IP_KEY].size(); idx++ )
            {
                IndividualProperty* p_ip = new IndividualProperty();
                p_ip->Read( idx, externalNodeId, rDemog[IP_KEY][idx], false );
                m_IPList.push_back( p_ip );
            }

            // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            // !!! Needed only to stay in sync with previous version
            // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            std::sort( m_IPList.begin(), m_IPList.end(), IndividualProperty::Compare );
        }
        else
        {
            if( rDemog[IP_KEY].size() != m_IPList.size() )
            {
                std::stringstream ss;
                ss << "Individual Properties were first intialized for nodeId=" << m_ExternalNodeIdOfFirst << " and it had " << m_IPList.size() << " propertie(s).\n";
                ss << "nodeID=" << externalNodeId << " has " << rDemog[IP_KEY].size() << " propertie(s).  All nodes must have the same keys and values.";
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }
            for( int idx = 0; idx < rDemog[IP_KEY].size(); idx++ )
            {
                IndividualProperty* p_ip = GetIP( rDemog[ IP_KEY ][ idx ][ IP_NAME_KEY ].AsString(), "", false );
                if( p_ip == nullptr )
                {
                    std::stringstream ss;
                    ss << "Individual Properties were first initialized for node " << m_ExternalNodeIdOfFirst << ".\n";
                    ss << "nodeID=" << externalNodeId << " has '" << rDemog[IP_KEY][idx][IP_NAME_KEY].AsString() << "' which is not in the first node.\n";
                    ss << "All nodes must have the same keys and values (and in the same order).";
                    throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
                }
                else
                {
                    p_ip->Read( idx, externalNodeId, rDemog[ IP_KEY ][ idx ], true );
                }
            }
        }
    }

    void IPFactory::WriteTransitionsFile()
    {
        JsonObjectDemog event_array(JsonObjectDemog::JSON_OBJECT_ARRAY );
        for( auto p_ip : m_IPList )
        {
            std::vector<JsonObjectDemog> list = p_ip->ConvertTransitions();
            for( auto jod : list )
            {
                event_array.PushBack( jod );
            }
        }

        JsonObjectDemog out(JsonObjectDemog::JSON_OBJECT_OBJECT );
        out.Add( "Use_Defaults", 1 );
        out.Add( "Events", event_array );

        std::string fn = FileSystem::Concat( EnvPtr->OutputPath, std::string(transitions_dot_json_filename) );
        out.WriteToFile( fn.c_str() );
    }

    void IPFactory::CheckIpKeyInWhitelist( const std::string& rKey, int numValues )
    {
        if( m_WhiteListEnabled )
        {
            if( m_KeyWhiteList.count( rKey ) == 0 )
            {
                std::ostringstream msg;
                msg << "Invalid Individual Property key '" << rKey << "' found in demographics file. Use one of: ";
                for (auto& key : m_KeyWhiteList)
                {
                    msg << "'" << key<< "', " ;
                }
                std::string msg_str = msg.str();
                msg_str = msg_str.substr( 0, msg_str.length()-2 );
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
            }

            if (((numValues > 5) && (rKey != "Geographic")) || (numValues > 125))
            {
                std::ostringstream msg;
                msg << "Too many values for Individual Property key " << rKey << ".  This key has " << numValues << " and the limit is 5, except for Geographic, which is 125." << std::endl;
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
            }
        }
    }

    void IPFactory::AddIP( uint32_t externalNodeId, const std::string& rKeyStr, const std::map<std::string,float>& rValues )
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
            known_keys = known_keys.substr(0, known_keys.length()-2 );
            std::ostringstream msg;
            msg << "Found existing IndividualProperty key = '" << rKeyStr << "'.  Can't create duplicate key.  ";
            msg << "Known keys are: " << known_keys;
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }

        IndividualProperty* p_ip = new IndividualProperty( externalNodeId, rKeyStr, rValues );
        m_IPList.push_back( p_ip );
    }

    IndividualProperty* IPFactory::GetIP( const std::string& rKey, const std::string& rParameterName, bool throwOnNotFound )
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

    std::vector<std::string> IPFactory::GetAllPossibleKeyValueCombinations() const
    {
        std::vector<std::string> possible_list;
        if( m_IPList.size() > 0 )
        {
            IPKeyValueContainer container = m_IPList[0]->GetValues();
            for( auto kv : container )
            {
                std::string kv_str = kv.ToString();
                possible_list.push_back( kv_str );
            }
        }
        for( int i = 1 ; i < m_IPList.size() ; i++ )
        {
            std::vector<std::string> new_list;
            IPKeyValueContainer container = m_IPList[i]->GetValues();
            for( auto kv : container )
            {
                std::string kv_str = kv.ToString();
                for( auto pv : possible_list )
                {
                    std::string new_value = pv + std::string(PROP_SEPARATOR) + kv_str;
                    new_list.push_back( new_value );
                }
            }
            possible_list = new_list;
        }
        return possible_list;
    }

    void IPFactory::AddKeyValue( IPKeyValueInternal* pkvi )
    {
        if( m_KeyValueMap.count( pkvi->m_KeyValueString ) != 0 )
        {
            std::ostringstream msg;
            msg << "The IndividualProperty key-value = " << pkvi->m_KeyValueString << " already exists.  ";
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }
        m_KeyValueMap[ pkvi->m_KeyValueString ] = pkvi;
    }

    IPKeyValueInternal* IPFactory::GetKeyValue( const std::string& rKeyValueString, const std::string& rParameterName )
    {
        if( m_KeyValueMap.count( rKeyValueString ) == 0 )
        {
            std::string keyStr, valueStr;
            IPFactory::ParseKeyValueString( rKeyValueString, keyStr, valueStr );
            IndividualProperty* pip = IPFactory::GetInstance()->GetIP( keyStr, rParameterName, false );
            std::ostringstream msg;
            if( !rParameterName.empty() )
            {
                msg << "Parameter '" << rParameterName << "' is invalid.  ";
            }
            if( pip == nullptr )
            {
                std::string keys = IPFactory::GetKeysAsString();
                msg << "Could not find the key("<< keyStr << ") for the key-value=" << rKeyValueString << ".  Possible keys are: " << keys;
            }
            else
            {
                std::string values = pip->GetValues().GetValuesToString();
                msg << "Could not find the value(" << valueStr << ") for the key(" << keyStr << ").  Possible values for the key are: " << values;
            }
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }

        return m_KeyValueMap.at( rKeyValueString );
    }

    IPKeyValueContainer IPFactory::GetInitialValues( uint32_t externalNodeId, RANDOMBASE* pRNG ) const
    {
        IPKeyValueContainer properties;

        for( auto pIP : m_IPList )
        {
            if( pIP->GetKeyAsString() != IP_AGE_BIN_PROPERTY )
            {
                IPKeyValue init_val = pIP->GetInitialValue( externalNodeId, pRNG );
                properties.Add( init_val );
            }
            else
            {
                // keep random numbers in sync with previous version
                float ran = pRNG->e();
            }
        }
        return properties;
    }

    std::set< std::string > IPFactory::GetKeysAsStringSet() const
    {
        std::set<std::string> keys;
        for( auto pIP : m_IPList )
        {
            keys.insert( pIP->GetKey().ToString() );
        }
        return keys;
    }

    std::string IPFactory::GetKeysAsString() const
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
