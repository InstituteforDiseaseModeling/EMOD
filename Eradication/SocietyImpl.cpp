/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

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
            parameters[    irel ] = nullptr ;
            rates[         irel ] = nullptr ;
            stats[         irel ] = nullptr ;
            pfa[           irel ] = nullptr ;
            controller[    irel ] = nullptr ;
            base_rate[     irel ] = 0.0f;
            update_period[ irel ] = 0.0f;
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

            delete parameters[ irel ] ;
            parameters[ irel ] = nullptr ;
        }
    }

    bool SocietyImpl::Configure(const Configuration *config)
    {
        // -------------------------------
        // --- Parameters from config.json
        // -------------------------------
        initConfigTypeMap( "Transitory_Formation_Rate_Heterosexual", &base_rate[RelationshipType::TRANSITORY], PFA_Transitory_Form_Rate_Hetero_DESC_TEXT, 0, 1, 0.0013699f  );
        initConfigTypeMap( "Informal_Formation_Rate_Heterosexual",   &base_rate[RelationshipType::INFORMAL  ], PFA_Informal_Form_Rate_Hetero_DESC_TEXT  , 0, 1, 0.001442f   );
        initConfigTypeMap( "Marriage_Formation_Rate_Heterosexual",   &base_rate[RelationshipType::MARITAL   ], PFA_Marriage_Form_Rate_Hetero_DESC_TEXT  , 0, 1, 9.1324e-05f );

        initConfigTypeMap( "PFA_Update_Period_Transitory",          &update_period[RelationshipType::TRANSITORY], PFA_Update_Period_Transitory_DESC_TEXT,      0, FLT_MAX, 0 );
        initConfigTypeMap( "PFA_Update_Period_Informal",            &update_period[RelationshipType::INFORMAL  ], PFA_Update_Period_Informal_DESC_TEXT  ,        0, FLT_MAX, 0 );
        initConfigTypeMap( "PFA_Update_Period_Marital",             &update_period[RelationshipType::MARITAL   ], PFA_Update_Period_Marital_DESC_TEXT   ,         0, FLT_MAX, 0 );

        initConfigTypeMap( "Extra_Relational_Rate_Ratio_Male",      &extra_relational_rate_ratio_male, PFA_Extra_Relational_Rate_Ratio_Male_DESC_TEXT, 1.0, FLT_MAX, 1.0f );
        initConfigTypeMap( "Extra_Relational_Rate_Ratio_Female",    &extra_relational_rate_ratio_female, PFA_Extra_Relational_Rate_Ratio_Female_DESC_TEXT, 1.0, FLT_MAX, 1.0f );

        initConfigTypeMap( "PFA_Cum_Prob_Selection_Threshold",      &pfa_selection_threshold, PFA_Cum_Prob_Selection_Threshold_DESC_TEXT, 0.0f, 1.0f, 0.2f );


        bool ret = JsonConfigurable::Configure( config );
        return ret ;
    }

    void SocietyImpl::SetParameters( const Configuration* config )
    {
        // --------------------------------
        // --- Parameters from Demographics
        // --------------------------------
        release_assert( config );

        for( int irel = 0; irel < RelationshipType::COUNT; irel++ )
        {
            std::string element_name = std::string("Pair_Formation_Parameters_") + RelationshipType::pairs::lookup_key(irel) ;
            auto config_param = Configuration::CopyFromElement( (*config)[ element_name ] );
 
            RelationshipType::Enum rel_type = (RelationshipType::Enum)irel ;
            RelationshipCreator rc = [this,rel_type](IIndividualHumanSTI*male,IIndividualHumanSTI*female) 
            { 
                relationship_manager->AddRelationship( RelationshipFactory::CreateRelationship( rel_type, male, female ) );
            }; 

            parameters[ irel ] = PairFormationParamsFactory::Create( rel_type, config_param, base_rate[ irel ], extra_relational_rate_ratio_male, extra_relational_rate_ratio_female );
            pfa[        irel ] = PfaFactory::CreatePfa( config_param, parameters[ irel ], update_period[ irel ], pfa_selection_threshold, randgen, rc );
            rates[      irel ] = RateTableFactory::CreateRateTable( parameters[ irel ] );
            stats[      irel ] = PairFormationStatsFactory::CreateStatistician( parameters[ irel ] );
            controller[ irel ] = FlowControllerFactory::CreateController( pfa[ irel ], stats[ irel ], rates[ irel ], parameters[ irel ] );
        }
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
            controller[ irel ]->UpdateEntryRates();
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
