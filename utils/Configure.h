/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "IdmApi.h"
#include "CajunIncludes.h"
#include "Configuration.h"      // for MetadataDescriptor namespace
#include "ConfigurationImpl.h"  // for MetadataDescriptor namespace
#include "Exceptions.h"
#include "Debug.h"
#include "Types.h"

// 'resource' files with description texts for schema
#include "config_params.rc"
#ifdef ENABLE_POLIO
#include "polio_params.rc"
#endif
#include "iv_params.rc"

#include <stdarg.h>
#include <stdexcept>
#include <sstream>
#include <fstream>
#include <map>
#include <set>
#include <vector>

#ifndef WIN32
#include <limits>
#include <values.h>
#endif

#include "Environment.h"
#include "Log.h"

namespace Kernel
{
    // TBD: Don't inherint from IConfigurable if we don't want to have to implement AddRef/Release/QI and Serialiation
    class IDMAPI IJsonConfigurable //: public IConfigurable
    {
        public:
        virtual bool Configure( const Configuration* ) = 0;
#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        static std::map<std::string, IJsonConfigurable*> generic_container;
#pragma warning( pop )
    };

    template <typename T>
    class Wrapper : public IJsonConfigurable
    {
        public:
        Wrapper()
        {
            //std::cout << "ctor for Wrapper<" << typeid( *this ).name() << std::endl;
        };
        //public methods
        virtual bool Configure( const Configuration* inputJson )
        {
            while( _labelToVariableMap.size() )
            {
                auto entry = _labelToVariableMap.begin();
                auto key = entry->first;
                auto value = entry->second;
                _labelToVariableMap.erase( _labelToVariableMap.begin() );
                value->ConfigureFromJsonAndKey( inputJson, key );
            }
            _labelToVariableMap.clear();
            return true;
        };
    //private:
        // First solution was static label-to-variable map (per templated subclass) but not sure this is a good idea. Persists,
        // and needs to be cleared. Why not make just a member of the object, and thus discarded when object goes out of scope?
        std::map< std::string, T * > _labelToVariableMap;
    };

#define FIXED_STRING_SET_LABEL "Fixed String Set"
#define DYNAMIC_STRING_SET_LABEL "Dynamic String Set"

    namespace jsonConfigurable
    {
        typedef std::set< std::string > tStringSet;
        class tStringSetBase : public tStringSet
        {
            public:
                virtual std::string getTypeName() const = 0;
        };

        class tFixedStringSet : public tStringSetBase
        {
            public:
                tFixedStringSet() {}
                virtual std::string getTypeName() const
                {
                    return std::string( FIXED_STRING_SET_LABEL );
                }
                tStringSet possible_values;
        };

        class tDynamicStringSet : public tStringSetBase
        {
            public:
                tDynamicStringSet() {}
                virtual std::string getTypeName() const
                {
                    return std::string( DYNAMIC_STRING_SET_LABEL );
                }
                std::string value_source;

                static void serialize( IArchive& ar, tDynamicStringSet& dyn )
                {
                    ar.startObject();
                    ar.labelElement("values");
                    size_t count = ar.IsWriter() ? dyn.size() : -1;

                    ar.startArray(count);
                    if( ar.IsWriter() )
                    {
                        for (auto& entry : dyn)
                        {
                            std::string str = entry;
                            ar & str;
                        }
                    }
                    else
                    {
                        for (size_t i = 0; i < count; ++i)
                        {
                            std::string entry;
                            ar & entry;
                            dyn.insert( entry );
                        }
                    }
                    ar.endArray();
                    ar.labelElement("value_source") & dyn.value_source;
                    ar.endObject();
                }
        };

        class ConstrainedString : public std::string
        {
            public:
                ConstrainedString()
                :constraint_param(nullptr)
                {};

                ConstrainedString( std::string &init_str );
                ConstrainedString( const char *init_str );

                virtual const ConstrainedString& operator=( const std::string& new_value );

                std::string constraints;
                tStringSet * constraint_param;
                std::string parameter_name;
        };

        typedef std::map< std::string, ConstrainedString * > tConStringConfigTypeMapType;
    }

    bool check_condition( const json::QuickInterpreter * pJson, const char * condition_key, const char * condition_value = nullptr );

    class IDMAPI JsonConfigurable : public IConfigurable
    {
        friend class InterventionFactory;
        friend class DemographicRestrictions;
        friend class DurationDistribution;
    public:
        typedef std::map< float, float > tFloatFloatMapConfigType;
        typedef std::map< std::string, float > tStringFloatMapConfigType;
        static const char * default_string;

    private:
        typedef std::map< std::string, bool * > tBoolConfigTypeMapType;
        typedef std::map< std::string, int * > tIntConfigTypeMapType;
        typedef std::map< std::string, float * > tFloatConfigTypeMapType;
        typedef std::map< std::string, double * > tDoubleConfigTypeMapType;
        typedef std::map< std::string, std::string * > tStringConfigTypeMapType;
        typedef std::map< std::string, void * > tEnumConfigTypeMapType;
        typedef std::map< std::string, std::set< std::string > * > tStringSetConfigTypeMapType;
        typedef std::map< std::string, std::vector< std::string > * > tVectorStringConfigTypeMapType;
        typedef std::map< std::string, std::vector< std::vector< std::string > > * > tVector2dStringConfigTypeMapType;
        typedef std::map< std::string, const std::set< std::string > * > tVectorStringConstraintsTypeMapType;
        typedef std::map< std::string, std::vector< float > * > tVectorFloatConfigTypeMapType;
        typedef std::map< std::string, std::vector< int > * > tVectorIntConfigTypeMapType;
        typedef std::map< std::string, std::vector< std::vector< float > > * > tVector2dFloatConfigTypeMapType;
        typedef std::map< std::string, std::vector< std::vector< int > > * > tVector2dIntConfigTypeMapType;
        typedef std::map< std::string, tFloatFloatMapConfigType * > tFloatFloatMapConfigTypeMapType;
        typedef std::map< std::string, tStringFloatMapConfigType * > tStringFloatMapConfigTypeMapType;
        typedef std::map< std::string, RangedFloat * > tRangedFloatConfigTypeMapType;
        typedef std::map< std::string, NaturalNumber * > tNNConfigTypeMapType;
        typedef std::map< std::string, JsonConfigurable * > tJsonConfigurableMapType;

    public:

        json::QuickBuilder GetSchema() { return json::QuickBuilder( jsonSchemaBase ); }

        static bool _dryrun;
        static bool _useDefaults;
        static bool _track_missing;
        static bool _possibleNonflatConfig;
        typedef json::QuickBuilder (*get_schema_funcptr_t)(void);
        typedef std::map< std::string, get_schema_funcptr_t > name2CreatorMapType;
        static name2CreatorMapType &get_registration_map();

        struct IDMAPI Registrator
        {
            Registrator( const char* class_name, get_schema_funcptr_t gs_callback );
        };

#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        static jsonConfigurable::tStringSet missing_parameters_set;

        // TEST ONLY - componentTests needs to clear this so that other tests don't fail
        static void ClearMissingParameters() { missing_parameters_set.clear() ; IJsonConfigurable::generic_container.clear(); }

    protected:
        tBoolConfigTypeMapType boolConfigTypeMap;
        tIntConfigTypeMapType intConfigTypeMap;
        tFloatConfigTypeMapType floatConfigTypeMap;
        tDoubleConfigTypeMapType doubleConfigTypeMap;
        tEnumConfigTypeMapType enumConfigTypeMap;
        tStringConfigTypeMapType stringConfigTypeMap;
        tStringSetConfigTypeMapType stringSetConfigTypeMap;
        jsonConfigurable::tConStringConfigTypeMapType conStringConfigTypeMap;
        tVectorStringConfigTypeMapType vectorStringConfigTypeMap;
        tVector2dStringConfigTypeMapType vector2dStringConfigTypeMap;
        tVectorStringConstraintsTypeMapType vectorStringConstraintsTypeMap;
        tVectorStringConstraintsTypeMapType vector2dStringConstraintsTypeMap;
        tVectorFloatConfigTypeMapType vectorFloatConfigTypeMap;
        tVectorIntConfigTypeMapType vectorIntConfigTypeMap;
        tVector2dFloatConfigTypeMapType vector2dFloatConfigTypeMap;
        tVector2dIntConfigTypeMapType vector2dIntConfigTypeMap;
        tFloatFloatMapConfigTypeMapType ffMapConfigTypeMap;
        tStringFloatMapConfigTypeMapType sfMapConfigTypeMap;
        tRangedFloatConfigTypeMapType rangedFloatConfigTypeMap;
        tNNConfigTypeMapType naturalNumberConfigTypeMap;
        tJsonConfigurableMapType jcTypeMap;

        json::Object jsonSchemaBase;
        static std::set< std::string > empty_set;
#pragma warning( pop )

        JsonConfigurable() {}

        static const char * default_description;

        void initSimTypes( unsigned int count, ... )
        {
            json::Array arraySimTypes;
            va_list ap;
            va_start( ap, count );
            for( unsigned int idx = 0; idx<count; idx++ )
            {
                arraySimTypes.Insert( json::String( va_arg( ap, const char* ) ) );
            }
            va_end( ap );
            jsonSchemaBase[ "Sim_Types" ] = arraySimTypes;
        }

        void initConfigTypeMap(
            const char* paramName,
            bool * pVariable,
            const char* description = default_description,
            bool defaultvalue = false,
            const char* condition_key = nullptr, const char* condition_value = nullptr
        );

        void initConfigTypeMap(
            const char* paramName,
            int * pVariable,
            const char* description = default_description,
            int min = INT_MIN, int max = INT_MAX, int defaultvalue = 0,
            const char* condition_key = nullptr, const char* condition_value = nullptr
        );

        void initConfigTypeMap(
            const char* paramName,
            float * pVariable,
            const char* description = default_description,
            float min = -FLT_MAX, float max = FLT_MAX, float defaultvalue = 1.0,
            const char* condition_key = nullptr, const char* condition_value = nullptr
        );

        void initConfigTypeMap(
            const char* paramName,
            double * pVariable,
            const char* description = default_description,
            double min = -DBL_MAX, double max = DBL_MAX, double defaultvalue = 1.0,
            const char* condition_key = nullptr, const char* condition_value = nullptr
        );

        void initConfigTypeMap(
            const char* paramName,
            std::string * pVariable,
            const char* description = default_description,
            const std::string& default_str = default_string,
            const char* condition_key = nullptr, const char* condition_value = nullptr
        );

        void initConfigTypeMap(
            const char* paramName,
            jsonConfigurable::ConstrainedString * pVariable,
            const char* description = default_description,
            const std::string& default_str = default_string,
            const char* condition_key = nullptr, const char* condition_value = nullptr
        );

        void initConfigTypeMap(
            const char* paramName,
            jsonConfigurable::tStringSetBase * pVariable,
            const char* description = default_description,
            const char* condition_key = nullptr, const char* condition_value = nullptr
        );

        void initConfigTypeMap(
            const char* paramName,
            std::vector< std::string > * pVariable,
            const char* description = default_description,
            const char* constraint_schema = nullptr,
            const std::set< std::string > &constraint_variable = empty_set,
            const char* condition_key = nullptr, const char* condition_value = nullptr
        );

        void initConfigTypeMap(
            const char* paramName,
            std::vector< std::vector< std::string > > * pVariable,
            const char* description = default_description,
            const char* constraint_schema = nullptr,
            const std::set< std::string > &constraint_variable = empty_set,
            const char* condition_key = nullptr, const char* condition_value = nullptr
        );

        void initConfigTypeMap(
            const char* paramName,
            std::vector< float > * pVariable,
            const char* description = default_description,
            float min = -FLT_MAX, float max = FLT_MAX, float defaultvalue = 1.0,
            const char* condition_key = nullptr, const char* condition_value = nullptr
        );

        void initConfigTypeMap(
            const char* paramName,
            std::vector< int > * pVariable,
            const char* description = default_description,
            int min = -INT_MAX, int max = INT_MAX, int defaultvalue = 1.0,
            const char* condition_key = nullptr, const char* condition_value = nullptr
        );

        void initConfigTypeMap(
            const char* paramName,
            std::vector< std::vector< float > > * pVariable,
            const char* description = default_description,
            float min = -FLT_MAX, float max = FLT_MAX, float defaultvalue = 1.0,
            const char* condition_key = nullptr, const char* condition_value = nullptr
        );

        void initConfigTypeMap(
            const char* paramName,
            std::vector< std::vector< int > > * pVariable,
            const char* description = default_description,
            int min = -INT_MAX, int max = INT_MAX, int defaultvalue = 1,
            const char* condition_key = nullptr, const char* condition_value = nullptr
        );

        void
        initConfigTypeMap(
            const char* paramName,
            tFloatFloatMapConfigType * pVariable,
            const char* defaultDesc
        );

       void
       initConfigTypeMap(
            const char* paramName,
            tFloatFloatMapConfigType * pVariable,
            const char* description,
            const char* condition_key, const char* condition_value
       );

       void
       initConfigTypeMap(
            const char* paramName,
            tStringFloatMapConfigType * pVariable,
            const char* defaultDesc
       );

       void initConfigTypeMap(
           const char* paramName,
           RangedFloat * pVariable,
           const char* description = default_description,
           float defaultvalue = 1.0,
           const char* condition_key = nullptr, const char* condition_value = nullptr
       );

       void initConfigTypeMap(
           const char* paramName,
           NonNegativeFloat * pVariable,
           const char* description = default_description,
           float max = 1.0,
           float defaultvalue = 1.0,
           const char* condition_key = nullptr, const char* condition_value = nullptr
       );

       void initConfigTypeMap(
           const char* paramName,
           NaturalNumber * pVariable,
           const char* description = default_description,
           unsigned int max = INT_MAX,
           NaturalNumber defaultvalue = 1,
           const char* condition_key = nullptr, const char* condition_value = nullptr
       );

       void
       initConfigTypeMap(
            const char* paramName,
            JsonConfigurable * pVariable,
            const char* defaultDesc,
            const char* condition_key=nullptr, const char* condition_value=nullptr
       );

        template< typename T >
        void EnforceParameterRange( const std::string& key, T value, json::QuickInterpreter& jsonObj )
        {
            T min = (T)jsonObj["min"].As<json::Number>();
            T max = (T)jsonObj["max"].As<json::Number>();

            if ( value > max )
            {
                throw Kernel::ConfigurationRangeException( __FILE__, __LINE__, __FUNCTION__, key.c_str(), value, max );
            }

            if ( value < min )
            {
                throw Kernel::ConfigurationRangeException( __FILE__, __LINE__, __FUNCTION__, key.c_str(), value, min );
            }
        }

        template< typename T >
        void EnforceVectorParameterRanges( const std::string& key, std::vector<T> values, json::QuickInterpreter& jsonObj )
        {
            for (T& value : values)
            {
                EnforceParameterRange<T>(key, value, jsonObj);
            }
        }

        template< typename myclass >
        bool initConfig(
            const char* key,
            myclass &thevar,
            const json::QuickInterpreter * pJson,
            const MetadataDescriptor::Enum &enum_md,
            const char* condition_key = nullptr, const char * condition_value = nullptr
        )
        {
            if( JsonConfigurable::_dryrun )
            {
                MetadataDescriptor::Enum * pEnumMd = const_cast<MetadataDescriptor::Enum *>(&enum_md);
                json::Element *elem_copy = _new_ json::Element(pEnumMd->GetSchemaElement());
                auto enumSchema = json::QuickBuilder( *elem_copy );

                if( condition_key )
                {
                    json::Object condition;
                    if( condition_value == nullptr )
                    {
                        condition[ condition_key ] = json::Number( 1 );
                    }
                    else
                    {
                        condition[ condition_key ] = json::String( condition_value );
                    }
                    enumSchema["depends-on"] = condition;
                }
                jsonSchemaBase[key] = enumSchema;
            }

            if( check_condition( pJson, condition_key, condition_value ) )
            {
                return true;
            }

            if (pJson && pJson->Exist(key) == false && _useDefaults )
            {
                if( (EnvPtr != nullptr) && EnvPtr->Log->CheckLogLevel(Logger::INFO, "JsonConfigurable"))
                {
                    EnvPtr->Log->LogF(Logger::INFO, "JsonConfigurable", "Using the default value ( \"%s\" : \"%s\" ) for unspecified parameter.\n", key, enum_md.enum_value_specs[0].first.c_str() );
                }
                thevar = (myclass) enum_md.enum_value_specs[0].second;
                if( _track_missing )
                {
                    missing_parameters_set.insert(key);
                }

                return false;
            }

            std::string enum_value_string = boost::to_lower_copy(GET_CONFIG_STRING(pJson, key));

            bool found = false;

            for (auto& vs : enum_md.enum_value_specs)
            {
                if (boost::to_lower_copy(vs.first) == enum_value_string)
                {
                    thevar = (myclass) vs.second;
                    found = true;
                    break;
                }
            }

            if( !found )
            {
                if( _dryrun )
                {
                    return false;
                }
                else
                {
                    std::ostringstream errorMsgFullList;
                    errorMsgFullList << "Failed to find enum match for value "
                                     << GET_CONFIG_STRING(pJson, key)
                                     << " and key "
                                     << key
                                     << ". Possible values are: ";
                    std::vector< std::string > enum_key_list;

                    // This funny code is to do a comma-separated list of strings without nasty code
                    // to special-handle first or last item. Stole this from some dude on StackOverflow.
                    // Ideally we wouldn't have to pre-pop the enum_key_list container, but don't know
                    // what iterator to use for the MetadataDescriptor::Enum::enum_value_spec pair list.
                    for (auto& vs : enum_md.enum_value_specs)
                    {
                        enum_key_list.push_back( vs.first );
                    }
                    std::copy(enum_key_list.begin(), enum_key_list.end() - 1, std::ostream_iterator<std::string>(errorMsgFullList, ", "));
                    errorMsgFullList << enum_key_list.back();
                    throw Kernel::GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, errorMsgFullList.str().c_str() );
                }
            }

            return found;
        }

        template< typename myclass >
        bool initVectorConfig(
            const char* key,
            std::vector<myclass> &thevector,
            const json::QuickInterpreter * pJson,
            const MetadataDescriptor::Enum &enum_md
        )
        {
            MetadataDescriptor::Enum * pEnumMd = const_cast<MetadataDescriptor::Enum *>(&enum_md);
            if ( _dryrun )
            {
                jsonSchemaBase[key] = pEnumMd->GetSchemaElement();
            }

            // parsing: unspecified case
            if (pJson && pJson->Exist(key) == false && _useDefaults )
            {
                if( (EnvPtr != nullptr) && EnvPtr->Log->CheckLogLevel(Logger::INFO, "JsonConfigurable"))
                {
                    EnvPtr->Log->LogF(Logger::INFO, "JsonConfigurable", "Using the default value ( \"%s\" : [ \"%s\" ] ) for unspecified parameter.\n", key, enum_md.enum_value_specs[0].first.c_str() );
                }
                thevector.push_back( (myclass) enum_md.enum_value_specs[0].second );
                if( _track_missing )
                {
                    missing_parameters_set.insert(key);
                }

                return false;
            }

            std::vector<std::string> enum_value_strings = GET_CONFIG_VECTOR_STRING(pJson, key);

            // parsing: specified case
            for (auto& enum_value_string : enum_value_strings)
            {
                bool found = false;

                for (auto& vs : enum_md.enum_value_specs)
                {
                    if ( boost::to_lower_copy(vs.first) == boost::to_lower_copy(enum_value_string) )
                    {
                        thevector.push_back( (myclass) vs.second );
                        found = true;
                        break;
                    }
                }

                // error handling
                if( found == false )
                {
                    if( _dryrun )
                    {
                        return false;
                    }
                    else
                    {
                        throw Kernel::GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, (std::string("Failed to find enum match for value ") + enum_value_string + " and key " + key).c_str() );
                    }
                }
            }

            return true;
        }

        // OK, so here's the story with initConfigComplexType. The JsonConfigurable base
        // class is for any class that has member variables that need to be initialized
        // from parameters in config.json or campaign.json. Up to now that has been done
        // by the class doing 1 of the following 3 things:
        // 1) initConfig. This is for enums only and does an immediate initialization of
        // the variable from the json (via the Configuration class, which is a
        // json::QuickInterpreter.)
        // 2) initConfigTypeMap. This stores a map of param names to variable pointers,
        // and is used for all types for which an overload of that function has been
        // created in JsonConfigurable. That includes plain-old-datatypes, plus a few
        // others.
        // 3) Custom JSON Parsing. More complex, customized structures -- especially
        // those initialized by nested json objects (or arrays) had been read in through
        // some custom parsing, and the schema had to be provided explicitly.
        //
        // initConfigComplexType replaces #3, and could ultimately replace some of #2.
        // In order to avoid adding one-off typemaps and initConfigTypeMap overloads,
        // the templated method here handles unknown classes.
        //
        // Each unique type that comes through initConfigComplexType gets a corresponding
        // Wrapper<type> instantiated that implements the IJsonConfigurable interface.
        // There is a static map (called IJsonConfigurable::generic_container) that holds
        // all Wrapper<> instances (1 per type). (The map uses the typename as the key.)
        // New instances of Wrapper are created in initConfigComplexType and registered
        // with generic_container. (Obviously existing instances are accessed from there.)
        //
        // The type-specific Wrapper<type> instance has a second (non-static) map, called
        // _labelToVariableMap that stores the actual variables of that type, mapped from
        // parameter_name (aka label) to the variable pointer.
        //
        // In order to access all these compile-time-created typemaps at Configure time,
        // generic_container is iterated over to access each registered Wrapper<type>
        // instance and each of these then iterates over its own map of registered variables.
        // The generic_container accesses the actual Wrapper<type> via the pure virtual
        // Configure method which is implemented at the concrete class level for each type.
        //
        // This accessing and invoking of Configure is done at the end of the Configure
        // method in Configure.cpp. This templated subclass (Wrapper<type>)
        // contains the templated param_name->variable_pointer map. So, in the
        // concrete (templated) incarnation of that class, the Configure function can
        // access the specific templated subclass version of the parameter_name->
        // variable_pointer map ConfigureFromJsonAndKey. From that map it can iterate
        // over all those variable pointers and invoke the ConfigureFromJsonAndKey function
        // that actually initializes variables of the complex custom class.
        //
        // initConfigComplexType< useful_type_T >( "param1", useful_type_T* put_value_here, <json> ):
        //     wrapper = IJsonConfigurable::generic_container[ "useful_type_T" ];
        //     wrapper->_labelToVariableMap[ "param1" ] = put_value_here;
        //
        // Configure:
        //     # uh-oh, I don't know about useful_type_T, just IJsonConfigurable! But that's ok...
        //     foreach IJsonConfigurable::generic_container * ijc_ptr:
        //         icj_ptr->Configure();
        //         ---> useful_class_T::Configure() {
        //              useful_class_T* pVar = _labelToVariableMap< string, useful_class_T* >[ "param1" ];
        //              pVar->ConfigureFromJsonAndKey( ... );
        //              ---> useful_class_T::ConfigureFromJsonAndKey( ... ) {
        //                   _variable1 = <json>[key];
        template< typename complexclass >
        void initConfigComplexType(
            const char* paramName,
            complexclass * pVariable,
            const char* description = default_description,
            const char* condition_key = nullptr, const char* condition_value = nullptr
        )
        {
            if( JsonConfigurable::_dryrun )
            {
                json::QuickBuilder custom_schema = pVariable->GetSchema();

                // going to get something back like : {
                //  "type_name" : "idmType:VectorAlleleEnumPair",
                //  "type_schema" : {
                //      "first" : ...,
                //      "second" : ...
                //      }
                //  }
                std::string custom_type_label = (std::string) custom_schema[ _typename_label() ].As<json::String>();
                json::String custom_type_label_as_json_string = json::String( custom_type_label );
                jsonSchemaBase[ custom_type_label ] = custom_schema[ _typeschema_label() ];
                json::Object newComplexTypeSchemaEntry;
                newComplexTypeSchemaEntry["description"] = json::String( description );
                newComplexTypeSchemaEntry["type"] = json::String( custom_type_label_as_json_string );
                if( condition_key && condition_value )
                {
                    json::Object condition;
                    condition[ condition_key ] = json::String( condition_value );
                    newComplexTypeSchemaEntry["depends-on"] = condition;
                }
                jsonSchemaBase[ paramName ] = newComplexTypeSchemaEntry;
            }

            //std::cout << "type = " << typeid( *pVariable ).name() << std::endl;
            //std::cout << "Storing param name to variable mapping in templated static map." << std::endl;
            Wrapper<complexclass> * wrapper = nullptr;
            if( IJsonConfigurable::generic_container.count( typeid( *pVariable ).name() ) == 0 )
            {
                //std::cout << "Creating new Wrapper object to contain the function to access this class." << std::endl;
                IJsonConfigurable::generic_container[ std::string( typeid( *pVariable ).name() ) ] = new Wrapper<complexclass>();
            }
            wrapper = (Wrapper<complexclass>*) IJsonConfigurable::generic_container.at( std::string( typeid( *pVariable ).name() ) );
            release_assert( wrapper );
            wrapper->_labelToVariableMap[ std::string( paramName ) ] = pVariable;

        }

        virtual bool Configure( const Configuration* inputJson );

        static const char * _typename_label() { return "type_name"; }
        static const char * _typeschema_label()  { return "type_schema"; }
        void handleMissingParam( const std::string& key );
    };

    // No, we don't need everything from JsonConfigurable. No, this is not the final solution.
    // Yes, there is an opportunity to doing something cleverer with base classes.
    class InterventionConfig : public JsonConfigurable
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()

        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) { return e_NOINTERFACE; }
        public:
            InterventionConfig();
            InterventionConfig(json::QuickInterpreter* qi);
            virtual json::QuickBuilder GetSchema();
            virtual void ConfigureFromJsonAndKey( const Configuration* inputJson, const std::string& key );
            json::Element _json;
            //json::QuickInterpreter _qi;

            static void serialize(IArchive&, InterventionConfig&);
    };

    class IndividualInterventionConfig : public InterventionConfig
    {
        public:
            IndividualInterventionConfig();
            virtual json::QuickBuilder GetSchema();
    };

    class NodeInterventionConfig : public InterventionConfig
    {
        public:
            virtual json::QuickBuilder GetSchema();
    };

    class NodeSetConfig : public JsonConfigurable
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) { return e_NOINTERFACE; }
        public:
            NodeSetConfig();
            NodeSetConfig(json::QuickInterpreter* qi);
            virtual json::QuickBuilder GetSchema();
            virtual void ConfigureFromJsonAndKey( const Configuration* inputJson, const std::string& key );
            json::Element _json;
    };

    class EventConfig : public JsonConfigurable
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) { return e_NOINTERFACE; }
        public:
            EventConfig();
            EventConfig(json::QuickInterpreter* qi);
            json::QuickBuilder GetSchema();
            virtual void ConfigureFromJsonAndKey( const Configuration* inputJson, const std::string& key );
            json::Element _json;
    };

    class WaningConfig : public JsonConfigurable
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) { return e_NOINTERFACE; }
        public:
            WaningConfig();
            WaningConfig(json::QuickInterpreter* qi);
            json::QuickBuilder GetSchema();
            virtual void ConfigureFromJsonAndKey( const Configuration* inputJson, const std::string& key );
            json::Element _json;
    };

    #define GET_SCHEMA_STATIC_WRAPPER(x)\
    static json::QuickBuilder GetSchemaImpl() \
    { \
        _dryrun = true; \
        x * newInst = new x(); \
        newInst->Configure(nullptr); \
        return newInst->GetSchema(); \
    } \
    private: \
    static Registrator registrator;

    #define GET_SCHEMA_STATIC_WRAPPER_IMPL(x,y) \
    JsonConfigurable::Registrator y::registrator( #x, &y::GetSchemaImpl );
}
