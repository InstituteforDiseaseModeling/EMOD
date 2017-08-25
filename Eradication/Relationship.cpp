/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "Relationship.h"
#include "Debug.h"
#include "Environment.h"
#include "Types.h"
#include "SimulationConfig.h"
#include "MathFunctions.h"
#include "RANDOM.h"
#include "INodeContext.h"
#include "STIInterventionsContainer.h" // for ISTIBarrierConsumer (which should be in ISTIBarrierConsumer.h) TODO
#include "IndividualSTI.h"
#include "ISociety.h"
#include "IdmString.h"

#include <map>
#include <ctime>
#include <algorithm>

SETUP_LOGGING( "Relationship" )

#define PROPERTY_KEY_PREFIX "Relationship."
#define PROPERTY_KEY_PREFIX_LENGTH (13)
#define SLOT_SEPARATOR ('-')

void 
howlong(
    clock_t before,
    const char * label = "NA"
)
{
    clock_t after = clock();
    clock_t diff = after - before;
    float thediff = diff/(float)CLOCKS_PER_SEC;

    //std::cout << label << " took " << thediff << " seconds." << std::endl;
}

namespace Kernel {

    // static
    bool alreadyInRelationship(
        IIndividualHumanSTI * pGuy,
        IIndividualHumanSTI * pGal
    )
    {
        auto guyRels = pGuy->GetRelationships();
        for( const auto relationship : guyRels)
        {
            auto partner = relationship->GetPartner( pGuy );
            if( partner && (partner->GetSuid().data == pGal->GetSuid().data) )
            {
                return true;
            }
        }
        return false;
    }

#define MALE_PARTNER_ID()       ( (absent_male_partner_id   == suids::nil_suid()) ? male_partner->GetSuid()   : absent_male_partner_id   )
#define FEMALE_PARTNER_ID()     ( (absent_female_partner_id == suids::nil_suid()) ? female_partner->GetSuid() : absent_female_partner_id )
#define PARTNERID( individual ) ( GetPartner( individual ) ? GetPartner( individual )->GetSuid() : suids::nil_suid()        )

    BEGIN_QUERY_INTERFACE_BODY(Relationship)
        HANDLE_INTERFACE(IRelationship)
        HANDLE_ISUPPORTS_VIA(IRelationship)
    END_QUERY_INTERFACE_BODY(Relationship)


    Relationship::Relationship()
        : IRelationship()
        , _suid(suids::nil_suid())
        , state( RelationshipState::NORMAL )
        , previous_state( RelationshipState::NORMAL )
        , relationship_type( RelationshipType::TRANSITORY )
        , termination_reason( RelationshipTerminationReason::NOT_TERMINATING )
        , p_rel_params( nullptr )
        , male_partner(nullptr)
        , female_partner(nullptr)
        , absent_male_partner_id(suids::nil_suid())
        , absent_female_partner_id(suids::nil_suid())
        , rel_timer(0)
        , rel_duration(0)
        , start_time(0)
        , scheduled_end_time(0)
        , propertyKey()
        , propertyName()
        , original_node_id(0)
        , act_prob_vec()
        , using_condom(false)
        , relMan(nullptr)
        , total_coital_acts(0)
        , has_migrated(false)
    {
    }

    Relationship::Relationship( const suids::suid& rRelId,
                                IRelationshipManager* pRelMan,
                                IRelationshipParameters* pParams,
                                IIndividualHumanSTI * male_partnerIn, 
                                IIndividualHumanSTI * female_partnerIn )
        : IRelationship()
        , _suid(rRelId)
        , state( RelationshipState::NORMAL )
        , previous_state( RelationshipState::NORMAL )
        , relationship_type( pParams->GetType() )
        , termination_reason( RelationshipTerminationReason::NOT_TERMINATING )
        , p_rel_params( pParams )
        , male_partner(male_partnerIn)
        , female_partner(female_partnerIn)
        , absent_male_partner_id(suids::nil_suid())
        , absent_female_partner_id(suids::nil_suid())
        , rel_timer(0)
        , rel_duration(0)
        , start_time(0)
        , scheduled_end_time(0)
        , propertyKey()
        , propertyName()
        , original_node_id(0)
        , act_prob_vec()
        , using_condom(false)
        , relMan(pRelMan)
        , total_coital_acts(0)
        , has_migrated(false)
    {
        release_assert( male_partner   );
        release_assert( female_partner );

        LOG_DEBUG_F( "%s: Creating relationship %d between %d and %d\n", __FUNCTION__, _suid.data, MALE_PARTNER_ID().data, FEMALE_PARTNER_ID().data );
        //LogRelationship( _id, MALE_PARTNER_ID(), FEMALE_PARTNER_ID(), p_rel_params->GetType() );

        rel_timer = DAYSPERYEAR * Environment::getInstance()->RNG->Weibull2( pParams->GetDurationWeibullScale(),
                                                                             pParams->GetDurationWeibullHeterogeneity() );

        IIndividualHuman* individual = nullptr;
        if( male_partnerIn->QueryInterface(GET_IID(IIndividualHuman), (void**)&individual) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "male_partnerIn", "IIndividualHuman", "IIndividualHumanSTI" );
        }

        start_time = float(individual->GetParent()->GetTime().time);
        scheduled_end_time = start_time + rel_timer;

        original_node_id = individual->GetParent()->GetExternalID();

        clock_t init = clock();
        // TBD: Get dynamic prop name from SetSpouse, for now duplicate.
        std::ostringstream dynamicPropertyName;
        dynamicPropertyName << _suid.data;
        propertyName = dynamicPropertyName.str();

        LOG_DEBUG_F( "%s: adding property [%s]:%s\n", __FUNCTION__, "Relationship", propertyName.c_str() );

        male_partner->AddRelationship( this );
        female_partner->AddRelationship( this );

        howlong( init, "R::Init" );
    }

    Relationship::~Relationship()
    {
        LOG_INFO_F( "(EEL) relationship %d between %d and %d just ended.\n", GetSuid().data, MALE_PARTNER_ID().data, FEMALE_PARTNER_ID().data );
        // ----------------------------------------------------------------------------------
        // --- 5-20-2015 DMB I commented this out because I'm also not null'ing the pointers
        // --- to the partners.  If the partner is being deleted and we try to access their
        // --- id, bad things happen.
        // ----------------------------------------------------------------------------------
        //LogRelationship( _id | 0x80000000, MALE_PARTNER_ID(), FEMALE_PARTNER_ID(), p_rel_params->GetType() );

        // ------------------------------------------------------
        // --- Do not own p_rel_params or relMan so don't delete
        // ------------------------------------------------------
    }

    RelationshipState::Enum Relationship::GetState() const
    {
        return state;
    }

    RelationshipState::Enum Relationship::GetPreviousState() const
    {
        return previous_state;
    }

    RelationshipMigrationAction::Enum Relationship::GetMigrationAction( RANDOMBASE* prng ) const
    {
        if( state == RelationshipState::PAUSED )
        {
            return RelationshipMigrationAction::PAUSE;
        }
        else if( state == RelationshipState::MIGRATING )
        {
            return RelationshipMigrationAction::MIGRATE;
        }
        else if( state == RelationshipState::TERMINATED )
        {
            std::ostringstream msg;
            msg << "Should not be trying to get a migration action when the relationship has been terminated. rel_id=" << GetSuid().data ;
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }
        else
        {
            release_assert( state == RelationshipState::NORMAL );

            const std::vector<RelationshipMigrationAction::Enum>& r_actions = p_rel_params->GetMigrationActions();

            if( r_actions.size() == 1 )
            {
                return r_actions[0];
            }
            else
            {
                const std::vector<float>& r_cdf = p_rel_params->GetMigrationActionsCDF();
                release_assert( r_actions.size() == r_cdf.size() );

                float ran = prng->e();
                for( int i = 0 ; i < r_cdf.size() ; i++)
                {
                    if( r_cdf[i] >= ran )
                    {
                        return r_actions[i];
                    }
                }
                std::ostringstream msg;
                msg << "Should have selected an action.  Is the last value not 1.0? ran=" << ran << " last_value=" << r_cdf[ r_cdf.size()-1 ] ;
                throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
            }
        }
    }

    RelationshipTerminationReason::Enum Relationship::GetTerminationReason() const
    {
        return termination_reason;
    }

    void Relationship::SetManager( IRelationshipManager* pRelManager, ISociety* pSociety )
    {
        if( relMan != pRelManager )
        {
            has_migrated = true;
        }
        relMan = pRelManager;

        if( pSociety == nullptr )
        {
            p_rel_params = nullptr ;
        }
        else
        {
            p_rel_params = pSociety->GetRelationshipParameters( relationship_type );
        }
    }

    bool
    Relationship::Update( float dt )
    {
        LOG_VALID_F( "%s: man = %d, woman = %d, timer getting decremented from %f to %f\n", __FUNCTION__, MALE_PARTNER_ID().data, FEMALE_PARTNER_ID().data, rel_timer, rel_timer - 1 );

        rel_duration += dt;
        rel_timer -= dt;
        if( rel_timer <= 0 )
        {
            LOG_DEBUG_F( "%s: relationship %d between %d and %d is over, timer = %f...\n", __FUNCTION__, GetSuid().data, MALE_PARTNER_ID().data, FEMALE_PARTNER_ID().data, rel_timer );
            return false;
        }
        return true;
    }

    void Relationship::Consummate( float dt )
    {
        if( state != RelationshipState::NORMAL )
        {
            return;
        }

        // DJK: If we don't care about logging individual coital acts, could check IsDiscordant here
        unsigned int nRels = 1;
        float coital_rate_attenuation_factor = 1;
        if( IndividualHumanSTIConfig::enable_coital_dilution )
        {
            nRels = std::max<size_t>( male_partner->GetRelationships().size(), female_partner->GetRelationships().size() );

            if( nRels == 2)
                coital_rate_attenuation_factor = IndividualHumanSTIConfig::coital_dilution_2_partners;
            else if( nRels == 3)
                coital_rate_attenuation_factor = IndividualHumanSTIConfig::coital_dilution_3_partners;
            else if( nRels >= 4)
                coital_rate_attenuation_factor = IndividualHumanSTIConfig::coital_dilution_4_plus_partners;

            if( nRels > 1 )
            {
                LOG_DEBUG_F( "Coital dilution in effect: nRels is %d so attenuating coital rate by %f\n", nRels, coital_rate_attenuation_factor );
            }
        }
        release_assert( coital_rate_attenuation_factor > 0 );

        /*LOG_INFO_F( "(EEL) Coital Act: rel = %d, insertive = %d, relationships = %d, receptive = %d, relationships = %d, time till next act = %f.\n",
                    GetSuid().data,
                    male_partner->GetSuid().data,
                    male_partner->GetRelationships().size(),
                    female_partner->GetSuid().data,
                    female_partner->GetRelationships().size(),
                    next_coital_act_timer
                  );*/

        double ratetime = dt * coital_rate_attenuation_factor * GetCoitalRate();
        unsigned int acts_this_dt = Environment::getInstance()->RNG->Poisson( ratetime );

        if( total_coital_acts == 0 && acts_this_dt == 0 && ratetime > 0)
        {
            acts_this_dt = 1;   // Force the first act
        }

        total_coital_acts += acts_this_dt;

        if( acts_this_dt > 0 )
        {
            male_partner->UpdateNumCoitalActs( acts_this_dt );
            female_partner->UpdateNumCoitalActs( acts_this_dt );

            ProbabilityNumber p_condom = (float) getProbabilityUsingCondomThisAct();
            NaturalNumber acts_using_condom_this_dt = Environment::getInstance()->RNG->binomial_approx( acts_this_dt, (float) p_condom );
            NaturalNumber acts_not_using_condom_this_dt = acts_this_dt - acts_using_condom_this_dt;
            LOG_DEBUG_F( "p_condom = %f, acts_using_condom_this_dt = %d\n", (float) p_condom, (int) acts_using_condom_this_dt );

            release_assert( relMan );

            using_condom = true;    // For coital act reporting
            relMan->ConsummateRelationship( this, acts_using_condom_this_dt );

            using_condom = false;   // For coital act reporting
            relMan->ConsummateRelationship( this, acts_not_using_condom_this_dt );

            // Only do work if couple is discordant
            if( !IsDiscordant() )
            {
                LOG_DEBUG_F( "Concordant relationship: skipped UpdateInfectiousness calls.\n" );
                return;
            }

            float condomTransmissionBlockingProbability = IndividualHumanSTIConfig::condom_transmission_blocking_probability;

            IIndividualHumanSTI *uninfected_individual = male_partner->IsInfected() ? female_partner : male_partner;
            IIndividualHumanSTI *infected_individual = male_partner->IsInfected() ? male_partner : female_partner;

            // STI risk factor if one partner has sti, avoid double risk
            float coInfectiveFactor1 = infected_individual->GetCoInfectiveTransmissionFactor();
            float coInfectiveFactor2 = uninfected_individual->GetCoInfectiveAcquisitionFactor();
            float sti_mult = max( coInfectiveFactor1, coInfectiveFactor2 );

            act_prob_vec.clear();
            act_prob_vec.resize(2);

            act_prob_vec[0].num_acts = acts_not_using_condom_this_dt;
            act_prob_vec[0].prob_per_act = sti_mult;

            act_prob_vec[1].num_acts = acts_using_condom_this_dt;
            act_prob_vec[1].prob_per_act = sti_mult * (1-condomTransmissionBlockingProbability);
            //LOG_DEBUG_F( "Setting prob_per_act to %f for %d 'unprotected' acts and prob_per_act to %f for %d 'protected' acts.\n", sti_mult, (int) acts_not_using_condom_this_dt, act_prob_vec[1].prob_per_act, (int) act_prob_vec[1].num_acts );

            uninfected_individual->NotifyPotentialExposure();
            infected_individual->UpdateInfectiousnessSTI( act_prob_vec, GetSuid().data );
        }
    }

    IIndividualHumanSTI* Relationship::MalePartner() const
    {
        return male_partner;
    }

    IIndividualHumanSTI* Relationship::FemalePartner()    const
    {
        return female_partner;
    }

    IIndividualHumanSTI*
    Relationship::GetPartner(
        IIndividualHumanSTI* individual
    )
    const
    {
        if ( individual == nullptr )
        {
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "GetPartner called with null pointer.\n" );
        }

        IIndividualHumanSTI* partner = nullptr;
        if (individual == male_partner)
        {
            partner = female_partner;
        }
        else if (individual == female_partner)
        {
            partner = male_partner;
        }
        else
        {
            std::ostringstream msg;
            msg << "Individual "<< individual->GetSuid().data << " is not in relationship " << GetSuid().data << "!\n";
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }

        // ----------------------------------------------------------------------------
        // --- We allow the return to be null.  If the relationship is paused and the
        // --- individual moved nodes, then the pointer could be null.
        // ----------------------------------------------------------------------------

        return partner;
    }

    const suids::suid& Relationship::GetSuid() const
    {
        return _suid;
    }

    const std::string&
    Relationship::GetPropertyName() const
    {
        return propertyName;
    }

    const std::string&
    Relationship::GetPropertyKey()
    {
        if( propertyKey == "" )
        {
            propertyKey = PROPERTY_KEY_PREFIX;
            propertyKey += std::to_string( male_partner->GetOpenRelationshipSlot() );
            propertyKey += SLOT_SEPARATOR;
            propertyKey += std::to_string( female_partner->GetOpenRelationshipSlot() ); // does this work? TBD
        }
        return propertyKey;
    }

    unsigned int Relationship::GetSlotNumberForPartner( bool forPartnerB ) const
    {
        // Note: this solution assumes max of MAX_SLOT relationships
        unsigned int slot_index = 0;
        if( forPartnerB ) // B=Female
        {
            slot_index = 1;
        }
        IdmString key( propertyKey.substr( PROPERTY_KEY_PREFIX_LENGTH ) );
        auto slot_strs = key.split( SLOT_SEPARATOR );

        auto slot_number = atoi( slot_strs[ slot_index ].c_str() );
        release_assert( slot_number <= MAX_SLOTS );
        return slot_number;
    }

    const tRelationshipMembers
    Relationship::GetMembers() const
    {
        tRelationshipMembers returnSet;
        returnSet.insert( male_partner );
        returnSet.insert( female_partner );
        return returnSet;
    }

    void Relationship::Pause( IIndividualHumanSTI* departee )
    {
        LOG_INFO_F( "%s: individual %d vacating relationship %d leaving individual %d alone.\n", __FUNCTION__, departee->GetSuid().data, GetSuid().data, PARTNERID( departee ).data );

        previous_state = state;
        state = RelationshipState::PAUSED;

        if( relMan != nullptr )
        {
            relMan->RemoveRelationship( this, (previous_state == RelationshipState::PAUSED) );
        }

        // -------------------------------------------------------------------------------
        // --- When Pausing, we need to create two instances of the relationship: 
        // --- one for the current node and one for the node the person is going to.
        // --- One reason for this is that Relationship::Update() is called by RelationshipManager::Update()
        // --- so that it is called once for the relationship.  Since we need two instances
        // --- for multi-core, it makes sense to use this solution for single core as well.
        // -------------------------------------------------------------------------------
        Relationship* p_staying_rel = this;
        Relationship* p_leaving_rel = nullptr;
        if( previous_state == RelationshipState::NORMAL )
        {
            p_leaving_rel = this->Clone();
            departee->GetRelationships().erase( p_staying_rel );
            departee->GetRelationships().insert( p_leaving_rel );
        }

        // -------------------------------------------------------------------------
        // --- When Pausing a relationship, each partner will have their own instance
        // --- of the relationship object.  This implies that we need these objects setup
        // --- so that the relationship object for Partner A (male) has himself still in
        // --- the relationship and his partner absent.  Partner B needs the opposite.
        // ---
        // --- We don't set both partners to absent since we want the actions of one
        // --- partner to reset the relationship.  For example, if one partner pauses
        // --- relationship due to migration, and then returns.  That individual's call
        // --- to Resume() should get the relationship back to normal.
        // ---
        // --- Not setting absent partner to null so that the pointer can be used in the Transmission report.
        // --- The couple could have consumated and then one of them decided to pause the relationship.
        // --- The report needs the pointer.
        // -------------------------------------------------------------------------
        if( p_staying_rel->male_partner == departee )
        {
            LOG_DEBUG_F( "%s: departee was male_partner, clearing male_partner from relationship %d\n", __FUNCTION__, p_staying_rel->GetSuid().data );

            p_staying_rel->absent_male_partner_id = departee->GetSuid();

            if( p_leaving_rel != nullptr )
            {
                release_assert( p_staying_rel->female_partner );
                p_leaving_rel->absent_female_partner_id = p_staying_rel->female_partner->GetSuid();
            }
        }
        else if( p_staying_rel->female_partner == departee )
        {
            LOG_DEBUG_F( "%s: departee was female_partner, clearing female_partner from relationship %d\n", __FUNCTION__, GetSuid().data );

            p_staying_rel->absent_female_partner_id = departee->GetSuid();

            if( p_leaving_rel != nullptr )
            {
                release_assert( p_staying_rel->male_partner );
                p_leaving_rel->absent_male_partner_id = p_staying_rel->male_partner->GetSuid();
            }
        }
        else
        {
            std::ostringstream msg;
            msg << "Individual "<< departee->GetSuid().data << " is not in relationship " << GetSuid().data << "!\n";
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }
    }

    void Relationship::UpdatePaused()
    {
        // ----------------------------------------------------------------------------------------------
        // --- If the relationship is paused, we want to set the partner pointers to null 
        // --- so that they are not accidentally used later when they are not valid.
        // --- We don't set them to null in Pause() because we need the pointers to be
        // --- valid for the Transmission report.  Setting the pointers to null at the end
        // --- of Individual::Update() is needed for the situation where partner A updates, 
        // --- infects partner B, and decides to pause the relationship.  Partner B won't update 
        // --- the Transmission report until his update.  At this point, we need the pointers in
        // --- relationship to be available for when partner B becomes infected and updates the report.
        // ----------------------------------------------------------------------------------------------
        if( state == RelationshipState::PAUSED )
        {
            if( absent_male_partner_id != suids::nil_suid() )
            {
                male_partner = nullptr;
            }
            if( absent_female_partner_id != suids::nil_suid() )
            {
                female_partner = nullptr;
            }
        }
    }

    void Relationship::Resume( IRelationshipManager* pRelMan, 
                               ISociety* pSociety, 
                               IIndividualHumanSTI* returnee )
    {
        release_assert( pRelMan  != nullptr );
        release_assert( pSociety != nullptr );
        release_assert( returnee != nullptr );

        if( state == RelationshipState::TERMINATED )
        {
            std::ostringstream msg;
            msg << "RelationshipId=" << GetSuid().data << " has already been terminated.  It cannot be resumed." ;
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }

        auto returneeId = returnee->GetSuid();
        if( (absent_male_partner_id == returneeId) || (male_partner == returnee) )
        {
            absent_male_partner_id = suids::nil_suid();
            male_partner = returnee;
            LOG_INFO_F( "%s: individual %d returning to relationship %d with individual %d \n", __FUNCTION__, returnee->GetSuid().data, GetSuid().data, PARTNERID( returnee ).data );
            LOG_DEBUG_F( "%s: returnee is male_partner, restoring male_partner to relationship %d\n", __FUNCTION__, GetSuid().data );
        }
        else if( (absent_female_partner_id == returneeId) || (female_partner == returnee) )
        {
            absent_female_partner_id = suids::nil_suid();
            female_partner = returnee;
            LOG_INFO_F( "%s: individual %d returning to relationship %d with individual %d \n", __FUNCTION__, returnee->GetSuid().data, GetSuid().data, PARTNERID( returnee ).data );
            LOG_DEBUG_F( "%s: returnee is female_partner, restoring female_partner to relationship %d\n", __FUNCTION__, GetSuid().data );
        }
        else
        {
            std::ostringstream msg;
            msg << "Unknown partner:  rel_id=" << GetSuid().data << " male_id=" << absent_male_partner_id.data << "  female_id=" << absent_female_partner_id.data << " returnee_id=" << returneeId.data ;
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }

        if( (absent_male_partner_id == suids::nil_suid()) && (absent_female_partner_id == suids::nil_suid()) )
        {
            release_assert( male_partner );
            release_assert( female_partner );
            if( male_partner->GetNodeSuid() == female_partner->GetNodeSuid() )
            {
                previous_state = state;
                state = RelationshipState::NORMAL;
            }
        }
        SetManager( pRelMan, pSociety );
        relMan->AddRelationship( this, false );
    }

    void Relationship::Terminate( RelationshipTerminationReason::Enum reason )
    {
        LOG_DEBUG_F( "%s: terminating relationship %d:%x with state=%s between individual %d and individual %d because %s.\n",
            __FUNCTION__,
            GetSuid().data,
            this, 
            RelationshipState::pairs::lookup_key( state ),
            MALE_PARTNER_ID().data, 
            FEMALE_PARTNER_ID().data,
            RelationshipTerminationReason::pairs::lookup_key( reason ));

        // -----------------------------------------------------------------------
        // --- Migrating would have set these values so we want to clear them to
        // --- make sure it is clear that the relationship has been terminated.
        // -----------------------------------------------------------------------
        if( state == RelationshipState::MIGRATING )
        {
            absent_male_partner_id   = suids::nil_suid();
            absent_female_partner_id = suids::nil_suid();
        }

        previous_state = state;
        state = RelationshipState::TERMINATED;
        termination_reason = reason;

        if( relMan != nullptr )
        {
            relMan->RemoveRelationship( this, true );
        }
        SetManager( nullptr, nullptr );

        if( (male_partner != nullptr) && (absent_male_partner_id == suids::nil_suid()) )
        { 
            male_partner->RemoveRelationship( this );
            // ----------------------------------------------------------
            // --- We don't set this to nullptr so that the relationship
            // --- can be used later to collect data at death
            // ----------------------------------------------------------
            //male_partner = nullptr;
        }

        if( (female_partner != nullptr) && (absent_female_partner_id == suids::nil_suid()) )
        {
            female_partner->RemoveRelationship( this );
            // ----------------------------------------------------------
            // --- We don't set this to nullptr so that the relationship
            // --- can be used later to collect data at death
            // ----------------------------------------------------------
            //female_partner = nullptr;
        }
    }

    void Relationship::Migrate()
    {
        if( state == RelationshipState::NORMAL )
        {
            previous_state = state;
            state = RelationshipState::MIGRATING;
            relMan->RemoveRelationship( this, true );

            absent_male_partner_id   = male_partner->GetSuid();
            absent_female_partner_id = female_partner->GetSuid();

            // -------------------------------------------------------------------
            // --- Don't set these to nullptr because if one of the partners dies
            // --- before they migrate, then we need the pointers so we can remove
            // --- the relationship.
            // -------------------------------------------------------------------
            //male_partner = nullptr;
            //female_partner = nullptr;
            // -------------------------------------------------------------------

            SetManager( nullptr, nullptr );
        }
        else if( state != RelationshipState::MIGRATING )
        {
            std::ostringstream msg;
            msg << "Trying to migrate relationship id=" << GetSuid().data << " when current state=" << state ;
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }
    }

    bool
    Relationship::IsDiscordant()
    const
    {
        bool bPart1Infected = (male_partner && male_partner->IsInfected());
        bool bPart2Infected = (female_partner && female_partner->IsInfected());
        if( bPart1Infected != bPart2Infected )
        {
            return true;
        }
        return false;
    }

    float
    Relationship::GetTimer()
    const
    {
        return rel_timer;
    }

    float
    Relationship::GetDuration()
    const
    {
        return rel_duration;
    }

    float Relationship::GetStartTime() const {
        return start_time;
    }

    float Relationship::GetScheduledEndTime() const {
        return scheduled_end_time;
    }

    bool Relationship::GetUsingCondom() const {
        return using_condom;
    }

    RelationshipType::Enum
    Relationship::GetType()
    const
    {
        return relationship_type;
    }

    unsigned int Relationship::_head = 0;
    unsigned int Relationship::_tail = 0;
    Relationship::LogEntry Relationship::_log[REL_LOG_COUNT];

    void Relationship::LogRelationship(
        unsigned int _id,
        suids::suid _male_partner,
        suids::suid _female_partner,
        RelationshipType::Enum _type
        )
    {
        _log[_head].id      = _id;
        _log[_head].male_partner = _male_partner;
        _log[_head].female_partner    = _female_partner;
        _log[_head].type    = _type;
        _head++;
        _head %= REL_LOG_COUNT;
        if (_head == _tail) {
            _tail++;
            _tail %= REL_LOG_COUNT;
        }
    }

    suids::suid Relationship::GetPartnerId( const suids::suid& myID ) const
    {
        if( myID == GetMalePartnerId() )
        {
            return GetFemalePartnerId();
        }
        else if( myID == GetFemalePartnerId() )
        {
            return GetMalePartnerId();
        }
        else
        {
            std::stringstream ss;
            ss << "Unknown partner: rel-id=" << _suid.data << " male-id=" << GetMalePartnerId().data << " female-id=" << GetFemalePartnerId().data << " unknown-id=" << myID.data;
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
    }

    suids::suid Relationship::GetMalePartnerId() const
    {
        return MALE_PARTNER_ID();
    }

    suids::suid Relationship::GetFemalePartnerId() const
    {
        return FEMALE_PARTNER_ID();
    }

    ExternalNodeId_t Relationship::GetOriginalNodeId() const
    {
        return original_node_id;
    }

    bool Relationship::IsMalePartnerAbsent() const
    {
        return (absent_male_partner_id != suids::nil_suid());
    }

    bool Relationship::IsFemalePartnerAbsent() const
    {
        return (absent_female_partner_id != suids::nil_suid());
    }

    float Relationship::GetCoitalRate() const
    {
        return p_rel_params->GetCoitalActRate();
    }

    ProbabilityNumber Relationship::getProbabilityUsingCondomThisAct() const
    {
        // TBD: In future we will get probs for both partners and use a configurable 
        // weighting but for now we'll just treat condoms as a male intervention in practice.
#define DECISION_WEIGHT (0.8f)
        //auto male_prob = male_partner->getProbabilityUsingCondomThisAct( GetType() );
        //auto female_prob = female_partner->getProbabilityUsingCondomThisAct( GetType() );
        //ProbabilityNumber prob = DECISION_WEIGHT * male_prob + (1.0f-DECISION_WEIGHT)*female_prob;
        ProbabilityNumber prob = male_partner->getProbabilityUsingCondomThisAct( p_rel_params );

        //LOG_DEBUG_F( "%s: returning %f from Sigmoid::vWAHS( %f, %f, %f, %f, %f )\n", __FUNCTION__, (float) prob, year, midyear, rate, early, late );
        return prob;
    }

    unsigned int Relationship::GetNumCoitalActs() const
    {
        return total_coital_acts;
    }

    bool Relationship::HasMigrated() const
    {
        return has_migrated;
    }

    void Relationship::serialize(IArchive& ar, Relationship* obj)
    {
        Relationship& rel = *obj;

        if( ar.IsWriter() )
        {
            if( (rel.absent_male_partner_id == suids::nil_suid()) && (rel.male_partner != nullptr) )
            {
                rel.absent_male_partner_id = rel.male_partner->GetSuid();
            }
            if( (rel.absent_female_partner_id == suids::nil_suid()) && (rel.female_partner != nullptr) )
            {
                rel.absent_female_partner_id = rel.female_partner->GetSuid();
            }
        }

        ar.labelElement("_suid"                   ) & rel._suid;
        ar.labelElement("state"                   ) & (uint32_t&)rel.state;
        ar.labelElement("previous_state"          ) & (uint32_t&)rel.previous_state;
        ar.labelElement("relationship_type"       ) & (uint32_t&)rel.relationship_type;
        ar.labelElement("termination_reason"      ) & (uint32_t&)rel.termination_reason;
        //p_rel_params don't serialize this
        //male_partner don't serialize this
        //female_partner don't serialize this
        ar.labelElement("absent_male_partner_id"  ) & rel.absent_male_partner_id;
        ar.labelElement("absent_female_partner_id") & rel.absent_female_partner_id;
        ar.labelElement("rel_timer"               ) & rel.rel_timer;
        ar.labelElement("rel_duration"            ) & rel.rel_duration;
        ar.labelElement("start_time"              ) & rel.start_time;
        ar.labelElement("scheduled_end_time"      ) & rel.scheduled_end_time;
        ar.labelElement("propertyKey"             ) & rel.propertyKey;
        ar.labelElement("propertyName"            ) & rel.propertyName;
        ar.labelElement("original_node_id"        ) & (uint32_t&)rel.original_node_id;
        ar.labelElement("act_prob_vec"            ) & rel.act_prob_vec;
        ar.labelElement("using_condom"            ) & rel.using_condom;
        //relMan don't serialize this
        ar.labelElement("total_coital_acts"       ) & rel.total_coital_acts;
        ar.labelElement("has_migrated"            ) & rel.has_migrated;
    }

    ///////////////////////////////////////////////////////////////////////////
    // SPECIFIC TYPES OF RELATIONSHIPS HERE //
    ///////////////////////////////////////////////////////////////////////////

    IRelationship* RelationshipFactory::CreateRelationship( const suids::suid& rRelId,
                                                            IRelationshipManager* pRelMan,
                                                            IRelationshipParameters* pParams, 
                                                            IIndividualHumanSTI* male_partner, 
                                                            IIndividualHumanSTI* female_partner )
    {
        IRelationship* p_rel = nullptr ;
        switch( pParams->GetType() )
        {
            case RelationshipType::TRANSITORY:
                p_rel = new TransitoryRelationship( rRelId, pRelMan, pParams, male_partner, female_partner );
                break;

            case RelationshipType::INFORMAL:
                p_rel = new InformalRelationship( rRelId, pRelMan, pParams, male_partner, female_partner );
                break;

            case RelationshipType::MARITAL:
                p_rel = new MarriageRelationship( rRelId, pRelMan, pParams, male_partner, female_partner );
                break;

            case RelationshipType::COMMERCIAL:
                p_rel = new CommercialRelationship( rRelId, pRelMan, pParams, male_partner, female_partner );
                break;

            default:
                throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__, 
                                                         "pParams->GetType()", 
                                                         pParams->GetType(), 
                                                         RelationshipType::pairs::lookup_key( pParams->GetType() ) );

        }
        return p_rel ;
    }

    // ------------------------------------------------------------------------
    // --- TransitoryRelationship
    // ------------------------------------------------------------------------
    BEGIN_QUERY_INTERFACE_DERIVED(TransitoryRelationship, Relationship)
    END_QUERY_INTERFACE_DERIVED(TransitoryRelationship, Relationship)

    TransitoryRelationship::TransitoryRelationship()
    : Relationship()
    {
    }

    TransitoryRelationship::TransitoryRelationship( const suids::suid& rRelId,
                                                    IRelationshipManager* pRelMan,
                                                    IRelationshipParameters* pParams,
                                                    IIndividualHumanSTI * male_partnerIn, 
                                                    IIndividualHumanSTI * female_partnerIn )
        : Relationship( rRelId, pRelMan, pParams, male_partnerIn, female_partnerIn )
    {
        LOG_INFO_F( "(EEL) Creating TransitoryRelationship %d between %s and %s of length %f.\n",
                    GetSuid().data,
                    male_partnerIn->toString().c_str(),
                    female_partnerIn->toString().c_str(),
                    rel_timer
                );
    }

    Relationship* TransitoryRelationship::Clone()
    {
        return new TransitoryRelationship( *this );
    }

    REGISTER_SERIALIZABLE(TransitoryRelationship);

    void TransitoryRelationship::serialize(IArchive& ar, TransitoryRelationship* obj)
    {
        Relationship::serialize( ar, obj );
        TransitoryRelationship& rel = *obj;
    }

    // ------------------------------------------------------------------------
    // --- InformalRelationship
    // ------------------------------------------------------------------------
    BEGIN_QUERY_INTERFACE_DERIVED(InformalRelationship, Relationship)
    END_QUERY_INTERFACE_DERIVED(InformalRelationship, Relationship)

    InformalRelationship::InformalRelationship()
    : Relationship()
    {
    }

    InformalRelationship::InformalRelationship( const suids::suid& rRelId,
                                                IRelationshipManager* pRelMan,
                                                IRelationshipParameters* pParams,
                                                IIndividualHumanSTI * male_partnerIn, 
                                                IIndividualHumanSTI * female_partnerIn )
        : Relationship( rRelId, pRelMan, pParams, male_partnerIn, female_partnerIn )
    {
        LOG_INFO_F( "(EEL) Creating InformalRelationship %d between %s and %s of length %f.\n",
                    GetSuid().data,
                    male_partnerIn->toString().c_str(),
                    female_partnerIn->toString().c_str(),
                    rel_timer
                  );
    }

    Relationship* InformalRelationship::Clone()
    {
        return new InformalRelationship( *this );
    }

    REGISTER_SERIALIZABLE(InformalRelationship);

    void InformalRelationship::serialize(IArchive& ar, InformalRelationship* obj)
    {
        Relationship::serialize( ar, obj );
        InformalRelationship& rel = *obj;
    }

    // ------------------------------------------------------------------------
    // --- MarriageRelationship
    // ------------------------------------------------------------------------
    BEGIN_QUERY_INTERFACE_DERIVED(MarriageRelationship, Relationship)
    END_QUERY_INTERFACE_DERIVED(MarriageRelationship, Relationship)

    MarriageRelationship::MarriageRelationship()
    : Relationship()
    {
    }

    MarriageRelationship::MarriageRelationship( const suids::suid& rRelId,
                                                IRelationshipManager* pRelMan,
                                                IRelationshipParameters* pParams,
                                                IIndividualHumanSTI * male_partnerIn, 
                                                IIndividualHumanSTI * female_partnerIn )
        : Relationship( rRelId, pRelMan, pParams, male_partnerIn, female_partnerIn )
    {
        LOG_INFO_F( "(EEL) Creating MaritalRelationship %d between %s and %s of length %f.\n",
                    GetSuid().data,
                    male_partnerIn->toString().c_str(),
                    female_partnerIn->toString().c_str(),
                    rel_timer
                  );
    }

    Relationship* MarriageRelationship::Clone()
    {
        return new MarriageRelationship( *this );
    }

    REGISTER_SERIALIZABLE(MarriageRelationship);

    void MarriageRelationship::serialize(IArchive& ar, MarriageRelationship* obj)
    {
        Relationship::serialize( ar, obj );
        MarriageRelationship& rel = *obj;
    }

    // ------------------------------------------------------------------------
    // --- CommercialRelationship
    // ------------------------------------------------------------------------
    BEGIN_QUERY_INTERFACE_DERIVED(CommercialRelationship, Relationship)
    END_QUERY_INTERFACE_DERIVED(CommercialRelationship, Relationship)

    CommercialRelationship::CommercialRelationship()
    : Relationship()
    {
    }

    CommercialRelationship::CommercialRelationship( const suids::suid& rRelId,
                                                          IRelationshipManager* pRelMan,
                                                          IRelationshipParameters* pParams,
                                                          IIndividualHumanSTI * male_partnerIn, 
                                                          IIndividualHumanSTI * female_partnerIn )
        : Relationship( rRelId, pRelMan, pParams, male_partnerIn, female_partnerIn )
    {
        LOG_INFO_F( "(EEL) Creating CommercialRelationship %d between %s and %s of length %f.\n",
                    GetSuid().data,
                    male_partnerIn->toString().c_str(),
                    female_partnerIn->toString().c_str(),
                    rel_timer
                  );
    }

    Relationship* CommercialRelationship::Clone()
    {
        return new CommercialRelationship( *this );
    }

    REGISTER_SERIALIZABLE(CommercialRelationship);

    void CommercialRelationship::serialize(IArchive& ar, CommercialRelationship* obj)
    {
        Relationship::serialize( ar, obj );
        CommercialRelationship& rel = *obj;
    }
}


