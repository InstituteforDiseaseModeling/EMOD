/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "UsageDependentBednet.h"
#include "InterventionFactory.h"
#include "Log.h"
#include "Contexts.h"
#include "IndividualEventContext.h"

SETUP_LOGGING( "UsageDependentBednet" )

namespace Kernel
{
    class WaningConfigList : public JsonConfigurable, public IComplexJsonConfigurable
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
    public:
        WaningConfigList();

        virtual QueryResult QueryInterface( iid_t iid, void **ppvObject ) { return e_NOINTERFACE; }
        json::QuickBuilder GetSchema() override;
        virtual void ConfigureFromJsonAndKey( const Configuration* inputJson, const std::string& key ) override;
        virtual bool HasValidDefault() const override { return true; }

        const std::vector<WaningConfig*>& GetList() const
        {
            return m_ConfigList;
        }

    protected:
        std::vector<WaningConfig*> m_ConfigList;
    };

    WaningConfigList::WaningConfigList()
        : JsonConfigurable()
        , m_ConfigList()
    {
    }

    json::QuickBuilder WaningConfigList::GetSchema()
    {
        json::QuickBuilder schema( GetSchemaBase() );
        auto tn = JsonConfigurable::_typename_label();
        auto ts = JsonConfigurable::_typeschema_label();
        schema[ tn ] = json::String( "idmType:WaningConfigList" );

        WaningConfig config;

        schema[ ts ] = json::Array();
        schema[ ts ][ 0 ] = config.GetSchema();

        return schema;
    }

    void WaningConfigList::ConfigureFromJsonAndKey( const Configuration* inputJson, const std::string& key )
    {
        json::QuickInterpreter qi( (*inputJson)[ key ] );
        try
        {
            json::Array config_array_json( qi.As<json::Array>() );
            for( int i = 0; i < config_array_json.Size(); i++ )
            {
                json::QuickInterpreter config_qi( config_array_json[ i ] );
                WaningConfig* p_config = new WaningConfig( &config_qi );
                m_ConfigList.push_back( p_config );
            }
        }
        catch( const json::Exception & )
        {
            throw Kernel::JsonTypeConfigurationException( __FILE__, __LINE__, __FUNCTION__, key.c_str(), qi, "Expected ARRAY" );
        }
    }


    // ------------------------------------------------------------------------
    // --- UsageDependentBednet
    // ------------------------------------------------------------------------
    BEGIN_QUERY_INTERFACE_DERIVED( UsageDependentBednet, AbstractBednet )
    END_QUERY_INTERFACE_DERIVED( UsageDependentBednet, AbstractBednet )

    IMPLEMENT_FACTORY_REGISTERED( UsageDependentBednet )

    UsageDependentBednet::UsageDependentBednet()
    : AbstractBednet()
    , m_UsageEffectList()
    , m_TriggerReceived()
    , m_TriggerUsing()
    , m_TriggerDiscard()
    , m_ExpirationDuration( DistributionFunction::FIXED_DURATION )
    , m_ExpirationTimer(0.0)
    , m_TimerHasExpired(false)
    {
        m_ExpirationTimer.handle = std::bind( &UsageDependentBednet::Callback, this, std::placeholders::_1 );
    }

    UsageDependentBednet::UsageDependentBednet( const UsageDependentBednet& master )
    : AbstractBednet( master )
    , m_UsageEffectList()
    , m_TriggerReceived( master.m_TriggerReceived )
    , m_TriggerUsing( master.m_TriggerUsing )
    , m_TriggerDiscard( master.m_TriggerDiscard )
    , m_ExpirationDuration( master.m_ExpirationDuration )
    , m_ExpirationTimer(master.m_ExpirationTimer)
    , m_TimerHasExpired(master.m_TimerHasExpired)
    {
        for( auto p_effect : master.m_UsageEffectList )
        {
            m_UsageEffectList.push_back( p_effect->Clone() );
        }
        m_ExpirationTimer.handle = std::bind( &UsageDependentBednet::Callback, this, std::placeholders::_1 );
    }

    UsageDependentBednet::~UsageDependentBednet()
    {
        for( auto p_effect : m_UsageEffectList )
        {
            delete p_effect;
        }
        m_UsageEffectList.clear();
    }

    bool UsageDependentBednet::ConfigureUsage( const Configuration * inputJson )
    {
        WaningConfigList usage_config_list;
        initConfigComplexType( "Usage_Config_List", &usage_config_list, UDBednet_Usage_Config_List_DESC_TEXT );

        bool configured = JsonConfigurable::Configure( inputJson ); // AbstractBednet is responsible for calling BaseIntervention::Configure()

        if( configured && !JsonConfigurable::_dryrun )
        {
            for( auto p_config : usage_config_list.GetList() )
            {
                IWaningEffect* p_effect = WaningEffectFactory::CreateInstance( *p_config );
                m_UsageEffectList.push_back( p_effect );
            }
        }

        return configured;
    }

    bool UsageDependentBednet::ConfigureEvents( const Configuration * inputJson )
    {
        m_ExpirationDuration.SetTypeNameDesc( "Expiration_Distribution_Type", UDBednet_Expiration_Distribution_Type_DESC_TEXT );
        m_ExpirationDuration.AddSupportedType( DistributionFunction::FIXED_DURATION,       "Expiration_Period",      UDBednet_Expiration_Period_DESC_TEXT,      "", "" );
        m_ExpirationDuration.AddSupportedType( DistributionFunction::UNIFORM_DURATION,     "Expiration_Period_Min",  UDBednet_Expiration_Period_Min_DESC_TEXT,  "Expiration_Period_Max",     UDBednet_Expiration_Period_Max_DESC_TEXT );
        m_ExpirationDuration.AddSupportedType( DistributionFunction::GAUSSIAN_DURATION,    "Expiration_Period_Mean", UDBednet_Expiration_Period_Mean_DESC_TEXT, "Expiration_Period_Std_Dev", UDBednet_Expiration_Period_Std_Dev_DESC_TEXT );
        m_ExpirationDuration.AddSupportedType( DistributionFunction::EXPONENTIAL_DURATION, "Expiration_Period",      UDBednet_Expiration_Period_DESC_TEXT, "", "" );

        m_ExpirationDuration .AddSupportedType( DistributionFunction::DUAL_TIMESCALE_DURATION,
                                               "Expiration_Period_1",            UDBednet_Expiration_Period_1_DESC_TEXT, 
                                               "Expiration_Period_2",            UDBednet_Expiration_Period_2_DESC_TEXT,
                                               "Expiration_Percentage_Period_1", UDBednet_Expiration_Percentage_Period_1_DESC_TEXT );

        m_ExpirationDuration.Configure( this, inputJson );

        initConfigTypeMap( "Received_Event", &m_TriggerReceived, UDBednet_Received_Event_DESC_TEXT );
        initConfigTypeMap( "Using_Event",    &m_TriggerUsing,    UDBednet_Using_Event_DESC_TEXT );
        initConfigTypeMap( "Discard_Event",  &m_TriggerDiscard,  UDBednet_Discard_Event_DESC_TEXT );

        bool configured = JsonConfigurable::Configure( inputJson ); // AbstractBednet is responsible for calling BaseIntervention::Configure()

        if( configured && !JsonConfigurable::_dryrun )
        {
            m_ExpirationDuration.CheckConfiguration();
        }
        return configured;
    }

    bool UsageDependentBednet::Distribute( IIndividualHumanInterventionsContext *context,
                                           ICampaignCostObserver * const pCCO )
    {
        bool distributed = AbstractBednet::Distribute( context, pCCO );
        if( distributed )
        {
            m_ExpirationTimer = m_ExpirationDuration.CalculateDuration();
            BroadcastEvent( m_TriggerReceived );
        }
        return distributed;
    }

    bool UsageDependentBednet::IsUsingBednet() const
    {
        bool is_using = false;

        float usage_effect = GetEffectUsage();

        if( usage_effect >= 1.0 )
        {
            is_using = true;
        }
        else if( usage_effect <= 0.0 )
        {
            is_using = false;
        }
        else
        {
            is_using = (usage_effect > parent->GetRng()->e());
        }

        if( is_using )
        {
            BroadcastEvent( m_TriggerUsing );
        }

        return is_using;
    }

    void UsageDependentBednet::UpdateUsage( float dt )
    {
        for( auto p_effect : m_UsageEffectList )
        {
            p_effect->Update( dt );
        }
    }

    float UsageDependentBednet::GetEffectUsage() const
    {
        float usage_effect = 1.0;
        for( auto p_effect : m_UsageEffectList )
        {
            usage_effect *= p_effect->Current();
        }
        return usage_effect;
    }

    bool UsageDependentBednet::CheckExpiration( float dt )
    {
        m_ExpirationTimer.Decrement( dt );

        return m_TimerHasExpired;
    }

    void UsageDependentBednet::Callback( float dt )
    {
        m_TimerHasExpired = true;
    }

    void UsageDependentBednet::SetExpired( bool isExpired )
    {
        AbstractBednet::SetExpired( isExpired );
        if( Expired() )
        {
            BroadcastEvent( m_TriggerDiscard );
        }
    }

    void UsageDependentBednet::SetContextTo( IIndividualHumanContext *context )
    {
        AbstractBednet::SetContextTo( context );
        for( auto p_effect : m_UsageEffectList )
        {
            p_effect->SetContextTo( context );
        }
    }


    REGISTER_SERIALIZABLE( UsageDependentBednet );

    void UsageDependentBednet::serialize( IArchive& ar, UsageDependentBednet* obj )
    {
        AbstractBednet::serialize( ar, obj );
        UsageDependentBednet& bednet = *obj;

        ar.labelElement( "m_UsageEffectList" ) & bednet.m_UsageEffectList;
        ar.labelElement( "m_TriggerReceived" ) & bednet.m_TriggerReceived;
        ar.labelElement( "m_TriggerUsing"    ) & bednet.m_TriggerUsing;
        ar.labelElement( "m_TriggerDiscard"  ) & bednet.m_TriggerDiscard;
        ar.labelElement( "m_ExpirationTimer" ) & bednet.m_ExpirationTimer;
        ar.labelElement( "m_TimerHasExpired" ) & bednet.m_TimerHasExpired;
    }
}
