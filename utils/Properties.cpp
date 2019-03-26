/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "Properties.h"
#include "PropertiesString.h"
#include "BasePropertiesTemplates.h"
#include "Log.h"
#include "IdmString.h"
#include "FileSystem.h"
#include "NoCrtWarnings.h"
#include "Types.h"
#include "Debug.h"
#include "Common.h"
#include "RANDOM.h"

SETUP_LOGGING( "Properties" )

const char* IP_AGE_BIN_PROPERTY = "Age_Bin";
const char* IP_AGE_BIN_VALUE_0  = "Age_Bin_Property_From_0";
std::string IP_AGE_BIN_VALUE_PREFIX  = "Age_Bin_Property_From_";

const char* IP_KEY                    = "IndividualProperties";
extern const char* IP_NAME_KEY;    // = "Property";
extern const char* IP_VALUES_KEY;  // = "Values";
extern const char* IP_INIT_KEY;    // = "Initial_Distribution";
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

namespace Kernel
{
    const char* IPFactory::transitions_dot_json_filename = "transitions.json";

    // ------------------------------------------------------------------------
    // --- IPKey
    // ------------------------------------------------------------------------

    const char* IPKey::GetConstrainedStringConstraintKey()
    {
        return "<demographics>::*.Individual_Properties.*.Property";
    }

    const char* IPKey::GetConstrainedStringConstraintValue()
    {
        return "<demographics>::*.Individual_Properties.*.Values";
    }

    const char* IPKey::GetConstrainedStringConstraintKeyValue()
    {
        return "'<demographics>::*.Individual_Properties.*.Property':'<demographics>::*.Individual_Properties.*.Values'";
    }

    const char* IPKey::GetConstrainedStringDescriptionKey()
    {
        return "Individual Property Key from demographics file.";
    }

    const char* IPKey::GetConstrainedStringDescriptionValue()
    {
        return "Individual Property Value from demographics file.";
    }

    IPKey::IPKey()
        : BaseKey()
    {
    }

    IPKey::IPKey( const BaseProperty* pip )
        : BaseKey( pip )
    {
    }

    IPKey::IPKey( const std::string& rKeyStr )
        : BaseKey()
    {
        m_pIP = IPFactory::GetInstance()->GetIP( rKeyStr );
    }

    IPKey::~IPKey()
    {
    }

    IPKey& IPKey::operator=( const std::string& rKeyStr )
    {
        m_pIP = IPFactory::GetInstance()->GetIP( rKeyStr, m_ParameterName );
        return *this;
    }

    bool IPKey::operator==( const IPKey& rThat ) const
    {
        return BaseKey::operator==( rThat );
    }

    bool IPKey::operator!=( const IPKey& rThat ) const
    {
        return !operator==( rThat ); 
    }

    static void key_assign_func( BaseKey* pkv, const std::string& rKeyStr )
    {
        IPKey* p_key = static_cast<IPKey*>(pkv);
        *p_key = rKeyStr;
    }

    void IPKey::serialize( IArchive& ar, IPKey& key )
    {
        BaseKey::serialize( ar, key, key_assign_func );
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
                                                         (to_value_str.find(IP_AGE_BIN_VALUE_0) !=  std::string::npos) ) )
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
        : BaseKeyValue()
    {
    }

    IPKeyValue::IPKeyValue( const std::string& rKeyValueStr )
        : BaseKeyValue()
    {
        m_pInternal = IPFactory::GetInstance()->GetKeyValue<IPKeyValueContainer>( IP_KEY, rKeyValueStr );
    }

    IPKeyValue::IPKeyValue( const std::string& rKeyStr, const std::string& rValueStr )
        : BaseKeyValue()
    {
        std::string kv_str = IPFactory::CreateKeyValueString( rKeyStr, rValueStr );
        m_pInternal = IPFactory::GetInstance()->GetKeyValue<IPKeyValueContainer>( IP_KEY, kv_str );
    }

    IPKeyValue::IPKeyValue( KeyValueInternal* pkvi )
        : BaseKeyValue( pkvi )
    {
    }

    IPKeyValue::~IPKeyValue()
    {
    }

    IPKeyValue& IPKeyValue::operator=( const std::string& rKeyValueStr )
    {
        m_pInternal =IPFactory::GetInstance()->GetKeyValue<IPKeyValueContainer>( IP_KEY, rKeyValueStr, m_ParameterName );
        return *this;
    }

    bool IPKeyValue::operator==( const IPKeyValue& rThat ) const 
    {
        return BaseKeyValue::operator==( rThat );
    }

    bool IPKeyValue::operator!=( const IPKeyValue& rThat ) const
    {
        return !operator==( rThat );
    }

    double IPKeyValue::GetInitialDistribution( uint32_t externalNodeId ) const
    {
        if( m_pInternal == nullptr )
        {
            throw NullPointerException( __FILE__, __LINE__, __FUNCTION__, "m_pInternal", "IPKeyValueInternal" );
        }
        return m_pInternal->GetInitialDistribution( externalNodeId );
    }

    static void key_value_assign_func( BaseKeyValue* pkva, const std::string& rKvStr )
    {
        IPKeyValue* p_kv = static_cast<IPKeyValue*>(pkva);
        *p_kv = rKvStr;
    }

    void IPKeyValue::serialize( IArchive& ar, IPKeyValue& kv )
    {
        BaseKeyValue::serialize( ar, kv, key_value_assign_func );
    }

    // ------------------------------------------------------------------------
    // --- IPKeyValueIterator
    // ------------------------------------------------------------------------
    // See BasePropertiesTempaltes.h for template classes/methods

    IPKeyValueIterator::IPKeyValueIterator( std::vector<KeyValueInternal*>::const_iterator it )
        : BaseIterator<IPKeyValue>( it )
    {
    }

    IPKeyValueIterator::~IPKeyValueIterator()
    {
    }

    bool IPKeyValueIterator::operator==( const IPKeyValueIterator& rThat ) const
    {
        return BaseIterator<IPKeyValue>::operator==( rThat );
    }

    bool IPKeyValueIterator::operator!=( const IPKeyValueIterator& rThat ) const
    {
        return !operator==( rThat );
    }

    IPKeyValueIterator& IPKeyValueIterator::operator++()
    {
        BaseIterator<IPKeyValue>::operator++();
        return *this;
    }

    // ------------------------------------------------------------------------
    // --- IPKeyValueContainer
    // ------------------------------------------------------------------------
    // See BasePropertiesTempaltes.h for template classes/methods

    IPKeyValueContainer::IPKeyValueContainer()
        : BaseKeyValueContainer<IPKey,IPKeyValue, IPKeyValueIterator>()
    {
    }

    IPKeyValueContainer::IPKeyValueContainer( const std::vector<KeyValueInternal*>& rInternalList )
        : BaseKeyValueContainer<IPKey, IPKeyValue, IPKeyValueIterator>( rInternalList )
    {
    }

    IPKeyValueContainer::IPKeyValueContainer( const IPKeyValueContainer& rThat )
        : BaseKeyValueContainer<IPKey, IPKeyValue, IPKeyValueIterator>( rThat )
    {
    }

    IPKeyValueContainer::~IPKeyValueContainer()
    {
    }

    IPKeyValueContainer& IPKeyValueContainer::operator=( const IPKeyValueContainer& rThat )
    {
        BaseKeyValueContainer<IPKey, IPKeyValue, IPKeyValueIterator>::operator=( rThat );
        return *this;
    }

    bool IPKeyValueContainer::operator==( const IPKeyValueContainer& rThat ) const
    {
        return BaseKeyValueContainer<IPKey, IPKeyValue, IPKeyValueIterator>::operator==( rThat );
    }

    bool IPKeyValueContainer::operator!=( const IPKeyValueContainer& rThat ) const
    {
        return !operator==( rThat );
    }

    void IPKeyValueContainer::Add( const IPKeyValue& rKeyValue )
    {
        BaseKeyValueContainer<IPKey, IPKeyValue, IPKeyValueIterator>::Add( rKeyValue.m_pInternal );
    }

    void IPKeyValueContainer::Remove( const IPKeyValue& rKeyValue )
    {
        BaseKeyValueContainer<IPKey, IPKeyValue, IPKeyValueIterator>::Remove( rKeyValue.m_pInternal );
    }

    void IPKeyValueContainer::Set( const IPKeyValue& rNewKeyValue )
    {
        BaseKeyValueContainer<IPKey, IPKeyValue, IPKeyValueIterator>::Set( rNewKeyValue.m_pInternal );
    }

    tProperties IPKeyValueContainer::GetOldVersion() const
    {
        tProperties old_map;
        for( auto p_kvi : m_Vector )
        {
            old_map.emplace( p_kvi->GetProperty()->GetKeyAsString(), p_kvi->GetValueAsString() );
        }
        return old_map;
    }

    IPKeyValue IPKeyValueContainer::FindFirst( const IPKeyValueContainer& rContainer ) const
    {
        return IPKeyValue( BaseKeyValueContainer::FindFirst( rContainer ) );
    }

    // ------------------------------------------------------------------------
    // --- IPIntraNodeTransmission
    // ------------------------------------------------------------------------

    IPIntraNodeTransmission::IPIntraNodeTransmission()
        : m_RouteName("contact")
        , m_Matrix()
    {
    }

    IPIntraNodeTransmission::~IPIntraNodeTransmission()
    {
    }

    void IPIntraNodeTransmission::ReadTxMatrix( const std::string& rKeyStr, const JsonObjectDemog& rDemog, int numValues )
    {
        if( rDemog.IsArray() &&
            rDemog.size() != numValues )
        {
            std::ostringstream msg;
            msg << "Invalid Transmission Matrix for property '" << rKeyStr << "'.  It has "
                << rDemog.size() << " rows when it should have " << numValues
                << ".  It should be square with one row/col per value for the property." ;
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }

        std::cout << "Parsing the matrix (rows & cols) itself." << std::endl;
        for( int i = 0 ; i < rDemog.size() ; i++ )
        {
            if( rDemog[i].size() != numValues )
            {
                std::ostringstream msg;
                msg << "Invalid Transmission Matrix for property '" << rKeyStr << "'.  Row " << i << " has "
                    << rDemog[i].size() << " columns when it should have " << numValues
                    << ".  It should be square with one row/col per value for the property." ;
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
            }

            std::vector<float> row ;
            for( int j = 0 ; j < rDemog[i].size() ; j++ )
            {
                float val = rDemog[i][j].AsDouble();
                row.push_back( val );
            }
            m_Matrix.push_back( row );
        }
    }

    void IPIntraNodeTransmission::Read( const std::string& rKeyStr, const JsonObjectDemog& rDemog, int numValues )
    {
        if( rDemog.Contains( IP_TM_KEY ) )
        {
            std::cout << "Found 'Transmission_Matrix' key." << std::endl;
            if( !rDemog[ IP_TM_KEY ].Contains( IP_TM_MATRIX_KEY ) )
            {
                std::cout << "Did NOT find 'Matrix' key." << std::endl;
                // Going to assume we have "version 2" format (multiroute), which is a map of routes to Matrices. Possible values are: "contact" and "environmental".
                std::set< std::string > routeNames { "contact", "environmental" };
                for( auto route : routeNames )
                {
                    if( rDemog[ IP_TM_KEY ].Contains( route.c_str() ) )
                    { 
                        if( rDemog[ IP_TM_KEY ][ route.c_str() ].Contains( IP_TM_MATRIX_KEY ) )
                        {
                            std::cout << "Found 'Matrix' key under route." << std::endl;
                            ReadTxMatrix( rKeyStr, rDemog[ IP_TM_KEY ][ route.c_str() ][ IP_TM_MATRIX_KEY ], numValues );
                            //m_RouteToMatrixMap.insert( std::make_pair( route, m_Matrix.back() ) );
                            m_RouteToMatrixMap[ route ] = m_Matrix;
                            m_Matrix.clear();
                        }
                    }
                }
                return;
            }

            if( rDemog[ IP_TM_KEY ].Contains( IP_TM_ROUTE_KEY ) )
            {
                std::cout << "Found 'Route' key." << std::endl;
                m_RouteName =  rDemog[ IP_TM_KEY ][ IP_TM_ROUTE_KEY ].AsString();
                std::transform(m_RouteName.begin(), m_RouteName.end(), m_RouteName.begin(), ::tolower);
            }
            else 
            {
                LOG_WARN_F("Missing '%s' in demographics for property '%s'. Will use default route 'contact'.\n",IP_TM_ROUTE_KEY, rKeyStr.c_str());
            }
            ReadTxMatrix( rKeyStr, rDemog[ IP_TM_KEY ][ IP_TM_MATRIX_KEY ], numValues );
        }
    }

    bool IPIntraNodeTransmission::HasMatrix() const
    {
        return m_Matrix.size() > 0 || m_RouteToMatrixMap.size() > 0;
    }

    const std::string& IPIntraNodeTransmission::GetRouteName() const
    {
        return m_RouteName;
    }

    const std::vector<std::vector<float>>& IPIntraNodeTransmission::GetMatrix() const
    {
        return m_Matrix;
    }

    const std::map< std::string, std::vector<std::vector<float>>>& IPIntraNodeTransmission::GetRouteToMatrixMap() const
    {
        return m_RouteToMatrixMap;
    }

    // ------------------------------------------------------------------------
    // --- IndividualProperty
    // ------------------------------------------------------------------------

    IndividualProperty::IndividualProperty()
        : BaseProperty()
        , m_Transitions()
        , m_IntraNodeTransmissionMap()
    {
    }

    IndividualProperty::IndividualProperty( uint32_t externalNodeId, const std::string& rKeyStr, const std::map<std::string,float>& rValues )
        : BaseProperty()
        , m_Transitions()
        , m_IntraNodeTransmissionMap()
    {
        m_Key = rKeyStr;

        float total_prob = 0.0;
        for( auto entry : rValues )
        {
            KeyValueInternal* pkvi = new KeyValueInternal( this, entry.first, externalNodeId, entry.second );
            IPFactory::GetInstance()->AddKeyValue( pkvi );
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

    IndividualProperty::~IndividualProperty()
    {
        for( auto p_tran : m_Transitions )
        {
            delete p_tran;
        }
        m_Transitions.clear();

        for( auto entry : m_IntraNodeTransmissionMap )
        {
            delete entry.second;
        }
        m_IntraNodeTransmissionMap.clear();
    }

    void IndividualProperty::Read( int idx, uint32_t externalNodeId, const JsonObjectDemog& rDemog, bool isNotFirstNode )
    {
        BaseProperty::Read( idx, externalNodeId, rDemog, isNotFirstNode );

        if( m_Key == IP_AGE_BIN_PROPERTY )
        {
            ReadPropertyAgeBin( idx, externalNodeId, rDemog, isNotFirstNode );
        }

        IPIntraNodeTransmission* p_transmission = new IPIntraNodeTransmission();
        p_transmission->Read( m_Key, rDemog, m_Values.size() );
        m_IntraNodeTransmissionMap[ externalNodeId ] = p_transmission;
    }

    KeyValueInternal* IndividualProperty::get_kvi_func( BaseFactory* pFact, const char* ip_key_str, const std::string& rKvStr )
    {
        IPFactory* p_ip_fact = static_cast<IPFactory*>(pFact);
        KeyValueInternal* p_kvi = p_ip_fact->GetKeyValue<IPKeyValueContainer>( ip_key_str, rKvStr );
        return p_kvi;
    }

    void IndividualProperty::ReadProperty( int idx, uint32_t externalNodeId, const JsonObjectDemog& rDemog, bool isNotFirstNode )
    {
        BaseProperty::ReadProperty( IP_KEY,
                                        IP_VALUES_KEY,
                                        IP_INIT_KEY,
                                        IPFactory::GetInstance(),
                                        idx,
                                        externalNodeId,
                                        rDemog,
                                        isNotFirstNode,
                                        IndividualProperty::get_kvi_func );

        if( !isNotFirstNode )
        {
            if( rDemog.Contains( IP_TRANS_KEY ) )
            {
                for( int itran = 0; itran < rDemog[ IP_TRANS_KEY ].size(); itran++ )
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
            bool contains_kv = Contains( m_Values, m_Key, value );
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
                KeyValueInternal* pkvi = new KeyValueInternal( this, value, externalNodeId, 0.0 );
                IPFactory::GetInstance()->AddKeyValue( pkvi );
                m_Values.push_back( pkvi );
            }
        }

        if( !isNotFirstNode )
        {
            IPFactory::GetInstance()->CheckIpKeyInWhitelist( IP_KEY, m_Key, m_Values.size() );

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

    std::vector<JsonObjectDemog> IndividualProperty::ConvertTransitions()
    {
        std::vector<JsonObjectDemog> main_list ;
        for( auto p_tran : m_Transitions )
        {
            std::vector<JsonObjectDemog> list = p_tran->ConvertToCampaignEvent( GetKey<IPKey>() );
            for( auto jod : list )
            {
                main_list.push_back( jod );
            }
        }
        return main_list;
    }

    const IPIntraNodeTransmission& IndividualProperty::GetIntraNodeTransmission( uint32_t externalNodeId ) const
    {
        return *m_IntraNodeTransmissionMap.at( externalNodeId );
    }

    // ------------------------------------------------------------------------
    // --- IPFactory
    // ------------------------------------------------------------------------

    IPFactory::IPFactory()
        : BaseFactory()
    {
    }

    IPFactory::~IPFactory()
    {
    }

    void IPFactory::ParseKeyValueString( const std::string& rKeyValueStr, std::string& rKeyStr, std::string& rValueStr )
    {
        BaseFactory::ParseKeyValueString( IP_KEY, rKeyValueStr, rKeyStr, rValueStr );
    }

    void IPFactory::CreateFactory()
    {
        if( Environment::getIPFactory() != nullptr )
        {
            return;
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
        return p_factory;
    }

    void IPFactory::Initialize( uint32_t externalNodeId, const JsonObjectDemog& rDemog, bool isWhitelistEnabled )
    {
       read_function_t read_fn =
            []( int idx, uint32_t externalNodeId, const JsonObjectDemog& rDemog, bool isNotFirstNode )
        {
           IndividualProperty* p_ip = new IndividualProperty();
           p_ip->Read( idx, externalNodeId, rDemog, isNotFirstNode );
           return p_ip;
       };

        BaseFactory::Initialize( IP_KEY, IP_NAME_KEY, read_fn, externalNodeId, rDemog, isWhitelistEnabled );
    }

    void IPFactory::WriteTransitionsFile()
    {
        JsonObjectDemog event_array(JsonObjectDemog::JSON_OBJECT_ARRAY );
        for( auto p_pa : m_IPList )
        {
            IndividualProperty* p_ip = static_cast<IndividualProperty*>(p_pa);
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

    BaseProperty* IPFactory::construct_ip( uint32_t externalNodeId,
                                               const std::string& rKeyStr,
                                               const std::map<std::string, float>& rValues )
    {
        return new IndividualProperty( externalNodeId, rKeyStr, rValues );
    }

    void IPFactory::AddIP( uint32_t externalNodeId, const std::string& rKeyStr, const std::map<std::string,float>& rValues )
    {
        BaseFactory::AddIP( externalNodeId, rKeyStr, rValues, IPFactory::construct_ip );
    }

    std::vector<IndividualProperty*> IPFactory::GetIPList() const
    {
        std::vector<IndividualProperty*> ret_list;
        for( auto p_pa : m_IPList )
        {
            ret_list.push_back( static_cast<IndividualProperty*>(p_pa) );
        }
        return ret_list;
    }

    bool IPFactory::HasIPs() const
    {
        return (m_IPList.size() > 0);
    }

    IndividualProperty* IPFactory::GetIP( const std::string& rKey, const std::string& rParameterName, bool throwOnNotFound )
    {
        BaseProperty* p_pa = BaseFactory::GetIP( rKey, rParameterName, throwOnNotFound );
        IndividualProperty* p_ip = static_cast<IndividualProperty*>(p_pa);
        return p_ip;
    }

    IPKeyValueContainer IPFactory::GetInitialValues( uint32_t externalNodeId, RANDOMBASE* pRNG ) const
    {
        IPKeyValueContainer properties;

        for( auto p_pa : m_IPList )
        {
            IndividualProperty* p_ip = static_cast<IndividualProperty*>(p_pa);
            if( p_ip->GetKeyAsString() != IP_AGE_BIN_PROPERTY )
            {
                IndividualProperty* p_ip = static_cast<IndividualProperty*>(p_pa);
                IPKeyValue init_val = p_ip->GetInitialValue<IPKeyValue>( externalNodeId, pRNG );
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

#if 0 // something not working here yet. function currently lives in PropertyReport as a static method.
    void
    IPFactory::GenerateAllPermutationsOnce(
        std::set< std::string > &keys,
        tKeyValuePair perm,
        tPermutations &permutationsSet
    )
    {
        if( keys.size() )
        {
            const std::string key = *keys.begin();
            keys.erase( key );
            const IndividualProperty * p_ip = IPFactory::GetInstance()->GetIP( key );
            for( auto kv : p_ip->GetValues<IPKeyValueContainer>() )
            {
                std::string value = kv.GetValueAsString();
                auto kvp = perm;
                kvp.insert( make_pair( key, value ) );
                GenerateAllPermutationsOnce( keys, kvp, permutationsSet );
            }
        }
        else
        { 
            permutationsSet.insert( perm );
        }
    }
#endif

    // -------------------------------------------------------------------------------
    // --- This defines the implementations for these templetes with these parameters.
    // --- If you comment these out, you will get unresolved externals when linking.
    // -------------------------------------------------------------------------------
    template IDMAPI IPKey BaseKeyValue::GetKey<IPKey>() const;
    template IDMAPI IPKey BaseProperty::GetKey<IPKey>() const;
    template IDMAPI IPKeyValueContainer BaseProperty::GetValues<IPKeyValueContainer>() const;
    template IDMAPI std::vector<std::string> BaseFactory::GetAllPossibleKeyValueCombinations<IPKeyValueContainer>() const;
    template IDMAPI KeyValueInternal* BaseFactory::GetKeyValue<IPKeyValueContainer>( const char* ip_key_str, const std::string& rKeyValueString, const std::string& rParameterName );
    template IDMAPI class BaseIterator<IPKeyValue>;
    template IDMAPI class BaseKeyValueContainer<IPKey, IPKeyValue, IPKeyValueIterator>;

}
