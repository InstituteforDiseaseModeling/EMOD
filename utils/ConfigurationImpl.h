/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <string>
#include <list>
#include <vector>
#include <functional>
#include "ISupports.h"
#include "EnumSupport.h"
#include <boost/type_traits/is_base_of.hpp>

namespace Kernel
{
    namespace MetadataDescriptor 
    {
        struct Enum; // forward decl
    }

    struct ConfigurationImpl // overloaded functions wrapping config retrieval
    {
        static void get_config_value(const Configuration* config, std::string name, float *value)
        {
            *value = GET_CONFIG_NUMBER(config, name);
        }

        static void get_config_value(const Configuration* config, std::string name, std::string *value)
        {
            *value = GET_CONFIG_STRING(config, name);
        }

        static void get_config_value(const Configuration* config, std::string name, bool *value)
        {
            bool  v = GET_CONFIG_DOUBLE(config, name) != 0; // redundant conversion to get rid of inexplicable warning C4800 on msvc. 
            (*value) = v;
        }

        static void get_config_value(const Configuration* config, std::string name, int *value)
        {
            *value = GET_CONFIG_INTEGER(config, name);
        }

        static void get_config_value(const Configuration* config, std::string name, IConfigurable *value)
        {
            // TODO: error handling here?...could also pass back return codes
            // need to catch json lookup exceptions here and report errors
            value->Configure((const Configuration*)&((*config)[name]));
        }

        // cache the configuration substructure with a named key
        /* static void get_config_value(const Configuration* config, std::string name, Configuration const** pp_cached_config)
        {
            // TODO: error handling here?...could also pass back return codes
            // need to catch json lookup exceptions here and report errors
            *pp_cached_config = (const Configuration*)(&(*config)[name]);
        }
        */

        static void get_config_value(const Configuration* config, std::string name, Configuration ** pp_cached_config)
        {
            // TODO: error handling here?...could also pass back return codes
            // need to catch json lookup exceptions here and report errors

            *pp_cached_config = Configuration::CopyFromElement( (*config)[name], config->GetDataLocation() );
        }

        /*
        static void get_schema_element_for_configuration(QuickBuilder *schema, std::string name, std::string description )
        {
            Array& members = (*schema)["members"].As<Array>();
            Element member = Object();
            QuickBuilder qb(member);
            qb["name"] = String(name);
            qb["type"] = String("configuration"); // TODO: may want this to work differently later, maybe rename this field to 'class' 
            qb["description"] = String(description);

            members.Insert( qb, members.End());
        }

        static void get_schema_element_for_configurable(QuickBuilder *schema, IConfigurable* configurable_member, std::string name, std::string description )
        {
            Array& members = (*schema)["members"].As<Array>();
            Element member = Object();
            QuickBuilder qb(member);
            qb["name"] = String(name);
            qb["type"] = String("object"); // TODO: may want this to work differently later, maybe rename this field to 'class' 
            qb["description"] = String(description);
            qb["schema"] = (configurable_member->GetSchema()); // todo: this is probably a bit sloppy; figure out whether in fact all elements constructed by the QB are deep copied...maybe GetSchema could just return QB (-*)

            members.Insert( qb, members.End());
        }

        static void get_schema_element_for_param(QuickBuilder *schema, std::string name, std::string type_name, float min, float max, std::string description )
        {
            // Use QB to construct a json element...maybe return that then instead of strings? anyway doesn't have to happen now
            //return std::string("{") + "\"name\" :\""+ name + "\", \"type\": \""+ type_name +"\" }"; // TODO: put other stuff in too
            Array& members = (*schema)["members"].As<Array>();
            Element member = Object();
            QuickBuilder qb(member);
            qb["name"] = String(name);
            qb["type"] = String(type_name);
            qb["min"] = Number(min);
            qb["max"] = Number(max);
            qb["description"] = String(description);
            members.Insert( qb, members.End() );
        }
        */
    };

    class ModeConfigure { };
    class ModeGetSchema { };

    namespace MetadataDescriptor
    {
        struct Base
        {
        public:
            std::string name;
            std::string description;
            virtual Element GetSchemaElement() = 0;

        protected: // Don't want outsiders constructing this
            Base(std::string _name, std::string _desc) : name(_name), description(_desc) {}
        };

        struct Enum : public Base
        {
            // convenience macro to save typing of these args
#define MDD_ENUM_ARGS(enum_name) enum_name::pairs::count(), enum_name::pairs::get_keys(), enum_name::pairs::get_values()

            Enum(std::string _name, std::string _desc, int count, const char ** strings, const int *values) : Base(_name, _desc)
            {
                for (int k = 0; k < count; k++)
                {
                    enum_value_specs.push_back(pair<std::string,int>(strings[k], values[k]));
                }
            }

            typedef pair<std::string,int> enum_value_spec_t;
            std::vector<enum_value_spec_t> enum_value_specs;

            virtual const char *GetTypeString()
            {
                return "enum";
            }

            virtual Element GetSchemaElement()
            {
                Element member = Object();
                QuickBuilder qb(member);
                qb["type"] = json::String(GetTypeString());

                for (int k = 0; k < enum_value_specs.size(); k++)
                {
                    qb["enum"][k] = json::String(enum_value_specs[k].first);
                }

                qb["description"] = json::String(description);
                qb["default"] = json::String(enum_value_specs[0].first);
                return member;
            }
        };

        struct VectorOfEnum : public Enum
        {
            VectorOfEnum(std::string _name, std::string _desc, int count, const char ** strings, const int *values) :
                Enum(_name, _desc, count, strings, values)
            {
            }

            virtual const char *GetTypeString()
            {
                return "Vector Enum";
            }
        };

        struct Bool : public Base
        {
            Bool(std::string _name, std::string _desc) : Base(_name, _desc) {}

            virtual Element GetSchemaElement()
            {
                Element member = Object();
                QuickBuilder qb(member);
                qb["name"] = json::String(name);
                qb["type"] = json::String("bool");
                qb["description"] = json::String(description);

                return member;
            }
        };

        struct String : public Base
        {
            String(std::string _name, std::string _desc) : Base(_name, _desc) {}

            virtual Element GetSchemaElement()
            {
                Element member = Object();
                QuickBuilder qb(member);
                qb["name"] = json::String(name);
                qb["type"] = json::String("string");
                qb["description"] = json::String(description);

                return member;
            }
        };

        struct Integer : public Base
        {
            Integer(std::string _name, std::string _desc) : Base(_name, _desc), has_min(false), has_max(false) {}
            Integer(std::string _name, std::string _desc, int _min, int _max) : Base(_name, _desc), m_min(_min), m_max(_max), has_min(true), has_max(true) {}

            bool has_min, has_max;
            int m_min, m_max;
            virtual Element GetSchemaElement()
            {
                Element member = Object();
                QuickBuilder qb(member);
                qb["name"] = json::String(name);
                qb["type"] = json::String("int");
                if (has_min)
                    qb["min"] = Number(m_min);
                if (has_max)
                    qb["max"] = Number(m_max);

                qb["description"] = json::String(description);

                return member;
            }
        };

        struct Real : public Base
        {
            Real(std::string _name, std::string _desc) : Base(_name, _desc), has_min(false), has_max(false) {}
            Real(std::string _name, std::string _desc, float _min, float _max) : Base(_name, _desc), m_min(_min), m_max(_max), has_min(true), has_max(true) {}

            bool has_min, has_max;
            float m_min, m_max;
            virtual Element GetSchemaElement()
            {
                Element member = Object();
                QuickBuilder qb(member);
                qb["name"] = json::String(name);
                qb["type"] = json::String("real");
                if (has_min) 
                    qb["min"] = Number(m_min);
                if (has_max)
                    qb["max"] = Number(m_max);

                qb["description"] = json::String(description);

                return member;
            }
        };

        struct Configuration : public Base
        {
            Configuration(std::string _name, std::string _desc) : Base(_name, _desc) {}

            virtual Element GetSchemaElement()
            {
                Element member = Object();
                QuickBuilder qb(member);
                qb["name"] = json::String(name);
                qb["type"] = json::String("configuration");
                qb["description"] = json::String(description);

                return member;
            }
        };

        struct Configurable : public Base
        {
            Configurable(std::string _name, std::string _desc, IConfigurable* _object) : Base(_name, _desc), object(_object) {}

            IConfigurable *object;
            virtual Element GetSchemaElement()
            {
                Element member = Object();
                QuickBuilder qb(member);
                qb["name"] = json::String(name);
                qb["type"] = json::String("object");
                qb["description"] = json::String(description);
#ifdef _WIN32
                qb["schema"] = object->GetSchema();
#else
#warning "line of code for GetSchema commented out to get to build on linux"
#endif
                return member;
            }
        };
    }

    /////////////////////////////////////////////////////
    // versions using metadata descriptors

#ifdef WIN32
    template <class MemberT, class MetadataDescriptorT, class ModeT>
    void process_configured_member_mdd_dispatch(ModeT &m,  MemberT *value, MetadataDescriptorT &mdd, const Configuration *&config, QuickBuilder *&schema)
    {
       process_configured_member_mdd<boost::is_base_of<Persist_impl_base_tag, MemberT>::value >::process(m, value, mdd, config, schema);
    }
#endif

    // TODO: add some exception handling in Configure and GetSchema to provide nesting context around exceptions thrown lower down

#define BEGIN_IMPLEMENT_CONFIGURED(classname)\
    BEGIN_IMPLEMENT_CONFIGURED_DETAIL(classname, {}, {})

#ifdef WIN32
#define BEGIN_IMPLEMENT_CONFIGURED_DERIVED(classname, baseclassname)\
    BEGIN_IMPLEMENT_CONFIGURED_DETAIL(classname, \
    {\
        if (!baseclassname::Configure(config)) return false;   /*doesn't actually support returning false from the internals yet but in case it does...*/\
    },\
    {\
        QuickBuilder base_schema = baseclassname::GetSchema(); \
        (*schema)["members"] = base_schema["members"]; \
    })
#else
#warning Some GetSchema code commented out to get gcc/linux build to work
#define BEGIN_IMPLEMENT_CONFIGURED_DERIVED(classname, baseclassname)\
    BEGIN_IMPLEMENT_CONFIGURED_DETAIL(classname, \
    {\
        if (!baseclassname::Configure(config)) return false;   /*doesn't actually support returning false from the internals yet but in case it does...*/\
    },\
    {\
        /*QuickBuilder base_schema = (QuickBuilder) baseclassname::GetSchema();*/ \
        /*(*schema)["members"] = base_schema["members"];*/ \
    })
#endif

#define BEGIN_IMPLEMENT_CONFIGURED_DETAIL(classname, base_class_configure_call_block, base_class_get_schema_call_block)\
    bool classname::Configure(const Configuration *config) \
    {\
        base_class_configure_call_block; \
        common_configured_dispatch<ModeConfigure>(config, nullptr);\
        return true; \
    }\
    QuickBuilder classname::GetSchema()\
    {\
        Element *obj = _new_ Object();\
        QuickBuilder *schema = _new_ QuickBuilder(*obj);\
        (*schema)["class"] = String(#classname);\
        (*schema)["members"] = Array();\
        base_class_get_schema_call_block; /* substitute the members list of the base class before the members of the derived class are added*/\
        common_configured_dispatch<ModeGetSchema>(nullptr, schema);\
        return *schema;\
    }\
    template<class Mode>\
    void classname::common_configured_dispatch(const Configuration *config, /*caller supplies base object, in-out param*/ QuickBuilder *schema)\
    {\
        Mode mode; Mode suppress_unreferenced_variable_warning_by_assigning = mode;\

    /*
#define CONFIGURE(member, name, min, max, description) \
    process_configured_member(mode, &member, name, min, max, description, config, schema);

    // TODO: should be possible to get rid of this via some clever template metaprogramming, but I'm too dumb to think of it now...
#define CONFIGURE_OBJECT(member, name, description) \
    process_configured_member_object(mode, &member, name, description, config, schema);
    */

// new style
#define CONFIGURE(member, metadata_descriptor_ctor) \
    process_configured_member_mdd_dispatch(mode, &member, metadata_descriptor_ctor, config, schema);

#define END_IMPLEMENT_CONFIGURED(classname)\
   }\
   template void classname::common_configured_dispatch<ModeConfigure>(const Configuration *config, QuickBuilder *schema);\
   template void classname::common_configured_dispatch<ModeGetSchema>(const Configuration *config, QuickBuilder *schema);
}
