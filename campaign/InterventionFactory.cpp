
#include "stdafx.h"
#include "InterventionFactory.h"
#include "Interventions.h"
#include "Log.h"
#include "ObjectFactoryTemplates.h"

SETUP_LOGGING( "InterventionFactory" )

namespace Kernel
{
    InterventionFactory* InterventionFactory::_instance = nullptr;

    template InterventionFactory* ObjectFactory<IDistributableIntervention, InterventionFactory>::getInstance();

    InterventionFactory::InterventionFactory()
        : ObjectFactory<IDistributableIntervention, InterventionFactory>()
        , m_UseDefaults( false )
    {
    }

    void InterventionFactory::Register( const char *classname, instantiator_function_t _if )
    {
        ObjectFactory<IDistributableIntervention, InterventionFactory>::Register( classname, _if );
    }

    // new, configurable method
    IDistributableIntervention* InterventionFactory::CreateIntervention( const json::Element& rJsonElement,
                                                                         const std::string& rDataLocation,
                                                                         const char* parameterName,
                                                                         bool throwIfNull )
    {
        // Keeping this simple. But bear in mind CreateInstanceFromSpecs can throw exception
        // and JC::_useDefaults will not be restored. But we won't keep running in that case.
        bool reset = JsonConfigurable::_useDefaults;
        JsonConfigurable::_useDefaults = m_UseDefaults;

        Configuration* p_config = Configuration::CopyFromElement( rJsonElement, rDataLocation );
        CheckElement( p_config, parameterName, false );

        IDistributableIntervention* p_di = CreateInstanceFromSpecs<IDistributableIntervention>( p_config, m_RegisteredClasses, true );

        if( p_di != nullptr )
        {
            CheckSimType( p_di );
        }
        else if( throwIfNull )
        {
            // if we get here it should mean that we are expecting an individual-level
            // intervention, but the user provided a node-level intervention
            std::string class_name = std::string((*p_config)[ "class" ].As<json::String>());

            std::stringstream ss;
            ss << "Error loading '" << class_name << "' via "
                << "'" << GetFactoryName() << "' for '" << parameterName << "' in <" << rDataLocation << ">.\n"
                << "This parameter only takes individual-level interventions.";
            throw FactoryCreateFromJsonException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }

        delete p_config;
        JsonConfigurable::_useDefaults = reset;

        return p_di;
    }

    void InterventionFactory::CreateInterventionList( const json::Element& rJsonElement,
                                                      const std::string& rDataLocation,
                                                      const char* parameterName,
                                                      std::vector<IDistributableIntervention*>& interventionsList )
    {
        if( rJsonElement.Type() == json::NULL_ELEMENT )
        {
            std::stringstream ss;
            ss << "'" << GetFactoryName()<< "' found the element to be NULL for '" << parameterName << "' in <" << rDataLocation << ">.";
            throw FactoryCreateFromJsonException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }

        if( rJsonElement.Type() != json::ARRAY_ELEMENT )
        {
            std::stringstream ss;
            ss << "'" << GetFactoryName() << "' found the element specified by '" << parameterName << "'\n"
               << "to NOT be a JSON ARRAY in <" << rDataLocation << ">.";
            throw FactoryCreateFromJsonException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
        const json::Array & interventions_array = json::QuickInterpreter(rJsonElement).As<json::Array>();

        if( interventions_array.Size() == 0 )
        {
            std::stringstream ss;
            ss << "'" << GetFactoryName() << "' found zero elements in JSON for '" << parameterName << "' in <" << rDataLocation << ">.";
            throw FactoryCreateFromJsonException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }

        for( int idx = 0; idx < interventions_array.Size(); ++idx )
        {
            std::stringstream param_name;
            param_name << parameterName << "[" << idx << "]";

            const json::Element& r_array_element = interventions_array[ idx ];
            if( r_array_element.Type() != json::OBJECT_ELEMENT )
            {
                std::stringstream ss;
                ss << "'" << GetFactoryName() << "' found the element specified by '" << param_name.str() << "'\n"
                   << "to NOT be a JSON OBJECT in <" << rDataLocation << ">.";
                throw FactoryCreateFromJsonException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }

            const json::Object& json_obj = json_cast<const json::Object&>(interventions_array[idx]);

            // Instantiate and distribute interventions
            IDistributableIntervention *di = InterventionFactory::getInstance()->CreateIntervention( json_obj,
                                                                                                     rDataLocation,
                                                                                                     param_name.str().c_str(),
                                                                                                     true );
            interventionsList.push_back( di );
        }
    }

    INodeDistributableIntervention* InterventionFactory::CreateNDIIntervention( const json::Element& rJsonElement,
                                                                                const std::string& rDataLocation,
                                                                                const char* parameterName,
                                                                                bool throwIfNull )
    {
        bool reset = JsonConfigurable::_useDefaults;
        JsonConfigurable::_useDefaults = m_UseDefaults;

        Configuration* p_config = Configuration::CopyFromElement( rJsonElement, rDataLocation );
        CheckElement( p_config, parameterName, false );

        INodeDistributableIntervention* p_ndi = CreateInstanceFromSpecs<INodeDistributableIntervention>( p_config, m_RegisteredClasses, true );

        if( p_ndi != nullptr )
        {
            CheckSimType( p_ndi );
        }
        else if( throwIfNull )
        {
            // if we get here it should mean that we are expecting an individual-level
            // intervention, but the user provided a node-level intervention
            std::string class_name = std::string((*p_config)[ "class" ].As<json::String>());

            std::stringstream ss;
            ss << "Error loading '" << class_name << "' via "
                << "'" << GetFactoryName() << "' for '" << parameterName << "' in <" << rDataLocation << ">.\n"
                << "This parameter only takes node-level interventions.";
            throw FactoryCreateFromJsonException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }

        delete p_config;
        JsonConfigurable::_useDefaults = reset;

        return p_ndi;
    }

    void InterventionFactory::CreateNDIInterventionList( const json::Element& rJsonElement,
                                                         const std::string& rDataLocation,
                                                         const char* parameterName,
                                                         std::vector<INodeDistributableIntervention*>& interventionsList )
    {
        if( rJsonElement.Type() == json::NULL_ELEMENT )
        {
            std::stringstream ss;
            ss << "'" << GetFactoryName()<< "' found the element to be NULL for '" << parameterName << "' in <" << rDataLocation << ">.";
            throw FactoryCreateFromJsonException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }

        if( rJsonElement.Type() != json::ARRAY_ELEMENT )
        {
            std::stringstream ss;
            ss << "'" << GetFactoryName() << "' found the element specified by '" << parameterName << "'\n"
               << "to NOT be a JSON ARRAY in <" << rDataLocation << ">.";
            throw FactoryCreateFromJsonException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
        const json::Array & interventions_array = json::QuickInterpreter(rJsonElement).As<json::Array>();

        if( interventions_array.Size() == 0 )
        {
            std::stringstream ss;
            ss << "'" << GetFactoryName() << "' found zero elements in JSON for '" << parameterName << "' in <" << rDataLocation << ">.";
            throw FactoryCreateFromJsonException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }

        for( int idx = 0; idx < interventions_array.Size(); ++idx )
        {
            std::stringstream param_name;
            param_name << parameterName << "[" << idx << "]";

            const json::Element& r_array_element = interventions_array[ idx ];
            if( r_array_element.Type() != json::OBJECT_ELEMENT )
            {
                std::stringstream ss;
                ss << "'" << GetFactoryName() << "' found the element specified by '" << param_name.str() << "'\n"
                   << "to NOT be a JSON OBJECT in <" << rDataLocation << ">.";
                throw FactoryCreateFromJsonException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }

            const json::Object& json_obj = json_cast<const json::Object&>(interventions_array[idx]);

            // Instantiate and distribute interventions
            INodeDistributableIntervention *di = InterventionFactory::getInstance()->CreateNDIIntervention( json_obj,
                                                                                                            rDataLocation,
                                                                                                            param_name.str().c_str(),
                                                                                                            true );
            interventionsList.push_back( di );
        }
    }

    void InterventionFactory::SetUseDefaults( bool useDefaults )
    {
        m_UseDefaults = useDefaults;
    }

    bool InterventionFactory::IsUsingDefaults() const
    {
        return m_UseDefaults;
    }

    void InterventionFactory::ModifySchema( json::QuickBuilder& rSchema, ISupports*pObject )
    {
        std::string type_string;
        IDistributableIntervention* p_intervention_individual = nullptr;
        INodeDistributableIntervention* p_intervention_node = nullptr;
        if( pObject->QueryInterface( GET_IID(IDistributableIntervention), (void**)&p_intervention_individual ) == s_OK )
        {
            type_string = "IndividualTargeted";
        }
        else if( pObject->QueryInterface( GET_IID(INodeDistributableIntervention), (void**)&p_intervention_node ) == s_OK )
        {
            type_string = "NodeTargeted";
        }
        else
        {
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                 "Intervention class is not 'IDistributableIntervention' or 'INodeDistributableIntervention'." );
        }
        rSchema[std::string("iv_type")] = json::String(type_string);
    }
}
