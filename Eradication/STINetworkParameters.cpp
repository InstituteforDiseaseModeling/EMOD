/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "STINetworkParameters.h"
#include "Exceptions.h"
#include "Node.h"
#include "IIndividualHuman.h"

static const char * _module = "STINetworkParameters";

namespace Kernel
{
    // ------------------------------------------------------------------------
    // --- STINetworkParameters
    // ------------------------------------------------------------------------

    STINetworkParameters::STINetworkParameters()
    : key_colon_val("UNINITIALIZED")
    , extra_relational_flag_type(ExtraRelationalFlagType::Correlated)
    {
        memset( prob_extra_relational, 0, sizeof(float)*RelationshipType::COUNT*Gender::COUNT );
        memset( max_simultaneous_rels, 0, sizeof(float)*RelationshipType::COUNT*Gender::COUNT );
    }

    STINetworkParameters::STINetworkParameters( const std::string& key )
    : key_colon_val(key)
    , extra_relational_flag_type(ExtraRelationalFlagType::Correlated)
    {
        memset( prob_extra_relational, 0, sizeof(float)*RelationshipType::COUNT*Gender::COUNT );
        memset( max_simultaneous_rels, 0, sizeof(float)*RelationshipType::COUNT*Gender::COUNT );
    }

    STINetworkParameters::~STINetworkParameters()
    { 
        LOG_DEBUG( "dtor\n" );
    }

    STINetworkParameters* STINetworkParameters::CreateSTINetworkParameters( const std::string& key, const Configuration* pConfig )
    { 
        LOG_DEBUG_F( "Create STINetworkParameters for key %s\n", key.c_str() );

        STINetworkParameters* params = _new_ STINetworkParameters(key);
        if( !JsonConfigurable::_dryrun )
        {
            params->Configure( pConfig );
        }
        LOG_DEBUG( "END STINetworkParameters\n" );
        return params;
    }

    bool STINetworkParameters::Configure( const ::Configuration *config )
    {
        LOG_DEBUG( "Configure\n" );

        initConfig( "Extra_Relational_Flag_Type", extra_relational_flag_type, config, MetadataDescriptor::Enum("extra_relational_flag_type", STI_Extra_Relational_Flag_Type_DESC_TEXT, MDD_ENUM_ARGS(ExtraRelationalFlagType)) ); 
        initConfigTypeMap( "Prob_Extra_Transitory_Relationship_Male", &(prob_extra_relational[RelationshipType::TRANSITORY][Gender::MALE]), STI_Prob_Extra_Transitory_Relationship_Male_DESC_TEXT, 0.0, 1.0f, 0.1f );
        initConfigTypeMap( "Prob_Extra_Transitory_Relationship_Female", &(prob_extra_relational[RelationshipType::TRANSITORY][Gender::FEMALE]), STI_Prob_Extra_Transitory_Relationship_Female_DESC_TEXT, 0.0, 1.0f, 0.1f );

        if( extra_relational_flag_type == ExtraRelationalFlagType::Independent )
        {
            initConfigTypeMap( "Prob_Extra_Informal_Relationship_Male", &(prob_extra_relational[RelationshipType::INFORMAL][Gender::MALE]), STI_Prob_Extra_Informal_Relationship_Male_DESC_TEXT, 0.0, 1.0f, 0.1f );
            initConfigTypeMap( "Prob_Extra_Marital_Relationship_Male", &(prob_extra_relational[RelationshipType::MARITAL][Gender::MALE]), STI_Prob_Extra_Marital_Relationship_Male_DESC_TEXT, 0.0, 1.0f, 0.1f );

            initConfigTypeMap( "Prob_Extra_Informal_Relationship_Female", &(prob_extra_relational[RelationshipType::INFORMAL][Gender::FEMALE]), STI_Prob_Extra_Informal_Relationship_Female_DESC_TEXT, 0.0, 1.0f, 0.1f );
            initConfigTypeMap( "Prob_Extra_Marital_Relationship_Female", &(prob_extra_relational[RelationshipType::MARITAL][Gender::FEMALE]), STI_Prob_Extra_Marital_Relationship_Female_DESC_TEXT, 0.0, 1.0f, 0.1f );
        }
        else
        {
            initConfigTypeMap( "Prob_Extra_Informal_Given_Extra_Transitory_Male", &(prob_extra_relational[RelationshipType::INFORMAL][Gender::MALE]), STI_Prob_Extra_Informal_Given_Extra_Transitory_Male_DESC_TEXT, 0.0, 1.0f, 0.1f );
            initConfigTypeMap( "Prob_Extra_Marital_Given_Extra_Informal_Male", &(prob_extra_relational[RelationshipType::MARITAL][Gender::MALE]), STI_Prob_Extra_Marital_Given_Extra_Informal_Male_DESC_TEXT, 0.0, 1.0f, 0.1f );

            initConfigTypeMap( "Prob_Extra_Informal_Given_Extra_Transitory_Female", &(prob_extra_relational[RelationshipType::INFORMAL][Gender::FEMALE]), STI_Prob_Extra_Informal_Given_Extra_Transitory_Female_DESC_TEXT, 0.0, 1.0f, 0.1f );
            initConfigTypeMap( "Prob_Extra_Marital_Given_Extra_Informal_Female", &(prob_extra_relational[RelationshipType::MARITAL][Gender::FEMALE]), STI_Prob_Extra_Marital_Given_Extra_Informal_Female_DESC_TEXT, 0.0, 1.0f, 0.1f );
        }

        initConfigTypeMap( "Max_Simultaneous_Transitory_Relationships_Males", &max_simultaneous_rels[RelationshipType::TRANSITORY][Gender::MALE], STI_Max_Simultaneous_Transitory_Relationships_Males_DESC_TEXT, 0, 10, 1 );
        initConfigTypeMap( "Max_Simultaneous_Informal_Relationships_Males", &max_simultaneous_rels[RelationshipType::INFORMAL][Gender::MALE], STI_Max_Simultaneous_Informal_Relationships_Males_DESC_TEXT, 0, 10, 1 );
        initConfigTypeMap( "Max_Simultaneous_Marital_Relationships_Males", &max_simultaneous_rels[RelationshipType::MARITAL][Gender::MALE], STI_Max_Simultaneous_Marital_Relationships_Males_DESC_TEXT, 0, 10, 1 );
        initConfigTypeMap( "Max_Simultaneous_Transitory_Relationships_Females", &max_simultaneous_rels[RelationshipType::TRANSITORY][Gender::FEMALE], STI_Max_Simultaneous_Transitory_Relationships_Females_DESC_TEXT, 0, 10, 1 );
        initConfigTypeMap( "Max_Simultaneous_Informal_Relationships_Females", &max_simultaneous_rels[RelationshipType::INFORMAL][Gender::FEMALE], STI_Max_Simultaneous_Informal_Relationships_Females_DESC_TEXT, 0, 10, 1 );
        initConfigTypeMap( "Max_Simultaneous_Marital_Relationships_Females", &max_simultaneous_rels[RelationshipType::MARITAL][Gender::FEMALE], STI_Max_Simultaneous_Marital_Relationships_Females_DESC_TEXT, 0, 10, 1 ); 

        return JsonConfigurable::Configure( config );
    }

    QueryResult STINetworkParameters::QueryInterface( iid_t iid, void **ppvObject )
    {
        throw NotYetImplementedException(  __FILE__, __LINE__, __FUNCTION__ );
    }

    void STINetworkParameters::serialize( Kernel::IArchive& ar, STINetworkParameters& parameters )
    {
        ar.startObject();

        ar.labelElement("key_colon_val") & parameters.key_colon_val;
        ar.labelElement("extra_relational_flag_type") & (uint32_t&)parameters.extra_relational_flag_type;

        size_t count = RelationshipType::COUNT;

        ar.labelElement("prob_extra_relational");
        ar.startArray(count);
        for( int i = 0 ; i < RelationshipType::COUNT ; i++ )
        {
            ar.serialize( parameters.prob_extra_relational[i], Gender::COUNT );
        }
        ar.endArray();

        ar.labelElement("max_simultaneous_rels");
        ar.startArray(count);
        for( int i = 0 ; i < RelationshipType::COUNT ; i++ )
        {
            ar.serialize( parameters.max_simultaneous_rels[i], Gender::COUNT );
        }
        ar.endArray();
        ar.endObject();
    }

    // ------------------------------------------------------------------------
    // --- STINetworkParametersMap
    // ------------------------------------------------------------------------

    STINetworkParametersMap::STINetworkParametersMap()
        : has_ip_params(false)
        , have_properties_been_validated(false)
        , property_name("NONE")
        , param_map()
    {
    }

    STINetworkParametersMap::~STINetworkParametersMap()
    { 
    }

    QueryResult STINetworkParametersMap::QueryInterface( iid_t iid, void **ppvObject )
    {
        throw NotYetImplementedException(  __FILE__, __LINE__, __FUNCTION__ );
    }

    void STINetworkParametersMap::ConfigureFromJsonAndKey( const Configuration* inputJson, 
                                                           const std::string& key )
    {
        // Temporary object created so we can 'operate' on json with the desired tools
        auto config = Configuration::CopyFromElement( (*inputJson)[key] );

        initConfigTypeMap( "Individual_Property_Name", &property_name, "TBD", "NONE" );
        JsonConfigurable::Configure( config );

        const auto& net_params_jo = json_cast<const json::Object&>( (*config) );
        for( auto data = net_params_jo.Begin(); data != net_params_jo.End(); ++data )
        {
            if( data->name != "Individual_Property_Name" )
            {
                Configuration* p_params_config = Configuration::CopyFromElement( data->element );

                STINetworkParameters * net_params_by_prop = STINetworkParameters::CreateSTINetworkParameters( data->name, p_params_config );
                param_map[ data->name ] = net_params_by_prop;

                delete p_params_config;
                p_params_config = nullptr;
            }
        }
        delete config;
        config = nullptr;

        if( param_map.size() == 0 )
        {
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, "Zero STINetworkParameters were defined.  You need to at least define a 'NONE' set of parameters." );
        }

        has_ip_params = param_map.size() > 1 ;
    }

    json::QuickBuilder STINetworkParametersMap::GetSchema()
    {
        STINetworkParameters tmp_params("NONE") ;\
        if( JsonConfigurable::_dryrun )
        {
            tmp_params.Configure( nullptr );
        }

        json::QuickBuilder schema( jsonSchemaBase );
        auto tn = JsonConfigurable::_typename_label();
        auto ts = JsonConfigurable::_typeschema_label();
        schema[ tn ] = json::String( "idmType:STINetworkParametersMap" );

        schema[ts] = json::Object();
        schema[ts]["Individual_Property_Name"] = json::Object();
        schema[ts]["Individual_Property_Name"][ "type" ] = json::String( "Constrained String" );
        schema[ts]["Individual_Property_Name"][ "constraints" ] = json::String( "<demographics>::Defaults.Individual_Properties.*.Property.<keys> or NONE" );
        schema[ts]["Individual_Property_Name"][ "description" ] = json::String( STI_Network_IP_Name_DESC_TEXT );
        schema[ts]["<Individual Property Value>"] = tmp_params.GetSchema().As<Object>();

        return schema;
    }

    const STINetworkParameters* STINetworkParametersMap::GetParameters( IIndividualHuman* pIndividual, 
                                                                        const char *prop, 
                                                                        const char* new_value )
    {
        if( !HasIndividualPropertyParams() )
        {
            // User does not want network parameters to vary by IP.  Use NONE as the default value 
            return GetFirst();
        }

        if( (prop != nullptr) && (new_value != nullptr) )
        {
            const STINetworkParameters* p_new_net_params = Find( prop, new_value );
            return p_new_net_params ;
        }

        // If we get here, it should be the first pass (called with default NULL values)

        tProperties* pProp = pIndividual->GetProperties();

        for (auto& pair : (*pProp))
        {
            const STINetworkParameters* p_new_net_params = Find( pair.first, pair.second );
            if( p_new_net_params != nullptr )
            {
                return p_new_net_params;
            }
        }

        // DJK: Being careful not to throw if PropertyChanger changes an unrelated IP
        if( !HasIndividualPropertyParams() )
        {
            std::ostringstream msg;
            msg << "Individual "
                <<  pIndividual->GetSuid().data
                << " could not determine property-dependent network parameters.  Check configuration of STI_Network_Params_By_Property in config and IndividualProperties in demographics"
                << std::endl;
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }

        return nullptr ;
    }

    bool STINetworkParametersMap::HasIndividualPropertyParams()
    {
        if( !have_properties_been_validated )
        {
            have_properties_been_validated = true ;
            ValidateProperties();
        }
        return has_ip_params ; 
    }

    const STINetworkParameters* STINetworkParametersMap::GetFirst()
    {
        release_assert( param_map.size() > 0 );
        return param_map.cbegin()->second ;
    }

    const STINetworkParameters* STINetworkParametersMap::Find( const std::string& rPropertyName, 
                                                               const std::string& rPropertyValue )
    {
        if( property_name != rPropertyName )
        {
            return nullptr ;
        }
        else if( param_map.count( rPropertyValue ) <= 0 )
        {
            return nullptr ;
        }
        else
        {
            return param_map[ rPropertyValue ] ;
        }
    }

    void STINetworkParametersMap::ValidateProperties()
    {
        if( property_name == "NONE" )
        {
            return;
        }

        std::vector<std::string> property_values ;
        try
        {
            property_values = Node::GetIndividualPropertyValuesList( property_name );
        }
        catch( DetailedException& )
        {
            std::stringstream ss ;
            ss <<  "Individual_Property_Name(=" << property_name << ") is not defined in the demographics." ;
            throw GeneralConfigurationException(__FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }

        bool invalid_axes = property_values.size() != param_map.size();
        for( auto entry : param_map )
        {
            invalid_axes |= std::find( property_values.begin(), property_values.end(), entry.first ) == property_values.end() ;
        }

        if( invalid_axes )
        {
            std::stringstream map_ip_values ;
            for( auto entry : param_map )
            {
                map_ip_values << "'" << entry.first << "' " ;
            }

            std::stringstream pv_values ;
            for( auto pv : property_values )
            {
                pv_values << "'" << pv << "' " ;
            }

            std::stringstream ss ;
            ss << "The STI Network Parameters requires that the element names"
               << "(=" << map_ip_values.str() << ") "
               << "match the property values"
               << "(=" << pv_values.str() << ") "
               << "defined in the demographics for Property=" << property_name << "." ;
            throw GeneralConfigurationException(__FILE__, __LINE__, __FUNCTION__, ss.str().c_str() ) ;
        }
    }
}
