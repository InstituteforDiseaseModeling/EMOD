/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include <typeinfo>
#include "IndividualSTI.h"

#include "Debug.h"
#include "IIndividualHuman.h"
#include "InfectionSTI.h"
#include "NodeEventContext.h"
#include "Relationship.h"
#include "RelationshipGroups.h"
#include "SimulationConfig.h"
#include "SusceptibilitySTI.h"
#include "STIInterventionsContainer.h"
#include "NodeSTI.h"
#include "Sigmoid.h"
#include "IPairFormationStats.h"
#include "IPairFormationRateTable.h"
#include "IPairFormationAgent.h"
#include "IConcurrency.h"
#include "StrainIdentity.h"
#include "EventTrigger.h"

SETUP_LOGGING( "IndividualSTI" )

// Assume MAX_SLOTS == 63 => 64-bits are full
#define SLOTS_FILLED (uint64_t(0xFFFFFFFFFFFFFFFF))

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!! If you change size of promiscuity_flags (unsigned char) or
// !!! add more RelationshipTypes (>7), change this macro.
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#define SUPER_SPREADER 0x80

#define IS_SUPER_SPREADER()   ((promiscuity_flags & SUPER_SPREADER) != 0)
#define IS_EXTRA_ALLOWED(rel) ((promiscuity_flags & EXTRA_RELATIONAL_ALLOWED((Kernel::RelationshipType::Enum)rel)) != 0)
#define MAX_RELATIONSHIPS_PER_INDIVIDUAL_ALL_TYPES (MAX_SLOTS)

// -------------------------------------------------------------------
// !!!! Duplicated in ReportRelationshipCensus !!!
#define THREE_MONTHS  ( 91) // ~3 months
#define SIX_MONTHS    (182) // ~6 months
#define NINE_MONTHS   (274) // ~9 months
#define TWELVE_MONTHS (365) // ~12 months

static const float PERIODS[] = { THREE_MONTHS, SIX_MONTHS, NINE_MONTHS, TWELVE_MONTHS };
static std::vector<float> UNIQUE_PARTNER_TIME_PERIODS( PERIODS, PERIODS + sizeof( PERIODS ) / sizeof( PERIODS[ 0 ] ) );
// -------------------------------------------------------------------


namespace Kernel
{
    float IndividualHumanSTIConfig::debutAgeYrsMale_inv_kappa = 1.0f;
    float IndividualHumanSTIConfig::debutAgeYrsMale_lambda = 1.0f;
    float IndividualHumanSTIConfig::debutAgeYrsFemale_inv_kappa = 1.0f;
    float IndividualHumanSTIConfig::debutAgeYrsFemale_lambda = 1.0f;
    float IndividualHumanSTIConfig::debutAgeYrsMin = 13.0f;

    float IndividualHumanSTIConfig::sti_coinfection_trans_mult = 1.0f;
    float IndividualHumanSTIConfig::sti_coinfection_acq_mult = 1.0f;

    float IndividualHumanSTIConfig::min_days_between_adding_relationships = 60.0f;

    float IndividualHumanSTIConfig::condom_transmission_blocking_probability = 0.0f;

    std::vector<float> IndividualHumanSTIConfig::maleToFemaleRelativeInfectivityAges;
    std::vector<float> IndividualHumanSTIConfig::maleToFemaleRelativeInfectivityMultipliers;

    bool IndividualHumanSTIConfig::enable_coital_dilution = true;
    float IndividualHumanSTIConfig::coital_dilution_2_partners      = 1.0f;
    float IndividualHumanSTIConfig::coital_dilution_3_partners      = 1.0f;
    float IndividualHumanSTIConfig::coital_dilution_4_plus_partners = 1.0f;

    GET_SCHEMA_STATIC_WRAPPER_IMPL(IndividualHumanSTI,IndividualHumanSTIConfig)
    BEGIN_QUERY_INTERFACE_BODY(IndividualHumanSTIConfig)
    END_QUERY_INTERFACE_BODY(IndividualHumanSTIConfig)

    bool IndividualHumanSTIConfig::Configure( const Configuration* config )
    {
        LOG_DEBUG( "Configure\n" );

        initConfigTypeMap( "Sexual_Debut_Age_Male_Weibull_Heterogeneity", &debutAgeYrsMale_inv_kappa, STI_Sexual_Debut_Age_Male_Weibull_Heterogeneity_DESC_TEXT, 0.0f, 50.0f, 20.0f );
        initConfigTypeMap( "Sexual_Debut_Age_Male_Weibull_Scale", &debutAgeYrsMale_lambda, STI_Sexual_Debut_Age_Male_Weibull_Scale_DESC_TEXT, 0.0f, 50.0f, 16.0f );     // Min was 13
        initConfigTypeMap( "Sexual_Debut_Age_Female_Weibull_Heterogeneity", &debutAgeYrsFemale_inv_kappa, STI_Sexual_Debut_Age_Female_Weibull_Heterogeneity_DESC_TEXT, 0.0f, 50.0f, 20.0f );
        initConfigTypeMap( "Sexual_Debut_Age_Female_Weibull_Scale", &debutAgeYrsFemale_lambda, STI_Sexual_Debut_Age_Female_Weibull_Scale_DESC_TEXT, 0.0f, 50.0f, 16.0f );   // Min was 13

        initConfigTypeMap( "Sexual_Debut_Age_Min", &debutAgeYrsMin, STI_Sexual_Debut_Age_Min_DESC_TEXT, 0.0f, FLT_MAX, 13.0f );

        initConfigTypeMap( "STI_Coinfection_Acquisition_Multiplier", &sti_coinfection_acq_mult, STI_Coinfection_Acquisition_Multiplier_DESC_TEXT, 0.0f, 100.0f, 10.0f );
        initConfigTypeMap( "STI_Coinfection_Transmission_Multiplier", &sti_coinfection_trans_mult, STI_Coinfection_Transmission_Multiplier_DESC_TEXT, 0.0f, 100.0f, 10.0f );

        initConfigTypeMap( "Min_Days_Between_Adding_Relationships", &min_days_between_adding_relationships, STI_Min_Days_Between_Adding_Relationships_DESC_TEXT, 0.0f, 365.0f, 60.0f );

        initConfigTypeMap( "Male_To_Female_Relative_Infectivity_Ages", &maleToFemaleRelativeInfectivityAges, STI_Male_To_Female_Relative_Infectivity_Ages_DESC_TEXT, 0.0f, FLT_MAX, 0.0f );
        initConfigTypeMap( "Male_To_Female_Relative_Infectivity_Multipliers", &maleToFemaleRelativeInfectivityMultipliers, STI_Male_To_Female_Relative_Infectivity_Multipliers_DESC_TEXT, 0.0f, 25.0f, 1.0f );

        initConfigTypeMap( "Condom_Transmission_Blocking_Probability", &condom_transmission_blocking_probability, STI_Condom_Transmission_Blocking_Probability_DESC_TEXT, 0.0f, 1.0f, 0.9f );

        initConfigTypeMap( "Enable_Coital_Dilution", &enable_coital_dilution, Enable_Coital_Dilution_DESC_TEXT, true );
        initConfigTypeMap( "Coital_Dilution_Factor_2_Partners", &coital_dilution_2_partners, Coital_Dilution_Factor_2_Partners_DESC_TEXT, FLT_EPSILON, 1.0f, 1.0f );
        initConfigTypeMap( "Coital_Dilution_Factor_3_Partners", &coital_dilution_3_partners, Coital_Dilution_Factor_3_Partners_DESC_TEXT, FLT_EPSILON, 1.0f, 1.0f);
        initConfigTypeMap( "Coital_Dilution_Factor_4_Plus_Partners", &coital_dilution_4_plus_partners, Coital_Dilution_Factor_4_Plus_Partners_DESC_TEXT, FLT_EPSILON, 1.0f, 1.0f );

        bool ret = JsonConfigurable::Configure( config );

        return ret;
    }

    bool RelationshipSetSorter::operator()(const IRelationship *rel1, const IRelationship *rel2) const
    {
        return rel1->GetSuid().data < rel2->GetSuid().data;
    }

    // The reason for this explicit QI (instead of macro) is to let us resolve 
    // the IJsonSerializable cast which is ambiguous and must be done via
    // a specific intermediate.
    Kernel::QueryResult
    IndividualHumanSTI::QueryInterface( iid_t iid, void **ppinstance )
    {
        if ( !ppinstance )
        {
            return e_NULL_POINTER;
        }

        ISupports* foundInterface = nullptr;

        if ( iid == GET_IID(IIndividualHuman)) 
            foundInterface = static_cast<IIndividualHuman*>(this);
        else if ( iid == GET_IID(IIndividualHumanSTI)) 
            foundInterface = static_cast<IIndividualHumanSTI*>(this);
        else if ( iid == GET_IID(ISupports)) 
            foundInterface = static_cast<ISupports*>(static_cast<IIndividualHumanSTI*>(this));
        else if( IndividualHuman::QueryInterface( iid, (void**)&foundInterface ) != s_OK )
            foundInterface = nullptr;

        QueryResult status = e_NOINTERFACE;
        if ( foundInterface )
        {
            foundInterface->AddRef();
            status = s_OK;
        }

        *ppinstance = foundInterface;
        return status;
    }

    void IndividualHumanSTI::SetConcurrencyParameters( const char *prop, const char* prop_value )
    {
        // Update promiscuity flags.  Begin with a reset.
        if( IS_SUPER_SPREADER() )
            promiscuity_flags = SUPER_SPREADER;
        else
            promiscuity_flags = 0;

        IConcurrency* p_concurrency = p_sti_node->GetSociety()->GetConcurrency();

        promiscuity_flags |= p_concurrency->GetProbExtraRelationalBitMask( prop, prop_value, Gender::Enum(GetGender()), IS_SUPER_SPREADER() );

        // Max allowable relationships
        NaturalNumber totalMax = 0;
        for( int rel = 0; rel < RelationshipType::COUNT; rel++)
        {
            max_relationships[rel] = p_concurrency->GetMaxAllowableRelationships( prop, prop_value, Gender::Enum(GetGender()), RelationshipType::Enum(rel) );
            totalMax += max_relationships[rel];
        }
        if( totalMax > MAX_RELATIONSHIPS_PER_INDIVIDUAL_ALL_TYPES )
        {
            throw ConfigurationRangeException( __FILE__, __LINE__, __FUNCTION__, "<sum of max number of relationships across types>", totalMax, MAX_RELATIONSHIPS_PER_INDIVIDUAL_ALL_TYPES );
        }
    }

    void IndividualHumanSTI::UpdateSTINetworkParams( const char *prop, const char* new_value )
    {
        IConcurrency* p_concurrency = p_sti_node->GetSociety()->GetConcurrency();

        if( (prop == nullptr) || p_concurrency->IsConcurrencyProperty( prop ) )
        {
            const char* prop_key = prop;
            if( prop == nullptr )
            {
                prop_key = p_concurrency->GetPropertyKey().c_str();
            }
            tProperties old_map = GetProperties()->GetOldVersion();
            const char* prop_value = p_concurrency->GetConcurrencyPropertyValue( &old_map, prop_key, new_value );
            SetConcurrencyParameters( prop_key, prop_value );
        }
    }

    IndividualHumanSTI *IndividualHumanSTI::CreateHuman(INodeContext *context, suids::suid id, float MCweight, float init_age, int gender, float init_poverty)
    {
        IndividualHumanSTI *newindividual = _new_ IndividualHumanSTI(id, MCweight, init_age, gender, init_poverty);

        newindividual->SetContextTo(context);
        newindividual->InitializeConcurrency();
        LOG_DEBUG_F( "Created human with age=%f and gender=%d\n", newindividual->m_age, gender );

        return newindividual;
    }

    void IndividualHumanSTI::InitializeConcurrency()
    {
        // -------------------------------------------------------------------------------------------------------------
        // --- DMB 6-11-2016  I'm not doing this in InitializeHuman() because it change the random number stream.
        // --- This used to be called in the constructor, but I can't do that because we need p_sti_node to be defined.
        // -------------------------------------------------------------------------------------------------------------

        // Sexual debut
        float min_age_sexual_debut_in_days = IndividualHumanSTIConfig::debutAgeYrsMin * DAYSPERYEAR;
        float debut_lambda = 0;
        float debut_inv_kappa = 0;
        if( GetGender() == Gender::MALE ) 
        {
            debut_inv_kappa = IndividualHumanSTIConfig::debutAgeYrsMale_inv_kappa;
            debut_lambda    = IndividualHumanSTIConfig::debutAgeYrsMale_lambda;
        }
        else
        {
            debut_inv_kappa = IndividualHumanSTIConfig::debutAgeYrsFemale_inv_kappa;
            debut_lambda    = IndividualHumanSTIConfig::debutAgeYrsFemale_lambda;
        }
        float debut_draw = float(DAYSPERYEAR * Environment::getInstance()->RNG->Weibull2( debut_lambda, debut_inv_kappa ));
        LOG_DEBUG_F( "debut_draw = %f with lamba %f and kappa %f\n", debut_draw, debut_lambda, debut_inv_kappa );
        sexual_debut_age = (std::max)(min_age_sexual_debut_in_days, debut_draw );

        LOG_DEBUG_F( "Individual ? will debut at age %f (yrs)f.\n", sexual_debut_age/DAYSPERYEAR );

        // Promiscuity flags, including behavioral super-spreader
        auto draw = randgen->e();
        if( draw < p_sti_node->GetSociety()->GetConcurrency()->GetProbSuperSpreader() )
        {
            promiscuity_flags |= SUPER_SPREADER;
        }
    }

    void IndividualHumanSTI::InitializeHuman()
    {
        IndividualHuman::InitializeHuman();

        UpdateSTINetworkParams( nullptr, nullptr );
    }

    void IndividualHumanSTI::CreateSusceptibility(float imm_mod, float risk_mod)
    {
        susceptibility = SusceptibilitySTI::CreateSusceptibility(this, m_age, imm_mod, risk_mod);
    }

    void IndividualHumanSTI::NotifyPotentialExposure()
    {
        potential_exposure_flag = true;
    }

    void IndividualHumanSTI::ExposeToInfectivity(float dt, const TransmissionGroupMembership_t* transmissionGroupMembership)
    {
        // call UPM if any relationships have been deposited to
        if( potential_exposure_flag )
        {
            UpdateGroupMembership(); // we "JIT" this function (just in time)
            LOG_DEBUG_F( "Exposing individual %d\n", GetSuid().data );
            release_assert( IsInfected() == false );
            parent->ExposeIndividual(this, transmissionGroupMembership, dt);
            potential_exposure_flag = false;
        }
    }

    void IndividualHumanSTI::Expose(const IContagionPopulation* cp, float dt, TransmissionRoute::Enum transmission_route)
    {
        if( IsInfected() )
        {
            return;
        }

        // First thing is to cast cp to IContagionProbabilities.
        IContagionProbabilities * pCP_as_Probs = nullptr;
        if ((const_cast<IContagionPopulation*>(cp))->QueryInterface(GET_IID(IContagionProbabilities), (void**)&pCP_as_Probs) != s_OK)
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "cp", "IContagionProbabilities", "IContagionPopulation" );
        }
        release_assert( pCP_as_Probs );
        auto probs = pCP_as_Probs->GetProbabilities();

        // Apply gender-based susceptibility multipliers here
        float mult = 1.0f;
        if( GetGender() == Gender::MALE )
        {
            if( IsCircumcised() )
            {
                mult *= (1.0 - m_pSTIInterventionsContainer->GetCircumcisedReducedAcquire());
            }
        }
        else if(IndividualHumanSTIConfig::maleToFemaleRelativeInfectivityAges.size() > 0 )
        {
            // Individual is female. If user specified a male-to-female infectivity multiplier, 
            // we need to apply that to the probability of infection. The base infectivity
            // is for female to male. Male to female can be equal or greater.
            NonNegativeFloat multiplier = 0.0f;
            NonNegativeFloat age_in_yrs = GetAge()/DAYSPERYEAR;
            unsigned int idx = 0;
            while( idx < IndividualHumanSTIConfig::maleToFemaleRelativeInfectivityAges.size() &&
                   age_in_yrs > IndividualHumanSTIConfig::maleToFemaleRelativeInfectivityAges[ idx ] )
            {
                idx++;
            }
            // 3 possible cases
            if( idx == 0 )
            {
                multiplier = IndividualHumanSTIConfig::maleToFemaleRelativeInfectivityMultipliers[0];
                LOG_DEBUG_F( "Using value of %f for age-asymmetric infection multiplier for female age (in yrs) %f.\n", float(multiplier), (float) age_in_yrs );
            }
            else if( idx == IndividualHumanSTIConfig::maleToFemaleRelativeInfectivityAges.size() )
            {
                multiplier = IndividualHumanSTIConfig::maleToFemaleRelativeInfectivityMultipliers.back();
                LOG_DEBUG_F( "Using value of %f for age-asymmetric infection multiplier for female age (in yrs) %f.\n", float(multiplier), (float) age_in_yrs );
            }
            else 
            {
                // do some linear interp math.
                float left_age = IndividualHumanSTIConfig::maleToFemaleRelativeInfectivityAges[idx-1];
                float right_age = IndividualHumanSTIConfig::maleToFemaleRelativeInfectivityAges[idx];
                float left_mult = IndividualHumanSTIConfig::maleToFemaleRelativeInfectivityMultipliers[idx-1];
                float right_mult = IndividualHumanSTIConfig::maleToFemaleRelativeInfectivityMultipliers[idx];
                multiplier = left_mult + ( age_in_yrs - left_age ) / ( right_age - left_age ) * ( right_mult - left_mult );
                LOG_DEBUG_F( "Using interpolated value of %f for age-asymmetric infection multiplier for age (in yrs) %f.\n", float(multiplier), (float) age_in_yrs );
            } 

            mult *= multiplier;
        }

        // now let's figure out if transmission occurred based on this set of per act probabilities
        float prob_non_infection = 1.0f;
        for( auto prob : probs )
        {
            if( prob.prob_per_act > 0 )
            {
                //LOG_INFO_F( "prob = %f\n", mult * prob.prob_per_act );
                //prob_non_infection *= (1 - mult * prob.prob_per_act);

                ProbabilityNumber capped_prob = ( mult * prob.prob_per_act > 1.0f ) ? 1.0f : ( mult * prob.prob_per_act );
                LOG_INFO_F( "prob = %f\n", float(capped_prob) );
                prob_non_infection *= (1.0f - capped_prob);
            }
        }
        float prob_infection = 1.0f - prob_non_infection;
        LOG_INFO_F( "prob_infection = %f\n", prob_infection );
        if( randgen->e() < prob_infection )
        {
            StrainIdentity strainId;
            // The ContagionProbability object can give us the ID of the Infector (Depositor)
            // We store this as the heretofore unused "Antigen ID" of the Infecting Strain.
            cp->ResolveInfectingStrain(&strainId); // get the substrain ID
            strainId.SetAntigenID(pCP_as_Probs->GetInfectorID());
            AcquireNewInfection( &strainId );
        }
    }

    void IndividualHumanSTI::ReportInfectionState()
    {
        //LOG_DEBUG_F( "Setting m_new_infection_state to NewInfection.\n" );
        m_new_infection_state = NewInfectionState::NewInfection;
    }

    IndividualHumanSTI::IndividualHumanSTI(suids::suid _suid, float monte_carlo_weight, float initial_age, int gender, float initial_poverty )
        : IndividualHuman(_suid, monte_carlo_weight, initial_age, gender, initial_poverty)
        , relationships()
        , migrating_because_of_partner(false)
        , promiscuity_flags(0)
        , sexual_debut_age(0.0f)
        , has_other_sti_co_infection(false)
        , transmissionInterventionsDisabled(false)
        , relationshipSlots(0)
        , delay_between_adding_relationships_timer(0.0f)
        , potential_exposure_flag(false)
        , m_pSTIInterventionsContainer( nullptr )
        , relationships_at_death()
        , num_lifetime_relationships()
        , last_6_month_relationships()
        , last_12_month_relationships()
        , p_sti_node(nullptr)
        //, m_AssortivityIndex()
        , m_TotalCoitalActs( 0 )
        , relationship_properties()
        , num_unique_partners()
    {
        ZERO_ARRAY( queued_relationships );
        ZERO_ARRAY( active_relationships );
        ZERO_ARRAY( num_lifetime_relationships );

        for (int i = 0; i < RelationshipType::COUNT; ++i)
        {
            m_AssortivityIndex[i] = -1;
        }

        for( int i = 0; i < UNIQUE_PARTNER_TIME_PERIODS.size(); ++i )
        {
            std::vector<PartnerIdToRelEndTimeMap_t> rel_type_vector_of_maps;
            for( int j = 0; j < RelationshipType::COUNT; ++j )
            {
                rel_type_vector_of_maps.push_back( PartnerIdToRelEndTimeMap_t() );
            }
            num_unique_partners.push_back( rel_type_vector_of_maps );
        }
    }

    const IPKeyValueContainer& IndividualHumanSTI::GetPropertiesConst() const
    {
        return Properties;
    }

    suids::suid IndividualHumanSTI::GetNodeSuid() const
    {
        if( parent == nullptr )
        {
            return suids::nil_suid();
        }
        else
        {
            return parent->GetSuid();
        }
    }

    void IndividualHumanSTI::Update(
        float currenttime,
        float dt
        )
    {
        IndividualHuman::Update( currenttime, dt );

        // ---------------------------------------------------------------
        // --- Update the individual pointers in the paused relationships.
        // --- This is to help against using invalid pointers.
        // ---------------------------------------------------------------
        for( auto rel : relationships )
        {
            rel->UpdatePaused();
        }
    }

    void IndividualHumanSTI::UpdateAge( float dt )
    {
        bool was_pre_debut = m_age < sexual_debut_age;

        IndividualHuman::UpdateAge( dt );

        // Check for debut
        bool is_post_debut = m_age >= sexual_debut_age;
        if( was_pre_debut && is_post_debut )
        {
            // Broadcast STIDebut
            broadcaster->TriggerNodeEventObservers( GetEventContext(), EventTrigger::STIDebut );
        }
    }

   void IndividualHumanSTI::UpdateHistory( const IdmDateTime& rCurrentTime, float dt )
   {
       while( (last_6_month_relationships.size() > 0) && ((rCurrentTime.time - last_6_month_relationships.front().second) > SIX_MONTHS) )
       {
            last_6_month_relationships.pop_front();
       }
       while( (last_12_month_relationships.size() > 0) && ((rCurrentTime.time - last_12_month_relationships.front().second) > TWELVE_MONTHS) )
       {
           last_12_month_relationships.pop_front();
       }

       for( int itp = 0; itp < UNIQUE_PARTNER_TIME_PERIODS.size(); ++itp )
       {
           float time_period = UNIQUE_PARTNER_TIME_PERIODS[ itp ];
           for( int irel = 0; irel < RelationshipType::COUNT; ++irel )
           {
               PartnerIdToRelEndTimeMap_t& r_partner_map = num_unique_partners[ itp ][ irel ];
               for( auto it = r_partner_map.begin(); it != r_partner_map.end();  )
               {
                   if( (rCurrentTime.time - it->second) > time_period )
                   {
                       it = r_partner_map.erase( it );
                   }
                   else
                   {
                       ++it;
                   }
               }
           }
       }
   }

    void IndividualHumanSTI::Die(
        HumanStateChange newState
    )
    {
        release_assert( (newState == HumanStateChange::DiedFromNaturalCauses) || (newState == HumanStateChange::KilledByInfection) );

        // Remove self from PFAs if queued up
        disengageFromSociety();

        for (auto iterator = relationships.begin(); iterator != relationships.end(); /**/ )
        {
            auto relationship = *iterator++;
            relationship->Terminate( RelationshipTerminationReason::SELF_DIED );
            relationships_at_death.insert( relationship );
            LOG_DEBUG_F("Relationship with %d terminated at time %f due to death\n", GetSuid().data, (float) GetParent()->GetTime().time );
        }

        IndividualHuman::Die( newState );
    }

    IndividualHumanSTI::~IndividualHumanSTI()
    {
        LOG_DEBUG_F( "%lu (STI) destructor.\n", suid.data );

        for( auto p_rel : relationships_at_death )
        {
            delete p_rel ;
        }
        relationships_at_death.clear();
    }

    IInfection* IndividualHumanSTI::createInfection( suids::suid _suid )
    {
        return InfectionSTI::CreateInfection(this, _suid);
    }

    void IndividualHumanSTI::InitializeStaticsSTI( const Configuration* config )
    {
        IndividualHumanSTIConfig individual_config;
        individual_config.Configure( config );
    }

    void IndividualHumanSTI::setupInterventionsContainer()
    {
        m_pSTIInterventionsContainer = _new_ STIInterventionsContainer();
        interventions = m_pSTIInterventionsContainer;
    }

    void IndividualHumanSTI::UpdateGroupMembership()
    {
        const RouteList_t& routes = parent->GetTransmissionRoutes();

        static_cast<NodeSTI*>(p_sti_node)->GetGroupMembershipForIndividual_STI( routes, &relationship_properties, &transmissionGroupMembership );
    }

    void IndividualHumanSTI::UpdateInfectiousnessSTI(act_prob_vec_t &act_prob_vec, unsigned int rel_id)
    {
        UpdateGroupMembership();
        LOG_DEBUG_F( "Doing %s for individual %d and relationship %d/pool_index %d.\n",
                    __FUNCTION__, GetSuid().data, rel_id, transmissionGroupMembership[ rel_id ] );

        // DJK: The following code seems overly complicated for the task at hand
        // TBD: This switcharoo thing could probably be replaced by a membership map in RelationshipGroups
        auto transmissionGroupMembershipSave = transmissionGroupMembership;
        transmissionGroupMembership.clear();
        transmissionGroupMembership[ rel_id ] = transmissionGroupMembershipSave[ rel_id ];

        //IndividualHuman::UpdateInfectiousness(dt);
        infectiousness = 0;

        if ( infections.size() == 0 ) 
            return;
        else if( infections.size() > 1 )
            throw NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "STI/HIV does not support superinfection yet." );

        for (auto infection : infections)
        {
            // For STI/HIV, infectiousness should be the per-act probability of transmission
            // including intrahost factors like stage and ART, but excluding condoms
            // Also excluding STI because they're handled separately
            infectiousness += infection->GetInfectiousness();
            float prob_per_act =  m_mc_weight * infection->GetInfectiousness() * susceptibility->getModTransmit() * interventions->GetInterventionReducedTransmit();
            LOG_DEBUG_F( "prob_per_act based on infectiousness of %f = %f.\n", infection->GetInfectiousness(), prob_per_act );

            StrainIdentity tmp_strainIDs;
            infection->GetInfectiousStrainID(&tmp_strainIDs);

            if( prob_per_act )
            {
                // Loop over acts, modifying act probability as needed
                for (auto &act_prob_i : act_prob_vec)
                {
                    for( unsigned int act_idx = 0; act_idx < act_prob_i.num_acts; ++act_idx )
                    {
                        LOG_DEBUG_F( "Individual %d depositing contagion PROBABILITY %f into (relationship) transmission group.\n", GetSuid().data, prob_per_act );
                        parent->DepositFromIndividual( tmp_strainIDs, prob_per_act * act_prob_i.prob_per_act, &transmissionGroupMembership);
                    }
                }
            }
        }

        infectiousness *= susceptibility->getModTransmit() * interventions->GetInterventionReducedTransmit();

        // Restore network
        transmissionGroupMembership = transmissionGroupMembershipSave;
    }

    void IndividualHumanSTI::UpdateInfectiousness(float dt)
    {
        if( delay_between_adding_relationships_timer > 0.0f )
        {
            delay_between_adding_relationships_timer -= dt;
        }

        // DJK: Why only consummate relationships of males?  What about MSM?  Designate one "consummator" per rel?  <ERAD-1868>
        if( GetGender() == Gender::MALE )
        {
            for (auto relationship : relationships)
            {
                release_assert( relationship );
                relationship->Consummate( dt );
            }
        }
    }

    void IndividualHumanSTI::AcquireNewInfection( const IStrainIdentity *infstrain, int incubation_period_override )
    {
        int numInfs = int(infections.size());
        if( (numInfs >= IndividualHumanConfig::max_ind_inf) ||
            (!IndividualHumanConfig::superinfection && numInfs > 0 )
          )
        {
            return;
        }

        LOG_INFO_F( "(EEL) %s: %s\n", __FUNCTION__, toString().c_str() );

        // Would be nice to log the infecting relationship.

        // Let's just put some paranoid code to make sure that we are in relationship with an infected individual
        if( infstrain )
        {
            LOG_INFO_F( "(EEL) individual %lu infected by relationship partner %lu\n", GetSuid().data, infstrain->GetAntigenID() );
        }
        else
        {
            LOG_INFO_F( "(EEL) individual %lu infected, but relationship partner cannot be identified\n", GetSuid().data );
        }

        if( infstrain && infstrain->GetAntigenID() == 0 )
        {
            release_assert( infstrain );
            LOG_INFO_F( "(EEL) individual %lu infected by relationship partner %lu\n", GetSuid().data, infstrain->GetAntigenID() );
            if( infstrain->GetAntigenID() == 0 )
            {
                // Can't throw exception because there are even non-outbreak cases where this is valid (e.g., infecting relationship ended this time step
                LOG_DEBUG_F( "%s: individual %lu got infected but not in relationship with an infected individual (num relationships=%d). OK if PrevIncrease Outbreak.!!!\n",
                            __FUNCTION__, GetSuid().data, relationships.size() );
            }
        }

        IndividualHuman::AcquireNewInfection( infstrain, incubation_period_override );

        broadcaster->TriggerNodeEventObservers( GetEventContext(), EventTrigger::STINewInfection );
    }

    void
    IndividualHumanSTI::UpdateEligibility()
    {
        // DJK: Could return if pre-sexual-debut, related to <ERAD-1869>
        release_assert( p_sti_node );
        ISociety* society = p_sti_node->GetSociety();

        for( int type=0; type<RelationshipType::COUNT; type++ )
        {
            RelationshipType::Enum rel_type = RelationshipType::Enum(type);
            if( AvailableForRelationship( rel_type ) )
            {
                RiskGroup::Enum risk_group = IS_EXTRA_ALLOWED(rel_type) ? RiskGroup::HIGH : RiskGroup::LOW;
                society->GetStats(rel_type)->UpdateEligible(m_age, m_gender, risk_group, 1);    // DJK: Should use MC weight <ERAD-1870>
            }
        }
    }

    void
    IndividualHumanSTI::ConsiderRelationships(float dt)
    {
        release_assert( p_sti_node );
        ISociety* society = p_sti_node->GetSociety();

        float ellapsed_time = 0;
        bool is_any_available = false;
        bool available[RelationshipType::COUNT];
        float formation_rates[RelationshipType::COUNT];
        ZERO_ARRAY(formation_rates);
        float cumulative_rate = 0.0f;
        for( int type = 0; type < RelationshipType::COUNT; type++ )
        {
            available[type] = AvailableForRelationship( RelationshipType::Enum(type));
            is_any_available |= available[type];

            if( available[type] )
            {
                RelationshipType::Enum rel_type = RelationshipType::Enum(type);
                RiskGroup::Enum risk_group = IS_EXTRA_ALLOWED(rel_type) ? RiskGroup::HIGH : RiskGroup::LOW;
                // Formation rates will remain constant across the dt timestep
                formation_rates[type] = society->GetRates(rel_type)->GetRateForAgeAndSexAndRiskGroup(m_age, m_gender, risk_group);
                cumulative_rate += formation_rates[type] ;
            }
        }

        while( cumulative_rate > 0.0f && is_any_available && ellapsed_time < dt )
        {
            if (LOG_LEVEL(DEBUG))
            {
                std::stringstream ss;
                ss << __FUNCTION__ << "individual " << suid.data << " availability { ";
                for( int i = 0 ; i < RelationshipType::COUNT ; ++i )
                {
                    ss << available[i];
                    if( (i+1) < RelationshipType::COUNT )
                    {
                        ss << ", ";
                    }
                    else
                    {
                        ss << " ";
                    }
                }
                ss << "\n";
                LOG_DEBUG_F( ss.str().c_str() );
            }

            // At least one relationship could be formed
            // Advance total ellapsed time by time to next relationship
            ellapsed_time += randgen->expdist(cumulative_rate);
            if (ellapsed_time <= dt)
            {
                float random_draw = randgen->e() * cumulative_rate;
                float running_sum = 0.0f;
                int type=0;
                for (type = 0; type < RelationshipType::COUNT; type++)
                {
                    running_sum += formation_rates[type];
                    if (running_sum > random_draw)
                    {
                        break;
                    }
                }
                release_assert(type < RelationshipType::COUNT);

                LOG_DEBUG_F( "%s: individual %d joining %s PFA.\n", __FUNCTION__, suid.data, RelationshipType::pairs::lookup_key(type) );

                society->GetPFA(RelationshipType::Enum(type))->AddIndividual(this);
                ++queued_relationships[type] ;
            }

            // Update statistics for the next round
            is_any_available = false;
            cumulative_rate = 0.0f;
            for( int type = 0; type < RelationshipType::COUNT; type++ )
            {
                available[type] = AvailableForRelationship( RelationshipType::Enum(type));
                is_any_available |= available[type];

                if( available[type] )
                {
                    cumulative_rate += formation_rates[type] ;
                }
            }
        } // while ellapsed_time < dt
    }

    void
    IndividualHumanSTI::AddRelationship(
        IRelationship * pNewRelationship
    )
    {
        relationships.insert(pNewRelationship);

        LOG_DEBUG_F( "%s: calling UpdateGroupMembership: %s=>%s.\n",
                    __FUNCTION__, pNewRelationship->GetPropertyKey().c_str(), pNewRelationship->GetPropertyName().c_str() );

        if( relationship_properties.find( pNewRelationship->GetPropertyKey() ) != relationship_properties.end() )
        {
            std::ostringstream msg;
            msg << "Individual "
                << GetSuid().data
                << "found existing relationship in the same slot, key ("
                << pNewRelationship->GetPropertyKey().c_str()
                << "): "
                << relationship_properties[ pNewRelationship->GetPropertyKey() ]
                << std::endl;
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }
        relationship_properties[ pNewRelationship->GetPropertyKey() ] = pNewRelationship->GetSuid().data;

        RelationshipType::Enum relationship_type = pNewRelationship->GetType();

        --queued_relationships[relationship_type] ;
        ++active_relationships[relationship_type] ;

        // set slot bit
        uint64_t slot = GetOpenRelationshipSlot();
        uint64_t bitmask = uint64_t(1) << slot;
        relationshipSlots |= bitmask;
        release_assert(relationshipSlots <= SLOTS_FILLED);
        LOG_DEBUG_F( "%s: relationshipSlots = %d, individual = %d\n", __FUNCTION__, relationshipSlots, GetSuid().data );
        slot2RelationshipDebugMap[ slot ] = pNewRelationship->GetSuid().data;
        LOG_DEBUG_F( "%s: Individual %d gave slot %d to relationship %d\n", __FUNCTION__, GetSuid().data, slot, pNewRelationship->GetSuid().data );

        if(IndividualHumanSTIConfig::min_days_between_adding_relationships > 0)
        {
            delay_between_adding_relationships_timer = IndividualHumanSTIConfig::min_days_between_adding_relationships;
        }
        // DJK: Can these counters live elsewhere?  Either reporter or something parallel to interventions container, e.g. counters container
        num_lifetime_relationships[ int(relationship_type) ]++;
        last_6_month_relationships.push_back( std::make_pair( int( relationship_type ), float( parent->GetTime().time ) ) );
        last_12_month_relationships.push_back( std::make_pair( int( relationship_type ), float( parent->GetTime().time ) ) );

        suids::suid partner_id = pNewRelationship->GetPartnerId( GetSuid() );
        for( int itp = 0; itp < UNIQUE_PARTNER_TIME_PERIODS.size(); ++itp )
        {
            PartnerIdToRelEndTimeMap_t& r_partner_map = num_unique_partners[ itp ][ int( relationship_type ) ];
            r_partner_map[ partner_id ] = FLT_MAX;
        }

        broadcaster->TriggerNodeEventObservers(GetEventContext(), EventTrigger::EnteredRelationship);
    }

    void
    IndividualHumanSTI::RemoveRelationship(
        IRelationship * pRelationship
    )
    {
        LOG_DEBUG_F( "%s: individual %lu removing relationship: erase property: %s\n",
                     __FUNCTION__, GetSuid().data, pRelationship->GetPropertyName().c_str() );
        release_assert( relationships.find(pRelationship) != relationships.end() );

        relationships.erase( pRelationship );
        relationship_properties.erase( pRelationship->GetPropertyKey() );

        RelationshipType::Enum relationship_type = pRelationship->GetType();
        // These are unsigned quantities, so we'll loop for wrap-around.
        --active_relationships[relationship_type] ;

        uint64_t slot = pRelationship->GetSlotNumberForPartner( GetGender() == Gender::FEMALE );
        uint64_t bitmask = uint64_t(1) << slot;
        relationshipSlots &= (~bitmask);
        release_assert(relationshipSlots < SLOTS_FILLED);
        LOG_DEBUG_F( "%s: relationshipSlots = %d, individual=%d\n", __FUNCTION__, relationshipSlots, GetSuid().data );
        LOG_DEBUG_F( "%s: individual %d freed up slot %d for relationship %d\n", __FUNCTION__, GetSuid().data, slot, pRelationship->GetSuid().data );
        release_assert( slot2RelationshipDebugMap[ slot ] == pRelationship->GetSuid().data );
        slot2RelationshipDebugMap[ slot ] = -1;

        delay_between_adding_relationships_timer = 0.0f;

        suids::suid partner_id = pRelationship->GetPartnerId( GetSuid() );
        for( int itp = 0; itp < UNIQUE_PARTNER_TIME_PERIODS.size(); ++itp )
        {
            PartnerIdToRelEndTimeMap_t& r_partner_map = num_unique_partners[ itp ][ int( relationship_type ) ];
            r_partner_map[ partner_id ] = float( parent->GetTime().time );
        }

        broadcaster->TriggerNodeEventObservers( GetEventContext(), EventTrigger::ExitedRelationship );
    }

    bool
    IndividualHumanSTI::IsBehavioralSuperSpreader() const
    {
        return IS_SUPER_SPREADER();
    }

    unsigned int
    IndividualHumanSTI::GetExtrarelationalFlags() const
    {
        unsigned int bitmask = 0;
        for( int i = 0 ; i < RelationshipType::COUNT ; ++i )
        {
            bitmask |= EXTRA_RELATIONAL_ALLOWED(i);
        }
        unsigned int flags = promiscuity_flags & bitmask;
        return flags;
    }

    void
    IndividualHumanSTI::SetStiCoInfectionState()
    {
        has_other_sti_co_infection = true;
    }

    void
    IndividualHumanSTI::ClearStiCoInfectionState()
    {
        has_other_sti_co_infection = false;
    }

    bool
    IndividualHumanSTI::HasSTICoInfection()
    const
    {
        return has_other_sti_co_infection;
    }

    float 
    IndividualHumanSTI::GetCoInfectiveAcquisitionFactor()
    const
    {
        return has_other_sti_co_infection ? IndividualHumanSTIConfig::sti_coinfection_acq_mult : 1.0f;
    }

    float 
    IndividualHumanSTI::GetCoInfectiveTransmissionFactor()
    const
    {
        if( has_other_sti_co_infection )
        {
            return IndividualHumanSTIConfig::sti_coinfection_trans_mult;
        }
        else
        {
            return 1.0f;
        }
    }

    bool IndividualHumanSTI::IsCircumcised() const
    {
        return m_pSTIInterventionsContainer->IsCircumcised();
    }

    void
    IndividualHumanSTI::onEmigrating()
    {
        disengageFromSociety();
    }

    void
    IndividualHumanSTI::onImmigrating()
    {
        LOG_DEBUG_F( "%s()\n", __FUNCTION__ );
        migrating_because_of_partner = false;

        release_assert( p_sti_node );
        auto society = p_sti_node->GetSociety();
        auto manager = p_sti_node->GetRelationshipManager();

        // copy the set of pointers so we can iterate through one and delete from the other.
        RelationshipSet_t tmp_relationships = relationships;

        for( auto p_rel : tmp_relationships )
        {
            IRelationship* p_existing_rel = manager->Immigrate( p_rel );

            p_existing_rel->Resume( manager, society, this );

            // ------------------------------------------------------------------------
            // --- If the two relationship objects are not the same object, then either
            // --- we are resolving them due to serialization or due to the relationship
            // --- being paused (we clone the relationship when we pause it).
            // ------------------------------------------------------------------------
            if( p_rel != p_existing_rel )
            {
                relationships.erase( p_rel );
                relationships.insert( p_existing_rel );
                delete p_rel;
                p_rel = nullptr;
            }
        }
    }

    bool IndividualHumanSTI::AvailableForRelationship(RelationshipType::Enum relType) const
    {
        unsigned int effective_rels[ RelationshipType::COUNT ];
        bool no_relationships_or_allowed_extra = true;
        unsigned int total_rels = 0;
        for( int type = 0; type < RelationshipType::COUNT; type++ )
        {
            effective_rels[ type ] = active_relationships[type] + queued_relationships[type];
            total_rels += effective_rels[ type ];
            no_relationships_or_allowed_extra &= ( (effective_rels[type] == 0) || 
                                                   ( (effective_rels[type] > 0) && IS_EXTRA_ALLOWED(type)));
        }

        // --------------------------------------------------------------------
        // --- An individual is eligible for a relationship of this type if:
        // --- 1) Older than debut age
        // --- 2) Sufficient time has passed since last rel end
        // --- 3) No rels of this type, or extra-relational flag is present
        // --- 4) Num active plus queued rels is less than max
        // --------------------------------------------------------------------
        bool ret = ( (GetAge() >= sexual_debut_age) &&
                     (delay_between_adding_relationships_timer <= 0.0f) &&
                     no_relationships_or_allowed_extra &&
                     (effective_rels[relType] < max_relationships[relType]) &&
                     total_rels <= MAX_SLOTS
                   );


        return ret;
    }

    RelationshipSet_t&
    IndividualHumanSTI::GetRelationships()
    {
        return relationships;
    }

    RelationshipSet_t&
    IndividualHumanSTI::GetRelationshipsAtDeath()
    {
        return relationships_at_death;
    }

    // This method finds the lowest 0 in the relationshipSlots bitmask
    unsigned int
    IndividualHumanSTI::GetOpenRelationshipSlot()
    const
    {
        release_assert( relationshipSlots < SLOTS_FILLED );
        uint64_t bit = 1;
        for( unsigned int counter = 0 ; counter <= MAX_SLOTS ; ++counter )
        {
            if( (relationshipSlots & bit) == 0 )
            {
                LOG_DEBUG_F( "%s: Returning %d as first open slot for individual %d\n", __FUNCTION__, counter, suid.data );
                release_assert( counter <= MAX_RELATIONSHIPS_PER_INDIVIDUAL_ALL_TYPES );
                return counter;
            }
            bit <<= 1;
        }
        std::stringstream ss;
        ss << "Cannot be in more than " << MAX_SLOTS << " simultaneous relationship." ;
        throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
    }

    NaturalNumber
    IndividualHumanSTI::GetLast6MonthRels()
    const
    {
        return last_6_month_relationships.size();
    }

    NaturalNumber
    IndividualHumanSTI::GetLast6MonthRels( RelationshipType::Enum ofType )
    const
    {
        auto func = [ ofType ]( std::pair<int, float> p ) { return (ofType == p.first); };
        int num = std::count_if( last_6_month_relationships.begin(), last_6_month_relationships.end(), func );
        return num;
    }

    NaturalNumber
    IndividualHumanSTI::GetLast12MonthRels( RelationshipType::Enum ofType )
    const
    {
        auto func = [ ofType ]( std::pair<int, float> p ) { return (ofType == p.first); };
        int num = std::count_if( last_12_month_relationships.begin(), last_12_month_relationships.end(), func );
        return num;
    }

    NaturalNumber
    IndividualHumanSTI::GetNumUniquePartners( int itp, int irel )
    const
    {
        return num_unique_partners[ itp ][ irel ].size();
    }

    NaturalNumber
    IndividualHumanSTI::GetLifetimeRelationshipCount()
    const
    {
        unsigned int sum = 0;
        for( int type = 0; type < RelationshipType::COUNT; type++ )
        {
            sum += num_lifetime_relationships[type];
        }
        return sum;
    }

    NaturalNumber
    IndividualHumanSTI::GetLifetimeRelationshipCount( RelationshipType::Enum ofType )
    const
    {
        return num_lifetime_relationships[ ofType ];
    }

    NaturalNumber
    IndividualHumanSTI::GetNumRelationshipsAtDeath()
    const
    {
        return relationships_at_death.size();
    }

    float
    IndividualHumanSTI::GetDebutAge()
    const
    {
        return sexual_debut_age;
    }

    std::string
    IndividualHumanSTI::toString()
    const
    {
        std::ostringstream me;
        me << "id="
           << GetSuid().data
           << ",gender="
           << ( GetGender() == Gender::MALE ? "male" : "female" )
           << ",age="
           << GetAge()/DAYSPERYEAR
           << ",num_infections="
           << infections.size()
           << ",num_relationships="
           << relationships.size()
           << ",num_relationships_lifetime="
           << GetLifetimeRelationshipCount()
           << ",num_relationships_last_6_months="
           << last_6_month_relationships.size()
           << ",promiscuity_flags="
           << std::hex << static_cast<unsigned>(promiscuity_flags)
           ;
        return me.str();
    }

    void IndividualHumanSTI::SetContextTo( INodeContext* context )
    {
        LOG_DEBUG_F( "%s: individual %d setting context to 0x%08X.\n", __FUNCTION__, suid.data, context );
        IndividualHuman::SetContextTo(context);
        if( context == nullptr )
        {
            p_sti_node = nullptr;
        }
        else
        {
            if (parent->QueryInterface(GET_IID(INodeSTI), (void**)&p_sti_node) != s_OK)
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "INodeSTI", "INodeContext" );
            }
            release_assert( p_sti_node );
        }
    }

    void
    IndividualHumanSTI::CheckForMigration(
        float currenttime, float dt
    )
    {
        if( migrating_because_of_partner )
        {
            StateChange = HumanStateChange::Migrating;
        }
        else
        {
            IndividualHuman::CheckForMigration( currenttime, dt );
            if( StateChange == HumanStateChange::Migrating )
            {
                release_assert( p_sti_node );
                auto manager = p_sti_node->GetRelationshipManager();

                // NOTE: This for loop is done this way since terminate() will remove relationships
                // from the set.  If you use range-based loops, you will get an errlr.
                for( auto iterator = relationships.begin(); iterator != relationships.end(); /**/)
                {
                    auto rel = *iterator++;
                    RelationshipMigrationAction::Enum migration_action = rel->GetMigrationAction( GetNodeEventContext()->GetRng() );
                    if( migration_action == RelationshipMigrationAction::MIGRATE )
                    {
                        // -------------------------------
                        // --- Migrating the relationship 
                        // -------------------------------

                        IIndividualHumanSTI* ipartner = rel->GetPartner( this );
                        IndividualHumanSTI* partner = dynamic_cast<IndividualHumanSTI*>(ipartner);
                        release_assert( partner );

                        rel->Migrate();

                        partner->migrating_because_of_partner = true ;
                        partner->StateChange                  = HumanStateChange::Migrating;
                        partner->migration_destination        = this->migration_destination ;
                        partner->migration_type               = this->migration_type;
                        partner->migration_time_until_trip    = this->migration_time_until_trip;
                        partner->migration_time_at_destination= this->migration_time_at_destination;
                        partner->migration_will_return        = this->migration_will_return;
                        partner->migration_outbound           = this->migration_outbound;
                        partner->waypoints                    = this->waypoints;
                        partner->waypoints_trip_type          = this->waypoints_trip_type;

                        for( auto partner_iterator = partner->relationships.begin(); partner_iterator != partner->relationships.end(); /**/)
                        {
                            auto partner_rel = *partner_iterator++;
                            if( partner_rel != rel )
                            {
                                partner_rel->Terminate( RelationshipTerminationReason::PARTNER_MIGRATING );
                                delete partner_rel;
                            }
                        }
                    }
                    else if( migration_action == RelationshipMigrationAction::TERMINATE )
                    {
                        // -------------------------------------------------------------------
                        // --- Relationship ends permanently when the person leaves the Node.
                        // -------------------------------------------------------------------
                        rel->Terminate( RelationshipTerminationReason::SELF_MIGRATING );
                        delete rel;
                    }
                    else if( migration_action == RelationshipMigrationAction::PAUSE )
                    {
                        // --------------------------------------------------------------------------------
                        // --- More permanent relationships can last/stay open until the person comes back
                        // --------------------------------------------------------------------------------
                        rel->Pause( this );
                        manager->Emigrate( rel );
                    }
                    else
                    {
                        throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__, 
                            "migration_action", 
                            migration_action, 
                            RelationshipMigrationAction::pairs::lookup_key( migration_action ) );
                    }
                }
            }
        }
    }

    void IndividualHumanSTI::disengageFromSociety()
    {
        release_assert( p_sti_node );
        ISociety* society = p_sti_node->GetSociety();

        for( int type = 0; type < RelationshipType::COUNT; type++ )
        {
            if (queued_relationships[type] > 0)
            {
                LOG_DEBUG_F( "%s: individual %lu is in %s PFA - removing.\n", __FUNCTION__, suid.data, RelationshipType::pairs::lookup_key(type) );
                society->GetPFA(RelationshipType::Enum(type))->RemoveIndividual(this);
                queued_relationships[type] = 0;
            }
        }
    }

    ProbabilityNumber IndividualHumanSTI::getProbabilityUsingCondomThisAct( const IRelationshipParameters* pRelParams ) const
    {
        const Sigmoid& probs = m_pSTIInterventionsContainer->GetSTIBarrierProbabilitiesByRelType( pRelParams );
        float year = float(GetParent()->GetTime().Year());
        ProbabilityNumber prob = probs.variableWidthAndHeightSigmoid( year );
        //LOG_DEBUG_F( "%s: returning %f from Sigmoid::vWAHS( %f, %f, %f, %f, %f )\n", __FUNCTION__, (float) prob, year, probs.midyear, probs.rate, probs.early, probs.late );
        return prob;
    }

    void IndividualHumanSTI::ClearAssortivityIndexes()
    {
        for (int i = 0; i < RelationshipType::COUNT; ++i)
        {
            m_AssortivityIndex[i] = -1;
        }
    }

    int IndividualHumanSTI::GetAssortivityIndex( RelationshipType::Enum type ) const
    {
        return m_AssortivityIndex[ type ];
    }
    void IndividualHumanSTI::SetAssortivityIndex( RelationshipType::Enum type, int index )
    {
        m_AssortivityIndex[ type ] = index;
    }

    void IndividualHumanSTI::UpdateNumCoitalActs( uint32_t numActs )
    {
        unsigned int prev_total_coital_acts = m_TotalCoitalActs;

        m_TotalCoitalActs += numActs;

        if( (prev_total_coital_acts == 0) && (m_TotalCoitalActs > 0) )
        {
            broadcaster->TriggerNodeEventObservers( GetEventContext(), EventTrigger::FirstCoitalAct );
        }
    }

    uint32_t IndividualHumanSTI::GetTotalCoitalActs() const
    {
        return m_TotalCoitalActs;
    }

    REGISTER_SERIALIZABLE(IndividualHumanSTI);

    void serialize_relationships( IArchive& ar, RelationshipSet_t& rel_set )
    {
        size_t count = ar.IsWriter() ? rel_set.size() : -1;

        ar.startArray(count);
        if (ar.IsWriter())
        {
            for( IRelationship* p_rel : rel_set )
            {
                ar & p_rel;
            }
        }
        else
        {
            for (size_t i = 0; i < count; ++i)
            {
                IRelationship* p_rel = nullptr;
                ar & p_rel;
                rel_set.insert( p_rel );
            }
        }
        ar.endArray();
    }

    void serialize_pair_list( IArchive& ar, std::list < std::pair<int, float>>& pairList )
    {
        size_t count = ar.IsWriter() ? pairList.size() : -1;

        ar.startArray( count );
        if( ar.IsWriter() )
        {
            for( auto& entry : pairList )
            {
                int first = entry.first;
                float second = entry.second;
                ar.startObject();
                ar.labelElement( "first" ) & first;
                ar.labelElement( "second" ) & second;
                ar.endObject();
            }
        }
        else
        {
            for( size_t i = 0; i < count; ++i )
            {
                int first = 0;
                float second = 0.0;
                ar.startObject();
                ar.labelElement( "first" ) & first;
                ar.labelElement( "second" ) & second;
                ar.endObject();
                pairList.push_back( std::make_pair( first, second ) );
            }
        }
        ar.endArray();
    }

    void IndividualHumanSTI::serialize(IArchive& ar, IndividualHumanSTI* obj)
    {
        size_t rel_count = RelationshipType::COUNT;

        IndividualHuman::serialize( ar, obj );
        IndividualHumanSTI& human_sti = *obj;

        ar.labelElement("relationships"                           ); serialize_relationships( ar, human_sti.relationships );
        ar.labelElement("max_relationships"                       ); ar.serialize( human_sti.max_relationships,    rel_count );
        ar.labelElement("queued_relationships"                    ); ar.serialize( human_sti.queued_relationships, rel_count );
        ar.labelElement("active_relationships"                    ); ar.serialize( human_sti.active_relationships, rel_count );
        ar.labelElement("migrating_because_of_partner"            ) & human_sti.migrating_because_of_partner;
        ar.labelElement("promiscuity_flags"                       ) & human_sti.promiscuity_flags;
        ar.labelElement("sexual_debut_age"                        ) & human_sti.sexual_debut_age;
        ar.labelElement("co_infective_factor"                     ) & human_sti.co_infective_factor;
        ar.labelElement("has_other_sti_co_infection"              ) & human_sti.has_other_sti_co_infection;
        ar.labelElement("transmissionInterventionsDisabled"       ) & human_sti.transmissionInterventionsDisabled;
        ar.labelElement("relationshipSlots"                       ) & human_sti.relationshipSlots;
        ar.labelElement("delay_between_adding_relationships_timer") & human_sti.delay_between_adding_relationships_timer;
        ar.labelElement("potential_exposure_flag"                 ) & human_sti.potential_exposure_flag;
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // !!! Do not serialize relationships_at_death.  If the person is dead, we are just trying to get them
        // !!! home to clean up who is a resident.  The use of relationships_at_death should have been used already
        // !!! when the death event was broadcasted.  Hence, no one should need this later.
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        //ar.labelElement("relationships_at_death"                  ); serialize_relationships( ar, human_sti.relationships_at_death );
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        ar.labelElement("num_lifetime_relationships"              ); ar.serialize( human_sti.num_lifetime_relationships, rel_count );
        ar.labelElement("last_6_month_relationships"              ); serialize_pair_list( ar, human_sti.last_6_month_relationships );
        ar.labelElement("last_12_month_relationships"             ); serialize_pair_list( ar, human_sti.last_12_month_relationships );
        ar.labelElement("slot2RelationshipDebugMap"               ) & human_sti.slot2RelationshipDebugMap;
        ar.labelElement("m_AssortivityIndex"                      ); ar.serialize( human_sti.m_AssortivityIndex, rel_count );
        ar.labelElement("m_TotalCoitalActs"                       ) & human_sti.m_TotalCoitalActs;
        ar.labelElement("relationship_properties"                 ) & human_sti.relationship_properties;
        ar.labelElement("num_unique_partners"                     ) & human_sti.num_unique_partners;

        if( human_sti.m_pSTIInterventionsContainer == nullptr )
        {
            human_sti.m_pSTIInterventionsContainer = static_cast<STIInterventionsContainer*>(human_sti.interventions);
        }
    }
}
