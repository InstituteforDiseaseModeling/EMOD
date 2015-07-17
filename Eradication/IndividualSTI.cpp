/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"

#include <typeinfo>
#include "IndividualSTI.h"

#include "Debug.h"
#include "MathFunctions.h"
#include "IndividualEventContext.h"
#include "IIndividualHuman.h"
#include "InfectionSTI.h"
#include "NodeEventContext.h"
#include "Relationship.h"
#include "RelationshipGroups.h"
#include "SimulationConfig.h"
#include "SusceptibilitySTI.h"
#include "STIInterventionsContainer.h"
#include "NodeSTI.h"

static const char* _module = "IndividualSTI";

#define EXTRA_RELATIONAL_ALLOWED(rel)   (1 << (unsigned int)rel)
#define EXTRA_TRANSITORY_ALLOWED    (EXTRA_RELATIONAL_ALLOWED(Kernel::RelationshipType::TRANSITORY))
#define EXTRA_INFORMAL_ALLOWED    (EXTRA_RELATIONAL_ALLOWED(Kernel::RelationshipType::INFORMAL))
#define EXTRA_MARITAL_ALLOWED    (EXTRA_RELATIONAL_ALLOWED(Kernel::RelationshipType::MARITAL))
#define SUPER_SPREADER 0x8

#define IS_SUPER_SPREADER()             (promiscuity_flags & SUPER_SPREADER)
#define IS_EXTRA_TRANSITORY_ALLOWED()   (promiscuity_flags & EXTRA_TRANSITORY_ALLOWED)
#define IS_EXTRA_INFORMAL_ALLOWED()     (promiscuity_flags & EXTRA_INFORMAL_ALLOWED)
#define IS_EXTRA_MARITAL_ALLOWED()      (promiscuity_flags & EXTRA_MARITAL_ALLOWED)
#define IS_EXTRA_ALLOWED(rel)           (promiscuity_flags & EXTRA_RELATIONAL_ALLOWED((Kernel::RelationshipType::Enum)rel))
#define EXTRARELATIONAL_FLAGS()         (promiscuity_flags & (EXTRA_TRANSITORY_ALLOWED | EXTRA_INFORMAL_ALLOWED | EXTRA_MARITAL_ALLOWED))
#define SIX_MONTHS (6*IDEALDAYSPERMONTH)

namespace Kernel
{
    STINetworkParametersMap IndividualHumanSTIConfig::net_param_map;

    float IndividualHumanSTIConfig::debutAgeYrsMale_inv_kappa = 1.0f;
    float IndividualHumanSTIConfig::debutAgeYrsMale_lambda = 1.0f;
    float IndividualHumanSTIConfig::debutAgeYrsFemale_inv_kappa = 1.0f;
    float IndividualHumanSTIConfig::debutAgeYrsFemale_lambda = 1.0f;
    float IndividualHumanSTIConfig::debutAgeYrsMin = 13.0f;

    float IndividualHumanSTIConfig::sti_coinfection_mult = 0.10f;

    float IndividualHumanSTIConfig::circumcision_reduced_acquire = 0.60f;

    float IndividualHumanSTIConfig::min_days_between_adding_relationships = 60.0f;

    float IndividualHumanSTIConfig::condom_usage_probability_in_marital_relationships_midyear = 0.0f;
    float IndividualHumanSTIConfig::condom_usage_probability_in_marital_relationships_rate = 0.0f;
    float IndividualHumanSTIConfig::condom_usage_probability_in_marital_relationships_early = 0.0f;
    float IndividualHumanSTIConfig::condom_usage_probability_in_marital_relationships_late = 0.0f;
    float IndividualHumanSTIConfig::condom_usage_probability_in_informal_relationships_midyear = 0.0f;
    float IndividualHumanSTIConfig::condom_usage_probability_in_informal_relationships_rate = 0.0f;
    float IndividualHumanSTIConfig::condom_usage_probability_in_informal_relationships_early = 0.0f;
    float IndividualHumanSTIConfig::condom_usage_probability_in_informal_relationships_late = 0.0f;
    float IndividualHumanSTIConfig::condom_usage_probability_in_transitory_relationships_midyear = 0.0f;
    float IndividualHumanSTIConfig::condom_usage_probability_in_transitory_relationships_rate = 0.0f;
    float IndividualHumanSTIConfig::condom_usage_probability_in_transitory_relationships_early = 0.0f;
    float IndividualHumanSTIConfig::condom_usage_probability_in_transitory_relationships_late = 0.0f;
    float IndividualHumanSTIConfig::condom_transmission_blocking_probability = 0.0f;

    std::vector<float> IndividualHumanSTIConfig::maleToFemaleRelativeInfectivityAges;
    std::vector<float> IndividualHumanSTIConfig::maleToFemaleRelativeInfectivityMultipliers;

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

        initConfigTypeMap( "STI_Coinfection_Multiplier", &sti_coinfection_mult, STI_Coinfection_Multiplier_DESC_TEXT, 0.0f, 100.0f, 10.0f );

        initConfigTypeMap( "Circumcision_Reduced_Acquire", &circumcision_reduced_acquire, STI_Circumcision_Reduced_Acquire_DESC_TEXT, 0.0f, 1.0f, 0.60f );

        initConfigTypeMap( "Min_Days_Between_Adding_Relationships", &min_days_between_adding_relationships, STI_Min_Days_Between_Adding_Relationships_DESC_TEXT, 0.0f, 365.0f, 60.0f );

        initConfigTypeMap( "Male_To_Female_Relative_Infectivity_Ages", &maleToFemaleRelativeInfectivityAges, STI_Male_To_Female_Relative_Infectivity_Ages_DESC_TEXT, 0.0f, FLT_MAX, 0.0f );
        initConfigTypeMap( "Male_To_Female_Relative_Infectivity_Multipliers", &maleToFemaleRelativeInfectivityMultipliers, STI_Male_To_Female_Relative_Infectivity_Multipliers_DESC_TEXT, 0.0f, 25.0f, 1.0f );

        initConfigTypeMap( "Condom_Transmission_Blocking_Probability", &condom_transmission_blocking_probability, STI_Condom_Transmission_Blocking_Probability_DESC_TEXT, 0.0f, 1.0f, 0.9f );

        initConfigTypeMap( "Condom_Usage_Probability_in_Marital_Relationships_Early", &condom_usage_probability_in_marital_relationships_early, CUPMRE_TEXT, 0.0f, 1.0f, 0.5f );
        initConfigTypeMap( "Condom_Usage_Probability_in_Marital_Relationships_Late", &condom_usage_probability_in_marital_relationships_late, CUPMRL_TEXT, 0.0f, 1.0f, 0.5f );
        initConfigTypeMap( "Condom_Usage_Probability_in_Marital_Relationships_Midyear", &condom_usage_probability_in_marital_relationships_midyear, CUPMRM_TEXT, 0.0f, 100000, 365 );
        initConfigTypeMap( "Condom_Usage_Probability_in_Marital_Relationships_Rate", &condom_usage_probability_in_marital_relationships_rate, CUPMRR_TEXT, 0.0f, 10.0f, 1.0f );

        initConfigTypeMap( "Condom_Usage_Probability_in_Informal_Relationships_Early", &condom_usage_probability_in_informal_relationships_early, CUPIRE_TEXT, 0.0f, 1.0f, 0.5f );
        initConfigTypeMap( "Condom_Usage_Probability_in_Informal_Relationships_Late", &condom_usage_probability_in_informal_relationships_late, CUPIRL_TEXT, 0.0f, 1.0f, 0.5f );
        initConfigTypeMap( "Condom_Usage_Probability_in_Informal_Relationships_Midyear", &condom_usage_probability_in_informal_relationships_midyear, CUPIRM_TEXT, 0.0f, 100000.0f, 365 );
        initConfigTypeMap( "Condom_Usage_Probability_in_Informal_Relationships_Rate", &condom_usage_probability_in_informal_relationships_rate, CUPIRR_TEXT, 0.0f, 10.0f, 1.0f );
        
        initConfigTypeMap( "Condom_Usage_Probability_in_Transitory_Relationships_Early", &condom_usage_probability_in_transitory_relationships_early, CUPTRE_TEXT, 0.0f, 1.0f, 0.5f );
        initConfigTypeMap( "Condom_Usage_Probability_in_Transitory_Relationships_Late", &condom_usage_probability_in_transitory_relationships_late, CUPTRL_TEXT, 0.0f, 1.0f, 0.5f );
        initConfigTypeMap( "Condom_Usage_Probability_in_Transitory_Relationships_Midyear", &condom_usage_probability_in_transitory_relationships_midyear, CUPTRM_TEXT, 0.0f, 100000.0f, 365 );
        initConfigTypeMap( "Condom_Usage_Probability_in_Transitory_Relationships_Rate", &condom_usage_probability_in_transitory_relationships_rate, CUPTRR_TEXT, 0.0f, 10.0f, 1.0f ); 

        initConfigComplexType( "STI_Network_Params_By_Property", &net_param_map, STI_Network_Params_By_Property_DESC_TEXT );

        bool ret = JsonConfigurable::Configure( config );

        return ret;
    }

    bool RelationshipSetSorter::operator()(const IRelationship *rel1, const IRelationship *rel2) const
    {
        return rel1->GetId() < rel2->GetId();
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
        else if ( iid == GET_IID(IJsonSerializable)) 
            foundInterface = static_cast<IJsonSerializable*>(static_cast<IndividualHuman*>(this));
        else if ( iid == GET_IID(ISupports)) 
            foundInterface = static_cast<ISupports*>(static_cast<IIndividualHumanSTI*>(this));
        else if( IndividualHuman::QueryInterface( iid, (void**)&foundInterface ) != s_OK )
            foundInterface = 0;

        QueryResult status = e_NOINTERFACE;
        if ( foundInterface )
        {
            foundInterface->AddRef();
            status = s_OK;
        }

        *ppinstance = foundInterface;
        return status;
    }

#define MAX_RELATIONSHIPS_PER_INDIVIDUAL_ALL_TYPES (9)
    void IndividualHumanSTI::SetSTINetworkParams( const STINetworkParameters& rNewNetParams )
    {
        net_params = rNewNetParams ;

        // Update promiscuity flags.  Begin with a reset.
        if( IS_SUPER_SPREADER() )
            promiscuity_flags = SUPER_SPREADER;
        else
            promiscuity_flags = 0;

        promiscuity_flags |= GetProbExtraRelationalBitMask( (Gender::Enum)GetGender() );

        // Max allowable relationships
        NaturalNumber totalMax = 0;
        for( int rel = 0; rel < RelationshipType::COUNT; rel++)
        {
            float max_num = GetMaxNumRels( (Gender::Enum)GetGender(), (RelationshipType::Enum)rel);
            float intpart = 0.0;

            float fractpart = modff(max_num , &intpart);
            max_relationships[rel] = (int)intpart + ((randgen->e() < fractpart) ? 1 : 0);
            totalMax += max_relationships[rel];
        }
        if( totalMax > MAX_RELATIONSHIPS_PER_INDIVIDUAL_ALL_TYPES )
        {
            throw ConfigurationRangeException( __FILE__, __LINE__, __FUNCTION__, "<sum of max number of relationships across types>", totalMax, MAX_RELATIONSHIPS_PER_INDIVIDUAL_ALL_TYPES );
        }
    }

    void IndividualHumanSTI::UpdateSTINetworkParams(const char *prop, const char* new_value)
    {
        IIndividualHuman* individual = NULL;
        if( QueryInterface( GET_IID( IIndividualHuman ), (void**)&individual ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "this", "IIndividualHuman", "IndividualHumanSTI" );
        }

        const STINetworkParameters* p_new_params = net_param_map.GetParameters( individual, prop, new_value );
        if( p_new_params != nullptr )
        {
            SetSTINetworkParams( *p_new_params );
        }
    }

    IndividualHumanSTI *IndividualHumanSTI::CreateHuman(INodeContext *context, suids::suid id, float MCweight, float init_age, int gender, float init_poverty)
    {
        IndividualHumanSTI *newindividual = _new_ IndividualHumanSTI(id, MCweight, init_age, gender, init_poverty);

        newindividual->SetContextTo(context);
        LOG_DEBUG_F( "Created human with age=%f\n", newindividual->m_age );

        return newindividual;
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
        potential_exposure_flag = true;;
    }

    void IndividualHumanSTI::ExposeToInfectivity(float dt, const TransmissionGroupMembership_t* transmissionGroupMembership)
    {
        // call UPM if any relationships have been deposited to
        if( potential_exposure_flag )
        {
            UpdateGroupMembership(); // we "JIT" this function (just in time)
            LOG_DEBUG_F( "Exposing individual %d\n", GetSuid().data );
            release_assert( IsInfected() == false );
            parent->ExposeIndividual((IInfectable*)this, transmissionGroupMembership, dt);
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
                mult *= 1 - circumcision_reduced_acquire;
            }
        }
        else if( maleToFemaleRelativeInfectivityAges.size() > 0 )
        {
            // Individual is female. If user specified a male-to-female infectivity multiplier, 
            // we need to apply that to the probability of infection. The base infectivity
            // is for female to male. Male to female can be equal or greater.
            NonNegativeFloat multiplier = 0.0f;
            NonNegativeFloat age_in_yrs = GetAge()/DAYSPERYEAR;
            unsigned int idx = 0;
            while( idx < maleToFemaleRelativeInfectivityAges.size() &&
                   age_in_yrs > maleToFemaleRelativeInfectivityAges[ idx ] )
            {
                idx++;
            }
            // 3 possible cases
            if( idx == 0 )
            {
                multiplier = maleToFemaleRelativeInfectivityMultipliers[0];
                LOG_DEBUG_F( "Using value of %f for age-asymmetric infection multiplier for female age (in yrs) %f.\n", (float) multiplier, (float) age_in_yrs );
            }
            else if( idx == maleToFemaleRelativeInfectivityAges.size() )
            {
                multiplier = maleToFemaleRelativeInfectivityMultipliers.back();
                LOG_DEBUG_F( "Using value of %f for age-asymmetric infection multiplier for female age (in yrs) %f.\n", (float) multiplier, (float) age_in_yrs );
            }
            else 
            {
                // do some linear interp math.
                float left_age = maleToFemaleRelativeInfectivityAges[idx-1];
                float right_age = maleToFemaleRelativeInfectivityAges[idx];
                float left_mult = maleToFemaleRelativeInfectivityMultipliers[idx-1];
                float right_mult = maleToFemaleRelativeInfectivityMultipliers[idx];
                multiplier = left_mult + ( age_in_yrs - left_age ) / ( right_age - left_age ) * ( right_mult - left_mult );
                LOG_DEBUG_F( "Using interpolated value of %f for age-asymmetric infection multiplier for age (in yrs) %f.\n", (float) multiplier, (float) age_in_yrs );
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
                LOG_INFO_F( "prob = %f\n", (float) capped_prob );
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
            AcquireNewInfection(&strainId);
        }
    }

    void IndividualHumanSTI::ReportInfectionState()
    {
        //LOG_DEBUG_F( "Setting m_new_infection_state to NewInfection.\n" );
        m_new_infection_state = NewInfectionState::NewInfection;
    }

    IndividualHumanSTI::IndividualHumanSTI(suids::suid _suid, float monte_carlo_weight, float initial_age, int gender, float initial_poverty)
        : IndividualHuman(_suid, monte_carlo_weight, initial_age, gender, initial_poverty)
        , relationships()
        , promiscuity_flags(0)
        , sexual_debut_age(0.0f)
        , has_other_sti_co_infection(false)
        , transmissionInterventionsDisabled(false)
        , relationshipSlots(0)
        , delay_between_adding_relationships_timer(0.0f)
        , relationships_at_death()
        , num_lifetime_relationships(0)
        , last_6_month_relationships()
        , age_for_transitory_stats(-42.0f)
        , age_for_informal_stats(-42.0f)
        , age_for_marital_stats(-42.0f)
        , transitory_eligibility(0)
        , informal_eligibility(0)
        , marital_elibigility(0)
        , potential_exposure_flag(false)
        , net_params( "<NONE>" )
    {
        ZERO_ARRAY( queued_relationships );
        ZERO_ARRAY( active_relationships );
        ZERO_ARRAY( remote_relationships );

        // Sexual debut
        float min_age_sexual_debut_in_days = debutAgeYrsMin * DAYSPERYEAR;
        float debut_lambda = 0;
        float debut_inv_kappa = 0;
        if( GetGender() == Gender::MALE ) 
        {
            debut_inv_kappa = debutAgeYrsMale_inv_kappa;
            debut_lambda    = debutAgeYrsMale_lambda;
        }
        else
        {
            debut_inv_kappa = debutAgeYrsFemale_inv_kappa;
            debut_lambda    = debutAgeYrsFemale_lambda;
        }
        float debut_draw = (float)(DAYSPERYEAR * Environment::getInstance()->RNG->Weibull2( debut_lambda, debut_inv_kappa ) );
        sexual_debut_age = (std::max)(min_age_sexual_debut_in_days, debut_draw );

        // Promiscuity flags, including behavioral super-spreader
        auto draw = Environment::getInstance()->RNG->e();
        if( draw < GET_CONFIGURABLE(SimulationConfig)->prob_super_spreader )
        {
            promiscuity_flags |= SUPER_SPREADER;
        }

        LOG_DEBUG_F( "Individual ? will debut at age %f (yrs)f.\n", sexual_debut_age/DAYSPERYEAR );
    }

    void IndividualHumanSTI::Update(
        float currenttime,
        float dt
        )
    {
        bool was_pre_debut = m_age < sexual_debut_age;

        IndividualHuman::Update( currenttime, dt );

        // Check for debut
        bool is_post_debut = m_age >= sexual_debut_age;
        if (was_pre_debut && is_post_debut)
        {
            // Broadcast STIDebut
            INodeTriggeredInterventionConsumer* broadcaster = nullptr;
            if (parent->GetEventContext()->QueryInterface(GET_IID(INodeTriggeredInterventionConsumer), (void**)&broadcaster) != s_OK)
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, 
                                               "parent->GetEventContext()",
                                               "INodeTriggeredInterventionConsumer",
                                               "IIndividualHumanEventContext" );
            }
            broadcaster->TriggerNodeEventObservers( GetEventContext(), IndividualEventTriggerType::STIDebut );
        }

        auto now = parent->GetTime().time;
        while( last_6_month_relationships.size() && ( now - last_6_month_relationships.back() ) > SIX_MONTHS ) // 60*30 is my 6 months
        {
            last_6_month_relationships.pop_back();
        }
    }

    void IndividualHumanSTI::Die(
        HumanStateChange newState
    )
    {
        release_assert( (newState == HumanStateChange::DiedFromNaturalCauses) || (newState == HumanStateChange::KilledByInfection) );

        // Remove self from PFAs if queued up
        disengageFromSociety();

        // Remove self from relationships if active
        INodeSTI* sti_node = nullptr;
        if (parent->QueryInterface(GET_IID(INodeSTI), (void**)&sti_node) != s_OK)
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "INodeSTI", "Node*" );
        }
        auto manager = sti_node->GetRelationshipManager();

        for (auto iterator = relationships.begin(); iterator != relationships.end(); /**/ )
        {
            auto relationship = *iterator++;
            relationship->terminate( manager );
            relationships_at_death.insert( relationship );
            LOG_DEBUG_F("Relationship with %d terminated at time %f due to death\n", GetSuid().data, (float) GetParent()->GetTime().time );
        }

        IndividualHuman::Die( newState );
    }

    IndividualHumanSTI::~IndividualHumanSTI()
    {
        LOG_DEBUG_F( "%lu (STI) destructor.\n", this->GetSuid().data );

        for( auto p_rel : relationships_at_death )
        {
            delete p_rel ;
        }
        relationships_at_death.clear();
    }

    Infection* IndividualHumanSTI::createInfection( suids::suid _suid )
    {
        return InfectionSTI::CreateInfection(this, _suid);
    }

    bool
    IndividualHumanSTI::Configure(
        const Configuration* config
    )
    {
        IndividualHumanSTIConfig adamInfection;
        adamInfection.Configure( config );
        return true;
    }

    void IndividualHumanSTI::setupInterventionsContainer()
    {
        interventions = _new_ STIInterventionsContainer();
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
            float prob_per_act =  m_mc_weight * infection->GetInfectiousness() * susceptibility->GetModTransmit() * interventions->GetInterventionReducedTransmit();
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
                        parent->DepositFromIndividual(&tmp_strainIDs, prob_per_act * act_prob_i.prob_per_act, &transmissionGroupMembership);
                    }
                }
            }
        }

        infectiousness *= susceptibility->GetModTransmit() * interventions->GetInterventionReducedTransmit();
        
        // Restore network
        transmissionGroupMembership = transmissionGroupMembershipSave;
    }

    void IndividualHumanSTI::UpdateInfectiousness(float dt)
    {
        LOG_DEBUG_F( "%s: indiv=%d\n", __FUNCTION__, GetSuid().data );
        if( delay_between_adding_relationships_timer > 0 )
        {
            delay_between_adding_relationships_timer -= dt;
        }

        LOG_DEBUG_F( "Individual %lu Updating infectiousness on %d relationships\n", GetSuid().data, relationships.size() );
        if( !AtHome() )
        {
            return; // DJK BUG: No consummation while away! <ERAD-1904>
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

    void IndividualHumanSTI::AcquireNewInfection(StrainIdentity *infstrain, int incubation_period_override )
    {
        int numInfs = (int)infections.size();
        if( (numInfs >= max_ind_inf) ||
            (!superinfection && numInfs > 0 )
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

        //release_assert( good == true );
        return IndividualHuman::AcquireNewInfection( infstrain, incubation_period_override );
    }

    void
    IndividualHumanSTI::UpdateEligibility()
    {
        // DJK: Could return if pre-sexual-debut, related to <ERAD-1869>

        INodeSTI* sti_parent = nullptr;
        if (parent->QueryInterface(GET_IID(INodeSTI), (void**)&sti_parent) != s_OK)
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "INodeSTI", "INodeContext" );
        }
        else
        {
            ISociety* society = sti_parent->GetSociety();

            for( int type=0; type<RelationshipType::COUNT; type++ )
            {
                RelationshipType::Enum rel_type = (RelationshipType::Enum) type;
                if( AvailableForRelationship( rel_type ) )
                {
                    RiskGroup::Enum risk_group = IS_EXTRA_ALLOWED(rel_type) ? RiskGroup::HIGH : RiskGroup::LOW;
                    society->GetStats(rel_type)->UpdateEligible(m_age, m_gender, risk_group, 1);    // DJK: Should use MC weight <ERAD-1870>
                }
            }
        }
    }

    void
    IndividualHumanSTI::ConsiderRelationships(float dt)
    {
        INodeSTI* sti_parent = nullptr;
        if (parent->QueryInterface(GET_IID(INodeSTI), (void**)&sti_parent) != s_OK)
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "INodeSTI", "INodeContext" );
        }

        ISociety* society = sti_parent->GetSociety();

        bool is_any_available = false ;
        bool available[RelationshipType::COUNT];
        float formation_rates[RelationshipType::COUNT];  // TRANSITORY, INFORMAL, MARITAL
        ZERO_ARRAY(formation_rates);
        float cumulative_rate = 0.0f;
        for( int type = 0; type < RelationshipType::COUNT; type++ )
        {
            available[type] = AvailableForRelationship( (RelationshipType::Enum)type);
            is_any_available |= available[type] ;

            if( available[type] )
            {
                RelationshipType::Enum rel_type = (RelationshipType::Enum) type;
                RiskGroup::Enum risk_group = IS_EXTRA_ALLOWED(rel_type) ? RiskGroup::HIGH : RiskGroup::LOW;
                formation_rates[type] = society->GetRates(rel_type)->GetRateForAgeAndSexAndRiskGroup(m_age, m_gender, risk_group);
                cumulative_rate += formation_rates[type] ;
            }
        }

        if( is_any_available )
        {
            LOG_DEBUG_F( "%s: individual %d availability { %d, %d, %d }\n",
                         __FUNCTION__, 
                         suid.data, 
                         available[RelationshipType::TRANSITORY], 
                         available[RelationshipType::INFORMAL], 
                         available[RelationshipType::MARITAL] );

            // At least one relationship could be formed
            if (cumulative_rate > 0.0f)
            {
                float time_to_relationship = randgen->expdist(cumulative_rate);
                if (time_to_relationship <= dt)
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

                    society->GetPFA((RelationshipType::Enum)type)->AddIndividual(this);
                    ++queued_relationships[type] ;
                    release_assert( queued_relationships[type] == 1 );
                }
            }
        }
    }

    void
    IndividualHumanSTI::AddRelationship(
        IRelationship * pNewRelationship
    )
    {
        // In future we will allow simultaneous multiple relationships but for now not so much.
        if( relationships.size() > 0 )
        {
            LOG_DEBUG_F( "%s: individual %lu is already in %d relationships, adding another one!\n",
                         __FUNCTION__, GetSuid().data, relationships.size() );
        }

        relationships.insert( pNewRelationship );

        LOG_DEBUG_F( "%s: calling UpdateGroupMembership: %s=>%s.\n",
                    __FUNCTION__, pNewRelationship->GetPropertyKey().c_str(), pNewRelationship->GetPropertyName().c_str() );

        if( Properties.find( pNewRelationship->GetPropertyKey() ) != Properties.end() )
        {
            std::ostringstream msg;
            msg << "Individual "
                << GetSuid().data
                << "found existing relationship in the same slot, key ("
                << pNewRelationship->GetPropertyKey().c_str()
                << "): "
                << Properties[ pNewRelationship->GetPropertyKey() ].c_str()
                << std::endl;
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }
        Properties[ pNewRelationship->GetPropertyKey() ] = pNewRelationship->GetPropertyName();

        RelationshipType::Enum relationship_type = pNewRelationship->GetType();

        --queued_relationships[relationship_type] ;
        ++active_relationships[relationship_type] ;
        release_assert( queued_relationships[relationship_type] == 0 );

        // set slot bit
        auto slot = GetOpenRelationshipSlot();
        int bitmask = 1 << slot;
        relationshipSlots |= bitmask;
        release_assert(relationshipSlots < 0x400);
        LOG_DEBUG_F( "%s: relationshipSlots = %d, individual = %d\n", __FUNCTION__, relationshipSlots, GetSuid().data );
        slot2RelationshipDebugMap[ slot ] = pNewRelationship->GetId();
        LOG_DEBUG_F( "%s: Individual %d gave slot %d to relationship %d\n", __FUNCTION__, GetSuid().data, slot, pNewRelationship->GetId() );

        if( min_days_between_adding_relationships > 0)
        {
            delay_between_adding_relationships_timer = min_days_between_adding_relationships;
        }
        // DJK: Can these counters live elsewhere?  Either reporter or something parallel to interventions container, e.g. counters container
        num_lifetime_relationships++;
        last_6_month_relationships.push_back( (float) parent->GetTime().time );
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
        Properties.erase( pRelationship->GetPropertyKey() );

        RelationshipType::Enum relationship_type = pRelationship->GetType();
        // These are unsigned quantities, so we'll loop for wrap-around.
        --active_relationships[relationship_type] ;

        // DJK: We'll need more than 10! <ERAD-1874>
        // Note: this solution assumes max of 10 relationships, i.e., exactly 1 decimal digit for relationship number
        unsigned int slotIndex = strlen( "Relationship" );
        if( GetGender() == Gender::FEMALE ) // yes, assumes heterosexual relationship
        {
            slotIndex++;
        }

        auto slotNumber = atoi( pRelationship->GetPropertyKey().substr( slotIndex, 1 ).c_str() );
        int bitmask = 1 << slotNumber;
        relationshipSlots &= (~bitmask);
        release_assert(relationshipSlots < 0x400);
        LOG_DEBUG_F( "%s: relationshipSlots = %d, individual=%d\n", __FUNCTION__, relationshipSlots, GetSuid().data );
        LOG_DEBUG_F( "%s: individual %d freed up slot %d for relationship %d\n", __FUNCTION__, GetSuid().data, slotNumber, pRelationship->GetId() );
        release_assert( slot2RelationshipDebugMap[ slotNumber ] == pRelationship->GetId() );
        slot2RelationshipDebugMap[ slotNumber ] = -1;

        delay_between_adding_relationships_timer = 0.0f;
    }

    void IndividualHumanSTI::VacateRelationship( IRelationship* relationship )
    {
        LOG_DEBUG_F( "%s: individual %lu vacating relationship %d\n", __FUNCTION__, suid.data, relationship->GetId() );
        relationship->pause( this );
        release_assert( relationships.find( relationship ) != relationships.end() );
        relationships.erase( relationship );
        Properties.erase( relationship->GetPropertyKey() );
        RelationshipType::Enum relationship_type = relationship->GetType();

        // TODO - do we even need to note a ONS left behind?
        // If not, don't allow restoration in RestoreRelationship().
        --active_relationships[RelationshipType::TRANSITORY] ;
        release_assert( active_relationships[RelationshipType::TRANSITORY] < max_relationships[RelationshipType::TRANSITORY]);
        if( relationship_type != RelationshipType::TRANSITORY )
        {
            remote_relationships[relationship_type]++;
            release_assert( (active_relationships[relationship_type] + remote_relationships[relationship_type]) <= max_relationships[relationship_type] );
        }
    }

    void IndividualHumanSTI::RejoinRelationship( IRelationship* relationship )
    {
        LOG_DEBUG_F( "%s: individual %lu rejoining relationship %d\n", __FUNCTION__, suid.data, relationship->GetId() );
        release_assert( relationships.find( relationship ) == relationships.end() );

        relationships.insert( relationship );
        if( Properties.find( relationship->GetPropertyKey() ) != Properties.end() )
        {
            std::ostringstream msg;
            msg << "Individual "
                << GetSuid().data
                << "found existing relationship in the same slot, key ("
                << relationship->GetPropertyKey().c_str()
                << "): "
                << Properties[ relationship->GetPropertyKey() ].c_str()
                << std::endl;
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }
        Properties[ relationship->GetPropertyKey() ] = relationship->GetPropertyName();

        RelationshipType::Enum relationship_type = relationship->GetType();
        // TODO - should we expect to resume a ONS?
        // See PauseRelationship().

        if( relationship_type != RelationshipType::TRANSITORY )
        {
            // These are unsigned quantities, so we'll loop for wrap-around.
            --remote_relationships[relationship_type] ;
            release_assert( remote_relationships[relationship_type] < max_relationships[relationship_type] );
        }
        active_relationships[relationship_type]++;
        release_assert( (active_relationships[relationship_type] + remote_relationships[relationship_type]) <= max_relationships[relationship_type]);

        relationship->resume( this );
    }

    bool
    IndividualHumanSTI::IsBehavioralSuperSpreader() const
    {
        return IS_SUPER_SPREADER();
    }

    unsigned int
    IndividualHumanSTI::GetExtrarelationalFlags() const
    {
        return EXTRARELATIONAL_FLAGS();
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
    IndividualHumanSTI::GetCoInfectiveFactor()
    const
    {
        if( has_other_sti_co_infection )
        {
            return sti_coinfection_mult;
        }
        else
        {
            return 1.0f;
        }
    }

    bool IndividualHumanSTI::IsCircumcised() const
    {
        ISTICircumcisionConsumer *ic = NULL;
        if (s_OK != interventions->QueryInterface(GET_IID( ISTICircumcisionConsumer ), (void**)&ic) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "interventions", "ISTICircumcisionConsumer", "interventions" );
        }
        
        return ic->IsCircumcised();
    }

    void
    IndividualHumanSTI::onEmigrating()
    {
        LOG_DEBUG_F( "%s()\n", __FUNCTION__ );
        if (m_age >= sexual_debut_age)
        {
            LOG_DEBUG_F( "%s: individual %lu is of age.\n", __FUNCTION__, suid.data );
            disengageFromSociety();
        }
    }

    void
    IndividualHumanSTI::onImmigratingToNode()
    {
        LOG_DEBUG_F( "%s()\n", __FUNCTION__ );
        if (m_age >= sexual_debut_age)
        {
            // Do something here?
        }
    }

    bool IndividualHumanSTI::AvailableForRelationship(RelationshipType::Enum relType) const
    {
        unsigned int effective_rels[ RelationshipType::COUNT ];
        bool no_queued_relationships = true ;
        bool no_relationships_or_allowed_extra = true ;
        for( int type = 0; type < RelationshipType::COUNT; type++ )
        {
            effective_rels[ type ] = active_relationships[type] + remote_relationships[type];
            no_queued_relationships &= (queued_relationships[ type ] == 0) ;
            no_relationships_or_allowed_extra &= ( (effective_rels[ type ] == 0) || 
                                                   ( (effective_rels[ type ] > 0) && IS_EXTRA_ALLOWED(type))) ;
        }

        // --------------------------------------------------------------------
        // --- An individual can be queued for only one relationship at a time.
        // --- checking queued_..._relationships prevents individuals without
        // --- promiscuity flags set from becoming concurrent.
        // --------------------------------------------------------------------
        bool ret = ( (GetAge() >= sexual_debut_age) &&
                     (delay_between_adding_relationships_timer <= 0.0f) &&
                     no_queued_relationships &&
                     no_relationships_or_allowed_extra &&
                     (effective_rels[relType] < max_relationships[relType])
                   );

        if( GetOpenRelationshipSlot() > 9 && ret == true )
        {
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "Individual reporting available when all slots filled up." );
        }

        LOG_DEBUG_F( "%s: individual %d returning %d at time %.2f from node %d \n",
                     __FUNCTION__, GetSuid().data, ret, (float) GetParent()->GetTime().time, GetParent()->GetSuid().data );

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

    std::string
    promiscuity_flags_as_string( unsigned char byteIn )
    {
        //std::ostringstream retThis;
        char retThis[5];
        //fprintf( stdout, "promiscuity_flags=%x\n", byteIn );
        retThis[0] = ( ( byteIn & SUPER_SPREADER ) ? 'S' : '-' );
        retThis[1] = ( ( byteIn & EXTRA_MARITAL_ALLOWED ) ? 'M' : '-' );
        retThis[2] = ( ( byteIn & EXTRA_INFORMAL_ALLOWED ) ? 'I' : '-' );
        retThis[3] = ( ( byteIn & EXTRA_TRANSITORY_ALLOWED ) ? 'T' : '-' );
        retThis[4] = '\0';
        //fprintf( stdout, "promiscuity_flags=%s\n", retThis );
        return retThis;
    }

    // This method finds the lowest 0 in the relationshipSlots bitmask
    unsigned int
    IndividualHumanSTI::GetOpenRelationshipSlot()
    const
    {
        release_assert(relationshipSlots < 0x400);
        int bit = 1, counter = 0;
        for( ; bit > 0; bit <<= 1, counter++ )
        {
            if( (relationshipSlots & bit ) == 0 )
            {
                LOG_DEBUG_F( "%s: Returning %d as first open slot for individual %d\n", __FUNCTION__, counter, suid.data );
                release_assert( counter <= MAX_RELATIONSHIPS_PER_INDIVIDUAL_ALL_TYPES );
                return counter;
            }
        }
        throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "Cannot be in 32 relationships." );
    }

    NaturalNumber
    IndividualHumanSTI::GetLast6MonthRels()
    const
    {
        return last_6_month_relationships.size();
    }

    NaturalNumber
    IndividualHumanSTI::GetLifetimeRelationshipCount()
    const
    {
        return num_lifetime_relationships;
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
           << num_lifetime_relationships
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
        if (context)
        {
            onImmigratingToNode();
        }
    }

    void
    IndividualHumanSTI::CheckForMigration(
        float currenttime, float dt
    )
    {
        IndividualHuman::CheckForMigration( currenttime, dt );
/* clorton
        if( StateChange == HumanStateChange::Migrating )
        {
            // if existing Transitory Relationship skip this, else do base class.
            for( auto &rel : relationships )
            {
                if( rel->GetType() == RelationshipType::TRANSITORY )
                {
                    LOG_DEBUG_F( "Individual %d staying put because in a transitory relationship.\n", GetSuid().data );
                    StateChange = HumanStateChange::None;
                    return;
                }
            }
        }
clorton */
    }

    unsigned char IndividualHumanSTI::GetProbExtraRelationalBitMask(Gender::Enum gender)
    {
        unsigned char ret = 0;
        float prob_extrarelational = 0;
        float draw = 0;

        if( IS_SUPER_SPREADER() ) 
        {
            LOG_DEBUG_F("Individual %d is a super spreader, enabling all extra-relational flags\n", (int)(GetSuid().data));
            for( int rel = 0; rel < RelationshipType::COUNT; rel++ )
            {
                ret |= EXTRA_RELATIONAL_ALLOWED( rel );
            }
        }
        else
        {
            for( int rel = 0; rel < RelationshipType::COUNT; rel++ )
            {
                prob_extrarelational = net_params.prob_extra_relational[rel][gender];
                draw = Environment::getInstance()->RNG->e();

                if( draw < prob_extrarelational )
                {
                    LOG_DEBUG_F("extra %s allowed for %d\n", RelationshipType::pairs::lookup_key(rel), (int)(GetSuid().data));
                    ret |= EXTRA_RELATIONAL_ALLOWED(rel);
                }
                else if( net_params.extra_relational_flag_type != ExtraRelationalFlagType::Independent )
                {
                    return ret ;
                }
            }
        }
        return ret;
    }

    float IndividualHumanSTI::GetMaxNumRels(Gender::Enum gender, RelationshipType::Enum type)
    {
        return net_params.max_simultaneous_rels[type][gender];
    }

    void IndividualHumanSTI::disengageFromSociety()
    {
        INodeSTI* sti_node = nullptr;
        if (parent->QueryInterface(GET_IID(INodeSTI), (void**)&sti_node) != s_OK)
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "INodeSTI", "INodeContext" );
        }
        ISociety* society = sti_node->GetSociety();

        for( int type = 0; type < RelationshipType::COUNT; type++ )
        {
            if (queued_relationships[type] > 0)
            {
                LOG_DEBUG_F( "%s: individual %lu is in %s PFA - removing.\n", __FUNCTION__, suid.data, RelationshipType::pairs::lookup_key(type) );
                society->GetPFA((RelationshipType::Enum)type)->RemoveIndividual(this);
                --queued_relationships[type] ;
                release_assert( queued_relationships[type] == 0 );
            }
        }
    }
    
    ProbabilityNumber IndividualHumanSTI::getProbabilityUsingCondomThisAct( RelationshipType::Enum relationship_type ) const
    {
        ISTIBarrierConsumer* p_barrier = nullptr;
        if (interventions->QueryInterface(GET_IID(ISTIBarrierConsumer), (void**)&p_barrier) != s_OK)
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "interventions", "ISTIBarrierConsumer", "InterventionsContainer" );
        }
        auto probs = p_barrier->GetSTIBarrierProbabilitiesByRelType( relationship_type );
        float year = (float) GetParent()->GetTime().Year();
        ProbabilityNumber prob = Sigmoid::variableWidthAndHeightSigmoid( year, probs.midyear, probs.rate, probs.early, probs.late );
        //LOG_DEBUG_F( "%s: returning %f from Sigmoid::vWAHS( %f, %f, %f, %f, %f )\n", __FUNCTION__, (float) prob, year, probs.midyear, probs.rate, probs.early, probs.late );
        return prob;
    }


}


#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
#include "InfectionSTI.h"
#include "SusceptibilitySTI.h"
BOOST_CLASS_EXPORT(Kernel::IndividualHumanSTI)
namespace Kernel
{
    template void serialize( boost::mpi::packed_skeleton_oarchive & ar, IndividualHumanSTI& human, const unsigned int  file_version );

    template<class Archive>
    void serialize(Archive & ar, IndividualHumanSTI& human, const unsigned int  file_version )
    {
        // Register derived types
        ar.template register_type<InfectionSTI>();
        ar.template register_type<SusceptibilitySTI>();

        // Serialize fields - N/A
        ar & human.max_relationships;
        ar & human.queued_relationships;
        ar & human.active_relationships;
        ar & human.remote_relationships;
        ar & human.promiscuity_flags;
        ar & human.sexual_debut_age;
        //ar & human.coInfectiveFactor;
        ar & human.transmissionInterventionsDisabled;
        ar & human.relationshipSlots;
        ar & human.delay_between_adding_relationships_timer;
        ar & human.num_lifetime_relationships;
        ar & human.last_6_month_relationships;
        //ar & humna.slot2RelationshipDebugMap; // for debug only
        ar & human.age_for_transitory_stats;
        ar & human.age_for_informal_stats;
        ar & human.age_for_marital_stats;
        ar & human.transitory_eligibility;
        ar & human.informal_eligibility;
        ar & human.marital_elibigility;

        // Serialize base class
        ar & boost::serialization::base_object<Kernel::IndividualHuman>(human);
    }
}
#endif

