/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include <iomanip>
#include "InfectionHIV.h"

#include "Interventions.h"
#include "RANDOM.h"
#include "Exceptions.h"
#include "IndividualEventContext.h"
#include "IIndividualHumanHIV.h"
#include "HIVInterventionsContainer.h"
#include "IIndividualHuman.h"

#include "Types.h" // for ProbabilityNumber and NaturalNumber
#include "Debug.h" // for release_assert

SETUP_LOGGING( "InfectionHIV" )

namespace Kernel
{
#define INITIAL_VIRAL_LOAD (10000)

    // Initialization params here 
    float InfectionHIVConfig::acute_duration_in_months = 0.0f;
    float InfectionHIVConfig::AIDS_duration_in_months = 0.0f;
    float InfectionHIVConfig::acute_stage_infectivity_multiplier = 0.0f;
    float InfectionHIVConfig::AIDS_stage_infectivity_multiplier = 0.0f;
    float InfectionHIVConfig::ART_viral_suppression_multiplier = 0.0f;
    float InfectionHIVConfig::personal_infectivity_scale = 0.0f;
    float InfectionHIVConfig::personal_infectivity_median = 0.0f;
    float InfectionHIVConfig::max_CD4_cox = 0.0f;
    FerrandAgeDependentDistribution InfectionHIVConfig::mortality_distribution_by_age;

    GET_SCHEMA_STATIC_WRAPPER_IMPL(HIV.Infection,InfectionHIVConfig)
    BEGIN_QUERY_INTERFACE_BODY(InfectionHIVConfig)
    END_QUERY_INTERFACE_BODY(InfectionHIVConfig)

    BEGIN_QUERY_INTERFACE_BODY(InfectionHIV)
        HANDLE_INTERFACE(IInfectionHIV)
    END_QUERY_INTERFACE_BODY(InfectionHIV)

    bool
    InfectionHIVConfig::Configure(
        const Configuration * config
    )
    {
        //read in configs here   
        initConfigTypeMap( "Acute_Duration_In_Months", &acute_duration_in_months, Acute_Duration_In_Months_DESC_TEXT, 0.0f, 5.0f, 2.9f, "Simulation_Type", "HIV_SIM, TBHIV_SIM");
        initConfigTypeMap( "AIDS_Duration_In_Months", &AIDS_duration_in_months, AIDS_Duration_In_Months_DESC_TEXT, 7.0f, 12.0f, 9.0f, "Simulation_Type", "HIV_SIM, TBHIV_SIM");
        initConfigTypeMap( "Acute_Stage_Infectivity_Multiplier", &acute_stage_infectivity_multiplier, Acute_Stage_Infectivity_Multiplier_DESC_TEXT, 1.0f, 100.0f, 26.0f, "Simulation_Type", "HIV_SIM");
        initConfigTypeMap( "AIDS_Stage_Infectivity_Multiplier", &AIDS_stage_infectivity_multiplier, AIDS_Stage_Infectivity_Multiplier_DESC_TEXT, 1.0f, 100.0f, 10.0f, "Simulation_Type", "HIV_SIM");
        initConfigTypeMap( "ART_Viral_Suppression_Multiplier", &ART_viral_suppression_multiplier, ART_Viral_Suppression_Multiplier_DESC_TEXT, 0.0f, 1.0f, 0.08f, "Simulation_Type", "HIV_SIM");
        initConfigTypeMap( "Heterogeneous_Infectiousness_LogNormal_Scale", &personal_infectivity_scale, Heterogeneous_Infectiousness_LogNormal_Scale_DESC_TEXT, 0.0f, 10.0f, 0.0f, "Simulation_Type", "HIV_SIM" );
        initConfigTypeMap( "ART_CD4_at_Initiation_Saturating_Reduction_in_Mortality", &max_CD4_cox, ART_CD4_at_Initiation_Saturating_Reduction_in_Mortality_DESC_TEXT, 0.0f, FLT_MAX, 350.0f, "Simulation_Type", "HIV_SIM, TBHIV_SIM");

        bool ret = JsonConfigurable::Configure( config );
        if( ret || JsonConfigurable::_dryrun )
        {
            ret = mortality_distribution_by_age.Configure( config );
        }

        // median = exp(mu), where mu=-sigma^2/2
        personal_infectivity_median = exp(-InfectionHIVConfig::personal_infectivity_scale * InfectionHIVConfig::personal_infectivity_scale / 2.0);

        return ret ;
    }

    InfectionHIV *InfectionHIV::CreateInfection(IIndividualHumanContext *context, suids::suid _suid)
    {
        InfectionHIV *newinfection = _new_ InfectionHIV(context);
        newinfection->Initialize(_suid);

        return newinfection;
    }

    void InfectionHIV::Initialize(suids::suid _suid)
    {
        InfectionSTI::Initialize(_suid);

        if( s_OK != parent->QueryInterface(GET_IID(IIndividualHumanHIV), (void**)&hiv_parent) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "IIndividualHumanHIV", "IndividualHuman" );
        }

        // Initialization of HIV infection members, start with constant-ish values
        ViralLoad = INITIAL_VIRAL_LOAD;

        SetupNonSuppressedDiseaseTimers();

        // calculate individual infectivity multiplier based on Log-Normal draw.
        m_hetero_infectivity_multiplier = 1;
        if (InfectionHIVConfig::personal_infectivity_scale > 0) {
            m_hetero_infectivity_multiplier = Probability::getInstance()->fromDistribution(DistributionFunction::LOG_NORMAL_DURATION, 
                // First argument is the median = exp(mu), where mu=-sigma^2/2, second arg is scale
                InfectionHIVConfig::personal_infectivity_median, InfectionHIVConfig::personal_infectivity_scale  );
        }
        //m_hetero_infectivity_multiplier = Environment::getInstance()->RNG->Weibull2(InfectionHIVConfig::personal_infectivity_scale, InfectionHIVConfig::personal_infectivity_heterogeneity);

        LOG_DEBUG_F( "Individual %d just entered (started) HIV Acute stage, heterogeneity multiplier = %f.\n", parent->GetSuid().data, m_hetero_infectivity_multiplier );
    }

    void InfectionHIV::SetContextTo( IIndividualHumanContext* context )
    {
        InfectionSTI::SetContextTo( context );

        if( s_OK != parent->QueryInterface(GET_IID(IIndividualHumanHIV), (void**)&hiv_parent) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "IIndividualHumanHIV", "IndividualHuman" );
        }
    }

#define WHO_STAGE_KAPPA_0 (0.9664f)
#define WHO_STAGE_KAPPA_1 (0.9916f)
#define WHO_STAGE_KAPPA_2 (0.9356f)
#define WHO_STAGE_LAMBDA_0 (0.26596f)
#define WHO_STAGE_LAMBDA_1 (0.19729f)
#define WHO_STAGE_LAMBDA_2 (0.34721f)

    void
    InfectionHIV::SetupNonSuppressedDiseaseTimers()
    {
        try
        {
            // These two constants will both need to be configurable in next checkin.
            m_acute_duration    = InfectionHIVConfig::acute_duration_in_months * DAYSPERYEAR / float(MONTHSPERYEAR);
            m_aids_duration     = InfectionHIVConfig::AIDS_duration_in_months * DAYSPERYEAR / float(MONTHSPERYEAR);

            IIndividualHumanEventContext* HumanEventContextWhoKnowsItsAge  = parent->GetEventContext();
            float age_at_HIV_infection = HumanEventContextWhoKnowsItsAge->GetAge();
            // |------------|----------------|--------------*
            //   (acute)         (latent)        (aids)     (death)
            // Note that these two 'timers' are apparently identical at this point. Maybe HIV-xxx is redundant?
            HIV_duration_until_mortality_without_TB = InfectionHIVConfig::mortality_distribution_by_age.invcdf(randgen->e(), age_at_HIV_infection);
            //HIV_duration_until_mortality_without_TB /= 2; // test hack
            infectious_timer = HIV_duration_until_mortality_without_TB;
            NO_LESS_THAN( HIV_duration_until_mortality_without_TB, (float)DAYSPERWEEK ); // no less than 7 days prognosis
            NO_MORE_THAN( m_acute_duration, HIV_duration_until_mortality_without_TB ); // Acute stage time can't be longer than total prognosis!
            NO_MORE_THAN( m_aids_duration, HIV_duration_until_mortality_without_TB-m_acute_duration ); // AIDS can't be longer than prognosis - acute
            NO_LESS_THAN( m_aids_duration, 0.0f ); // can't be negative either!
            // Latent gets what's left
            m_latent_duration  = HIV_duration_until_mortality_without_TB - m_acute_duration - m_aids_duration;
            NO_LESS_THAN( m_latent_duration, 0.0f );  // days -- can't be negative
            LOG_DEBUG_F( "HIV Infection initialized for individual %d with prognosis %f, acute_duration %f, latent_duration %f, and aids_duration %f (all in years)\n", 
                         parent->GetSuid().data, HIV_duration_until_mortality_without_TB/DAYSPERYEAR, m_acute_duration/DAYSPERYEAR, m_latent_duration/DAYSPERYEAR, m_aids_duration/DAYSPERYEAR );

            // Pre-calculate for WHO stage: ///////////////////////////////////////////////////////////////
            float kappas [ NUM_WHO_STAGES-1 ]    = { WHO_STAGE_KAPPA_0, WHO_STAGE_KAPPA_1, WHO_STAGE_KAPPA_2 };
            float lambdas[ NUM_WHO_STAGES-1 ]    = { WHO_STAGE_LAMBDA_0, WHO_STAGE_LAMBDA_1, WHO_STAGE_LAMBDA_2 };

            float remaining_fraction = 1.0f;
            for(int stage=0; stage<NUM_WHO_STAGES-1; ++stage)      // NOTE: stage=0 indicates WHO stage 1 (and so forth)
            {
                // Note that these "fractions" do not add to 1.  In particular, any one value could be greater than 1, indicating that 
                // the individual never leaves this WHO stage.
                m_fraction_of_prognosis_spent_in_stage[ stage ] = Environment::getInstance()->RNG->Weibull(lambdas[stage], kappas[stage]); //  lambdas[stage]*pow(-log(1-randgen->e()), 1/kappas[stage]);
                remaining_fraction -= m_fraction_of_prognosis_spent_in_stage[ stage ];

                release_assert( m_fraction_of_prognosis_spent_in_stage[ stage ] != 0.0 ); // used as a divisor later so can't allow to be zero
            }
            //NO_LESS_THAN( remaining_fraction, 0 );

            m_fraction_of_prognosis_spent_in_stage[ NUM_WHO_STAGES-1 ] = std::max<float>( 0.0f, float(remaining_fraction) );

            std::ostringstream msg;
            for( int idx=0; idx<=NUM_WHO_STAGES-1; idx++ )
            {
                msg << "m_fraction_of_prognosis_spent_in_stage[" << idx << "] = " << m_fraction_of_prognosis_spent_in_stage[idx] << " - ";
            }
            msg << std::endl;
            LOG_DEBUG_F( msg.str().c_str() );
            duration = 0.0f;
            ///////////////////////////////////////////////////////////////////////////////////////////////
            // SCHEDULE NATURAL DISEASE PROGRESSION, TODO put parameters into config //////////////////////
            m_infection_stage = HIVInfectionStage::ACUTE;
        }
        catch( DetailedException &exc )
        {
            throw InitializationException( __FILE__, __LINE__, __FUNCTION__, exc.GetMsg() );
        }
    }

    void
    InfectionHIV::SetupSuppressedDiseaseTimers()
    {
        m_infection_stage = HIVInfectionStage::ON_ART;
        m_acute_duration  = INACTIVE_DURATION;
        m_latent_duration = INACTIVE_DURATION;
        m_aids_duration   = INACTIVE_DURATION;

        // store HIV (relative) prognosis just in case
        HIV_natural_duration_until_mortality = HIV_duration_until_mortality_without_TB;
        // Clear HIV natural duration so it's clear we're not using this
        HIV_duration_until_mortality_without_TB = INACTIVE_DURATION;
        // Set our new prognosis based on being on ART
        HIV_duration_until_mortality_with_viral_suppression = ComputeDurationFromEnrollmentToArtAidsDeath();
        // There are redundant variables that now need to be set also (TBD: let's use just 1 soon)
        total_duration = HIV_duration_until_mortality_with_viral_suppression;

        // This is risky. Reset our infection duration so other code works But we have lost our history.
        // Maybe store duration in another variable?
        duration = 0;
        LOG_DEBUG_F( "Replaced hiv natural mortality timer of %f with ART-based mortality timer of %f for individual %d\n",
                     HIV_natural_duration_until_mortality,
                     HIV_duration_until_mortality_with_viral_suppression,
                     parent->GetSuid().data
                   );
    }

    void InfectionHIV::SetParameters( IStrainIdentity* infstrain, int incubation_period_override)
    {
        LOG_DEBUG_F( "New HIV infection for individual %d; incubation_period_override = %d.\n", parent->GetSuid().data, incubation_period_override );
        // Don't call down into baseclass. Copied two lines below to repro required functionality.
        incubation_timer = incubation_period_override;
        CreateInfectionStrain(infstrain);

        if( incubation_period_override == 0 )
        {
            // This means we're part of an outbreak. Fastforward infection. 
            float fast_forward = HIV_duration_until_mortality_without_TB * randgen->e(); // keep this as member?
            duration += fast_forward;
            m_time_infected -= fast_forward;    // Move infection time backwards
            hiv_parent->GetHIVSusceptibility()->FastForward( this, fast_forward );
            LOG_DEBUG_F( "Individual is outbreak seed, fast forward infection by %f.\n", fast_forward );
            SetStageFromDuration();
        }
        total_duration = HIV_duration_until_mortality_without_TB;
        // now we have 3 variables doing the same thing?
        infectiousness = InfectionConfig::base_infectivity;
        StateChange    = InfectionStateChange::None;
    }

    void InfectionHIV::SetStageFromDuration()
    {
        // DJK*: This function updates stage rather than sets it.  
        //  E.g. if someone is in ACUTE or ON_ART stage, their stage is unchanged
        // DJK*: Note, this function _also_ sets StateChange to Fatal.


        LOG_DEBUG_F( "Individual %d, duration = %f, prognosis = %f, m_infection_stage = %d\n",
                     parent->GetSuid().data,
                     duration,
                     GetPrognosis()/DAYSPERYEAR,
                     m_infection_stage );

        if (duration >= GetPrognosis() )
        {
            StateChange = InfectionStateChange::Fatal;
            LOG_DEBUG_F( "Individual %d just died of HIV in stage %s with CD4 count of %f.\n", parent->GetSuid().data, HIVInfectionStage::pairs::lookup_key(m_infection_stage), hiv_parent->GetHIVSusceptibility()->GetCD4count() );
        }
        else if ( ( m_infection_stage == HIVInfectionStage::ACUTE || m_infection_stage == HIVInfectionStage::LATENT ) && duration >= m_acute_duration+m_latent_duration )
        {
            m_infection_stage = HIVInfectionStage::AIDS;
            LOG_DEBUG_F( "Individual %d just entered of HIV AIDS stage.\n", parent->GetSuid().data );
        }
        else if (m_infection_stage == HIVInfectionStage::ACUTE && duration >= m_acute_duration )
        {
            m_infection_stage = HIVInfectionStage::LATENT;
            LOG_DEBUG_F( "Individual %d just entered HIV Latent stage.\n", parent->GetSuid().data );
        }
    }

    void InfectionHIV::ApplySuppressionDropout()
    {
        LOG_DEBUG_F( "Looks like ART dropout for %d, recalc all timers for non-ART HIV progression.\n", parent->GetSuid().data );
        // draw new prognosis
        SetupNonSuppressedDiseaseTimers();
        // But we have to fast-forward based on current CD4. 
        //
        // Let's say our individual starts with CD4=550 and a prognosis of 15 years, with CD4=50 at death.
        // Then he drops to CD4=350 over time, then gets ART, then ART brings CD4 up to, say, 450 before drop out.
        // Then we draw a new prognosis, say 12 years. But we "fast-forward" that 12 years based on their current 
        // CD4 count of 450. And we figure out how much percentage to fast-forward based on the fact that 450 is 
        // 20% of the way from from 550 to 50, so we fast-forward 20% of 12 years or 2.4 years, for a resulting 
        // prognosis of 9.6 years.
        ProbabilityNumber fraction_prognosis_completed = hiv_parent->GetHIVSusceptibility()->GetPrognosisCompletedFraction();
        hiv_parent->GetHIVSusceptibility()->TerminateSuppression( GetPrognosis() * (1-fraction_prognosis_completed) );

        HIV_duration_until_mortality_without_TB *= (1.0 - fraction_prognosis_completed); // if 0 completed, prog is unchanged. If 1.0 completed, prog = 0.
        infectious_timer = HIV_duration_until_mortality_without_TB;
        total_duration = HIV_duration_until_mortality_without_TB;
        m_infection_stage = HIVInfectionStage::LATENT;
        m_acute_duration = 0;   // DJK*: With this, acute+latent+AIDS durations will not sum to prognosis!
        SetStageFromDuration();
        LOG_DEBUG_F( "Post-dropout prognosis for individual %d = %f\n", parent->GetSuid().data, HIV_duration_until_mortality_without_TB/DAYSPERYEAR );
    }

    void InfectionHIV::ApplySuppressionFailure()
    {
        LOG_DEBUG_F( "Looks like ART failure for %d, recalc timers for AIDS-stage progression.\n", parent->GetSuid().data );
        // Should be just 9 months to live. Timers shouldn't need to be reset.
        m_infection_stage = HIVInfectionStage::AIDS;
    }

    void InfectionHIV::Update(float dt, ISusceptibilityContext* immunity)
    {
        InfectionSTI::Update(dt, immunity);

        SetStageFromDuration();

        // update viral load per timestep
        ViralLoad = INITIAL_VIRAL_LOAD; 

        ApplyDrugEffects(dt, immunity);

        // TODO: there aren't any strains yet, but any differences between strains 
        // (especially drug resistance) can be handled this way
        // EvolveStrain(immunity, dt);

        // Make sure we didn't accidentally clear.  Remove this line for sterilizing cure.
        release_assert( StateChange != InfectionStateChange::Cleared );
    }

    float
    InfectionHIV::GetInfectiousness() const
    {
        float retInf = InfectionSTI::GetInfectiousness();
        LOG_DEBUG_F( "infectiousness from STI = %f\n", retInf );
        // TBD: a STATE is not an EVENT. Split up this enum.
        if( m_infection_stage == HIVInfectionStage::ACUTE )
        {
            retInf *= InfectionHIVConfig::acute_stage_infectivity_multiplier; //  26.0f;
        }
        else if( m_infection_stage == HIVInfectionStage::AIDS )
        {
            retInf *= InfectionHIVConfig::AIDS_stage_infectivity_multiplier;
        }

        // ART reduces infectivity, but we don't want to put ART-specific knowledge and code in the infection object.
        // Prefer instead to apply the _effects_ of art, and prefer to keep all ART knowledge encapsulated inside
        // the HIVInterventionsContainer.
        // TBD: Use QI instead of cast
        IHIVInterventionsContainer * pHIC = nullptr;
        if ( s_OK != parent->GetInterventionsContext()->QueryInterface(GET_IID(IHIVInterventionsContainer), (void**)&pHIC) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent->GetInterventionsContext()", "IHIVInterventionsContainer", "IIndividualHumanInterventionsContext" );
        }
        retInf *= pHIC->GetInfectivitySuppression();
        retInf *= m_hetero_infectivity_multiplier;
        
        LOG_DEBUG_F( "infectiousness from HIV = %f\n", retInf );
        return retInf;
    }

    NaturalNumber InfectionHIV::GetViralLoad()
    const
    {
        return ViralLoad;
    }

    float
    InfectionHIV::GetPrognosis()
    const
    {
        // These may be the WRONG tests. Timers can be sub-zero but really one should die immediately after that update occurs.
        // Works now but possibly very sensitive to code flow that could change.
        if( HIV_duration_until_mortality_without_TB >= 0.0f )
        {
            return HIV_duration_until_mortality_without_TB;
        }
        else if( HIV_duration_until_mortality_with_viral_suppression >= 0.0f )
        {
            return HIV_duration_until_mortality_with_viral_suppression;
        }
        else
        {
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "No valid mortality timers!" );
        }
    }


    float InfectionHIV::GetTimeInfected() const
    {
        return m_time_infected;
    }

    float InfectionHIV::GetDaysTillDeath() const
    {
        return GetPrognosis() - duration;
    }

    const HIVInfectionStage::Enum&
    InfectionHIV::GetStage()
    const
    {
        return m_infection_stage;
    }

    bool InfectionHIV::ApplyDrugEffects(float dt, ISusceptibilityContext* immunity)
    {
        // Check for valid inputs
        if (dt <= 0 || !immunity) 
        {
            LOG_WARN("ApplyDrugEffects takes the following arguments: time step (dt), Susceptibility (immunity)\n");
            return false;
        }

        // Query for drug effects
        IIndividualHumanContext *patient = GetParent();
        IHIVDrugEffects *ihivde = nullptr;

        if (!patient->GetInterventionsContext()) 
        {
            if ( s_OK != patient->GetInterventionsContextbyInfection(this)->QueryInterface(GET_IID(IHIVDrugEffects), (void**)&ihivde) )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent->GetInterventionsContextbyInfection()", "IHIVDrugEffects", "IIndividualHumanInterventionsContext" );
            }
        }
        else
        {
            if ( s_OK != patient->GetInterventionsContext()->QueryInterface(GET_IID(IHIVDrugEffects), (void **)&ihivde) )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent->GetInterventionsContext()", "IHIVDrugEffects", "IIndividualHumanInterventionsContext" );
            }
        }

        // float HIV_drug_inactivation_rate = ihivde->GetDrugInactivationRate(); //no action for Drug Inactivation for now
        float HIV_drug_clearance_rate    = ihivde->GetDrugClearanceRate();

        if ( HIV_drug_clearance_rate > 0 && randgen->e() < dt * HIV_drug_clearance_rate)
        {
            // Infection cleared. InfectionStateChange reporting (in SetNewInfectionState) is TB specific, but this allows us to delete the HIV infections if we get drugs and are cleared
            StateChange = InfectionStateChange::Cleared;
            LOG_DEBUG("HIV drugs cured my HIV infection \n");
            return true;
        }

        // Otherwise drug did not have an effect in this timestep
        return false;
    }

    InfectionHIV::~InfectionHIV(void) { }
    InfectionHIV::InfectionHIV()
        : ViralLoad(0)
        , HIV_duration_until_mortality_without_TB(INACTIVE_DURATION )
        , HIV_natural_duration_until_mortality(INACTIVE_DURATION ) // need some value that says Ignore-Me
        , HIV_duration_until_mortality_with_viral_suppression(INACTIVE_DURATION )
        , m_time_infected ( 0 )
        , hiv_parent( nullptr )
    {
    }

    InfectionHIV::InfectionHIV(IIndividualHumanContext *context)
        : InfectionSTI(context)
        , HIV_duration_until_mortality_without_TB(INACTIVE_DURATION )
        , HIV_natural_duration_until_mortality(INACTIVE_DURATION ) // need some value that says Ignore-Me
        , HIV_duration_until_mortality_with_viral_suppression(INACTIVE_DURATION )
        , hiv_parent( nullptr )
    {
        // TODO: Consider moving m_time_infected to Infection layer.  Alternatively, track only in reporters.
        IIndividualHuman *human_parent;
        if( s_OK != parent->QueryInterface(GET_IID(IIndividualHuman), (void**)&human_parent) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "IIndividualHuman", "IIndividualHumanContext" );
        }
        m_time_infected = human_parent->GetParent()->GetTime().time;
    }

    float
    InfectionHIV::GetWHOStage()
    const
    {
        // DJK TODO: Needs to return float that interpolates stages, e.g. for weight.  partial not used. <ERAD-1860>
        float ret = MAX_WHO_HIV_STAGE-1;
        double fractionOfPrognosisCompleted = hiv_parent->GetHIVSusceptibility()->GetPrognosisCompletedFraction();
        double cum_stage = 0;        // Could be greater than one
        double partial = 0;
        // |__________________|
        //   1  ^ 2 ^  3 ^  4
        for( NaturalNumber stage=MIN_WHO_HIV_STAGE; // from 1
             stage < MAX_WHO_HIV_STAGE-1; // to 3
             stage++)
        {
            NaturalNumber stage_idx = stage-1;
            cum_stage += m_fraction_of_prognosis_spent_in_stage[ stage_idx ];
            if( cum_stage >= fractionOfPrognosisCompleted )
            {
                auto partial_raw = ( fractionOfPrognosisCompleted - ( cum_stage - m_fraction_of_prognosis_spent_in_stage[ stage_idx ] ) ) /m_fraction_of_prognosis_spent_in_stage[ stage_idx ];
                if( partial_raw > 1.0f )
                {
                    std::ostringstream msg;
                    msg << "partial_raw calculated to greater than 1.0f: " << std::setprecision(12) << partial_raw << " with divisor " << m_fraction_of_prognosis_spent_in_stage[ stage_idx ] << std::endl;
                    LOG_WARN_F( msg.str().c_str() );
                    partial_raw = 1.0f;
                }
                partial = partial_raw;
                ret = stage; // max 2
                break;
            }
            release_assert( int(stage_idx) < NUM_WHO_STAGES-1 );
            //LOG_DEBUG_F( "cum_stage now = %f after adding %f because fraction_completed = %f, stage_idx = %d\n", (float) cum_stage, (float) m_fraction_of_prognosis_spent_in_stage[ stage_idx ], (float) fractionOfPrognosisCompleted, (int) stage_idx );
        }
        
        ret += partial;

#if 0
        std::ostringstream msg;
        msg << "Looks like we are in WHO stage "
            << (int) ret 
            << " with remainder "
            << (float) partial
            << " based on prognosis fraction of "
            << (float) fractionOfPrognosisCompleted
            << " and stage fractions/cum fractions of: ";
        float cum = 0.0f;
        for( int idx=0; idx<3; idx++ )
        {
            msg << m_fraction_of_prognosis_spent_in_stage[idx] << "/";
            cum += m_fraction_of_prognosis_spent_in_stage[idx];
            msg << cum << " -- ";
        }
        msg << std::endl;
        //LOG_DEBUG_F( "Looks like we are in WHO stage %d with remainder %f based on prognosis fraction of %f.\n", (int) stage, (float) partial, (float) fractionOfPrognosisCompleted );
        LOG_DEBUG_F( msg.str().c_str() );
#endif
        LOG_DEBUG_F( "%s returning %f for individual %d\n", __FUNCTION__, ret, parent->GetSuid().data );
        return ret;
    }

    // static: doesn't use any member variables, just lives in this class for convenience.
    // Purpose: This function converts a WHO HIV stage (1-4 really, but 1.0-4.99 for us)
    // into a human body weight in kg. It does a linear interpolation on the hardcoded array.
    float
    InfectionHIV::GetWeightInKgFromWHOStage(
        float whoStage
    )
    {
        if( whoStage < MIN_WHO_HIV_STAGE || whoStage > MAX_WHO_HIV_STAGE)
        {
            throw OutOfRangeException( __FILE__, __LINE__, __FUNCTION__, "whoStage", whoStage, ( whoStage < MIN_WHO_HIV_STAGE ? MIN_WHO_HIV_STAGE : MAX_WHO_HIV_STAGE ) );
        }

        // All hardcoded here because these are the weight boundaries for WHO HIV stages.
        // !!! SEE BELOW FOR EXPLANATION !!!
        float weights[] = { 65.0f, 62.1f, 57.0f, 50.0f, 40.1f }; // Kg

        int left  = int(floor(whoStage))-1;
        int right = int(ceil(whoStage))-1;

        float weightInKilograms = weights[ left ] + (weights[ right ] - weights[ left ]) * (whoStage - floor(whoStage));
        LOG_DEBUG_F( "%s returning %f for stage %f\n", __FUNCTION__, weightInKilograms, whoStage );
        return weightInKilograms;
    }

    // -------------------------------------------------------------------------------------------------------
    // !!! WEIGHTS EXPLANATION - Values used above. !!! 5/14/2015 AB
    // The goal of the weight model was to combine information about WHO guideline and clinical progression 
    // in order to estimate body weight over the course of untreated HIV infection. This in turn is used to 
    // estimate body weight at treatment initiation, which is used to compute survivorship based to reflect
    // that of the IeDEA cohorts. The WHO guidelines for clinical staging (revised 2005) specify that WHO 
    // stage I/II is defined as <10% loss in body weight and WHO stage III/IV is a >10% loss in body weight. 
    // Thus, we placed the transition between stages II and III at approximately 10% loss of body weight. 
    // IeDEA uses absolute weight rather than percentage change from pre-HIV baseline or BMI, because 
    // pre-HIV weight and height are often unavailable in patient records. We therefore assumed an intial
    // weight of 65kg. Clinically, we know that weight loss during AIDS is not linear: in Stage I/II there 
    // is usually relatively slow weight loss, whereas in Stage IV weight loss can be rapid due to the 
    // compounding effects of malabsorption, difficulty eating/loss of appetite, the metabolic demands of
    // opportunistic infections and viral replication. This is why the rate of decline in weight increases
    // as the clinical stages advance.
    // -------------------------------------------------------------------------------------------------------


#define IEDEA_KAPPA (0.34f)
#define IEDEA_LAMBDA_BASE_IN_YRS (123.83f)
#define IEDEA_FEMALE_MULTIPLIER (0.6775f)
#define AGE_40_YRS (40.0f*DAYSPERYEAR)
#define IEDEA_OVER_AGE_40Y_MULTIPLIER (1.4309f)
#define WHO_STAGE_THRESHOLD_FOR_COX (3)
#define IEDEA_STAGE_3_PLUS_MULTIPLIER (2.7142f)
#define COX_PROP_CONSTANT_1 (-0.00758256281931556)
#define COX_PROP_CONSTANT_2 (0.282851687024819)
#define COX_PROP_CONSTANT_3 (-0.0731529900006081)
#define COX_PROP_CONSTANT_4 (3.05043211490958)

    // This is an implementation of the Cox Proportional Model (e.g., http://en.wikipedia.org/wiki/Cox_proportional_hazards_model)
    // The values are taken from the IeDEA website (http://www.iedea-sa.org/index.php?id=2856)
    float
    InfectionHIV::ComputeDurationFromEnrollmentToArtAidsDeath()
    const
    {
        float whoStageContinuous = GetWHOStage();
        float weight = GetWeightInKgFromWHOStage(whoStageContinuous);
        // presumably it's at enrollment which is now, why would we be doing this calc any other time?
        float cd4AtArtEnrollment = hiv_parent->GetHIVSusceptibility()->GetCD4count(); 
        //release_assert( cd4AtArtEnrollment > 30.0f );
        if( cd4AtArtEnrollment < 30.0f )
        {
            LOG_WARN_F( "Individual %d had low CD4 at ART enrollment: %f\n", parent->GetSuid().data, cd4AtArtEnrollment );
        }
        //release_assert( cd4AtArtEnrollment < 850.0f );
        if( cd4AtArtEnrollment > 850.0f )
        {
            LOG_WARN_F( "Individual %d had high CD4 at ART enrollment: %f\n", parent->GetSuid().data, cd4AtArtEnrollment );
        }
        float multiplier = float(exp( COX_PROP_CONSTANT_1 * min(InfectionHIVConfig::max_CD4_cox, cd4AtArtEnrollment) + COX_PROP_CONSTANT_2)); // Stop at 350
        release_assert( multiplier > 0.0f );
        //release_assert( multiplier < 10.0f );

        // have to cast to get gender?!
        if( dynamic_cast<IIndividualHuman*>(parent)->GetGender() == Gender::FEMALE)
        {
            multiplier *= IEDEA_FEMALE_MULTIPLIER;
        }

        /*float hostAge = infectionInternals->GetCommunityCharacteristics()->UseFakeAgeForIntrahost() ? 
                        infectionInternals->GetHost()->FakeAge() : infectionInternals->GetHost()->Age();*/
        if (dynamic_cast<IIndividualHuman*>(parent)->GetAge() > AGE_40_YRS)
        {
            multiplier *= IEDEA_OVER_AGE_40Y_MULTIPLIER;
        }

        if (whoStageContinuous >= WHO_STAGE_THRESHOLD_FOR_COX)
        {
            multiplier *= IEDEA_STAGE_3_PLUS_MULTIPLIER;
        }

        multiplier *= float(exp( COX_PROP_CONSTANT_3 * weight + COX_PROP_CONSTANT_4));
        LOG_DEBUG_F( "multiplier = %f\n", multiplier );
        float lambda_divisor = pow(multiplier, 1.0f/IEDEA_KAPPA);

        float lambda_corrected = IEDEA_LAMBDA_BASE_IN_YRS / lambda_divisor;
        float ret = DAYSPERYEAR * lambda_corrected * pow(-log(1.0f-Environment::getInstance()->RNG->e() ), 1.0f/IEDEA_KAPPA);
        // This might be a little too verbose for production.
        LOG_DEBUG_F( "%s returning %f from WHO stage of %f, age of %f, weight of %f, and gender %s: multiplier was %f, lambda divisor = %f, lambda (actual) %f.\n",
                     __FUNCTION__,
                     ret,
                     whoStageContinuous,
                     dynamic_cast<IIndividualHuman*>(parent)->GetAge()/DAYSPERYEAR,
                     weight,
                     ( dynamic_cast<IIndividualHuman*>(parent)->GetGender()==Gender::MALE ? "MALE" : "FEMALE" ),
                     multiplier,
                     lambda_divisor,
                     lambda_corrected
                   );
        return ret;
    }

    REGISTER_SERIALIZABLE(InfectionHIV);

    void InfectionHIV::serialize(IArchive& ar, InfectionHIV* obj)
    {
        InfectionSTI::serialize( ar, obj );
        InfectionHIV& inf_hiv = *obj;
        ar.labelElement("ViralLoad"                                          ) & inf_hiv.ViralLoad;
        ar.labelElement("HIV_duration_until_mortality_without_TB"            ) & inf_hiv.HIV_duration_until_mortality_without_TB;
        ar.labelElement("HIV_natural_duration_until_mortality"               ) & inf_hiv.HIV_natural_duration_until_mortality;
        ar.labelElement("HIV_duration_until_mortality_with_viral_suppression") & inf_hiv.HIV_duration_until_mortality_with_viral_suppression;
        ar.labelElement("m_time_infected"                                    ) & inf_hiv.m_time_infected;
        ar.labelElement("prognosis_timer"                                    ) & inf_hiv.prognosis_timer;
        ar.labelElement("m_infection_stage"                                  ) & (uint32_t&)inf_hiv.m_infection_stage;
        ar.labelElement("m_fraction_of_prognosis_spent_in_stage"             ); ar.serialize( inf_hiv.m_fraction_of_prognosis_spent_in_stage, NUM_WHO_STAGES );
        ar.labelElement("m_acute_duration"                                   ) & inf_hiv.m_acute_duration;
        ar.labelElement("m_latent_duration"                                  ) & inf_hiv.m_latent_duration;
        ar.labelElement("m_aids_duration"                                    ) & inf_hiv.m_aids_duration;
        ar.labelElement("m_hetero_infectivity_multiplier"                    ) & inf_hiv.m_hetero_infectivity_multiplier;

        //hiv_parent assigned in SetContextTo()
    }
}
