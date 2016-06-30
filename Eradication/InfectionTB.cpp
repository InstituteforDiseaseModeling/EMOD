/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include <math.h>
#include "MathFunctions.h"
#ifdef ENABLE_TB

//for age dependent reactivation disease as non-homogeneous Poisson process ONLY (Latent-slow to active presymptomatic)
#define AGE_DEP_REACTIVATION_ALPHA (2.4e-7) //(15.4e-7)
#define AGE_DEP_REACTIVATION_BETA (-2.5f) //(-2.0f)

#include "InfectionTB.h"

#include "Interventions.h"
#include "TBInterventionsContainer.h"
#include "InterventionsContainer.h"
#include "SimulationConfig.h"
#include "SusceptibilityTB.h"
#include "RANDOM.h"
#include "Exceptions.h"
#include "config_params.rc"
#ifdef ENABLE_TBHIV
#include "IndividualCoinfection.h"
#else
#include "TBContexts.h"
#include "Individual.h"
#endif

static const char * _module = "InfectionTB";

namespace Kernel
{
    float InfectionTBConfig::TB_latent_cure_rate = 0.0f;
    float InfectionTBConfig::TB_fast_progressor_rate = 0.0f;
    float InfectionTBConfig::TB_slow_progressor_rate = 0.0f;
    float InfectionTBConfig::TB_active_cure_rate = 0.0f;
    float InfectionTBConfig::TB_inactivation_rate = 0.0f;
    float InfectionTBConfig::TB_active_mortality_rate = 0.0f;
    float InfectionTBConfig::TB_extrapulmonary_mortality_multiplier = 0.0f;
    float InfectionTBConfig::TB_smear_negative_mortality_multiplier = 0.0f;
    float InfectionTBConfig::TB_active_presymptomatic_infectivity_multiplier = 0.0f;
    float InfectionTBConfig::TB_presymptomatic_rate = 0.0f;
    float InfectionTBConfig::TB_presymptomatic_cure_rate = 0.0f;
    float InfectionTBConfig::TB_smear_negative_infectivity_multiplier = 0.0f;
    float InfectionTBConfig::TB_Drug_Efficacy_Multiplier_MDR = 0.0f;
    float InfectionTBConfig::TB_Drug_Efficacy_Multiplier_Failed = 0.0f;
    float InfectionTBConfig::TB_Drug_Efficacy_Multiplier_Relapsed = 0.0f;
    float InfectionTBConfig::TB_MDR_Fitness_Multiplier = 0.0f;
    float InfectionTBConfig::TB_relapsed_to_active_rate = 0.0f;
    DistributionFunction::Enum InfectionTBConfig::TB_active_period_distribution = DistributionFunction::EXPONENTIAL_DURATION;
    float InfectionTBConfig::TB_active_period_std_dev = 0.0f;
    map <float,float> InfectionTBConfig::CD4_map;
    vector <float> InfectionTBConfig::CD4_strata_act_vec;
    vector <float> InfectionTBConfig::TB_cd4_activation_vec;

    GET_SCHEMA_STATIC_WRAPPER_IMPL(TB.Infection,InfectionTBConfig)
    BEGIN_QUERY_INTERFACE_BODY(InfectionTBConfig)
    END_QUERY_INTERFACE_BODY(InfectionTBConfig)

    BEGIN_QUERY_INTERFACE_BODY(InfectionTB)
    HANDLE_INTERFACE(IInfectionTB)
    END_QUERY_INTERFACE_BODY(InfectionTB)

    bool
    InfectionTBConfig::Configure(
        const Configuration * config
    )
    {
        LOG_DEBUG( "Configure\n" );
        initConfigTypeMap( "TB_Latent_Cure_Rate", &TB_latent_cure_rate, TB_Latent_Cure_Rate_DESC_TEXT, 0.0f, 1.0f, 0.0005479f ); // tb
        initConfigTypeMap( "TB_Fast_Progressor_Rate", &TB_fast_progressor_rate, TB_Fast_Progressor_Rate_DESC_TEXT, 0.0f, 1.0f, 0.000041096f ); // tb
        initConfigTypeMap( "TB_Slow_Progressor_Rate", &TB_slow_progressor_rate, TB_Slow_Progressor_Rate_DESC_TEXT, 0.0f, 1.0f, 0.000002054f ); // tb, -1 only to turn on AgeDepSlowProgression
        initConfigTypeMap( "TB_Active_Cure_Rate", &TB_active_cure_rate, TB_Active_Cure_Rate_DESC_TEXT, 0.0f, 1.0f, 0.0f ); // tb
        initConfigTypeMap( "TB_Inactivation_Rate", &TB_inactivation_rate, TB_Inactivation_Rate_DESC_TEXT, 0.0f, 1.0f, 0.00041096f ); // tb
        initConfigTypeMap( "TB_Active_Mortality_Rate", &TB_active_mortality_rate, TB_Active_Mortality_Rate_DESC_TEXT, 0.0f, 1.0f, 0.0009589f ); // tb
        initConfigTypeMap( "TB_Extrapulmonary_Mortality_Multiplier", &TB_extrapulmonary_mortality_multiplier, TB_Extrapulmonary_Mortality_Multiplier_DESC_TEXT, 0.0f, 1.0f, 0.4f ); // tb
        initConfigTypeMap( "TB_Smear_Negative_Mortality_Multiplier", &TB_smear_negative_mortality_multiplier, TB_Smear_Negative_Mortality_Multiplier_DESC_TEXT, 0.0f, 1.0f, 0.4f ); // tb
        initConfigTypeMap( "TB_Active_Presymptomatic_Infectivity_Multiplier", &TB_active_presymptomatic_infectivity_multiplier, TB_Active_Presymptomatic_Infectivity_Multiplier_DESC_TEXT, 0.0f, 1.0f, 0.0274f );
        initConfigTypeMap( "TB_Presymptomatic_Rate", &TB_presymptomatic_rate, TB_Presymptomatic_Rate_DESC_TEXT, 0.0f, 1.0f, 0.0274f ); // tb
        initConfigTypeMap( "TB_Presymptomatic_Cure_Rate", &TB_presymptomatic_cure_rate, TB_Presymptomatic_Cure_Rate_DESC_TEXT, 0.0f, 1.0f, 0.0274f ); // tb
        initConfigTypeMap( "TB_Smear_Negative_Infectivity_Multiplier", &TB_smear_negative_infectivity_multiplier, TB_Smear_Negative_Infectivity_Multiplier_DESC_TEXT, 0.0f, 1.0f, 0.25f );
        initConfigTypeMap( "TB_Drug_Efficacy_Multiplier_MDR", &TB_Drug_Efficacy_Multiplier_MDR, TB_Drug_Efficacy_Multiplier_MDR_DESC_TEXT, 0.0f, 1.0f, 1.0f ); // 1 means equal efficacy if MDR or not
        initConfigTypeMap( "TB_Drug_Efficacy_Multiplier_Failed", &TB_Drug_Efficacy_Multiplier_Failed, TB_Drug_Efficacy_Multiplier_Failed_DESC_TEXT, 0.0f, 1.0f, 1.0f ); // 1 means equal efficacy if failed
        initConfigTypeMap( "TB_Drug_Efficacy_Multiplier_Relapsed", &TB_Drug_Efficacy_Multiplier_Relapsed, TB_Drug_Efficacy_Multiplier_Relapsed_DESC_TEXT, 0.0f, 1.0f, 1.0f ); // 1 means equal efficacy if relapsed
        initConfigTypeMap( "TB_MDR_Fitness_Multiplier", &TB_MDR_Fitness_Multiplier, TB_MDR_Fitness_Multiplier_DESC_TEXT, 0.0f, 1.0f, 1.0f ); // 1 means equal fitness for MDR strain 
        initConfigTypeMap( "TB_Relapsed_to_Active_Rate", &TB_relapsed_to_active_rate, TB_Relapsed_to_Active_Rate_DESC_TEXT, 0.0f, 1.0f, 0.000041096f ); // default equal to fast progressor
        
        initConfig( "TB_Active_Period_Distribution", TB_active_period_distribution, config, MetadataDescriptor::Enum("TB_active_distribution", TB_Active_Period_Distribution_DESC_TEXT, MDD_ENUM_ARGS(DistributionFunction)) ); 
        //note for both exponential and gaussian duration, the mean is defined by the total_rate of active_cure_rate and active_mortality_rate, corrected for smear status
        if( TB_active_period_distribution == DistributionFunction::GAUSSIAN_DURATION || JsonConfigurable::_dryrun )
        {
            initConfigTypeMap( "TB_Active_Period_Std_Dev", &TB_active_period_std_dev, TB_Active_Period_Std_Dev_DESC_TEXT, 0.0f, FLT_MAX, 1.0f );
        }
        if( JsonConfigurable::_dryrun == false )
        {
            if( TB_active_period_distribution == DistributionFunction::FIXED_DURATION ||
                TB_active_period_distribution == DistributionFunction::UNIFORM_DURATION ||
                TB_active_period_distribution == DistributionFunction::POISSON_DURATION ||
                TB_active_period_distribution == DistributionFunction::LOG_NORMAL_DURATION ||
                TB_active_period_distribution == DistributionFunction::BIMODAL_DURATION
              )
            {
                throw NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "Only EXPONENTIAL_DURATION and GAUSSIAN_DURATION are supported for TB_active_period_distribution." );
            }
        }

#ifdef ENABLE_TBHIV
        if( JsonConfigurable::_dryrun || GET_CONFIGURABLE( SimulationConfig )->sim_type == SimType::TBHIV_SIM ) // Hmmm...
        {
            initConfigTypeMap( "TB_CD4_Activation_Vector", &TB_cd4_activation_vec, TB_cd4_activation_vec_DESC_TEXT,0.0f, FLT_MAX, 1.0f);
            initConfigTypeMap( "CD4_Strata_Activation", &CD4_strata_act_vec, CD4_Strata_Activation_DESC_TEXT, 1.0f, 2000.0f);
        }
#endif

        bool cRet = JsonConfigurable::Configure( config );

        auto it_activation_factor = TB_cd4_activation_vec.cbegin();
        auto it_cd4_strata = CD4_strata_act_vec.cbegin();

        while (  (it_activation_factor!= TB_cd4_activation_vec.cend() ) && (it_cd4_strata != CD4_strata_act_vec.cend() ) )
        {
            CD4_map[*it_cd4_strata++] = *it_activation_factor++;
        }
        return cRet;
    }

    std::map <float, float> InfectionTBConfig::GetCD4Map()
    {
        return CD4_map;
    }
   
    InfectionTB *InfectionTB::CreateInfection(IIndividualHumanContext *context, suids::suid _suid)
    {
        InfectionTB *newinfection = _new_ InfectionTB(context);
        newinfection->Initialize(_suid);
        LOG_DEBUG("Create infection called in InfectionTB \n");
        return newinfection;
    }

    void InfectionTB::Initialize(suids::suid _suid)
    {
        InfectionAirborne::Initialize(_suid);

        // Initialization of TB infection members
        m_recover_fraction   = 0;
        m_death_fraction     = 0;
        m_is_active          = false; // all infections begin as latent
        m_is_smear_positive  = false;
        m_is_extrapulmonary  = false;
        m_is_fast_progressor = false;
        m_evolved_resistance = false; //watches to see if you evolve resistance in this infection
        m_is_pending_relapse = false;
        m_shows_symptoms = false;

        if( parent->QueryInterface( GET_IID( IIndividualHumanCoinfection ), (void**)&human_coinf ) != s_OK )
        {
            LOG_DEBUG_F( "parent is just tb person, not co-inf.\n" );
            human_coinf = nullptr;
        }
        m_duration_since_init_infection = 0.0f;
    }

    void InfectionTB::SetParameters(StrainIdentity* infstrain, int incubation_period_override)
    {
        CreateInfectionStrain(infstrain);

        // Defer setting duration of incubation/infection periods 
        // to InitInfectionImmunology function which is called next
        // in AcquireNewInfection since it takes a Susceptibility argument
        // allowing age/coinfection/etc. dependencies

    }

    // TODO: PUT A BIG EXPLANATION OF THE TB S-E-I-R NATURAL HISTORY HERE
    // including FAST and SLOW progression from exposed to infectious
    // and heterogeneity in infectivity of active disease cases
    void InfectionTB::Update(float dt, ISusceptibilityContext* immunity)
    {
        StateChange = InfectionStateChange::None; // reset state change of previous time-step
        LOG_DEBUG("InfectionTB Update \n");
        m_duration_since_init_infection += dt;

        //natural history of disease progression only allowed when not on drugs
        TBDrugEffects_t total_drug_effects = GetTotalDrugEffectsForThisInfection();

        if (total_drug_effects.inactivation_rate == 0 && total_drug_effects.clearance_rate == 0 && total_drug_effects.mortality_rate == 0 && total_drug_effects.relapse_rate == 0) 
        {
            duration +=dt;

            LOG_DEBUG_F("Your incubation_timer is %f \n", incubation_timer);
            //LATENT
            if (!m_is_active && duration > incubation_timer) 
            {
                // Latent-to-Cured
                float rand = randgen->e();
                if (rand < m_recover_fraction)
                {
                    StateChange = InfectionStateChange::Cleared; 
                    // immune loss counter is handled later in the same timestep, i.e. susc->Update(1) in Individual::Update
                }

                // Latent-to-Death not simulated (except for non-disease mortality handled elsewhere)
                // Latent-to-ActivePresymptomatic
                else
                {
                    LOG_DEBUG("Progress from Latent to Active Presymptomatic \n");
                    InitializeActivePresymptomaticInfection(immunity);
                    StateChange = InfectionStateChange::TBActivationPresymptomatic;
                }
            }

            //ACTIVE PRESYMPTOMATIC
            else if (m_is_active && !m_shows_symptoms && duration > infectious_timer ) 
            {
                float rand = randgen->e();
                //Active presymptomatic-to-Cured
                if (rand < m_recover_fraction)
                {
                    StateChange = InfectionStateChange::Cleared; 
                }

                //Active presymptomatic-to-Death not simulated
                //Active presymptomatic-to Active with Symptoms
                else
                {
                    InitializeActiveInfection(immunity);

                    //log state change for Active patients
                    //Make a distinction between different symptom presentations
                    if (IsSmearPositive() == true)
                    {
                        StateChange = InfectionStateChange::TBActivationSmearPos;
                    }
                    else if (IsExtrapulmonary() == true)
                    {
                        StateChange = InfectionStateChange::TBActivationExtrapulm;
                    }
                    else if (IsSmearPositive() == false && IsExtrapulmonary() == false) //smear neg patients
                    {
                        StateChange = InfectionStateChange::TBActivationSmearNeg;
                    }
                }
            }

            //ACTIVE SYMPTOMATIC PEOPLE
            else if (m_is_active && m_shows_symptoms && duration > infectious_timer) //USE CAUTION IF YOUR DURATION IS ZERO! YOU WILL GET LOTS OF PEOPLE ROLLING ZERO AND GOING TO LATENT!
            {
                // Active-to-Cured
                float rand = randgen->e();
                if (rand < m_recover_fraction)
                {
                    StateChange = InfectionStateChange::Cleared;
                }

                // Active-to-Death
                else if (rand > 1-m_death_fraction) //!!!NOTE TB_Active_Mortality_Rate OVERRIDES the Enable_Disease_mortality
                {
                    StateChange = InfectionStateChange::Fatal; 
                }

                // Active-to-Latent (without intervention)
                else
                {
                    StateChange = InfectionStateChange::TBInactivation;
                    InitializeLatentInfection(immunity);
                }
            }
            else
            {
                LOG_DEBUG("Not time for you to progress from one state to another \n");
            }
        }
        // Evolution of drug resistance is handled here 
        EvolveStrain(immunity, dt);

        // Inactivation of active disease or clearance of latent/active disease
        ApplyDrugEffects(dt, immunity);
    }

    void InfectionTB::InitInfectionImmunology(ISusceptibilityContext* _immunity)
    {
        LOG_DEBUG("Init InfectionImmunology \n");
        if(!_immunity)
        {
            //std::cerr << "Trying to do InfectionTB::InitInfectionImmunology before SusceptibilityTB is initialized" << std::endl;
            throw NullPointerException( __FILE__, __LINE__, __FUNCTION__, "_immunity", "Susceptibility" );
        }

        _immunity->InitNewInfection();

        if (InfectionConfig::incubation_distribution.GetType() != DistributionFunction::EXPONENTIAL_DURATION )
        {
            LOG_DEBUG("TB incubation timers will use exponential distributions in spite of 'incubation_distribution' settings\n");
        }
        if (InfectionConfig::infectious_distribution.GetType() != InfectionTBConfig::TB_active_period_distribution)
        {
            LOG_DEBUG("TB active period timers will use the TB_active_period_distribution, NOT the infectious_distribution \n");
        }
        InitializeLatentInfection(_immunity);

    }

    void InfectionTB::InitializeLatentInfection(ISusceptibilityContext* immunity)
    {
        LOG_DEBUG( "Initializing a latent infection.\n" ); 
        StateChange = InfectionStateChange::TBLatent; 

        if(Environment::getInstance()->Log->CheckLogLevel(Logger::DEBUG, "EEL"))
        {
            std::ostringstream msg;
            msg << "t=" << ((INodeContext*)((IndividualHuman*)parent)->GetParent())->GetTime().time;
            msg << ",hum_id=" << parent->GetSuid().data;
            msg << ",new_inf_state=10"; //10 is TBlatent
            msg << ",inf_id=-1";;
            msg << ",inf_mdr=" << IsMDR();
            Environment::getInstance()->Log->LogF(Logger::DEBUG, "EEL", "%s\n", msg.str().c_str()  );
        }
        ISusceptibilityTB* immunityTB = nullptr;
        if( immunity->QueryInterface( GET_IID( ISusceptibilityTB ), (void**)&immunityTB ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "immunity", "Susceptibility", "SusceptibilityTB" );
        }

        // Figure out individual dependence of disease activation
        float fast_fraction = immunityTB->GetFastProgressorFraction();

        // Reset timer
        duration = 0.0f;

        // Set the timer for state changes from latent TB i.e. activation, recovery, death
        //Latent fast

        if(randgen->e() < fast_fraction)
        {
            m_is_fast_progressor = true;

            float total_rate = InfectionTBConfig::TB_fast_progressor_rate + InfectionTBConfig::TB_latent_cure_rate;
            incubation_timer = float(randgen->expdist(total_rate)); 

            m_recover_fraction = (total_rate > 0 ? InfectionTBConfig::TB_latent_cure_rate/total_rate : 0);
        }

        //Latent slow
        else
        {
            m_is_fast_progressor = false;

            //Latent slow by exponential rate
            if (InfectionTBConfig::TB_slow_progressor_rate >= 0)
            {
                float total_rate = InfectionTBConfig::TB_slow_progressor_rate + InfectionTBConfig::TB_latent_cure_rate;
                incubation_timer = float(randgen->expdist(total_rate));

                m_recover_fraction = (total_rate > 0 ? InfectionTBConfig::TB_latent_cure_rate/total_rate : 0);
            }

            //Latent slow by age (set slow progressor rate to -1)
            else
            {    
                incubation_timer = CalculateTimerAgeDepSlowProgression(immunity);
                m_recover_fraction = 0; // if your slow progression goes by age you can't recover naturally, this would be latent -> cured which is indistinguishable from non-disease 
            }
        }

#ifdef ENABLE_TBHIV
        // HIV-specific code that needs to be encapsulated.
        if( human_coinf != nullptr && immunityTB->GetCD4ActFlag() )
        {

            incubation_timer = randgen->time_varying_rate_dist( human_coinf->GetTBactivationvector(), human_coinf->GetCD4TimeStep(), InfectionTBConfig::TB_latent_cure_rate);
            float total_rate = human_coinf->GetNextLatentActivation(incubation_timer) + InfectionTBConfig::TB_latent_cure_rate;
            m_recover_fraction = (total_rate > 0 ? InfectionTBConfig::TB_latent_cure_rate/total_rate : 0);
        }
#endif

        // And the fractional allocation to each state
        //set flags for all latent disease
        m_is_active = false;
        m_is_pending_relapse = false;
        m_is_smear_positive = false;
        m_is_extrapulmonary = false;
        m_shows_symptoms = false;

        // Latent to death not simulated, recover_fraction set in the relevant latent section
        m_death_fraction = 0;

        // Reset infectiousness
        infectiousness = 0;

    }

    void InfectionTB::InitializePendingRelapse(ISusceptibilityContext* immunity)
    {
        LOG_DEBUG( "Initializing a pending relapse.\n" );
        ISusceptibilityTB* immunityTB = nullptr;
        if( immunity->QueryInterface( GET_IID( ISusceptibilityTB ), (void**)&immunityTB ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "immunity", "Susceptibility", "SusceptibilityTB" );
        }

        // Reset timer
        duration = 0.0f;

        // Set the timer for pending relapse
        float relapse_rate = InfectionTBConfig::TB_relapsed_to_active_rate; 

        incubation_timer = float(randgen->expdist(relapse_rate));
        m_is_active = false;
        m_is_smear_positive = false;
        m_is_extrapulmonary = false;
        m_shows_symptoms = false;

        // And the fractional allocation to each state 
        //properties of pendingrelapse are the same as latent infection, but you cannot recover or die, until your incubation_timer goes off
        m_recover_fraction = 0;
        m_death_fraction = 0;

        // Reset infectiousness
        infectiousness = 0;

        m_is_pending_relapse = true;
        //tracking of ever_relapsed done directly by the drug and stored in TB IVC now

    }
    void InfectionTB::InitializeActivePresymptomaticInfection(ISusceptibilityContext* immunity)
    {
        LOG_DEBUG( "InitializeActivePresymptomaticInfection\n" );
        ISusceptibilityTB* immunityTB = nullptr;
        if( immunity->QueryInterface( GET_IID( ISusceptibilityTB ), (void**)&immunityTB ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "immunity", "ISusceptibilityTB", "Susceptibility" );
        }

        // Reset timer
        duration = 0.0f;

        // Set infectiousness
        infectiousness = InfectionConfig::base_infectivity * InfectionTBConfig::TB_active_presymptomatic_infectivity_multiplier * immunityTB->GetCoughInfectiousness();

        //fitness penalty for MDR
        if ( infection_strain->GetGeneticID() == TBInfectionDrugResistance::FirstLineResistant ) 
        {
            infectiousness *= InfectionTBConfig::TB_MDR_Fitness_Multiplier;
            LOG_DEBUG("Infectiousness lowered because the drug is FirstLineCombo and I have a resistant strain \n");
        }

        //Relapsers do not spend any time in Presymptomatic infection, they go directly to active disease
        IIndividualHumanTB2* tb_ind = nullptr;
        
        IIndividualHumanContext * patient = GetParent();
        if(patient->GetEventContext()->QueryInterface( GET_IID( IIndividualHumanTB2 ), (void**)&tb_ind ) != s_OK)
        { 
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "pIndividual", "IIndividualHumanTB2", "IIndividualHumanEventContext" );
        }

        if ( tb_ind->HasEverRelapsedAfterTreatment() )
        {
            m_recover_fraction = 0;
            infectious_timer = 0.0f;
        }
        //Everyone else use the pre-symptomatic rate
        else
        {
            float total_rate = InfectionTBConfig::TB_presymptomatic_cure_rate + InfectionTBConfig::TB_presymptomatic_rate;
            m_recover_fraction = InfectionTBConfig::TB_presymptomatic_cure_rate / total_rate;
            infectious_timer = float(randgen->expdist(total_rate));
        }

        m_is_active = true;
        m_is_smear_positive = false;
        m_is_extrapulmonary = false;
        m_is_pending_relapse = false;
        m_shows_symptoms = false;    
    }
    
    void InfectionTB::InitializeActiveInfection(ISusceptibilityContext* immunity)
    {
        dynamic_cast<IIndividualHumanTB2*>(parent)->onInfectionIncidence();
        if (infection_strain->GetGeneticID() == TBInfectionDrugResistance::FirstLineResistant)
        {
            dynamic_cast<IIndividualHumanTB2*>(parent)->onInfectionMDRIncidence();
        }
        LOG_DEBUG( "InitializeActiveInfection\n" );
        ISusceptibilityTB* immunityTB = nullptr;
        if( immunity->QueryInterface( GET_IID( ISusceptibilityTB ), (void**)&immunityTB ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "immunity", "ISusceptibilityTB", "Susceptibility" );
        }

        // Figure out individual dependence of active disease presentation
        float smear_positive_fraction = immunityTB->GetSmearPositiveFraction();
        float extrapulmonary_fraction = immunityTB->GetExtraPulmonaryFraction();

        // Reset timer
        duration = 0.0f;

        // Set infectiousness and mortality rates dependent on disease progression
        float rand = randgen->e();

        // To query for mortality-reducing effects of drugs or vaccines
        // TODO: depending on the decay profile of a mortality-reducing vaccine,
        //       we may prefer to change "death as a compartmental transition" to "death as a daily update"
        //       so that we aren't picking the death rate based on the efficacy of a vaccine at the beginning of the infection alone.
        IDrugVaccineInterventionEffects* idvie = nullptr;
        if ( s_OK != parent->GetInterventionsContext()->QueryInterface(GET_IID(IDrugVaccineInterventionEffects), (void**)&idvie) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent->GetInterventionsContext()", "IDrugVaccineInterventionEffects", "IIndividualHumanEventContext" );
        }

        float death_rate = InfectionTBConfig::TB_active_mortality_rate * immunity->getModMortality() * idvie->GetInterventionReducedMortality();

        infectiousness = InfectionConfig::base_infectivity * immunityTB->GetCoughInfectiousness();

        //fitness penalty for MDR
        if ( infection_strain->GetGeneticID() == TBInfectionDrugResistance::FirstLineResistant ) 
        {
            infectiousness *= InfectionTBConfig::TB_MDR_Fitness_Multiplier;
            LOG_DEBUG("Infectiousness lowered because I have a resistant strain \n");
        }

        m_is_smear_positive = true;
        m_is_extrapulmonary = false;

        if (rand > (1-extrapulmonary_fraction)) //extra-pulmonary patients
        {
            infectiousness  = 0;
            m_is_extrapulmonary = true;
            m_is_smear_positive = false;
        } 
        else if (rand > smear_positive_fraction) //smear negative patients
        {
            infectiousness *= InfectionTBConfig::TB_smear_negative_infectivity_multiplier;
            m_is_smear_positive = false;
        }

        // Set the timer for state changes from active TB
        // i.e. inactivation, recovery, death
        float inactivation_rate = InfectionTBConfig::TB_inactivation_rate;
        float cure_rate = InfectionTBConfig::TB_active_cure_rate;
        float total_rate = inactivation_rate + cure_rate + death_rate;
        
        switch(InfectionTBConfig::TB_active_period_distribution ) 
        {
            case DistributionFunction::EXPONENTIAL_DURATION:
                infectious_timer = float(randgen->expdist(total_rate));
                break;

            case DistributionFunction::GAUSSIAN_DURATION:
                infectious_timer = float(Probability::getInstance()->fromDistribution(  DistributionFunction::GAUSSIAN_DURATION, (log(2.0f)/total_rate), InfectionTBConfig::TB_active_period_std_dev ));
                break;

            default:
                throw NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "TB infectious period distribution can only be exponential or Gaussian distribution right now" );
        }
        // FOR CHINA ONLY infectious_timer = (float) Probability::getInstance()->fromDistribution(  DistributionFunction::GAUSSIAN_DURATION, (log(2.0f)/total_rate), 200 );
        m_is_active = true;
        m_is_pending_relapse = false;
        m_shows_symptoms = true;

        // And the fractional allocation to each state
        m_recover_fraction = (total_rate > 0 ? cure_rate/total_rate : 0);
        m_death_fraction = (total_rate > 0 ? death_rate/total_rate : 0);

        //correct the fractional allocation by smear status, reduction in death correlates to higher recovery, inactivation fraction stays the same
        if (rand > (1-extrapulmonary_fraction)) //extra-pulmonary patients
        {
            m_recover_fraction += ( m_death_fraction * ( 1 - InfectionTBConfig::TB_extrapulmonary_mortality_multiplier ) );
            m_death_fraction *= InfectionTBConfig::TB_extrapulmonary_mortality_multiplier;
        } 
        else if (rand > smear_positive_fraction) //smear negative patients
        {
            m_recover_fraction += ( m_death_fraction * ( 1 - InfectionTBConfig::TB_smear_negative_mortality_multiplier ) );
            m_death_fraction *= InfectionTBConfig::TB_smear_negative_mortality_multiplier;
        }
    }

    TBDrugEffects_t InfectionTB::GetTotalDrugEffectsForThisInfection()
    {
        //Gets total drug effects and modulates by infection strain and infection history

        float TB_drug_inactivation_rate = 0;
        float TB_drug_clearance_rate = 0;
        float TB_drug_resistance_rate = 0;
        float TB_drug_relapse_rate = 0;
        float TB_drug_mortality_rate = 0;

        IIndividualHumanContext *patient              = nullptr;
        IIndividualHumanInterventionsContext *context = nullptr;
        ITBDrugEffects *itde                          = nullptr;

        patient = GetParent();
        context = patient->GetInterventionsContext();

        if (s_OK ==  context->QueryInterface(GET_IID(ITBDrugEffects), (void**)&itde))
        {
            TBDrugEffectsMap_t TB_drug_effects = itde->GetDrugEffectsMap();

            //add up all drug effects
            for (auto& drug_effect : TB_drug_effects)
            {
                //modulate the clearance rate and inactivation rate by the drug_strain_multiplier, which accounts for mismatch between FirstLineCombo and FirstLineResistant strain
                float drug_strain_multiplier = 1.0f; 

                if (infection_strain->GetGeneticID() == TBInfectionDrugResistance::FirstLineResistant ) //could add this drug_effect.first == TBDrugType::FirstLineCombo if wanted to be specific to getting FirstLineCombo
                {
                    LOG_DEBUG( "Received a drug (does not have to be FirstLineCombo), but have DR strain. TB drug clearance/inactivation rate adjusted\n" );
                    drug_strain_multiplier = InfectionTBConfig::TB_Drug_Efficacy_Multiplier_MDR;
                }
                
                //add to total drug clearance/inactivation rate here
                TB_drug_inactivation_rate += (drug_effect.second.inactivation_rate * drug_strain_multiplier);
                TB_drug_clearance_rate += (drug_effect.second.clearance_rate * drug_strain_multiplier);

                //TODO: consider if it is correct to add the resistance rates
                TB_drug_resistance_rate += (drug_effect.second.resistance_rate );
                TB_drug_relapse_rate += (drug_effect.second.relapse_rate ); 
                TB_drug_mortality_rate += (drug_effect.second.mortality_rate );
            }
        }

        //Modulate drug clearance rate depending if person has ever failed (prob should be specific for drug type and for this particular infection in future)
        IIndividualHumanTB2* tb_ind = nullptr;
        if(patient->GetEventContext()->QueryInterface( GET_IID( IIndividualHumanTB2 ), (void**)&tb_ind ) != s_OK)
        { 
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "pIndividual", "IIndividualHumanTB2", "IIndividualHumanEventContext" );
        }

        if ( tb_ind->HasFailedTreatment() )
        {
            TB_drug_inactivation_rate *= InfectionTBConfig::TB_Drug_Efficacy_Multiplier_Failed;
            TB_drug_clearance_rate *= InfectionTBConfig::TB_Drug_Efficacy_Multiplier_Failed; 
            LOG_DEBUG_F("Adjusting drug clearance rate because I have failed before, new rate is %f \n", TB_drug_clearance_rate);
            
            //TODO: Could also modulate the resistance, relapse and mortality rate if infection has ever failed (also should be specific for drug type in future)
        }

        //Modulate drug clearance rate depending if infection has ever relapsed (also should be specific for drug type in future)
        if ( tb_ind->HasEverRelapsedAfterTreatment() )
        {
            TB_drug_inactivation_rate *= InfectionTBConfig::TB_Drug_Efficacy_Multiplier_Relapsed; 
            TB_drug_clearance_rate *= InfectionTBConfig::TB_Drug_Efficacy_Multiplier_Relapsed; 
            LOG_DEBUG_F("Adjusting drug clearance rate because I have relapsed before, new rate is %f \n", TB_drug_clearance_rate);

            //TODO: Could also modulate the resistance, relapse and mortality rate if infection has ever failed (also should be specific for drug type in future)
        }

        TBDrugEffects_t total_drug_effect_on_infection;
        total_drug_effect_on_infection.clearance_rate = TB_drug_clearance_rate;
        total_drug_effect_on_infection.inactivation_rate = TB_drug_inactivation_rate;
        total_drug_effect_on_infection.resistance_rate = TB_drug_resistance_rate;
        total_drug_effect_on_infection.relapse_rate = TB_drug_relapse_rate;
        total_drug_effect_on_infection.mortality_rate = TB_drug_mortality_rate;
        return total_drug_effect_on_infection;

    }


    void InfectionTB::EvolveStrain(ISusceptibilityContext* _immunity, float dt)
    {
        // TODO: here we have only FirstLineResistant strain type of drug resistance
        // later can add new types of resistance here, back evolution from resistant to sensitive strain etc
        
        //only evolve if you are DS, if already resistant, skip all
        if (infection_strain->GetGeneticID() == TBInfectionDrugResistance::DrugSensitive ) 
        {
            TBDrugEffects_t total_drug_effects = GetTotalDrugEffectsForThisInfection();

            //If no active drugs, there is no probability of evolving drug resistance
            //If active and on drugs with non-zero resistance rate, fixed higher probability of evolving resistance from drug pressure
            if (total_drug_effects.resistance_rate != 0 && total_drug_effects.clearance_rate > 0 && IsActive() == true) 
            {
                if (randgen->e() < total_drug_effects.resistance_rate * dt ) 
                {
                    infection_strain->SetGeneticID(TBInfectionDrugResistance::FirstLineResistant);
                    m_evolved_resistance = true;
                    dynamic_cast<IIndividualHumanTB2*>(parent)->onInfectionMDRIncidence(); //alert observer to new MDR incident case (evolved)
                    LOG_DEBUG("Evolved drug resistance strain while having active disease and on drugs \n");

                    infectiousness *= InfectionTBConfig::TB_MDR_Fitness_Multiplier;     //fitness penalty for MDR
                    
                    if(Environment::getInstance()->Log->CheckLogLevel(Logger::DEBUG, "EEL"))
                    {
                        std::ostringstream msg;
                        msg << "t=" << ((INodeContext*)((IndividualHuman*)parent)->GetParent())->GetTime().time;
                        msg << ",hum_id=" << parent->GetSuid().data;
                        msg << ",inf_id=-1";
                        msg << ",ev=MDR";
                        Environment::getInstance()->Log->LogF(Logger::DEBUG, "EEL", "%s\n", msg.str().c_str()  );
                    }
                }
            }
        }
    }

    bool InfectionTB::ApplyDrugEffects(float dt, ISusceptibilityContext* immunity)
    {
        // Check for valid inputs
        if (dt <= 0 || !immunity) 
        {
            LOG_WARN("ApplyDrugEffects takes the following arguments: time step (dt), Susceptibility (immunity)\n");
            return false;
        }

        TBDrugEffects_t total_drug_effects = GetTotalDrugEffectsForThisInfection();
        
        //if no drugs on board now, don't need to do all the following computations
        float total_event_rate = total_drug_effects.clearance_rate + total_drug_effects.relapse_rate + total_drug_effects.mortality_rate + total_drug_effects.inactivation_rate;
        if (total_event_rate == 0)
        { 
            return false;
        }

        //ALLOW FOR DIFFERENT DRUG EFFECTS BASED ON STAGE OF DISEASE
        //(if no event occurs during the treatment, that's ret = return false)
        bool ret = false;
        
        //ACTIVE DISEASE CAN GO TO CLEARED, PENDING_RELAPSE, INACTIVE TO LATENT, or DEATH
        if (IsActive() == true) 
        {
            //draw to see if the event occurs now
            if (randgen->e() < (dt * total_event_rate))
            {
                float rand2 = randgen->e();

                // Infection cleared
                if ( rand2 < (total_drug_effects.clearance_rate / total_event_rate) )
                {
                    LOG_DEBUG( "TB drug truly cleared my infection \n" );
                    StateChange = InfectionStateChange::Cleared;
                    ret = true;
                }
                
                //Infection will relapse
                else if ( rand2 < ( (total_drug_effects.clearance_rate + total_drug_effects.relapse_rate) / total_event_rate) )
                {
                    LOG_DEBUG( "TB drug cleared my infection, but I will relapse in the future \n" );
                    StateChange = InfectionStateChange::ClearedPendingRelapse;
                    InitializePendingRelapse( immunity );
                    ret = true;
                }

                // Active-to-latent
                else if (rand2 > (1 - (total_drug_effects.inactivation_rate / total_event_rate ) ) )
                {
                    StateChange = InfectionStateChange::TBInactivation;
                    InitializeLatentInfection( immunity );
                    ret = true;
                }

                // Active-to-death
                else
                {
                    LOG_DEBUG_F("Died from active disease on drugs, death_rate is %f, rand2 is %f \n", total_drug_effects.mortality_rate);
                    StateChange = InfectionStateChange::Fatal; 
                    ret = true;
                }
            }
        }
        else    //LATENT AND PENDING RELAPSE CAN ONLY GO TO CLEARED or DEATH
        {
            //PENDING RELAPSE - NOT ALLOWED TO GET TREATMENT
            //IF STILL ON TREATMENT INCREMENT THE INCUBATION TIMER so that you are not allowed to re-advance to active disease on this treatment 
            if (m_is_pending_relapse) 
            {
                incubation_timer = incubation_timer + dt; // NOTE: this is only place in code where the incubation_timer is incremented.???
            }
            else
            { 
                float rand = randgen->e();

                // Latent -to- cleared
                if (rand < (dt * total_drug_effects.clearance_rate))
                {
                    LOG_DEBUG( "TB drug truly cleared my infection from latent state \n" );
                    StateChange = InfectionStateChange::Cleared;
                    ret = true;
                }
            
                // Latent-to-death
                else if (rand > (1 - (dt * total_drug_effects.mortality_rate)))
                {
                    LOG_DEBUG_F("Died from latent disease on drugs, death_rate is %f, rand2 is %f \n", total_drug_effects.mortality_rate);
                    StateChange = InfectionStateChange::Fatal; 
                    ret = true;
                }
            }
        }      

        // Otherwise drug did not have an effect in this timestep
        return ret;
    }

    float InfectionTB::CalculateTimerAgeDepSlowProgression(ISusceptibilityContext* immunity)
    {
        /*Non-homogeneous Poisson process using AGE_DEP_REACTIVATION_ALPHA and AGE_DEP_REACTIVATION_BETA to define slow progressors time until activation 
        with increasing probability as people get older, using parameters A, and B where B = 1-beta and A = alpha / B
        http://www.itl.nist.gov/div898/handbook/apr/section1/apr172.htm; http://www.itl.nist.gov/div898/handbook/apr/section1/apr191.htm#Simulating NHPP Power Law Data
        Basically we generalize the exponential CDF from 1-exp(-rt) to a form where the exponent is an integral over a rate r(t).
        If r(t) is a well-behaved form like a power-law, it is pretty straightforward to do the integration analytically.
        Inverting the cumulative probability of activation we can use a uniform random draw to pick a random time-to-activation (for any ‘age’ value).
        t = age at next event = ((T^B)-1/A*log(rand))^(1/B);
        (t-T) is the time until the next event from current age of T
        */
    
        float bigB = 1- AGE_DEP_REACTIVATION_BETA;
        float bigA = AGE_DEP_REACTIVATION_ALPHA / bigB;
        float rand = randgen->e();
        float current_age_years = immunity->getAge() / DAYSPERYEAR;
        float reactivation_age_years = pow( pow(current_age_years, bigB) - (1 / bigA) * log(rand), (1/bigB));
        float calculated_incubation_timer = DAYSPERYEAR * (reactivation_age_years - current_age_years);

        return calculated_incubation_timer; 
    }

    InfectionTB::~InfectionTB(void) { }
    
    bool
    InfectionTB::IsActive() const
    {
        return m_is_active;
    }
    
    bool InfectionTB::IsSmearPositive() const  
    { 
        return m_is_smear_positive; 
    }
    
    bool InfectionTB::IsExtrapulmonary() const 
    { 
        return m_is_extrapulmonary; 
    }
    
    bool InfectionTB::IsFastProgressor() const 
    { 
       return m_is_fast_progressor; 
    }
    
    bool InfectionTB::IsMDR() const 
    { 
        return infection_strain->GetGeneticID() == TBInfectionDrugResistance::FirstLineResistant; 
    }
    
    bool InfectionTB::EvolvedResistance() const 
    { 
        return m_evolved_resistance; 
    }

    bool InfectionTB::IsPendingRelapse() const 
    { 
        return m_is_pending_relapse; 
    }

    float InfectionTB::GetDurationSinceInitialInfection() const 
    { 
        return m_duration_since_init_infection; 
    }    
    
    bool InfectionTB::IsSymptomatic() const 
    { 
        return m_shows_symptoms; 
    }    

    void InfectionTB::SetIncubationTimer (float new_timer) 
    {
        incubation_timer = new_timer; 
    }
    
    float InfectionTB::GetLatentCureRate() const
    {
        return InfectionTBConfig::TB_latent_cure_rate;
    }
    
    void InfectionTB::ResetRecoverFraction(float new_fraction)
    {
        m_recover_fraction = new_fraction;
    }
    
    void InfectionTB::ResetDuration()
        {
            duration = 0;
        }
    
    InfectionTB::InfectionTB() { }
    InfectionTB::InfectionTB(IIndividualHumanContext *context) : InfectionAirborne(context) { }
    //const SimulationConfig* InfectionTB::params() { return GET_CONFIGURABLE(SimulationConfig); }

    REGISTER_SERIALIZABLE(InfectionTB);

    void InfectionTB::serialize(IArchive& ar, InfectionTB* obj)
    {
        InfectionAirborne::serialize(ar, obj);
        InfectionTB& infection = *obj;
        ar.labelElement("m_is_active") & infection.m_is_active;
        ar.labelElement("m_recover_fraction") & infection.m_recover_fraction;
        ar.labelElement("m_death_fraction") & infection.m_death_fraction;
        ar.labelElement("m_is_smear_positive") & infection.m_is_smear_positive;
        ar.labelElement("m_is_extrapulmonary") & infection.m_is_extrapulmonary;
        ar.labelElement("m_is_fast_progressor") & infection.m_is_fast_progressor;
        ar.labelElement("m_evolved_resistance") & infection.m_evolved_resistance;
        ar.labelElement("m_is_pending_relapse") & infection.m_is_pending_relapse;
        ar.labelElement("m_shows_symptoms") & infection.m_shows_symptoms;
        ar.labelElement("m_duration_since_init_infection") & infection.m_duration_since_init_infection;
    }
}

#endif // ENABLE_TB
