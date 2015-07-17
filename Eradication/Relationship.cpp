/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"
#include "Relationship.h"
#include "Debug.h"
#include "Environment.h"
#include "Types.h"
#include "SimulationConfig.h"
#include "MathFunctions.h"
#include "Node.h"
#include "Individual.h"
#include "STIInterventionsContainer.h" // for ISTIBarrierConsumer (which should be in ISTIBarrierConsumer.h) TODO
#include "IndividualSTI.h"

#include <map>
#include <ctime>
#include <algorithm>

static const char * _module = "Relationship";

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

    unsigned long int Relationship::counter = 1;

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

#define MALE_PARTNER_ID()             ( male_partner ? male_partner->GetSuid() : absent_male_partner_id )
#define FEMALE_PARTNER_ID()                ( female_partner    ? female_partner->GetSuid()    : absent_female_partner_id )
#define PARTNERID( individual)  ( GetPartner( individual ) ? GetPartner( individual )->GetSuid() : suids::nil_suid() )

    Relationship::Relationship( IIndividualHumanSTI * male_partnerIn, IIndividualHumanSTI * female_partnerIn, RelationshipType::Enum type )
        : male_partner(male_partnerIn)
        , female_partner(female_partnerIn)
        , absent_male_partner_id(suids::nil_suid())
        , absent_female_partner_id(suids::nil_suid())
        , _suid(suids::nil_suid())
        , relationship_type(type)
        , original_node_id(suids::nil_suid())
        , rel_duration(0)
        , rel_timer(0)
        , start_time(0)
        , scheduled_end_time(0)
        , using_condom(false)
    {
        original_node_id = dynamic_cast<IIndividualHuman*>(male_partner)->GetParent()->GetSuid();
        _id = counter++;
        LOG_DEBUG_F( "%s: Creating relationship %d between %d and %d\n", __FUNCTION__, _id, MALE_PARTNER_ID().data, FEMALE_PARTNER_ID().data );
        //LogRelationship( _id, MALE_PARTNER_ID(), FEMALE_PARTNER_ID(), relationship_type );
    }

    Relationship::~Relationship()
    {
        LOG_INFO_F( "(EEL) relationship %d between %d and %d just ended.\n", GetId(), MALE_PARTNER_ID().data, FEMALE_PARTNER_ID().data );
        // Might be interesting to track how often discordant relationships end discordantly, but 
        // not obvious how best to measure and report that.
        /*bool bPart1Infected = ( male_partner && male_partner->IsInfected() );
        bool bPart2Infected = ( female_partner && female_partner->IsInfected() );
        if( bPart1Infected != bPart2Infected )
        {
            std::cout << "Discordant relationship ended!" << std::endl;
        }*/
        // ----------------------------------------------------------------------------------
        // --- 5-20-2015 DMB I commented this out because I'm also not null'ing the pointers
        // --- to the partners.  If the partner is being deleted and we try to access their
        // --- id, bad things happen.
        // ----------------------------------------------------------------------------------
        //LogRelationship( _id | 0x80000000, MALE_PARTNER_ID(), FEMALE_PARTNER_ID(), relationship_type );
    }

    void
    Relationship::Initialize( IRelationshipManager* manager )
    {
        LOG_DEBUG_F( "%s()\n", __FUNCTION__);
        // TBD: Get dynamic prop name from SetSpouse, for now duplicate.
        relMan = manager;
        clock_t init = clock();
        std::ostringstream dynamicPropertyName;
        dynamicPropertyName << _id;
        propertyName = dynamicPropertyName.str();

        LOG_DEBUG_F( "%s: adding property [%s]:%s\n", __FUNCTION__, "Relationship", propertyName.c_str() );

        notifyIndividuals();

        howlong( init, "R::Init" );
    }

    void
    Relationship::notifyIndividuals()
    {
        male_partner->AddRelationship( this );
        female_partner->AddRelationship( this );
    }

    bool
    Relationship::Update( float dt )
    {
        LOG_VALID_F( "%s: man = %d, woman = %d, timer getting decremented from %f to %f\n", __FUNCTION__, MALE_PARTNER_ID().data, FEMALE_PARTNER_ID().data, rel_timer, rel_timer - 1 );

        rel_duration += dt;
        rel_timer -= dt;
        if( rel_timer <= 0 )
        {
            LOG_DEBUG_F( "%s: relationship %d between %d and %d is over, timer = %f...\n", __FUNCTION__, GetId(), MALE_PARTNER_ID().data, FEMALE_PARTNER_ID().data, rel_timer );
            return false;
        }
        return true;
    }

#define MALE_IS_ABSENT() ( absent_male_partner_id.data != suids::nil_suid().data )
#define FEMALE_IS_ABSENT() ( absent_female_partner_id.data != suids::nil_suid().data )

    void Relationship::Consummate( float dt )
    {
        if( MALE_IS_ABSENT() || FEMALE_IS_ABSENT() )
        {
            return;
        }

        // DJK: If we don't care about logging individual coital acts, could check IsDiscordant here
        unsigned int nRels = 1;
        float coital_rate_attenuation_factor = 1;
        if( GET_CONFIGURABLE(SimulationConfig)->enable_coital_dilution )
        {
            nRels = std::max<size_t>( male_partner->GetRelationships().size(), female_partner->GetRelationships().size() );

            if( nRels == 2)
                coital_rate_attenuation_factor = GET_CONFIGURABLE(SimulationConfig)->coital_dilution_2_partners;
            else if( nRels == 3)
                coital_rate_attenuation_factor = GET_CONFIGURABLE(SimulationConfig)->coital_dilution_3_partners;
            else if( nRels >= 4)
                coital_rate_attenuation_factor = GET_CONFIGURABLE(SimulationConfig)->coital_dilution_4_plus_partners;

            if( nRels > 1 )
            {
                LOG_DEBUG_F( "Coital dilution in effect: nRels is %d so attenuating coital rate by %f\n", nRels, coital_rate_attenuation_factor );
            }
        }
        release_assert( coital_rate_attenuation_factor > 0 );

        /*LOG_INFO_F( "(EEL) Coital Act: rel = %d, insertive = %d, relationships = %d, receptive = %d, relationships = %d, time till next act = %f.\n",
                    GetId(),
                    male_partner->GetSuid().data,
                    male_partner->GetRelationships().size(),
                    female_partner->GetSuid().data,
                    female_partner->GetRelationships().size(),
                    next_coital_act_timer
                  );*/

        double ratetime = dt * coital_rate_attenuation_factor * GetCoitalRate();
        unsigned int acts_this_dt = Environment::getInstance()->RNG->Poisson( ratetime );

        if( acts_this_dt > 0 ) {
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

            // STI risk factor if one partner has sti, avoid double risk
            float coInfectiveFactor1 = male_partner->GetCoInfectiveFactor();
            float coInfectiveFactor2 = female_partner->GetCoInfectiveFactor();
            float sti_mult = max( coInfectiveFactor1, coInfectiveFactor2 );
            
            act_prob_vec.clear();
            act_prob_vec.resize(2);

            act_prob_vec[0].num_acts = acts_not_using_condom_this_dt;
            act_prob_vec[0].prob_per_act = sti_mult;

            act_prob_vec[1].num_acts = acts_using_condom_this_dt;
            act_prob_vec[1].prob_per_act = sti_mult * (1-condomTransmissionBlockingProbability);
            //LOG_DEBUG_F( "Setting prob_per_act to %f for %d 'unprotected' acts and prob_per_act to %f for %d 'protected' acts.\n", sti_mult, (int) acts_not_using_condom_this_dt, act_prob_vec[1].prob_per_act, (int) act_prob_vec[1].num_acts );

            IIndividualHumanSTI *uninfected_individual = male_partner->IsInfected() ? female_partner : male_partner;
            uninfected_individual->NotifyPotentialExposure();
            IIndividualHumanSTI *infected_individual = male_partner->IsInfected() ? male_partner : female_partner;
            infected_individual->UpdateInfectiousnessSTI( act_prob_vec, GetId() );
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
        if ( individual == NULL )
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
            LOG_ERR_F( "%s: individual %lu is not in relationship %d!\n", __FUNCTION__, individual->GetSuid().data, GetId() );
            release_assert( false );
        }

        if (!partner)
        {
            LOG_WARN_F( "%s: partner of individual %lu in relationship %d is absent or deceased.\n", __FUNCTION__, individual->GetSuid().data, GetId() );
        }

        return partner;
    }

    unsigned long int
    Relationship::GetId()
    const
    {
        return _id;
    }

    void Relationship::SetSuid(suids::suid id)
    {
        _suid = id;
    }

    suids::suid Relationship::GetSuid()
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
            propertyKey = "Relationship";
            propertyKey += std::to_string( male_partner->GetOpenRelationshipSlot() );
            propertyKey += std::to_string( female_partner->GetOpenRelationshipSlot() ); // does this work? TBD
        }
        return propertyKey;
    }

    const tRelationshipMembers
    Relationship::GetMembers() const
    {
        tRelationshipMembers returnSet;
        returnSet.insert( male_partner );
        returnSet.insert( female_partner );
        return returnSet;
    }

    void Relationship::pause(
        IIndividualHumanSTI* departee
        )
    {
        LOG_INFO_F( "%s: individual %d vacating relationship %d leaving individual %d alone.\n", __FUNCTION__, departee->GetSuid().data, GetId(), PARTNERID( departee ).data );
        if( male_partner == departee )
        {
            LOG_DEBUG_F( "%s: departee was male_partner, clearing male_partner from relationship %d\n", __FUNCTION__, GetId() );
            male_partner = nullptr;
            absent_male_partner_id = departee->GetSuid();
        }
        else if( female_partner == departee )
        {
            LOG_DEBUG_F( "%s: departee was female_partner, clearing female_partner from relationship %d\n", __FUNCTION__, GetId() );
            female_partner = nullptr;
            absent_female_partner_id = departee->GetSuid();
        }
        else
        {
            release_assert(false);
        }
    }

    void Relationship::resume(
        IIndividualHumanSTI* returnee
        )
    {
        auto returneeId = returnee->GetSuid();
        if( absent_male_partner_id == returneeId )
        {
            absent_male_partner_id = suids::nil_suid();
            male_partner = returnee;
            LOG_INFO_F( "%s: individual %d returning to relationship %d with individual %d \n", __FUNCTION__, returnee->GetSuid().data, GetId(), PARTNERID( returnee ).data );
            LOG_DEBUG_F( "%s: returnee is male_partner, restoring male_partner to relationship %d\n", __FUNCTION__, GetId() );
        }
        else if( absent_female_partner_id == returneeId )
        {
            absent_female_partner_id = suids::nil_suid();
            female_partner = returnee;
            LOG_INFO_F( "%s: individual %d returning to relationship %d with individual %d \n", __FUNCTION__, returnee->GetSuid().data, GetId(), PARTNERID( returnee ).data );
            LOG_DEBUG_F( "%s: returnee is female_partner, restoring female_partner to relationship %d\n", __FUNCTION__, GetId() );
        }
        else
        {
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "I don't remember you! :(\n" );
        }
    }

    void Relationship::terminate(
        IRelationshipManager* manager
        )
    {
        LOG_INFO_F( "%s: terminating relationship %d between individual %d and individual %d.\n", __FUNCTION__, GetId(), MALE_PARTNER_ID().data, FEMALE_PARTNER_ID().data );

        manager->RemoveRelationship( this );

        if( male_partner )
        { 
            male_partner->RemoveRelationship( this );
            //male_partner = nullptr;
        }

        if( female_partner )
        {
            female_partner->RemoveRelationship( this );
            //female_partner = nullptr;
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

    suids::suid Relationship::GetMalePartnerId() const
    {
        return MALE_PARTNER_ID();
    }

    suids::suid Relationship::GetFemalePartnerId() const
    {
        return FEMALE_PARTNER_ID();
    }

    suids::suid Relationship::GetOriginalNodeId() const
    {
        return original_node_id;
    }

    float Relationship::GetCoitalRate() const
    {
        return GET_CONFIGURABLE(SimulationConfig)->coital_act_rate[relationship_type];
    }

    ProbabilityNumber Relationship::getProbabilityUsingCondomThisAct() const
    {
        // TBD: In future we will get probs for both partners and use a configurable 
        // weighting but for now we'll just treat condoms as a male intervention in practice.
#define DECISION_WEIGHT (0.8f)
        //auto male_prob = male_partner->getProbabilityUsingCondomThisAct( GetType() );
        //auto female_prob = female_partner->getProbabilityUsingCondomThisAct( GetType() );
        //ProbabilityNumber prob = DECISION_WEIGHT * male_prob + (1.0f-DECISION_WEIGHT)*female_prob;
        ProbabilityNumber prob = male_partner->getProbabilityUsingCondomThisAct( GetType() );

        //LOG_DEBUG_F( "%s: returning %f from Sigmoid::vWAHS( %f, %f, %f, %f, %f )\n", __FUNCTION__, (float) prob, year, midyear, rate, early, late );
        return prob;
    }

    ///////////////////////////////////////////////////////////////////////////
    // SPECIFIC TYPES OF RELATIONSHIPS HERE //
    ///////////////////////////////////////////////////////////////////////////

    IRelationship* RelationshipFactory::CreateRelationship( RelationshipType::Enum relType, 
                                                            IIndividualHumanSTI* male_partner, 
                                                            IIndividualHumanSTI* female_partner )
    {
        IRelationship* p_rel = nullptr ;
        switch( relType )
        {
            case RelationshipType::TRANSITORY:
                p_rel = new TransitoryRelationship( male_partner, female_partner );
                break;

            case RelationshipType::INFORMAL:
                p_rel = new InformalRelationship( male_partner, female_partner );
                break;

            case RelationshipType::MARITAL:
                p_rel = new MarriageRelationship( male_partner, female_partner );
                break;

            default:
                throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__, "relType", relType, RelationshipType::pairs::lookup_key( relType ) );

        }
        return p_rel ;
    }


    TransitoryRelationship::TransitoryRelationship( IIndividualHumanSTI * male_partnerIn, IIndividualHumanSTI * female_partnerIn )
        : Relationship( male_partnerIn, female_partnerIn, RelationshipType::TRANSITORY )
    {
        rel_timer = DAYSPERYEAR * Environment::getInstance()->RNG->Weibull2( GET_CONFIGURABLE(SimulationConfig)->transitoryRel_lambda, 
                                                                             GET_CONFIGURABLE(SimulationConfig)->transitoryRel_inv_kappa );

        IIndividualHuman* individual = NULL;
        if( male_partnerIn->QueryInterface(GET_IID(IIndividualHuman), (void**)&individual) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "male_partnerIn", "IIndividualHuman", "IIndividualHumanSTI" );
        }

        start_time = (float) individual->GetParent()->GetTime().time;
        scheduled_end_time = start_time + rel_timer;

        LOG_INFO_F( "(EEL) Creating TransitoryRelationship %d between %s and %s of length %f.\n",
                    GetId(),
                    male_partnerIn->toString().c_str(),
                    female_partnerIn->toString().c_str(),
                    rel_timer
                );
    }


    InformalRelationship::InformalRelationship( IIndividualHumanSTI * male_partnerIn, IIndividualHumanSTI * female_partnerIn )
        : Relationship( male_partnerIn, female_partnerIn, RelationshipType::INFORMAL )
    {
        rel_timer = DAYSPERYEAR * Environment::getInstance()->RNG->Weibull2( GET_CONFIGURABLE(SimulationConfig)->informalRel_lambda, 
                                                                             GET_CONFIGURABLE(SimulationConfig)->informalRel_inv_kappa );

        IIndividualHuman* individual = NULL;
        if( male_partnerIn->QueryInterface(GET_IID(IIndividualHuman), (void**)&individual) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "male_partnerIn", "IIndividualHuman", "IIndividualHumanSTI" );
        }

        start_time = (float) individual->GetParent()->GetTime().time;
        scheduled_end_time = start_time + rel_timer;

        LOG_INFO_F( "(EEL) Creating InformalRelationship %d between %s and %s of length %f.\n",
                    GetId(),
                    male_partnerIn->toString().c_str(),
                    female_partnerIn->toString().c_str(),
                    rel_timer
                  );
    }



    MarriageRelationship::MarriageRelationship( IIndividualHumanSTI * male_partnerIn, IIndividualHumanSTI * female_partnerIn )
        : Relationship( male_partnerIn, female_partnerIn, RelationshipType::MARITAL )
    {
        rel_timer = DAYSPERYEAR * Environment::getInstance()->RNG->Weibull2( GET_CONFIGURABLE(SimulationConfig)->maritalRel_lambda, 
                                                                             GET_CONFIGURABLE(SimulationConfig)->maritalRel_inv_kappa );

        IIndividualHuman* individual = NULL;
        if( male_partnerIn->QueryInterface(GET_IID(IIndividualHuman), (void**)&individual) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "male_partnerIn", "IIndividualHuman", "IIndividualHumanSTI" );
        }

        start_time = (float) individual->GetParent()->GetTime().time;
        scheduled_end_time = start_time + rel_timer;

        LOG_INFO_F( "(EEL) Creating MaritalRelationship %d between %s and %s of length %f.\n",
                    GetId(),
                    male_partnerIn->toString().c_str(),
                    female_partnerIn->toString().c_str(),
                    rel_timer
                  );
    }
}


