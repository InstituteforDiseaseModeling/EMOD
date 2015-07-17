/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"

#include "Environment.h"
#include "Configuration.h"
#include "ConfigurationImpl.h"
#include "Sugar.h"
#include "SimpleTypemapRegistration.h"
#include "CajunIncludes.h"
#include "Exceptions.h"
#include "Configure.h"
#ifdef EMBEDDED_PYTHON_DEMO
#include "Python.h"
#endif

#include "Serializer.h"

static const char * _module = "Configuration";

//////////////////////////////////////////////////////////////////////////
// Implementation of basic Configuration wrapper

using namespace json;
using namespace std;

void JsonUtility::logJsonException( const json::ParseException &pe, std::string& err_msg )
{
    std::ostringstream errMsgStream;
    errMsgStream << "Json Parse error at: " << endl;
    errMsgStream << "Doc " <<pe.m_locTokenBegin.m_nDocOffset << "," << endl;
    errMsgStream << "Line " << pe.m_locTokenBegin.m_nLine << "," << endl;
    errMsgStream << "Char " << pe.m_locTokenBegin.m_nLineOffset << "," << endl;
    errMsgStream << pe.what() << endl;
    err_msg = errMsgStream.str();
}

void JsonUtility::logJsonException( const json::ScanException &se, std::string& err_msg )
{
    std::ostringstream errMsgStream;
    errMsgStream << "Json Scan error at: " << endl;
    errMsgStream << "Doc " <<se.m_locError.m_nDocOffset << "," << endl;
    errMsgStream << "Line " << se.m_locError.m_nLine << "," << endl;
    errMsgStream << "Char " << se.m_locError.m_nLineOffset<< "," << endl;
    errMsgStream << se.what() << endl;
    err_msg = errMsgStream.str();
}

bool Configuration::CheckElementByName(const std::string& elementName) const
{
    return Exist(elementName);
}

Configuration* Configuration::LoadFromPython(
    const std::string& filename
)
{
#ifdef EMBEDDED_PYTHON_DEMO
    PyObject * pName, *pModule, *pDict, *pFunc, *pValue;

    //std::cout << "Invoking python (hopefully)!" << std::endl;
    Py_Initialize();
    PyErr_Print();
    pName = PyUnicode_FromString( "emod_config" ); // need to have script emod_config.py in site-packages (or other path visible to python)
    PyErr_Print();
    assert( pName );
    pModule = PyImport_Import( pName );
    PyErr_Print();
    pDict = PyModule_GetDict( pModule );
    PyErr_Print();
    pFunc = PyDict_GetItemString( pDict, "doSomething" ); // need function in script called doSomething
    PyErr_Print();
    PyObject * param_str = PyString_FromString( filename.c_str() );
    PyObject * vars = PyTuple_New( 1 );
    PyTuple_SetItem( vars, 0, param_str );
    pValue = PyObject_CallObject( pFunc, vars );
    PyErr_Print();

    //std::cout << PyString_AsString(pValue) << std::endl;
    std::istringstream testJsonFromPy( PyString_AsString(pValue) );
    
    Py_DECREF( pValue );

    using namespace json;

    Element* elemRootFile = _new_ String(); // this object becomes owned by the configuration

    Reader::Read(*elemRootFile, testJsonFromPy);
    
    Configuration* conf = _new_ Configuration(elemRootFile); 

    return conf;
#else
    throw Kernel::IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "Need to enable EMBEDDED_PYTHON_DEMO in compiler." );
    return NULL;
#endif
}

Configuration* Configuration::Load( string configFileName )
{
#ifdef EMBEDDED_PYTHON_DEMO
    if( getenv( "LOAD_FROM_PYTHON" ) )
    {
        return LoadFromPython( configFileName );
    }
    else
#endif
    {
        //using namespace json;
        //using namespace std;
        // open config file more gingerly for debugging on temperamental network file systems
        std::ifstream ifs_configfile;
        ifs_configfile.open(configFileName.c_str());
        if (ifs_configfile.fail())
        {
            throw Kernel::FileNotFoundException( __FILE__, __LINE__, __FUNCTION__, configFileName.c_str() );
        }

        Configuration* p_config = Load( ifs_configfile, configFileName );
        ifs_configfile.close();
        return p_config ;
    }
}

Configuration* Configuration::Load( std::istream& rInputStream, const std::string& rDataLocation )
{
    Configuration *conf = NULL;
    std::ostringstream final_msg;
    try 
    {
        conf = loadInternal(rInputStream);
    }
    catch (json::ParseException &pe)
    {
        std::string err_msg;
        JsonUtility::logJsonException(pe, err_msg);
        final_msg << "Caught the following json::ParseException while attempting to read data from: " << rDataLocation << ". " << err_msg;
        throw Kernel::GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, final_msg.str().c_str() );
    }
    catch (json::ScanException &se)
    {
        std::string err_msg;
        JsonUtility::logJsonException(se, err_msg);
        final_msg << "Caught the following json::ScanException while attempting to read data from: " << rDataLocation << ". " << err_msg;
        throw Kernel::GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, final_msg.str().c_str() );
    }
    catch (json::Exception &e)
    {
        std::string raw_msg = std::string( e.what() );
        if( raw_msg.find( "Object name not found" ) != std::string::npos )
        {
            unsigned pos = raw_msg.find( ":" );
            std::string param_name = raw_msg.substr( pos+2 );
            throw Kernel::MissingParameterFromConfigurationException( __FILE__, __LINE__, __FUNCTION__, rDataLocation.c_str(), param_name.c_str() );
        }
        else
        {
            final_msg << "Caught the following json::Exception while attempting to read data from: " << rDataLocation << ". " << e.what();
            throw Kernel::GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, final_msg.str().c_str() );
        }
    }
    return conf;
}

Configuration *Configuration_Load( const std::string& rFilename )
{
    return Configuration::Load( rFilename );
}


/*
Configuration* Configuration::Load( istream &config_file )
{    
    return loadInternal(config_file);
}

Configuration* Configuration::loadInternalWrapper( std::istream &config_file )
{
    return loadInternal(config_file);
}
*/


Configuration* Configuration::loadInternal( istream &is_config_file )
{
    using namespace json;

    //Object objRoot;
    Element* elemRootFile = _new_ String(); // this object becomes owned by the configuration

    Reader::Read(*elemRootFile, is_config_file);

    // this constructor and base class constructor have no logic, so we shouldn't have any exceptions
    // we would need to clean up after

    Configuration* conf = _new_ Configuration(elemRootFile); 
    // (we keep a handle to the elemRootFile object in the instance object and clean it up when we're done)

    return conf;
}

Configuration* Configuration::CopyFromElement( const json::Element &elem )
{
    Element *elem_copy = _new_ Element(elem);
    return _new_ Configuration(elem_copy);
}

#if USE_JSON_SERIALIZATION

// IJsonSerializable Interfaces
void Configuration::JSerialize( Kernel::IJsonObjectAdapter* root, Kernel::JSerializer* helper ) const
{
    root->BeginObject();
    ostringstream oss(ostringstream::out);
    Writer::Write(*pElement, oss);
    // Trying 2-step for linux
    std::string stringToSerialize = oss.str();
    root->Insert("SerializedConfigurationString", stringToSerialize.c_str());
    root->EndObject();
}

void Configuration::JDeserialize( Kernel::IJsonObjectAdapter* root, Kernel::JSerializer* helper )
{
}
#endif

#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI

BOOST_CLASS_EXPORT(Configuration)
template<class Archive>
void Configuration::serialize( Archive &ar, const unsigned int v )
{
    // hacked serialization....converts to string then serializes that, rather than trying to do an efficient serialization of the cajun objects themselves

    if (typename Archive::is_loading())
    {
        string data;
        ar & data;

        istringstream iss(data, istringstream::in);

        Element *elem = _new_ String();
        Reader::Read(*elem, iss);

        pElement = elem;
#ifdef WIN32
        this->QuickInterpreter::QuickInterpreter(*pElement); // reinit base class
#else
#warning "code commented out to get linux build to work"
#endif

    } else
    {
        ostringstream oss(ostringstream::out);
        Writer::Write(*pElement, oss);
        // Trying 2-step for linux
        std::string stringToSerialize = oss.str();
        ar & stringToSerialize;
    }


}


template void Configuration::serialize(boost::archive::binary_oarchive &ar, unsigned int);
template void Configuration::serialize(boost::archive::binary_iarchive &ar, unsigned int);

// Commenting these out (everywhere) saves 1.2M in release build. :) 
//template void Configuration::serialize(boost::archive::text_oarchive &ar, unsigned int);
//template void Configuration::serialize(boost::archive::text_iarchive &ar, unsigned int);

template void Configuration::serialize(boost::mpi::detail::content_oarchive &ar, unsigned int);
template void Configuration::serialize(boost::mpi::detail::mpi_datatype_oarchive &ar, unsigned int);
template void Configuration::serialize(boost::mpi::packed_iarchive &ar, unsigned int);
template void Configuration::serialize(boost::mpi::packed_oarchive &ar, unsigned int);
template void Configuration::serialize(boost::mpi::packed_skeleton_iarchive &ar, unsigned int);
template void Configuration::serialize(boost::mpi::packed_skeleton_oarchive &ar, unsigned int);
#endif

/////////////////////////////////////////////////////////////////////////////////////////
// config/json loading wrappers

vector<float> GET_CONFIG_VECTOR_FLOAT(const QuickInterpreter* parameter_source, const char *name)
{
    vector<float> values;

    if(parameter_source == NULL)
    {
        if( Kernel::JsonConfigurable::_dryrun )
        {
            return values;
        }
        else
        {
            throw std::runtime_error("Null pointer!  Invalid config passed for parsing");
        }
    }
    try
    {
        json::QuickInterpreter json_array( (*parameter_source)[name].As<json::Array>() );
        for( unsigned int idx = 0; idx < (*parameter_source)[name].As<json::Array>().Size(); idx++ )
        {
            float value = (float)json_array[idx].As<json::Number>();
            values.push_back(value);
        }
    }
    catch (json::Exception& e)
    {
        string full_description(e.what());
        if (strcmp(e.what(), "Bad json_cast") == 0)
        {
            full_description += ": ";
            full_description += name;
            full_description += " (expected JSON array of numeric)";
        }

        if( Kernel::JsonConfigurable::_dryrun )
        {
            return values;
        }
        else
        {
            throw json::Exception(full_description);
        }
    }

    return values;
}

vector<vector<float>> GET_CONFIG_VECTOR2D_FLOAT(const QuickInterpreter* parameter_source, const char *name)
{
    vector<vector<float>> matrix;

    if(parameter_source == NULL)
    {
        if( Kernel::JsonConfigurable::_dryrun )
        {
            return matrix;
        }
        else
        {
            throw std::runtime_error("Null pointer!  Invalid config passed for parsing");
        }
    }
    try
    {
        unsigned int num_elements_x = (*parameter_source)[name].As<json::Array>().Size();

        json::QuickInterpreter json_array_of_arrays( (*parameter_source)[name].As<json::Array>() );
        for( unsigned int idx = 0; idx < num_elements_x; idx++ )
        {
            json::QuickInterpreter json_array( json_array_of_arrays[idx].As<json::Array>() );

            unsigned int num_elements_y = json_array_of_arrays[idx].As<json::Array>().Size() ;
            std::vector<float> values;

            for( unsigned int idy = 0; idy < num_elements_y; idy++ )
            {
                float value = (float)json_array[idy].As<json::Number>();
                values.push_back(value);
            }
            matrix.push_back( values );
        }
    }
    catch (json::Exception& e)
    {
        string full_description(e.what());
        if (strcmp(e.what(), "Bad json_cast") == 0)
        {
            full_description += ": ";
            full_description += name;
            full_description += " (expected JSON array of numeric)";
        }

        if( Kernel::JsonConfigurable::_dryrun )
        {
            return matrix;
        }
        else
        {
            throw json::Exception(full_description);
        }
    }

    return matrix;
}

vector<string> GET_CONFIG_VECTOR_STRING(const QuickInterpreter* parameter_source, const char *name)
{
    vector<string> values;

    if(parameter_source == NULL)
    {
        //if( getenv( "DRYRUN" ) )
        if( Kernel::JsonConfigurable::_dryrun )
        {
            return values;
        }
        else
        {
            throw std::runtime_error("Null pointer!  Invalid config passed for parsing");
        }
    }
    try
    {
        json::QuickInterpreter json_array( (*parameter_source)[name].As<json::Array>() );
        for( unsigned int idx = 0; idx < (*parameter_source)[name].As<json::Array>().Size(); idx++ )
        {
            string value = (string)json_array[idx].As<json::String>();
            values.push_back(value);
        }
    }
    catch (json::Exception& e)
    {
        string full_description(e.what());
        if (strcmp(e.what(), "Bad json_cast") == 0)
        {
            full_description += ": ";
            full_description += name;
            full_description += " (expected json array of strings)";
        }

        //if( getenv( "DRYRUN" ) )
        if( Kernel::JsonConfigurable::_dryrun )
        {
            return values;
        }
        else
        {
            throw json::Exception(full_description);
        }
    }

    return values;
}

set<string> GET_CONFIG_STRING_SET(const QuickInterpreter* parameter_source, const char *name)
{
    set<string> values;

    if(parameter_source == NULL)
    {
        //if( getenv( "DRYRUN" ) )
        if( Kernel::JsonConfigurable::_dryrun )
        {
            return values;
        }
        else
        {
            throw std::runtime_error("Null pointer!  Invalid config passed for parsing");
        }
    }
    try
    {
        json::QuickInterpreter json_array( (*parameter_source)[name].As<json::Array>() );
        for( unsigned int idx = 0; idx < (*parameter_source)[name].As<json::Array>().Size(); idx++ )
        {
            string value = (string)json_array[idx].As<json::String>();
            values.insert(value);
        }
    }
    catch (json::Exception& e)
    {
        string full_description(e.what());
        if (strcmp(e.what(), "Bad json_cast") == 0)
        {
            full_description += ": ";
            full_description += name;
            full_description += " (expected json array of strings)";
        }

        //if( getenv( "DRYRUN" ) )
        if( Kernel::JsonConfigurable::_dryrun )
        {
            return values;
        }
        else
        {
            throw json::Exception(full_description);
        }
    }

    return values;
}


string GET_CONFIG_STRING(const QuickInterpreter* parameter_source, const char *name)
{
    string value = "";

    if(parameter_source == NULL)
    {
        //if( getenv( "DRYRUN" ) )
        if( Kernel::JsonConfigurable::_dryrun )
        {
            return value;
        }
        else
        {
            throw std::runtime_error("Null pointer!  Invalid config passed for parsing");
        }
    }

    try {
        value = (string)((*parameter_source)[name].As<json::String>());
    }
    catch (json::Exception& e)
    {
        string full_description(e.what());
        if (strcmp(e.what(), "Bad json_cast") == 0)
        {
            full_description += ": ";
            full_description += name;
            full_description += " (expected string)";
        }

        if( Kernel::JsonConfigurable::_dryrun )
        //if( getenv( "DRYRUN" ) )
        {
            return value;
        }
        else
        {
            throw json::Exception(full_description);
        }
    }

    return value;
}


double GET_CONFIG_DOUBLE(
    const QuickInterpreter* parameter_source,
    const char *name,
    int min
)
{
    double value = 0.0;

    if(parameter_source == NULL)
    {
        //if( getenv( "DRYRUN" ) )
        if( Kernel::JsonConfigurable::_dryrun )
        {
            return 0.0f;
        }
        else
        {
            throw std::runtime_error("Null pointer!  Invalid config passed for parsing");
        }
    }
    try {
        value = (*parameter_source)[name].As<json::Number>();
    }
    catch (json::Exception& e)
    {
        string full_description(e.what());
        if (strcmp(e.what(), "Bad json_cast") == 0)
        {
            full_description += ": ";
            full_description += name;
            full_description += " (expected numeric)";
        }

        if( Kernel::JsonConfigurable::_dryrun )
        {
            return 0.0f;
        }
        else
        {
            throw json::Exception(full_description);
        }
    }

    return value;
}


/*bool GET_CONFIG_BOOLEAN(const json::QuickInterpreter* parameter_source, const char *name)
{
    double d = GET_CONFIG_DOUBLE(parameter_source, name);

    if(d != 0.0 && d != 1.0)
        throw std::runtime_error(("Non-boolean value found for config parameter: " + std::string(name)).c_str());

    return (d == 1.0);
}*/


namespace Kernel
{

    //////////////////////////////////////////////////////////////////////////
    // Implementation support for Configurable functionality


    void ConfigurationImpl::get_config_enum_value( const Configuration* config, string name, int *value, MetadataDescriptor::Enum &enum_md )
    {
        string enum_value_string = boost::to_lower_copy(GET_CONFIG_STRING(config, name));
        
        bool found = false;

        for (auto& vs : enum_md.enum_value_specs)
        {
            if (boost::to_lower_copy(vs.first) == enum_value_string)
            {
                *value = vs.second; found = true; break;
            }
        }


        if (!found)
        {
            string full_description = "Error parsing enum field '" + name + "'; value '" + enum_value_string + "' not recognized.\n";
            full_description+="Valid values are: ";
            for (auto& vs : enum_md.enum_value_specs)
            {
                full_description += vs.first + " ";
            }
            full_description += "\n";

            throw json::Exception(full_description); // TODO: this properly makes more sense as a configuration exception or a json parse exception...left to future tidying
        }
    }

#if TEST_OLD_STUFF

   /////////////////////////////////////////////////////////////////////////
   // Test Classes
 
    ENUM_DEFINE(Style, 
        ENUM_VALUE_SPEC(Fez,0) 
        ENUM_VALUE_SPEC(Pirate,1)
        ENUM_VALUE_SPEC(Wizard,665))

    class TinfoilHat : public IConfigurable// dummy intervention
    {
        DECLARE_SERIALIZABLE(TinfoilHat)
    public:
        DECLARE_CONFIGURED(TinfoilHat)
        DECLARE_QUERY_INTERFACE()
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        
            
        // configured attributes
        BEGIN_PERSIST()
            Persist<float> thickness;
            Persist<bool> has_carrying_case;
            Persist<Style::Enum> style;
        END_PERSIST()



    private:
        ///////////////////////////////////////////////////////////////////////////
        // Serialization
        friend class ::boost::serialization::access;

        template<class Archive>
        void serialize_inner(Archive &ar, const unsigned int v)
        {
            typemap.serialize(this, ar, v);
        }


    };

    IMPLEMENT_SERIALIZABLE(TinfoilHat)
    IMPL_QUERY_INTERFACE1(TinfoilHat, IConfigurable)

    BEGIN_IMPLEMENT_CONFIGURED(TinfoilHat)
#ifdef WIN32
//        CONFIGURE(thickness, "thickness", 0, 1, "Foil thickness in mm")
//        CONFIGURE(has_carrying_case, "has_carrying_case", 0, 1, "Convenient travel pouch?")
        CONFIGURE(thickness, MetadataDescriptor::Real("thickness", "Foil thickness in mm", 0, 1))
        CONFIGURE(has_carrying_case, MetadataDescriptor::Bool("has_carrying_case", "Convenient travel pouch?"))
       // CONFIGURE(style, "has_carrying_case", 0, 1, "Convenient travel pouch?")
       CONFIGURE(style, MetadataDescriptor::Enum("style", "Hat style", MDD_ENUM_ARGS(Style)))
#endif
    END_IMPLEMENT_CONFIGURED(TinfoilHat)


    class ConfiguredTest : public IConfigurable
    {
        DECLARE_SERIALIZABLE(ConfiguredTest)
    public:
        DECLARE_CONFIGURED(ConfiguredTest)
        DECLARE_QUERY_INTERFACE()
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()

        ConfiguredTest() : hat_config(NULL) { }
        virtual ~ConfiguredTest() { delete hat_config; }
        

        
        BEGIN_PERSIST()
            Persist<int> foo;
            Persist<float> bar;
        END_PERSIST()
        Configuration *hat_config;

        TinfoilHat configured_hat;

        std::vector<TinfoilHat> hats;

        // how to implement configure for the sub objects? in general we wont know how many or when they are to be created, so how about we just keep a pointer around
        // to the config sub structure that contains the info we want



    private:
        ///////////////////////////////////////////////////////////////////////////
        // Serialization
        friend class ::boost::serialization::access;

        template<class Archive>
        void serialize_inner(Archive &ar, const unsigned int v)
        {
            typemap.serialize(this, ar, v);
            ar & hat_config;
            ar & configured_hat;
            ar & hats;
        }

    };




    IMPLEMENT_SERIALIZABLE(ConfiguredTest)

    IMPL_QUERY_INTERFACE1(ConfiguredTest, IConfigurable)

    BEGIN_IMPLEMENT_CONFIGURED(ConfiguredTest)
#ifdef WIN32
/*        CONFIGURE(foo, "foo", 0, 10, "This is Foo.")
        CONFIGURE(bar, "bar", -100, .001f, "Standard bar.")
        CONFIGURE(hat_config, "hat_config", 0, 0, "An arbitrary configuration, intended to be for a hat")
        CONFIGURE_OBJECT(configured_hat, "configured_hat", "Configuration of embedded hat" )
        */
        CONFIGURE(foo, MetadataDescriptor::Real("foo", "This is member foo", 0, 10))
        CONFIGURE(bar, MetadataDescriptor::Real("bar", "Standard bar", -100, .001f))
        CONFIGURE(hat_config, MetadataDescriptor::Configuration("hat_config", "An arbitrary configuration, intended to be for a hat"))
        CONFIGURE(configured_hat, MetadataDescriptor::Configurable("configured_hat", "Configuration of a subordinate hat", &configured_hat))
#endif
    END_IMPLEMENT_CONFIGURED(ConfiguredTest)
/*   }
   template void ConfiguredTest::common_configured_dispatch<ModeConfigure>(const Configuration *config, string *schema);
   template void ConfiguredTest::common_configured_dispatch<ModeGetSchema>(const Configuration *config, string *schema);
   */

    class ConfiguredDerived : public ConfiguredTest
    {
        DECLARE_SERIALIZABLE(ConfiguredTest)
    public:
        DECLARE_CONFIGURED(ConfiguredDerived)
        std::string baz;


    private:
        ///////////////////////////////////////////////////////////////////////////
        // Serialization
        friend class ::boost::serialization::access;

        template<class Archive>
        void serialize(Archive &ar, const unsigned int v)
        {
            ar & baz;
            ar & boost::serialization::base_object<Kernel::ConfiguredTest>(*this);
        }
    };

    BEGIN_IMPLEMENT_CONFIGURED_DERIVED(ConfiguredDerived, ConfiguredTest)
#ifdef WIN32
        CONFIGURE(baz, MetadataDescriptor::String("baz", "The standard baz string"))
#endif
    END_IMPLEMENT_CONFIGURED(ConfiguredDerived)



    class ConfiguredTestFactory
    {

    public:

        static ISupports* CreateInstance(const Configuration *config) // general-looking interface but only knows how to create ConfiguredTest instances at the moment
        {
            // get classname, then see if we know how to instantiate one of those

            string classname = GET_CONFIG_STRING(config, "class");
            ISupports* obj = NULL;
            if (classname == "ConfiguredTest")
            {                            
                obj = _new_ ConfiguredTest;
                obj->AddRef();
            } // else...more
            if (classname == "ConfiguredDerived")
            {                            
                obj = _new_ ConfiguredDerived;
                obj->AddRef();
            } // else...more

            if (NULL == obj)
            {
                throw FactoryCreateFromJsonException( __FILE__, __LINE__, __FUNCTION__, classname.c_str() );
            } else
            {
                IConfigurable *conf_obj = NULL;
                if (s_OK == obj->QueryInterface(GET_IID(IConfigurable), (void**)&conf_obj))
                {
                    if (!conf_obj->Configure(config))
                    {
                        throw FactoryCreateFromJsonException( __FILE__, __LINE__, __FUNCTION__, "Failed in call to Configure." );
                        conf_obj->Release();
                        obj->Release();
                        return NULL;
                    }
                    // need to release when done?
                    conf_obj->Release();
                }

            }
            return obj;

        }

    };
#endif



}

