/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "TyphoidWASH.h"
#include "InterventionsContainer.h" 
#include "NodeEventContext.h"
#include "IdmDateTime.h"

SETUP_LOGGING( "TyphoidWASH" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(TyphoidWASH)
        HANDLE_INTERFACE(IConfigurable)
        HANDLE_INTERFACE(INodeDistributableIntervention)
        HANDLE_INTERFACE(IBaseIntervention)
        HANDLE_ISUPPORTS_VIA(INodeDistributableIntervention)
    END_QUERY_INTERFACE_BODY(TyphoidWASH)

    IMPLEMENT_FACTORY_REGISTERED(TyphoidWASH)

    bool
    TyphoidWASH::Configure(
        const Configuration * inputJson
    )
    {
        WaningConfig changing_config;

        initConfig( "Mode", vaccine_mode, inputJson, MetadataDescriptor::Enum("Mode", TW_Mode_DESC_TEXT, MDD_ENUM_ARGS(TyphoidVaccineMode)) );
        initConfigTypeMap("Effect", &effect, TW_Effect_DESC_TEXT, 0.0, 1.0, 1.0 ); 
        initConfigTypeMap("Targeted_Individual_Properties", &targeted_individual_properties, TW_TIP_DESC_TEXT, "default" );
        initConfigTypeMap("Use_Property_Targeting", &use_property_targeting, TW_UPT_DESC_TEXT, 1 );
        initConfigComplexType("Changing_Effect", &changing_config, TW_CE_DESC_TEXT );

        bool configured = BaseNodeIntervention::Configure( inputJson );

        // It's a misconfiguration if user says to use property targeting (explicitly or by omission) and omits a targeted ip value.
        if( !JsonConfigurable::_dryrun && use_property_targeting && targeted_individual_properties == "default" )
        {
            throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, "Use_Property_Targeting", "'True' (1)", "Targeted_Individual_Properties", "'default' (not specified)" );
        }

        // Zero out targeted_ip variable in case it was set but flag says to ignore.
        if( use_property_targeting == false )
        {
            targeted_individual_properties == "default";
        }

        if( !JsonConfigurable::_dryrun )
        {
            auto tmp_waning = Configuration::CopyFromElement( changing_config._json, inputJson->GetDataLocation() );
            // Would really like to find the right way to see if user omitted the Changing_Config instead of using exception handling.
            try {
                changing_effect = WaningEffectFactory::CreateInstance( tmp_waning );
            }
            catch( JsonTypeConfigurationException e )
            {
                LOG_INFO_F( "Looks like we're just going with fixed-value effect and not a variable-over-time effect structure.\n" );
            }
            delete tmp_waning;
            tmp_waning = nullptr;
        }
        LOG_DEBUG_F( "Vaccine configured with type %d and effect %f.\n", vaccine_mode, effect );
        return configured;
    }

    TyphoidWASH::TyphoidWASH() 
    : changing_effect( nullptr )
    , effect( 1.0f )
    , vaccine_mode( TyphoidVaccineMode::Shedding )
    , targeted_individual_properties( "default" )
    , use_property_targeting( true )
    {
        initSimTypes(1, "TYPHOID_SIM");
    }

    TyphoidWASH::TyphoidWASH( const TyphoidWASH& master )
    : changing_effect( nullptr )
    , effect( master.effect )
    , vaccine_mode( master.vaccine_mode )
    , use_property_targeting( master.use_property_targeting )
    {
        if( master.changing_effect != nullptr )
        {
            changing_effect = master.changing_effect->Clone();
        }
        else
        {
            LOG_INFO_F( "Looks like we're just going with fixed-value effect and not a variable-over-time effect structure.\n" );
        }
    }

    TyphoidWASH::~TyphoidWASH()
    {
        delete changing_effect;
        changing_effect = nullptr;
    }

    void 
    TyphoidWASH::getUpdatePointer()
    {
        // store itvc for apply
        release_assert( parent );
        if (s_OK != parent->QueryInterface(GET_IID(INodeTyphoidInterventionEffectsApply), (void**)&itvc) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context->GetInterventionsContext()", "INodeTyphoidInterventionEffectsApply", "INodeContext" );
        }
        changing_effect->SetCurrentTime( parent->GetTime().time );
        //LOG_DEBUG_F( "Vaccine configured with type %d and take %f for individual %d\n", vaccine_type, vaccine_take, parent->GetSuid().data );
    }

    bool
    TyphoidWASH::Distribute(
        INodeEventContext *context,
        IEventCoordinator2 * pCCO
    )
    {
        SetContextTo( context );
        getUpdatePointer();

        //pNodeContext->PurgeExisting( typeid(*this).name() ); // hmm?  let's come back to this and query the right interfaces everywhere.
        bool distribute = BaseNodeIntervention::Distribute( context, pCCO ); 
        changing_effect->SetCurrentTime( context->GetTime().time );
        return distribute;
    }

    void TyphoidWASH::Update( float dt )
    {
        release_assert(itvc);
        auto _effect = effect; // this is bad and confusing. Don't do this. Talking to myself here...
        if( changing_effect )
        {
            changing_effect->Update( dt );
            _effect = changing_effect->Current();
        }
        auto multiplier = 1.0f-_effect;
        LOG_VALID_F( "multiplier = %f, mode = %s.\n", multiplier, TyphoidVaccineMode::pairs::lookup_key( vaccine_mode ) );
        switch( vaccine_mode )
        {
            case TyphoidVaccineMode::Shedding:
            itvc->ApplyReducedSheddingEffect( multiplier, targeted_individual_properties );
            break;

            case TyphoidVaccineMode::Dose:
            itvc->ApplyReducedDoseEffect( multiplier, targeted_individual_properties );
            break;
        
            case TyphoidVaccineMode::Exposures:
            itvc->ApplyReducedNumberExposuresEffect( multiplier, targeted_individual_properties );
            break;

            default:
            throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__, "vaccine_mode", vaccine_mode, TyphoidVaccineMode::pairs::lookup_key( vaccine_mode ) );
        }
    }

    void TyphoidWASH::SetContextTo(
        INodeEventContext *context
    )
    {
        parent = context;
        getUpdatePointer();
    } // needed for VaccineTake

    //REGISTER_SERIALIZABLE(TyphoidWASH);

    /*void TyphoidWASH::serialize(IArchive& ar, TyphoidWASH* obj)
    {
        BaseIntervention::serialize( ar, obj );
        TyphoidWASH& vaccine = *obj;
        //ar.labelElement("acquire_effect")                 & vaccine.acquire_effect;
    }*/
}
