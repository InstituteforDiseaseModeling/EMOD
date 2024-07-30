
#include "stdafx.h"

#include "TargetingLogic.h"
#include "AdditionalRestrictionsFactory.h"

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(TargetingLogic)
        HANDLE_INTERFACE(IConfigurable)
        HANDLE_INTERFACE(IAdditionalRestrictions)
    END_QUERY_INTERFACE_BODY(TargetingLogic)

    IMPLEMENT_FACTORY_REGISTERED(TargetingLogic)
    REGISTER_SERIALIZABLE(TargetingLogic)

    TargetingLogic::TargetingLogic()
    {
        initSimTypes( 1, "*");
    }

    TargetingLogic::~TargetingLogic()
    {
        for( auto& r_outer : m_Restrictions )
        {
            for( auto p_ar : r_outer )
            {
                delete p_ar;
            }
        }
        m_Restrictions.clear();
    }

    bool TargetingLogic::Configure(const Configuration* config)
    {
        initConfigComplexType( "Logic", this, AR_Logic_DESC_TEXT );

        bool configured = JsonConfigurable::Configure(config);
        if( configured && !JsonConfigurable::_dryrun )
        {
            if( m_Restrictions.size() == 0 )
            {
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                     "'Logic' cannot be an empty array.  The user must define restrictions in this 2D array." );
            }
            for( auto& r_outer : m_Restrictions )
            {
                if( r_outer.size() == 0 )
                {
                    throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                         "'Logic' cannot have an empty inner array.  This is a 2D array and each inner array must not be empty." );
                }
            }
        }
        return configured;
    }
    
    void TargetingLogic::ConfigureFromJsonAndKey(const Configuration* inputJson, const std::string& key)
    {
        json::QuickInterpreter qi_2d( (*inputJson)[ key ] );
        if( qi_2d.operator const json::Element &().Type() != json::ARRAY_ELEMENT )
        {
            throw Kernel::JsonTypeConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                          key.c_str(), (*inputJson)[key], "Expected 2D ARRAY of OBJECTs of type 'AdditionalTargetingConfig'" );
        }

        const auto& json_array = json_cast<const json::Array&>((*inputJson)[key]);
        for (auto data = json_array.Begin(); data != json_array.End(); ++data)
        {
            if( data->Type() != json::ARRAY_ELEMENT )
            {
                throw Kernel::JsonTypeConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                              key.c_str(), (*inputJson)[key], "Expected 2D ARRAY of OBJECTs of type 'AdditionalTargetingConfig'" );

            }
            const auto& json_array_inner = json_cast<const json::Array&>((*data));

            vector<IAdditionalRestrictions*> innerm_Restrictions;
            for (auto data_inner = json_array_inner.Begin(); data_inner != json_array_inner.End(); ++data_inner)
            {
                if( data_inner->Type() != json::OBJECT_ELEMENT )
                {
                    throw Kernel::JsonTypeConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                                  key.c_str(), (*inputJson)[key], "Expected 2D ARRAY of OBJECTs of type 'AdditionalTargetingConfig'" );

                }

                Configuration* p_object_config = Configuration::CopyFromElement(*data_inner, inputJson->GetDataLocation());

                AdditionalTargetingConfig targeting_config(p_object_config);
                IAdditionalRestrictions* additionalm_Restrictions = AdditionalRestrictionsFactory::getInstance()->CreateInstance( targeting_config._json,
                                                                                                                                  inputJson->GetDataLocation(),
                                                                                                                                  key.c_str(),
                                                                                                                                  false );
                innerm_Restrictions.push_back(additionalm_Restrictions);

                delete p_object_config;
            }
            m_Restrictions.push_back(innerm_Restrictions);
        }
    }

    bool TargetingLogic::IsQualified(IIndividualHumanEventContext* pContext) const
    {
        bool result = false;

        for (auto& outer : m_Restrictions)
        {
            bool result_inner = true;
            for (auto inner : outer)
            {
                result_inner &= inner->IsQualified(pContext);
            }
            result |= result_inner;
        }

        return result;
    }

    void TargetingLogic::serialize(IArchive& ar, TargetingLogic* obj)
    {
        // TODO: implement me
        release_assert( false );
    }

    json::QuickBuilder TargetingLogic::GetSchema()
    {
        // TODO: do this right
        json::QuickBuilder schema(GetSchemaBase());
        auto tn = JsonConfigurable::_typename_label();
        auto ts = JsonConfigurable::_typeschema_label();

        schema[tn] = json::String("idmType:Vector2d idmType:AdditionalRestrictions");
        schema[ts] = json::String( "idmType:AdditionalRestrictions" );

        return schema;
    }

    bool TargetingLogic::HasValidDefault() const
    {
        return false;
    }
}