/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "SocietyImpl.h"
#include "PairFormationParamsFactory.h"
#include "PfaFactory.h"
#include "RateTableFactory.h"
#include "PairFormationStatsFactory.h"
#include "FlowControllerFactory.h"
#include "Relationship.h"
#include "RelationshipParameters.h"
#include "IIdGeneratorSTI.h"
#include "ConcurrencyParameters.h"

#include "Log.h"

SETUP_LOGGING( "SocietyImpl" )

namespace Kernel {

    ISociety* SocietyImpl::Create( IRelationshipManager* manager )
    {
        return _new_ SocietyImpl(manager);
    }

    GET_SCHEMA_STATIC_WRAPPER_IMPL(SocietyImpl,SocietyImpl)

    BEGIN_QUERY_INTERFACE_BODY(SocietyImpl)
        HANDLE_INTERFACE(ISociety)
    END_QUERY_INTERFACE_BODY(SocietyImpl)

    SocietyImpl::SocietyImpl( IRelationshipManager* manager )
        : relationship_manager(manager)
        , p_concurrency( new ConcurrencyConfiguration() )
        , pfa_selection_threshold(0.2f)
    {
        for( int irel = 0; irel < RelationshipType::COUNT; irel++ )
        {
            rel_params[    irel ] = nullptr ;
            form_params[   irel ] = nullptr ;
            rates[         irel ] = nullptr ;
            stats[         irel ] = nullptr ;
            pfa[           irel ] = nullptr ;
            controller[    irel ] = nullptr ;
        }
    }

    SocietyImpl::~SocietyImpl()
    {
        // relationship_manager - don't delete because we don't own

        for( int irel = 0; irel < RelationshipType::COUNT; irel++ )
        {
            delete controller[ irel ] ;
            controller[ irel ] = nullptr ;

            delete pfa[ irel ] ;
            pfa[ irel ] = nullptr ;

            delete rates[ irel ] ;
            rates[ irel ] = nullptr ;

            delete stats[ irel ] ;
            stats[ irel ] = nullptr ;

            delete form_params[ irel ] ;
            form_params[ irel ] = nullptr ;

            delete rel_params[ irel ] ;
            rel_params[ irel ] = nullptr ;
        }

        delete p_concurrency;
        p_concurrency = nullptr;
    }

    bool SocietyImpl::Configure(const Configuration *config)
    {
        // -------------------------------
        // --- Parameters from config.json
        // -------------------------------
        initConfigTypeMap( "PFA_Cum_Prob_Selection_Threshold", &pfa_selection_threshold, PFA_Cum_Prob_Selection_Threshold_DESC_TEXT, 0.0f, 1.0f, 0.2f );

        bool ret = JsonConfigurable::Configure( config );
        return ret ;
    }

    void SocietyImpl::SetParameters( RANDOMBASE* pRNG, IIdGeneratorSTI* pIdGen, const Configuration* config )
    {
        bool prev_use_defaults = JsonConfigurable::_useDefaults ;
        bool resetTrackMissing = JsonConfigurable::_track_missing;
        JsonConfigurable::_track_missing = false;
        JsonConfigurable::_useDefaults = false ;

        // --------------------------------
        // --- Parameters from Demographics
        // --------------------------------
        release_assert( config );

        Configuration* concurrency_config = Configuration::CopyFromElement( (*config)[ "Concurrency_Configuration" ], config->GetDataLocation() );

        p_concurrency->Initialize( concurrency_config );

        delete concurrency_config;
        concurrency_config = nullptr;

        const std::string& r_con_prop_key = p_concurrency->GetPropertyKey();

        for( int irel = 0; irel < RelationshipType::COUNT; irel++ )
        {
            std::string main_element_name = RelationshipType::pairs::lookup_key(irel);
            Configuration* config_con  = Configuration::CopyFromElement( (*config)[ main_element_name ][ "Concurrency_Parameters"    ], config->GetDataLocation() );
            Configuration* config_form = Configuration::CopyFromElement( (*config)[ main_element_name ][ "Pair_Formation_Parameters" ], config->GetDataLocation() );
            Configuration* config_rel  = Configuration::CopyFromElement( (*config)[ main_element_name ][ "Relationship_Parameters"   ], config->GetDataLocation() );
 
            RelationshipType::Enum rel_type = (RelationshipType::Enum)irel ;

            ConcurrencyParameters* p_cp = new ConcurrencyParameters();
            p_cp->Initialize( main_element_name, r_con_prop_key, config_con );
            p_concurrency->AddParameters( rel_type, p_cp );

            RelationshipParameters* p_rel_params = new RelationshipParameters( rel_type );
            p_rel_params->Configure( config_rel );

            RelationshipCreator rc = [this,pRNG,pIdGen,p_rel_params](IIndividualHumanSTI*male,IIndividualHumanSTI*female) 
            { 
                suids::suid rel_id = pIdGen->GetNextRelationshipSuid();
                IRelationship* p_rel = RelationshipFactory::CreateRelationship( pRNG, rel_id, relationship_manager, p_rel_params, male, female );
                relationship_manager->AddRelationship( p_rel, true );
            }; 

            rel_params[  irel ] = p_rel_params ;
            form_params[ irel ] = PairFormationParamsFactory::Create( rel_type, config_form );
            pfa[         irel ] = PfaFactory::CreatePfa( config_form, form_params[ irel ], pfa_selection_threshold, pRNG, rc );
            rates[       irel ] = RateTableFactory::CreateRateTable( form_params[ irel ] );
            stats[       irel ] = PairFormationStatsFactory::CreateStatistician( form_params[ irel ] );
            controller[  irel ] = FlowControllerFactory::CreateController( pfa[ irel ], stats[ irel ], rates[ irel ], form_params[ irel ] );

            delete config_con;
            delete config_form;
            delete config_rel;
            config_con  = nullptr;
            config_form = nullptr;
            config_rel  = nullptr;
        }
        JsonConfigurable::_useDefaults = prev_use_defaults ;
        JsonConfigurable::_track_missing = resetTrackMissing;
    }

    IRelationshipParameters* SocietyImpl::GetRelationshipParameters( RelationshipType::Enum type )
    {
        for( int irel = 0; irel < RelationshipType::COUNT; irel++ )
        {
            if( rel_params[ irel ]->GetType() == type )
            {
                return rel_params[ irel ];
            }
        }
        std::ostringstream msg;
        msg << "Unknown RelationshipType = " << type << " / " << RelationshipType::pairs::lookup_key( type );
        throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
    }

    void SocietyImpl::BeginUpdate()
    {
        for( int irel = 0; irel < RelationshipType::COUNT; irel++ )
        {
            stats[ irel ]->ResetEligible();
        }
    }

    void
    SocietyImpl::UpdatePairFormationRates( const IdmDateTime& rCurrentTime, float dt )
    {
        LOG_INFO_F( "%s: --------------------========== START society->UpdatePairFormationRates() ==========--------------------\n", __FUNCTION__ );
        LOG_DEBUG_F("%s()\n", __FUNCTION__);
        for( int irel = 0; irel < RelationshipType::COUNT; irel++ )
        {
            controller[ irel ]->UpdateEntryRates( rCurrentTime, dt );
        }
        LOG_INFO_F( "%s: --------------------========== society->UpdatePairFormationRates() FINISH ==========--------------------\n", __FUNCTION__ );
    }

    void
    SocietyImpl::UpdatePairFormationAgents( const IdmDateTime& rCurrentTime, float dt )
    {
        LOG_INFO_F( "%s: --------------------========== START society->UpdatePairFormationAgents() ==========--------------------\n", __FUNCTION__ );
        LOG_DEBUG_F("%s()\n", __FUNCTION__);
        for( int irel = 0; irel < RelationshipType::COUNT; irel++ )
        {
            pfa[ irel ]->Update( rCurrentTime, dt );
        }
        LOG_INFO_F( "%s: --------------------========== society->UpdatePairFormationAgents() FINISH ==========--------------------\n", __FUNCTION__ );


        if (LOG_LEVEL(INFO)) 
        {
            for( int irel = 0; irel < RelationshipType::COUNT; irel++ )
            {
                pfa[ irel ]->Print( RelationshipType::pairs::lookup_key(irel) );
            }
        }
    }

    const IPairFormationRateTable* SocietyImpl::GetRates(RelationshipType::Enum type)
    {
        return rates[ type ] ;
    }

    IPairFormationAgent* SocietyImpl::GetPFA(RelationshipType::Enum type)
    { 
        return pfa[ type ] ;
    }

    IPairFormationStats* SocietyImpl::GetStats(RelationshipType::Enum type)
    { 
        return stats[ type ] ;
    }

    IConcurrency* SocietyImpl::GetConcurrency()
    {
        return p_concurrency;
    }
}
