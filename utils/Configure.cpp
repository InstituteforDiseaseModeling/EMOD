/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "Configuration.h"
#include "ConfigurationImpl.h"
#include "Configure.h"
#include "Exceptions.h"
#include "IdmString.h"
#include "Log.h"

static const char * _module = "JsonConfigurable";

namespace Kernel
{
    /// NodeSetConfig
    NodeSetConfig::NodeSetConfig()
    {}

    NodeSetConfig::NodeSetConfig(json::QuickInterpreter* qi)
    : _json(*qi)
    {
    }

    json::QuickBuilder
    NodeSetConfig::GetSchema()
    {
        json::QuickBuilder schema( jsonSchemaBase );
        auto tn = JsonConfigurable::_typename_label();
        auto ts = JsonConfigurable::_typeschema_label();
        schema[ tn ] = json::String( "idmType:NodeSet" );
        schema[ ts ]= json::Object();
        schema[ ts ][ "base" ] = json::String( "interventions.idmType.NodeSet" );
        //json::Writer::Write( schema, std::cout );
        return schema;
    }

    void
    NodeSetConfig::ConfigureFromJsonAndKey(
        const Configuration* inputJson,
        const std::string& key
    )
    {
        if( !inputJson->Exist( key ) )
        {
            throw MissingParameterFromConfigurationException( __FILE__, __LINE__, __FUNCTION__, "campaign.json", key.c_str() );
        }

        _json = (*inputJson)[key];
        //std::cout << "NodeSetConfig::Configure called with json blob." << std::endl;
        //json::Writer::Write( _json, std::cout );
    }

    /// END NodeSetConfig

    /// EventConfig
    EventConfig::EventConfig()
    {}

    EventConfig::EventConfig(json::QuickInterpreter* qi)
        : _json(*qi)
    {
    }

    void
    EventConfig::ConfigureFromJsonAndKey(
        const Configuration* inputJson,
        const std::string& key
    )
    {
        if( !inputJson->Exist( key ) )
        {
            throw MissingParameterFromConfigurationException( __FILE__, __LINE__, __FUNCTION__, "campaign.json", key.c_str() );
        }
        _json = (*inputJson)[key];
        //std::cout << "EventConfig::Configure called with json blob." << std::endl;
        //json::Writer::Write( _json, std::cout );
    }

    json::QuickBuilder
    EventConfig::GetSchema()
    {
        json::QuickBuilder schema( jsonSchemaBase );
        auto tn = JsonConfigurable::_typename_label();
        auto ts = JsonConfigurable::_typeschema_label();
        schema[ tn ] = json::String( "idmType:EventCoordinator" );
        schema[ ts ]= json::Object();
        schema[ ts ][ "base" ] = json::String( "interventions.idmType.EventCoordinator" );
        //json::Writer::Write( schema, std::cout );
        return schema;
    }

    /// InterventionConfig
    InterventionConfig::InterventionConfig()
    //: _qi( _json )
    {}

    InterventionConfig::InterventionConfig(json::QuickInterpreter* qi)
        : _json(*qi)
        //, _qi(*qi)
    {
        //json::Writer::Write( _json, std::cout );
        //std::cout << "Yay, we can write out our _json!" << std::endl;
    }

    void
    InterventionConfig::ConfigureFromJsonAndKey(
        const Configuration* inputJson,
        const std::string& key
    )
    {
        if( !inputJson->Exist( key ) )
        {
            throw MissingParameterFromConfigurationException( __FILE__, __LINE__, __FUNCTION__, "campaign.json", key.c_str() );
        }
        // we need to set _json to inputJson
        _json = (*inputJson)[key];

        // This might be useful for debugging, but couldn't agree on how to whether to include or not. Uncomment if you need it.
        /*std::ostringstream msg;
        msg << "InterventionConfig::" << __FUNCTION__ << " called with json blob." << std::endl;
        json::Writer::Write( _json, msg );
        LOG_DEBUG_F( "%s", msg.str().c_str() );*/
    }

    json::QuickBuilder
    InterventionConfig::GetSchema()
    {
        json::QuickBuilder schema( jsonSchemaBase );
        auto tn = JsonConfigurable::_typename_label();
        auto ts = JsonConfigurable::_typeschema_label();
        schema[ tn ] = json::String( "idmType:Intervention" );
        schema[ ts ]= json::Object();
        schema[ ts ][ "base" ] = json::String( "interventions.idmAbstractType.Intervention" );
        //json::Writer::Write( schema, std::cout );
        return schema;
    }

    void InterventionConfig::serialize(IArchive& ar, InterventionConfig& config)
    {
#if defined(WIN32)
        if ( ar.IsWriter() )
        {
            std::ostringstream string_stream;
            json::Writer::Write( config._json, string_stream );
            ar & string_stream.str();
        }
        else
        {
            std::string json;
            ar & json;
            std::istringstream string_stream( json );
            json::Reader::Read( config._json, string_stream );
        }
#endif
    }

// clorton    template < class Archive >
// clorton    void serialize( Archive &ar, InterventionConfig& config, unsigned int file_version )
// clorton    {
// clorton        ar & config._json;
// clorton    }

    IndividualInterventionConfig::IndividualInterventionConfig()
    {
    }

    json::QuickBuilder
    IndividualInterventionConfig::GetSchema()
    {
        json::QuickBuilder schema = InterventionConfig::GetSchema();
        auto tn = JsonConfigurable::_typename_label();
        auto ts = JsonConfigurable::_typeschema_label();
        schema[ tn ] = json::String( "idmType:IndividualIntervention" );
        schema[ ts ][ "base" ] = json::String( "interventions.idmAbstractType.IndividualIntervention" );
        return schema;
    }

    json::QuickBuilder
    NodeInterventionConfig::GetSchema()
    {
        json::QuickBuilder schema = InterventionConfig::GetSchema();
        auto tn = JsonConfigurable::_typename_label();
        auto ts = JsonConfigurable::_typeschema_label();
        schema[ tn ] = json::String( "idmType:NodeIntervention" );
        schema[ ts ][ "base" ] = json::String( "interventions.idmAbstractType.NodeIntervention" );
        return schema;
    }

    /// END OF InterventionConfig


    /// WaningConfig
    WaningConfig::WaningConfig()
    {}

    WaningConfig::WaningConfig(json::QuickInterpreter* qi)
    : _json(*qi)
    {
    }

    json::QuickBuilder
    WaningConfig::GetSchema()
    {
        json::QuickBuilder schema( jsonSchemaBase );
        auto tn = JsonConfigurable::_typename_label();
        auto ts = JsonConfigurable::_typeschema_label();
        schema[ tn ] = json::String( "idmType:WaningEffect" );
        schema[ ts ]= json::Object();
        schema[ ts ][ "base" ] = json::String( "interventions.idmType.WaningEffect" );
        //json::Writer::Write( schema, std::cout );
        return schema;
    }

    void
    WaningConfig::ConfigureFromJsonAndKey(
        const Configuration* inputJson,
        const std::string& key
    )
    {
        if( !inputJson->Exist( key ) )
        {
            throw MissingParameterFromConfigurationException( __FILE__, __LINE__, __FUNCTION__, "campaign.json", key.c_str() );
        }
        _json = (*inputJson)[key];
        //std::cout << "WaningConfig::Configure called with json blob." << std::endl;
        //json::Writer::Write( _json, std::cout );
    }

    /// END WaningConfig

    std::map< std::string, IJsonConfigurable* > IJsonConfigurable::generic_container;

    namespace jsonConfigurable
    {
        ConstrainedString::ConstrainedString( std::string &init_str )
        : constraint_param(nullptr)
        {
            *((std::string*)(this)) = init_str;
        }

        ConstrainedString::ConstrainedString( const char* init_str )
        : constraint_param(nullptr)
        {
            *((std::string*)(this)) = std::string( init_str );
        }

        const ConstrainedString&
        ConstrainedString::operator=( const std::string& new_value )
        {
            *((std::string*)(this)) = new_value;
            //release_assert( constraint_param );
            if( constraint_param && (constraint_param->count( new_value ) == 0) && (new_value != JsonConfigurable::default_string)  )
            {
                std::ostringstream msg;
                msg << "Constrained String" ;
                if( !parameter_name.empty() )
                {
                    msg << " (" << parameter_name << ")" ;
                }
                msg << " with specified value "
                    << new_value
                    << " invalid. Possible values are: " << std::endl ;
                for( auto value : (*constraint_param) )
                {
                    msg << value << std::endl;
                }
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
            }
            return *this ;
        }
    }

    const char * JsonConfigurable::default_description = "No Description Yet";
    const char * JsonConfigurable::default_string = "UNINITIALIZED STRING";
    bool JsonConfigurable::_dryrun = false;
    bool JsonConfigurable::_useDefaults = false;
    bool JsonConfigurable::_track_missing = true;
    bool JsonConfigurable::_possibleNonflatConfig = false;
    std::set< std::string > JsonConfigurable::empty_set;

    void
    JsonConfigurable::initConfigTypeMap(
        const char* paramName,
        bool * pVariable,
        const char* description,
        bool defaultvalue
    )
    {
        LOG_DEBUG_F("initConfigTypeMap<bool>: %s\n", paramName);
        boolConfigTypeMap[ paramName ] = pVariable;
        json::Object newIntSchema;
        /* Use this when boolean configuration parameters are actually 'true'/'false'.
        newIntSchema["default"] = json::Boolean(defaultvalue); */
        newIntSchema["default"] = json::Number(defaultvalue ? 1 : 0);
        if ( _dryrun )
        {
            newIntSchema["description"] = json::String(description);
            newIntSchema["type"] = json::String( "bool" );
    }
        jsonSchemaBase[paramName] = newIntSchema;
    }

    void
    JsonConfigurable::initConfigTypeMap(
        const char* paramName,
        int * pVariable,
        const char * description,
        int min, int max, int defaultvalue,
        const char* condition_key, const char* condition_value
    )
    {
        LOG_DEBUG_F( "initConfigTypeMap<int>: %s\n", paramName);
        intConfigTypeMap[ paramName ] = pVariable;
        json::Object newIntSchema;
            newIntSchema["min"] = json::Number(min);
        newIntSchema["max"] = json::Number(max);
        newIntSchema["default"] = json::Number(defaultvalue);
        if ( _dryrun )
        {
            newIntSchema["description"] = json::String(description);
            newIntSchema["type"] = json::String( "integer" );

            if( condition_key && condition_value )
            {
                json::Object condition;
                condition[ condition_key ] = json::String( condition_value );
                newIntSchema["depends-on"] = condition;
            }
        }
        jsonSchemaBase[paramName] = newIntSchema;
    }

    void
    JsonConfigurable::initConfigTypeMap(
        const char* paramName,
        float * pVariable,
        const char * description,
        float min, float max, float defaultvalue,
        const char* condition_key, const char* condition_value
    )
    {
        LOG_DEBUG_F( "initConfigTypeMap<float>: %s\n", paramName);
        floatConfigTypeMap[ paramName ] = pVariable;
        json::Object newFloatSchema;
            newFloatSchema["min"] = json::Number(min);
        newFloatSchema["max"] = json::Number(max);
        newFloatSchema["default"] = json::Number(defaultvalue);
        if ( _dryrun )
        {
            newFloatSchema["description"] = json::String(description);
            newFloatSchema["type"] = json::String( "float" );

            if( condition_key && condition_value )
            {
                json::Object condition;
                condition[ condition_key ] = json::String( condition_value );
                newFloatSchema["depends-on"] = condition;
            }
        }
        jsonSchemaBase[paramName] = newFloatSchema;
    }

    void
    JsonConfigurable::initConfigTypeMap(
        const char* paramName,
        double * pVariable,
        const char * description,
        double min, double max, double defaultvalue
    )
    {
        LOG_DEBUG_F( "initConfigTypeMap<double>: %s\n", paramName);
        doubleConfigTypeMap[ paramName ] = pVariable;
        json::Object newDoubleSchema;
        if ( _dryrun )
        {
            newDoubleSchema["description"] = json::String(description);
            newDoubleSchema["type"] = json::String("double");
        }
        newDoubleSchema["min"] = json::Number(min);
        newDoubleSchema["max"] = json::Number(max);
        newDoubleSchema["default"] = json::Number(defaultvalue);
        jsonSchemaBase[paramName] = newDoubleSchema;
    }

    void
    JsonConfigurable::initConfigTypeMap(
        const char* paramName,
        std::string * pVariable,
        const char * description,
        const std::string& default_str
    )
    {
        LOG_DEBUG_F( "initConfigTypeMap<string>: %s\n", paramName);
        stringConfigTypeMap[ paramName ] = pVariable;
        json::Object newStringSchema;
        newStringSchema["default"] = json::String(default_str);
        if ( _dryrun )
        {
            newStringSchema["description"] = json::String(description);
            newStringSchema["type"] = json::String("string");
        }
        jsonSchemaBase[paramName] = newStringSchema;
    }

    void
    JsonConfigurable::initConfigTypeMap(
        const char* paramName,
        jsonConfigurable::ConstrainedString * pVariable,
        const char * description,
        const std::string& default_str
    )
    {
        LOG_DEBUG_F( "initConfigTypeMap<ConstrainedString>: %s\n", paramName);
        conStringConfigTypeMap[ paramName ] = pVariable;
        json::Object newConStringSchema;
        newConStringSchema["default"] = json::String(default_str); // would be nice if this always in the constraint list!
        if ( _dryrun )
        {
            newConStringSchema["description"] = json::String(description);
            newConStringSchema["type"] = json::String("Constrained String");
            newConStringSchema["value_source"] = json::String( pVariable->constraints );
        }
        jsonSchemaBase[paramName] = newConStringSchema;
    }


    void
    JsonConfigurable::initConfigTypeMap(
        const char* paramName,
        jsonConfigurable::tStringSetBase * pVariable,
        const char* description,
        const char* condition_key,
        const char* condition_value
    )
    {
        LOG_DEBUG_F( "initConfigTypeMap<set<string>>: %s\n", paramName);
        stringSetConfigTypeMap[ paramName ] = pVariable;
        json::Object root;
        json::QuickBuilder newStringSetSchema( root );
        newStringSetSchema["default"] = json::Array();
        if ( _dryrun )
        {
            newStringSetSchema["description"] = json::String(description);
            newStringSetSchema["type"] = json::String( pVariable->getTypeName() );

            if( pVariable->getTypeName() == FIXED_STRING_SET_LABEL )
            {
                unsigned int counter = 0;
                newStringSetSchema["possible_values"] = json::Array();
                for( auto& value : ((jsonConfigurable::tFixedStringSet*)pVariable)->possible_values )
                {
                    newStringSetSchema["possible_values"][counter++] = json::String( value );
                }
            }
            else if( pVariable->getTypeName() == DYNAMIC_STRING_SET_LABEL )
            {
                newStringSetSchema["value_source"] = json::String( ((jsonConfigurable::tDynamicStringSet*)pVariable)->value_source );
            }
            else
            {
                // just a regular old string set, no problem.
            }

            if( condition_key && condition_value )
            {
                json::Object condition;
                condition[ condition_key ] = json::String( condition_value );
                newStringSetSchema["depends-on"] = condition;
            }
        }
        jsonSchemaBase[paramName] = newStringSetSchema.As<json::Object>();
    }

    void
    JsonConfigurable::initConfigTypeMap(
        const char* paramName,
        std::vector< std::string > * pVariable,
        const char* description,
        const char* constraint_schema,
        const std::set< std::string > &constraint_variable
    )
    {
        LOG_DEBUG_F( "initConfigTypeMap<vector<string>>: %s\n", paramName);
        vectorStringConfigTypeMap[ paramName ] = pVariable;
        vectorStringConstraintsTypeMap[ paramName ] = &constraint_variable;
        json::Object newVectorStringSchema;
        if ( _dryrun )
        {
            newVectorStringSchema["description"] = json::String(description);
            newVectorStringSchema["type"] = json::String("Vector String");

            if( constraint_schema )
            {
                newVectorStringSchema["value_source"] = json::String( constraint_schema );
            }
        }
        jsonSchemaBase[paramName] = newVectorStringSchema;
    }

    void
    JsonConfigurable::initConfigTypeMap(
        const char* paramName,
        std::vector< std::vector< std::string > > * pVariable,
        const char* description,
        const char* constraint_schema,
        const std::set< std::string > &constraint_variable
    )
    {
        LOG_DEBUG_F( "initConfigTypeMap<vector<vector<string>>>: %s\n", paramName);
        vector2dStringConfigTypeMap[ paramName ] = pVariable;
        vector2dStringConstraintsTypeMap[ paramName ] = &constraint_variable;
        json::Object newVectorStringSchema;
        newVectorStringSchema["description"] = json::String(description);
        newVectorStringSchema["type"] = json::String("Vector 2d String");
        if( constraint_schema )
        {
            newVectorStringSchema["value_source"] = json::String( constraint_schema );
        }
        jsonSchemaBase[paramName] = newVectorStringSchema;
    }

    void
    JsonConfigurable::initConfigTypeMap(
        const char* paramName,
        std::vector< float > * pVariable,
        const char* description,
        float min, float max, float defaultvalue
    )
    {
        LOG_DEBUG_F( "initConfigTypeMap<vector<float>>: %s\n", paramName);
        vectorFloatConfigTypeMap[ paramName ] = pVariable;
        json::Object newVectorFloatSchema;
        if ( _dryrun )
        {
            newVectorFloatSchema["description"] = json::String(description);
            newVectorFloatSchema["type"] = json::String("Vector Float");
        }
        newVectorFloatSchema["min"] = json::Number(min);
        newVectorFloatSchema["max"] = json::Number(max);
        newVectorFloatSchema["default"] = json::Number(defaultvalue);
        jsonSchemaBase[paramName] = newVectorFloatSchema;
    }

    void
    JsonConfigurable::initConfigTypeMap(
        const char* paramName,
        std::vector< int > * pVariable,
        const char* description,
        int min, int max, int defaultvalue
    )
    {
        LOG_DEBUG_F( "initConfigTypeMap<vector<int>>: %s\n", paramName);
        vectorIntConfigTypeMap[ paramName ] = pVariable;
        json::Object newVectorIntSchema;
        if ( _dryrun )
        {
            newVectorIntSchema["description"] = json::String(description);
            newVectorIntSchema["type"] = json::String("Vector Int");
        }
        newVectorIntSchema["min"] = json::Number(min);
        newVectorIntSchema["max"] = json::Number(max);
        newVectorIntSchema["default"] = json::Number(defaultvalue);
        jsonSchemaBase[paramName] = newVectorIntSchema;
    }

    void
    JsonConfigurable::initConfigTypeMap(
        const char* paramName,
        std::vector< std::vector< float > > * pVariable,
        const char* description,
        float min, float max, float defaultvalue
    )
    {
        LOG_DEBUG_F( "initConfigTypeMap<vector,vector<float>>>: %s\n", paramName);
        vector2dFloatConfigTypeMap[ paramName ] = pVariable;
        json::Object newVector2dFloatSchema;
        if ( _dryrun )
        {
            newVector2dFloatSchema["description"] = json::String(description);
            newVector2dFloatSchema["type"] = json::String("Vector2d Float");
        }
        newVector2dFloatSchema["min"] = json::Number(min);
        newVector2dFloatSchema["max"] = json::Number(max);
        newVector2dFloatSchema["default"] = json::Number(defaultvalue);
        jsonSchemaBase[paramName] = newVector2dFloatSchema;
    }

    void
    JsonConfigurable::initConfigTypeMap(
        const char* paramName,
        std::vector< std::vector< int > > * pVariable,
        const char* description,
        int min, int max, int defaultvalue
    )
    {
        LOG_DEBUG_F( "initConfigTypeMap<vector,vector<int>>>: %s\n", paramName);
        vector2dIntConfigTypeMap[ paramName ] = pVariable;
        json::Object newVector2dIntSchema;
        if ( _dryrun )
        {
            newVector2dIntSchema["description"] = json::String(description);
            newVector2dIntSchema["type"] = json::String("Vector2d Int");
        }
        newVector2dIntSchema["min"] = json::Number(min);
        newVector2dIntSchema["max"] = json::Number(max);
        newVector2dIntSchema["default"] = json::Number(defaultvalue);
        jsonSchemaBase[paramName] = newVector2dIntSchema;
    }

    // We have sets/vectors from json arrays, now add maps from json dictonaries
    // This will be for specific piece-wise constant maps of dates (fractional years)
    // to config values (floats first).
    void
    JsonConfigurable::initConfigTypeMap(
        const char* paramName,
        tFloatFloatMapConfigType * pVariable,
        const char* defaultDesc
    )
    {
        LOG_DEBUG_F( "initConfigTypeMap<pwcMap>: %s\n", paramName);
        ffMapConfigTypeMap[ paramName ] = pVariable;
        json::Object newNestedSchema;
        if ( _dryrun )
        {
            newNestedSchema["description"] = json::String(defaultDesc);
            newNestedSchema["type"] = json::String("nested json object (of key-value pairs)");
        }
        jsonSchemaBase[paramName] = newNestedSchema;
    }

    void
    JsonConfigurable::initConfigTypeMap(
        const char* paramName,
        tFloatFloatMapConfigType * pVariable,
        const char* description,
        const char* condition_key, const char* condition_value
    )
    {
        LOG_DEBUG_F( "initConfigTypeMap<pwcMap>: %s\n", paramName);
        ffMapConfigTypeMap[ paramName ] = pVariable;
        json::Object newNestedSchema;
        if ( _dryrun )
        {
            newNestedSchema["description"] = json::String(description);
            newNestedSchema["type"] = json::String("nested json object (of key-value pairs)");
            if( condition_key && condition_value )
            {
                json::Object condition;
                condition[ condition_key ] = json::String( condition_value );
                newNestedSchema["depends-on"] = condition;
            }
        }
        jsonSchemaBase[paramName] = newNestedSchema;
    }

    void
    JsonConfigurable::initConfigTypeMap(
        const char* paramName,
        tStringFloatMapConfigType * pVariable,
        const char* defaultDesc
    )
    {
        LOG_DEBUG_F( "initConfigTypeMap<pwcMap>: %s\n", paramName);
        sfMapConfigTypeMap[ paramName ] = pVariable;
        json::Object newNestedSchema;
        if ( _dryrun )
        {
            newNestedSchema["description"] = json::String(defaultDesc);
            newNestedSchema["type"] = json::String("nested json object (of key-value pairs)");
        }
        jsonSchemaBase[paramName] = newNestedSchema;
    }

    void
    JsonConfigurable::initConfigTypeMap(
        const char* paramName,
        RangedFloat * pVariable,
        const char * description,
        float defaultvalue,
        const char* condition_key, const char* condition_value
    )
    {
        LOG_DEBUG_F( "initConfigTypeMap<RangedFloat>: %s\n", paramName);
        rangedFloatConfigTypeMap[ paramName ] = pVariable;
        json::Object newFloatSchema;
        newFloatSchema["min"] = json::Number( pVariable->getMin() );
        newFloatSchema["max"] = json::Number( pVariable->getMax() );
        newFloatSchema["default"] = json::Number(defaultvalue);
        if ( _dryrun )
        {
            newFloatSchema["description"] = json::String(description);
            newFloatSchema["type"] = json::String( "float" );
            if( condition_key && condition_value )
            {
                json::Object condition;
                condition[ condition_key ] = json::String( condition_value );
                newFloatSchema["depends-on"] = condition;
            }
        }
        jsonSchemaBase[paramName] = newFloatSchema;
    }

    void
    JsonConfigurable::initConfigTypeMap(
        const char* paramName,
        NonNegativeFloat * pVariable,
        const char * description,
        float max,
        float defaultvalue,
        const char* condition_key, const char* condition_value
    )
    {
        RangedFloat * pTmp = (RangedFloat*) pVariable;
        return initConfigTypeMap( paramName, pTmp, description, defaultvalue, condition_key, condition_value );
    }


    void
    JsonConfigurable::initConfigTypeMap(
        const char* paramName,
        NaturalNumber * pVariable,
        const char * description,
        unsigned int max,
        NaturalNumber defaultvalue,
        const char* condition_key, const char* condition_value
    )
    {
        LOG_DEBUG_F( "initConfigTypeMap<NaturalNumber>: %s\n", paramName);
        naturalNumberConfigTypeMap[ paramName ] = pVariable;
        json::Object newNNSchema;
        newNNSchema["min"] = json::Number( 0 );
        newNNSchema["max"] = json::Number( max );
        newNNSchema["default"] = json::Number(defaultvalue);
        if ( _dryrun )
        {
            newNNSchema["description"] = json::String(description);
            newNNSchema["type"] = json::String( "NaturalNumber" );
            if( condition_key && condition_value )
            {
                json::Object condition;
                condition[ condition_key ] = json::String( condition_value );
                newNNSchema["depends-on"] = condition;
            }
        }
        jsonSchemaBase[paramName] = newNNSchema;
    }

    void
    JsonConfigurable::initConfigTypeMap(
        const char* paramName,
        JsonConfigurable * pVariable,
        const char* defaultDesc,
        const char* condition_key, const char* condition_value
    )
    {
        LOG_DEBUG_F( "initConfigTypeMap<JsonConfigurable>: %s\n", paramName);

        // --------------------------------------------------------
        // --- Get the type of variable and declare it an "idmType"
        // --------------------------------------------------------
        std::string variable_type = typeid(*pVariable).name() ;
        if( variable_type.find( "class Kernel::" ) == 0 )
        {
            variable_type = variable_type.substr( 14 );
        }
        else if( variable_type.find( "struct Kernel::" ) == 0 )
        {
            variable_type = variable_type.substr( 15 );
        }
        variable_type = std::string("idmType:") + variable_type ;

        // ------------------------------------------------------
        // --- Put the schema for the special type into map first
        // --- so that its definition appears before it is used.
        // ------------------------------------------------------
        jcTypeMap[ paramName ] = pVariable;

        if ( _dryrun )
        {
            bool tmp = _dryrun ;
            _dryrun = true ;
            pVariable->Configure( nullptr );
            _dryrun = tmp ;
            jsonSchemaBase[ variable_type ] = pVariable->GetSchema();

            json::Object newNestedSchema;
            newNestedSchema["description"] = json::String(defaultDesc);
            newNestedSchema["type"] = json::String(variable_type);
            if( condition_key && condition_value )
            {
                json::Object condition;
                condition[ condition_key ] = json::String( condition_value );
                newNestedSchema["depends-on"] = condition;
            }
            jsonSchemaBase[paramName] = newNestedSchema;
        }
    }

    void
    JsonConfigurable::handleMissingParam( const std::string& key )
    {
        if( _track_missing )
        {
            missing_parameters_set.insert(key);
        }
        else
        {
            throw MissingParameterFromConfigurationException( __FILE__, __LINE__, __FUNCTION__, "N/A", key.c_str() );
        }
    }

    bool JsonConfigurable::Configure( const Configuration* inputJson )
    {
        if( _dryrun )
        {
            LOG_DEBUG("Returning from Configure because doing dryrun\n");
            return true;
        }

        // Desired logic
        //
        //  | SPECIFIED | USE_DEFAULTS | BEHAVIOUR |
        //  |--------------------------------------|
        //  |   TRUE    |     TRUE     | USE_JSON  |
        //  |   TRUE    |     FALSE    | USE_JSON  |
        //  |   FALSE   |     TRUE     | USE_DEF   |
        //  |   FALSE   |     FALSE    |   ERROR   |
        //
        //  This reduces to:
        //
        //  |   TRUE    |       X      | USE_JSON  |
        //  |   FALSE   |     TRUE     | USE_DEF   |
        //  |   FALSE   |     FALSE    |   ERROR   |
        //
        // INIT STAGE
        // initVarFromConfig: iterate over all config keys...
        // until we figure that out, go the other way

        // ---------------------------------- BOOL ------------------------------------
        for (auto& entry : boolConfigTypeMap)
        {
            const std::string& key = entry.first;
            json::QuickInterpreter schema = jsonSchemaBase[key];

            // check if parameter was specified in input json (TODO: improve performance by getting the iterator here with Find() and reusing instead of GET_CONFIG_BOOLEAN below)
            if( inputJson->Exist(key) )
            {
                *(entry.second) = (bool)GET_CONFIG_BOOLEAN(inputJson,key.c_str());
            }
            else
            {
                if( _useDefaults )
                {
                    // using the default value
                    bool defaultValue = ((int)schema["default"].As<json::Number>() == 1);
                    *(entry.second) = defaultValue;
                    LOG_DEBUG_F( "Using the default value ( \"%s\" : %d ) for unspecified parameter.\n", key.c_str(), defaultValue );
                }
                else
                {
                    handleMissingParam( key );
                }
            }

            LOG_DEBUG_F("the key %s = bool %d\n", key.c_str(), *(entry.second));
        }

        // ---------------------------------- INT -------------------------------------
        for (auto& entry : intConfigTypeMap)
        {
            const std::string& key = entry.first;
            json::QuickInterpreter schema = jsonSchemaBase[key];
            int val = -1;

            // check if parameter was specified in input json (TODO: improve performance by getting the iterator here with Find() and reusing instead of GET_CONFIG_INTEGER below)
            if( inputJson->Exist(key) )
            {
                double jsonValueAsDouble = GET_CONFIG_DOUBLE( inputJson, key.c_str() );
                if( jsonValueAsDouble != (int) jsonValueAsDouble )
                {
                    std::ostringstream errMsg; // using a non-parameterized exception.
                    errMsg << "Value from json appears to be decimal ("
                           << jsonValueAsDouble
                           << ") but needs to be integer." << std::endl;
                    throw Kernel::GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, errMsg.str().c_str() );
                }
                // get specified configuration parameter
                val = (int)GET_CONFIG_INTEGER(inputJson,key.c_str());
                // throw exception if value is outside of range
                EnforceParameterRange<int>( key, val, schema );
                *(entry.second) = val;
            }
            else
            {
                if( _useDefaults )
                {
                    // using the default value
                    val = (int)schema["default"].As<json::Number>();
                    LOG_INFO_F( "Using the default value ( \"%s\" : %d ) for unspecified parameter.\n", key.c_str(), val );
                    *(entry.second) = val;
                }
                else 
                {
                    handleMissingParam( key );
                }
            }


            LOG_DEBUG_F("the key %s = int %d\n", key.c_str(), *(entry.second));
        }

        // ---------------------------------- FLOAT ------------------------------------
        for (auto& entry : floatConfigTypeMap)
        {
            const std::string& key = entry.first;
            json::QuickInterpreter schema = jsonSchemaBase[key];
            float val = -1.0f;

            // Check if parameter was specified in input json (TODO: improve performance by getting the iterator here with Find() and reusing instead of GET_CONFIG_DOUBLE below)
            if( inputJson->Exist(key) )
            {
                val = (float) GET_CONFIG_DOUBLE( inputJson, key.c_str() );
                // throw exception if specified value is outside of range
                EnforceParameterRange<float>( key, val, schema);
                *(entry.second) = val;
            }
            else
            {
                if ( _useDefaults )
                {
                    // using the default value
                    val = (float)schema["default"].As<json::Number>();
                    LOG_INFO_F( "Using the default value ( \"%s\" : %f ) for unspecified parameter.\n", key.c_str(), val );
                    *(entry.second) = val;
                }
                else 
                {
                    handleMissingParam( key );
                }
            }

            LOG_DEBUG_F("the key %s = float %f\n", key.c_str(), *(entry.second));
        }

        // ---------------------------------- DOUBLE ------------------------------------
        for (auto& entry : doubleConfigTypeMap)
        {
            const std::string& key = entry.first;
            json::QuickInterpreter schema = jsonSchemaBase[key];
            double val = -1.0;

            // Check if parameter was specified in input json (TODO: improve performance by getting the iterator here with Find() and reusing instead of GET_CONFIG_DOUBLE below)
            if( inputJson->Exist(key) )
            {
                val = GET_CONFIG_DOUBLE( inputJson, key.c_str() );
                *(entry.second) = val;
                // throw exception if specified value is outside of range
                EnforceParameterRange<double>( key, val, schema);
            }
            else
            {
                if ( _useDefaults )
                {
                    // using the default value
                    val = schema["default"].As<json::Number>();
                    LOG_INFO_F( "Using the default value ( \"%s\" : %f ) for unspecified parameter.\n", key.c_str(), val );
                    *(entry.second) = val;
                }
                else 
                {
                    handleMissingParam( key );
                }
            }
            LOG_DEBUG_F("the key %s = double %f\n", key.c_str(), *(entry.second));
        }

        // ---------------------------------- RANGEDFLOAT ------------------------------------
        for (auto& entry : rangedFloatConfigTypeMap)
        {
            const std::string& key = entry.first;
            json::QuickInterpreter schema = jsonSchemaBase[key];
            float val = -1.0f;

            // Check if parameter was specified in input json
            LOG_DEBUG_F( "useDefaults = %d\n", _useDefaults );
            if( inputJson->Exist(key) )
            {
                // get specified configuration parameter
                val = GET_CONFIG_DOUBLE( inputJson, key.c_str() );
                *(entry.second) = val;
                // throw exception if specified value is outside of range even though this datatype enforces ranges inherently.
                EnforceParameterRange<float>( key, val, schema);
            }
            else
            {
                if( _useDefaults )
                {
                    // using the default value
                    val = (float)schema["default"].As<json::Number>();
                    LOG_INFO_F( "Using the default value ( \"%s\" : %f ) for unspecified parameter.\n", key.c_str(), val );
                    *(entry.second) = val;
                }
                else 
                {
                    handleMissingParam( key );
                }
            }

            LOG_DEBUG_F("the key %s = float %f\n", key.c_str(), (float) *(entry.second));
        }

        // ---------------------------------- NATURALNUMBER ------------------------------------
        for (auto& entry : naturalNumberConfigTypeMap)
        {
            const std::string& key = entry.first;
            json::QuickInterpreter schema = jsonSchemaBase[key];
            int val = 0;

            // Check if parameter was specified in input json
            LOG_DEBUG_F( "useDefaults = %d\n", _useDefaults );
            if( inputJson->Exist(key) )
            {
                // get specified configuration parameter
                val = (int) GET_CONFIG_INTEGER( inputJson, key.c_str() );
                *(entry.second) = val;
                EnforceParameterRange<int>( key, val, schema);
            }
            else
            {
                if( _useDefaults )
                {
                    // using the default value
                    val = (int)schema["default"].As<json::Number>();
                    LOG_INFO_F( "Using the default value ( \"%s\" : %f ) for unspecified parameter.\n", key.c_str(), val );
                    *(entry.second) = val;
                }
                else 
                {
                    handleMissingParam( key );
                }
            }

            LOG_DEBUG_F("the key %s = int %f\n", key.c_str(), (int) *(entry.second));
        }

        // ---------------------------------- STRING ------------------------------------
        for (auto& entry : stringConfigTypeMap)
        {
            const std::string& key = entry.first;
            json::QuickInterpreter schema = jsonSchemaBase[key];
            std::string val = schema["default"].As<json::String>();
            if( inputJson->Exist(key) )
            {
                *(entry.second) = (std::string) GET_CONFIG_STRING( inputJson, (entry.first).c_str() );
            }
            else
            {
                if( _useDefaults )
                {
                    val = (std::string)schema["default"].As<json::String>();
                    LOG_INFO_F( "Using the default value ( \"%s\" : \"%s\" ) for unspecified parameter.\n", key.c_str(), val.c_str() );
                    *(entry.second) = val;
                }
                else
                {
                    handleMissingParam( key );
                }
            }
        }

        for (auto& entry : conStringConfigTypeMap)
        {
            const std::string& key = entry.first;
            entry.second->parameter_name = key ;
            json::QuickInterpreter schema = jsonSchemaBase[key];
            std::string val = schema["default"].As<json::String>();
            if( inputJson->Exist(key) )
            {
                *(entry.second) = (std::string) GET_CONFIG_STRING( inputJson, (entry.first).c_str() );
            }
            else
            {
                if( _useDefaults )
                {
                    val = (std::string)schema["default"].As<json::String>();
                    LOG_INFO_F( "Using the default value ( \"%s\" : \"%s\" ) for unspecified parameter.\n", key.c_str(), val.c_str() );
                    *(entry.second) = val;
                }
                else
                {
                    handleMissingParam( key );
                }
            }
        }

        // ---------------------------------- SET of STRINGs ------------------------------------
        for (auto& entry : stringSetConfigTypeMap)
        {
            const std::string& key = entry.first;
            json::QuickInterpreter schema = jsonSchemaBase[key];
            if ( inputJson->Exist(key) )
            {
                *(entry.second) = GET_CONFIG_STRING_SET( inputJson, (entry.first).c_str() );
            }
            else
            { 
                if( _useDefaults )
                {
                    //auto val = schema["default"].As<json::Array>();
                    //LOG_INFO_F( "Using the default value ( \"%s\" : <empty string set> ) for unspecified string set parameter.\n", key.c_str() );
                    //*(entry.second) = val;
                }
                else
                {
                    handleMissingParam( key );
                }
            }
        }

        // ---------------------------------- VECTOR of STRINGs ------------------------------------
        for (auto& entry : vectorStringConfigTypeMap)
        {
            const std::string& key = entry.first;
            json::QuickInterpreter schema = jsonSchemaBase[key];
            if ( inputJson->Exist(key) )
            {
                *(entry.second) = GET_CONFIG_VECTOR_STRING( inputJson, (entry.first).c_str() );
            }
            else
            {
                if( _useDefaults )
                {
                    // using the default value
                    LOG_INFO_F( "Using the default value ( \"%s\" : <empty string vector> ) for unspecified string vector parameter.\n", key.c_str() );
                }

                if( _track_missing )
                {
                    missing_parameters_set.insert(key);
                }
            }

            auto allowed_values = vectorStringConstraintsTypeMap[ key ];
            for( auto &candidate : *(entry.second) )
            {
                if( allowed_values->size() > 0 && std::find( allowed_values->begin(), allowed_values->end(), candidate ) == allowed_values->end() )
                {
                    std::ostringstream msg;
                    msg << "Constrained strings (dynamic enum) with specified value "
                        << candidate
                        << " invalid. Possible values are: ";
                    for( auto value: *allowed_values )
                    {
                        msg << value << "...";
                    }
                    throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
                }
            }
        }

        // ---------------------------------- VECTOR VECTOR of STRINGs ------------------------------------
        for (auto& entry : vector2dStringConfigTypeMap)
        {
            const std::string& key = entry.first;
            json::QuickInterpreter schema = jsonSchemaBase[key];
            if ( inputJson->Exist(key) )
            {
                *(entry.second) = GET_CONFIG_VECTOR2D_STRING( inputJson, (entry.first).c_str() );
            }
            else
            {
                if( _useDefaults )
                {
                    // using the default value
                    LOG_INFO_F( "Using the default value ( \"%s\" : <empty string Vector2D> ) for unspecified string Vector2D parameter.\n", key.c_str() );
                }

                if( _track_missing )
                {
                    missing_parameters_set.insert(key);
                }
            }
            auto allowed_values = vector2dStringConstraintsTypeMap[ key ];
            for( auto &candidate_vector : *(entry.second) )
            {
                for( auto &candidate : candidate_vector )
                {
                    if( allowed_values->size() > 0 && std::find( allowed_values->begin(), allowed_values->end(), candidate ) == allowed_values->end() )
                    {
                        std::ostringstream msg;
                        msg << "Constrained strings (dynamic enum) with specified value " 
                            << candidate 
                            << " invalid. Possible values are: ";
                        for( auto value: *allowed_values )
                        {
                            msg << value << "...";
                        }
                        throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
                    }
                }
            }
        }

        //----------------------------------- VECTOR of FLOATs ------------------------------
        for (auto& entry : vectorFloatConfigTypeMap)
        {
            const std::string& key = entry.first;
            json::QuickInterpreter schema = jsonSchemaBase[key];
            if( inputJson->Exist(key) )
            {
                std::vector<float> configValues = GET_CONFIG_VECTOR_FLOAT( inputJson, (entry.first).c_str() );
                *(entry.second) = configValues;

                EnforceVectorParameterRanges<float>(key, configValues, schema);
            }
            else if( !_useDefaults )
            {
                handleMissingParam( key );
            }
        }

        //----------------------------------- VECTOR of INTs ------------------------------
        for (auto& entry : vectorIntConfigTypeMap)
        {
            const std::string& key = entry.first;
            json::QuickInterpreter schema = jsonSchemaBase[key];
            if( inputJson->Exist(key) )
            {
                std::vector<int> configValues = GET_CONFIG_VECTOR_INT( inputJson, (entry.first).c_str() );
                *(entry.second) = configValues;

                EnforceVectorParameterRanges<int>(key, configValues, schema);
            }
            else if( !_useDefaults )
            {
                handleMissingParam( key );
            }
        }

        //----------------------------------- VECTOR VECTOR of FLOATs ------------------------------
        for (auto& entry : vector2dFloatConfigTypeMap)
        {
            const std::string& key = entry.first;
            json::QuickInterpreter schema = jsonSchemaBase[key];
            if( inputJson->Exist(key) )
            {
                std::vector<std::vector<float>> configValues = GET_CONFIG_VECTOR2D_FLOAT( inputJson, (entry.first).c_str() );
                *(entry.second) = configValues;

                for( auto values : configValues )
                {
                    EnforceVectorParameterRanges<float>(key, values, schema);
                }
            }
            else if( !_useDefaults )
            {
                handleMissingParam( key );
            }
        }

        //----------------------------------- VECTOR VECTOR of INTs ------------------------------
        for (auto& entry : vector2dIntConfigTypeMap)
        {
            const std::string& key = entry.first;
            json::QuickInterpreter schema = jsonSchemaBase[key];
            if( inputJson->Exist(key) )
            {
                std::vector<std::vector<int>> configValues = GET_CONFIG_VECTOR2D_INT( inputJson, (entry.first).c_str() );
                *(entry.second) = configValues;

                for( auto values : configValues )
                {
                    EnforceVectorParameterRanges<int>(key, values, schema);
                }
            }
            else if( !_useDefaults )
            {
                handleMissingParam( key );
            }
        }
/////////////////// END FIX BOUNDARY

        // Let's see if we can iterate over our template base class generic container!
        //std::cout << "IJsonConfigurable::generic_container.size() = " << IJsonConfigurable::generic_container.size() << std::endl;
        for( auto iter = IJsonConfigurable::generic_container.begin();
             IJsonConfigurable::generic_container.size() > 0;
             )
        {
            auto temp_cont = *iter;
            IJsonConfigurable::generic_container.erase( iter++ );
            temp_cont.second->Configure( inputJson );
        }

        // ---------------------------------- FLOAT-FLOAT MAP ------------------------------------
        for (auto& entry : ffMapConfigTypeMap)
        {
            // NOTE that this could be used for general float to float, but right now hard-coding year-as-int to float
            const auto & key = entry.first;
            tFloatFloatMapConfigType * pFFMap = entry.second;
            const auto& tvcs_jo = json_cast<const json::Object&>( (*inputJson)[key] );
            for( auto data = tvcs_jo.Begin();
                      data != tvcs_jo.End();
                      ++data )
            {
                float year = atof( data->name.c_str() );
                auto tvcs = inputJson->As< json::Object >()[ key ];
                float constant = (float) ((json::QuickInterpreter( tvcs ))[ data->name ].As<json::Number>());
                LOG_DEBUG_F( "Inserting year %f and delay %f into map.\n", year, constant );
                pFFMap->insert( std::make_pair( year, constant ) );
            }
        }

        // ---------------------------------- STRING-FLOAT MAP ------------------------------------
        for (auto& entry : sfMapConfigTypeMap)
        {
            // NOTE that this could be used for general float to float, but right now hard-coding year-as-int to float
            const auto & key = entry.first;
            tStringFloatMapConfigType * pSFMap = entry.second;
            const auto& tvcs_jo = json_cast<const json::Object&>( (*inputJson)[key] );
            for( auto data = tvcs_jo.Begin();
                      data != tvcs_jo.End();
                      ++data )
            {
                auto tvcs = inputJson->As< json::Object >()[ key ];
                float value = (float) ((json::QuickInterpreter( tvcs ))[ data->name ].As<json::Number>());
                LOG_DEBUG_F( "Inserting string %s and value %f into map.\n", data->name.c_str(), value );
                pSFMap->insert( std::make_pair( data->name, value ) );
            }
        }

        // ---------------------------------- JsonConfigurable MAP ------------------------------------
        for (auto& entry : jcTypeMap)
        {
            const auto & key = entry.first;
            JsonConfigurable * pJc = entry.second;

            if( inputJson->Exist( key ) )
            {
                Configuration * p_config = Configuration::CopyFromElement( (*inputJson)[key] );

                pJc->Configure( p_config );

                delete p_config ;
                p_config = nullptr;
            }
            else if( !_useDefaults )
            {
                handleMissingParam( key );
            }
        }

        return true;
    }

    /*QuickBuilder JsonConfigurable::GetSchema()
    {
        return QuickBuilder( jsonSchemaBase );
    }*/

    std::set< std::string > JsonConfigurable::missing_parameters_set;

    JsonConfigurable::name2CreatorMapType&
    JsonConfigurable::get_registration_map()
    {
        static JsonConfigurable::name2CreatorMapType name2CreatorMap;
        return name2CreatorMap;
    }

    JsonConfigurable::Registrator::Registrator( const char* classname, get_schema_funcptr_t gs_callback )
    {
        const std::string stored_class_name = std::string( classname );
        get_registration_map()[ stored_class_name ] = gs_callback;
    }
}

#if 0
namespace Kernel
{
    namespace jsonConfigurable
    {
        template <class Archive>
        void serialize( Archive &ar, Kernel::jsonConfigurable::ConstrainedString& cs, const unsigned int file_version )
        {
          ar & cs.constraints;
          ar & cs.constraint_param;
        }
    }
}
#endif
