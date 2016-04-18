/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

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
#include "Contexts.h"   // for randgen
#include "Relationship.h"
#include "RelationshipParameters.h"
#include "IIdGeneratorSTI.h"

#include "Log.h"
static const char * _module = "SocietyImpl";

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
        , extra_relational_rate_ratio_male(1.0f)
        , extra_relational_rate_ratio_female(1.0f)
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
    }

    bool SocietyImpl::Configure(const Configuration *config)
    {
        // -------------------------------
        // --- Parameters from config.json
        // -------------------------------
        initConfigTypeMap( "Extra_Relational_Rate_Ratio_Male",      &extra_relational_rate_ratio_male, PFA_Extra_Relational_Rate_Ratio_Male_DESC_TEXT, 1.0, FLT_MAX, 1.0f );
        initConfigTypeMap( "Extra_Relational_Rate_Ratio_Female",    &extra_relational_rate_ratio_female, PFA_Extra_Relational_Rate_Ratio_Female_DESC_TEXT, 1.0, FLT_MAX, 1.0f );
        initConfigTypeMap( "PFA_Cum_Prob_Selection_Threshold",      &pfa_selection_threshold, PFA_Cum_Prob_Selection_Threshold_DESC_TEXT, 0.0f, 1.0f, 0.2f );

        bool ret = JsonConfigurable::Configure( config );
        return ret ;
    }

    void SocietyImpl::SetParameters( IIdGeneratorSTI* pIdGen, const Configuration* config )
    {
        // --------------------------------
        // --- Parameters from Demographics
        // --------------------------------
        release_assert( config );

        for( int irel = 0; irel < RelationshipType::COUNT; irel++ )
        {
            std::string main_element_name = RelationshipType::pairs::lookup_key(irel);
            auto config_form = Configuration::CopyFromElement( (*config)[ main_element_name ][ "Pair_Formation_Parameters" ] );
            auto config_rel  = Configuration::CopyFromElement( (*config)[ main_element_name ][ "Relationship_Parameters"   ] );
 
            RelationshipType::Enum rel_type = (RelationshipType::Enum)irel ;

            RelationshipParameters* p_rel_params = new RelationshipParameters( rel_type );
            p_rel_params->Configure( config_rel );

            RelationshipCreator rc = [this,pIdGen,p_rel_params](IIndividualHumanSTI*male,IIndividualHumanSTI*female) 
            { 
                suids::suid rel_id = pIdGen->GetNextRelationshipSuid();
                IRelationship* p_rel = RelationshipFactory::CreateRelationship( rel_id, relationship_manager, p_rel_params, male, female );
                relationship_manager->AddRelationship( p_rel, true );
            }; 

            rel_params[  irel ] = p_rel_params ;
            form_params[ irel ] = PairFormationParamsFactory::Create( rel_type, config_form, extra_relational_rate_ratio_male, extra_relational_rate_ratio_female );
            pfa[         irel ] = PfaFactory::CreatePfa( config_form, form_params[ irel ], pfa_selection_threshold, randgen, rc );
            rates[       irel ] = RateTableFactory::CreateRateTable( form_params[ irel ] );
            stats[       irel ] = PairFormationStatsFactory::CreateStatistician( form_params[ irel ] );
            controller[  irel ] = FlowControllerFactory::CreateController( pfa[ irel ], stats[ irel ], rates[ irel ], form_params[ irel ] );

            delete config_form;
            delete config_rel;
            config_form = nullptr;
            config_rel  = nullptr;
        }
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
}
