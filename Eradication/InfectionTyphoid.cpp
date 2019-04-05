/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#ifdef ENABLE_TYPHOID

#include "InfectionTyphoid.h"
#include "SusceptibilityTyphoid.h"
#include "IndividualTyphoid.h"
#include "InterventionsContainer.h"
#include "TyphoidDefs.h"
#include "Environment.h"
#include "Debug.h"

#include "Common.h"
#include "MathFunctions.h"
#include "RANDOM.h"
#include "Exceptions.h"
#include "SimulationConfig.h"
#include "IdmDateTime.h"
#include "INodeContext.h"
#include "DistributionFactory.h"

using namespace std;

SETUP_LOGGING( "InfectionTyphoid" )

namespace Kernel
{
#define UNINIT_TIMER (-100.0f)

    GET_SCHEMA_STATIC_WRAPPER_IMPL(Typhoid.Infection,InfectionTyphoidConfig)
    BEGIN_QUERY_INTERFACE_BODY(InfectionTyphoidConfig)
    END_QUERY_INTERFACE_BODY(InfectionTyphoidConfig)
    
    IDistribution* InfectionTyphoidConfig::p_log_normal_distribution = nullptr;

    // Incubation period by transmission route (taken from Glynn's dose response analysis) assuming low dose for environmental.
    // mean and std dev of log normal distribution
    //
    const float ageThresholdInYears = 30.0f;

    const float InfectionTyphoid::mpl = 2.2350f;
    const float InfectionTyphoid::spl = 0.4964f;
    const float InfectionTyphoid::mph = 1.5487f; // math.log(4.7)
    const float InfectionTyphoid::sph = 0.3442f;

    // Subclinical infectious duration parameters: mean and standard deviation under and over 30 (refitted without carriers) 
    const float InfectionTyphoid::mso30=1.2584990f;
    const float InfectionTyphoid::sso30=0.7883767f;
    const float InfectionTyphoid::msu30=1.171661f;
    const float InfectionTyphoid::ssu30=0.483390f;

    // Acute infectious duration parameters: mean and standard deviation under and over 30 
    const float InfectionTyphoid::mao30=1.2584990f;
    const float InfectionTyphoid::sao30=0.7883767f;
    const float InfectionTyphoid::mau30=1.171661f;
    const float InfectionTyphoid::sau30=0.483390f;

    const int GallstoneDataLength= 9;
    const double FemaleGallstones[GallstoneDataLength] = {0.0, 0.097, 0.234, 0.431, 0.517, 0.60, 0.692, 0.692, 0.555}; // 10-year age bins
    const double MaleGallstones[GallstoneDataLength] = {0.0, 0.0, 0.045, 0.134, 0.167, 0.198, 0.247, 0.435, 0.4};

    const float InfectionTyphoid::P10 = 0.0f; // probability of clinical immunity from a subclinical infection

    const int InfectionTyphoid::_chronic_duration = 100000000;

    const float InfectionTyphoid::CFRU = 0.00f;   // case fatality rate?
    const float InfectionTyphoid::CFRH = 0.00f; // hospitalized case fatality rate?
    const float InfectionTyphoid::treatmentprobability = 0.00f;  // probability of treatment seeking for an acute case. we are in santiago so assume 100%

    inline float generateRandFromLogNormal( float m, float s, RANDOMBASE* pRNG )
    {
        InfectionTyphoidConfig::p_log_normal_distribution->SetParameters( m, s, 0.0 );
        auto draw = InfectionTyphoidConfig::p_log_normal_distribution->Calculate( pRNG );
        //LOG_VALID_F( "Drew log-normal value of %f from mu=%f and sigma=%f.\n", draw, exp(m), s );
        return draw;
    }


    bool
    InfectionTyphoidConfig::Configure(
        const Configuration * config
    )
    {
        LOG_DEBUG( "Configure\n" );
        
        p_log_normal_distribution = DistributionFactory::CreateDistribution( DistributionFunction::LOG_NORMAL_DISTRIBUTION );
        
        //initConfigTypeMap( "Enable_Contact_Tracing", &tracecontact_mode, Enable_Contact_Tracing_DESC_TEXT, false ); // polio
        bool bRet = JsonConfigurable::Configure( config );
        return bRet;
    }

    BEGIN_QUERY_INTERFACE_BODY(InfectionTyphoid)
        HANDLE_INTERFACE(IInfectionTyphoid)
    END_QUERY_INTERFACE_BODY(InfectionTyphoid)

    InfectionTyphoid::InfectionTyphoid()
    {
    }

    const SimulationConfig*
    InfectionTyphoid::params()
    {
        return GET_CONFIGURABLE(SimulationConfig);
    }

#define SUSCEPT_STATE_LABEL "SUS"
#define PREPAT_STATE_LABEL "PRE"
#define ACUTE_STATE_LABEL "ACU"
#define SUBCLINICAL_STATE_LABEL "SUB"
#define CHRONIC_STATE_LABEL "CHR"
#define DEAD_STATE_LABEL "DED"

    InfectionTyphoid::InfectionTyphoid(IIndividualHumanContext *context) : InfectionEnvironmental(context)
    {
        treatment_multiplier = 1;
        chronic_timer = UNINIT_TIMER;
        subclinical_timer = UNINIT_TIMER;
        chronic_timer_2 = UNINIT_TIMER;
        acute_timer = UNINIT_TIMER; 
        prepatent_timer = UNINIT_TIMER; 
        treatment_timer = UNINIT_TIMER;
        _subclinical_duration = _prepatent_duration = _acute_duration = treatment_day= 0;

        isDead = false;
        last_state_reported = SUSCEPT_STATE_LABEL;
        // TBD: Nasty cast: prefer QI.
        auto doseTracking = ((IndividualHumanTyphoid*)context)->getDoseTracking();

        float mu = mpl;
        float sigma = spl;
        if (doseTracking == "High")
        {
            mu = mph; sigma = sph;
        }

        if (doseTracking == "Low")
        {
            mu = mpl; sigma = spl;
        }
        else
        {
            // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            // !!! The logic here doesn't make sense to me.  
            // !!! We are saying that if doseTracking is not Low, then it is not set.
            // !!! Many of the SFTs are getting this warning but we seem to be ignoring it,
            // !!! hence, I'm commenting this out and creatinga ticket to review it.
            // !!! See GH-2582
            // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            //LOG_WARN_F( "doseTracking not set. This will lead to bad prepatent durations.\n" );
        }

        _prepatent_duration = generateRandFromLogNormal( mu, sigma, context->GetRng() );
        LOG_VALID_F( "Calculated prepatent duration = %f using Log-Normal draw; mu = %f, sigma = %f, doseTracking = %s.\n",
                     _prepatent_duration, mu, sigma, doseTracking.c_str() );
        prepatent_timer = _prepatent_duration + 1;
        //prepatent_timer.handle = std::bind( &InfectionTyphoid::handlePrepatentExpiry, this );
        state_to_report=PREPAT_STATE_LABEL;
        state_changed=true;

        //std::cout << "Initialized prepatent_timer to " << prepatent_timer << " using doseTracking value of " << doseTracking << std::endl;
    }

    void InfectionTyphoid::Initialize(suids::suid _suid)
    {
        InfectionEnvironmental::Initialize(_suid);
    }

    InfectionTyphoid *InfectionTyphoid::CreateInfection(IIndividualHumanContext *context, suids::suid _suid)
    {
        InfectionTyphoid *newinfection = _new_ InfectionTyphoid(context);
        newinfection->Initialize(_suid);

        return newinfection;
    }

    InfectionTyphoid::~InfectionTyphoid()
    {
    }

    void InfectionTyphoid::SetParameters( IStrainIdentity* infstrain, int incubation_period_override )
    {
        CreateInfectionStrain(infstrain);

        if(infstrain == NULL)
        {
            // using default strainIDs
            //infection_strain->SetAntigenID(default_antigen);
        }
        else
        {
            infection_strain->SetAntigenID( infstrain->GetAntigenID() );
            infection_strain->SetGeneticID( infstrain->GetGeneticID() );
        }
    }

    void InfectionTyphoid::InitInfectionImmunology(ISusceptibilityContext* _immunity)
    {
        ISusceptibilityTyphoid* immunity = NULL;
        if( _immunity->QueryInterface( GET_IID( ISusceptibilityTyphoid ), (void**)&immunity ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "_immunity", "ISusceptibilityTyphoid", "Susceptibility" );
        }

        StateChange = InfectionStateChange::New;
        return InfectionEnvironmental::InitInfectionImmunology( _immunity );
    }

    void InfectionTyphoid::handlePrepatentExpiry()
    {
        auto age = dynamic_cast<IIndividualHuman*>(parent)->GetAge() / DAYSPERYEAR;
        auto sex = dynamic_cast<IIndividualHuman*>(parent)->GetGender();
        auto mort = dynamic_cast<IDrugVaccineInterventionEffects*>(parent->GetInterventionsContext())->GetInterventionReducedMortality();
        //LOG_DEBUG_F("hasclin subclinical dur %d, pre %d\n", _subclinical_duration, prepatent_timer); 
        prepatent_timer=UNINIT_TIMER; 
        LOG_DEBUG_F( "Deciding post-prepatent tx using typhoid_symptomatic_fraction=%f.\n", IndividualHumanTyphoidConfig::typhoid_symptomatic_fraction );
        float mu = 0.0f;
        float sigma = 0.0f;

        if( parent->GetRng()->SmartDraw( IndividualHumanTyphoidConfig::typhoid_symptomatic_fraction*mort ) )
        {
            if (age < ageThresholdInYears)
            {
                mu = mau30; sigma = sau30;
            } else {
                mu = mao30; sigma = sao30;
            }
            _acute_duration = generateRandFromLogNormal( mu, sigma, parent->GetRng() ) * DAYSPERWEEK;
            treatment_day = generateRandFromLogNormal( 2.33219066f, 0.5430f, parent->GetRng() );
            LOG_DEBUG_F("Infection stage transition: Individual=%d, Age=%f, Prepatent->Acute: acute dur=%f\n", parent->GetSuid().data, age, _acute_duration);

            acute_timer = int(_acute_duration);
            treatment_timer = int(treatment_day);
            //LOG_INFO_F("treatment timer=%i\n", treatment_timer);

            state_to_report=ACUTE_STATE_LABEL;
        }
        else
        {
            if (age <= ageThresholdInYears)
            {
                mu = msu30; sigma = ssu30;
            } else {
                mu = mso30; sigma = sso30;
            }
            _subclinical_duration = generateRandFromLogNormal( mu, sigma, parent->GetRng() ) * DAYSPERWEEK;
            LOG_DEBUG_F("Infection stage transition: Individual=%d, Age=%f, Prepatent->SubClinical: subc dur=%f\n", parent->GetSuid().data, age, _subclinical_duration );
            subclinical_timer = int(_subclinical_duration);
            chronic_timer_2 = subclinical_timer;
            state_to_report=SUBCLINICAL_STATE_LABEL;
        }
    }

    void InfectionTyphoid::handleAcuteExpiry()
    {
        auto age = dynamic_cast<IIndividualHuman*>(parent)->GetAge() / DAYSPERYEAR;
        auto sex = dynamic_cast<IIndividualHuman*>(parent)->GetGender();

        if( parent->GetRng()->SmartDraw( CFRU ) && (treatment_multiplier < 1) ) // untreated at end of period has higher fatality rate
        {
            isDead = true;
            state_to_report = DEAD_STATE_LABEL;
            LOG_INFO_F( "[Update] Somebody died from their infection.\n" );
        }
        else
        {
            //if they survived, calculate probability of being a carrier
            float p3=0.0; // P3 is age dependent so is determined below. Probability of becoming a chronic carrier from a CLINICAL infection
            float carrier_prob = 0;
            int agebin = int(floor(age/10));
            if (agebin>=GallstoneDataLength)
            {
                agebin=GallstoneDataLength-1;
            }
            if (sex==Gender::FEMALE)
            {
                p3=FemaleGallstones[agebin];
                carrier_prob = IndividualHumanTyphoidConfig::typhoid_carrier_probability;
            } 
            else // if (sex==Gender::MALE)
            {
                p3=MaleGallstones[agebin];
                carrier_prob = IndividualHumanTyphoidConfig::typhoid_carrier_probability;
            }
            

            LOG_DEBUG_F( "Deciding whether to go from acute->chronic based on probability=%f.\n", p3*carrier_prob);
            if( parent->GetRng()->SmartDraw( p3*carrier_prob ) )
            {
                chronic_timer = _chronic_duration;
                LOG_DEBUG_F( "Individual %d age %f, sex %s, just went chronic (from acute) with new timer %d based on gallstone probability of %f and carrier probability of %f.\n",
                             parent->GetSuid().data, age, ( sex==0 ? "Male" : "Female" ), chronic_timer, p3, carrier_prob
                           );
                state_to_report = CHRONIC_STATE_LABEL;
            }
            else
            {
                LOG_VALID_F( "Individual %d age %f, sex %s, just recovered (from acute).\n",
                             parent->GetSuid().data, age, ( sex==0 ? "Male" : "Female" )
                           );
                state_to_report = SUSCEPT_STATE_LABEL;
            }
        } 
        acute_timer = UNINIT_TIMER;
        treatment_multiplier = 1;
    }

    void InfectionTyphoid::handleSubclinicalExpiry()
    {
        auto age = dynamic_cast<IIndividualHuman*>(parent)->GetAge() / DAYSPERYEAR;
        auto sex = dynamic_cast<IIndividualHuman*>(parent)->GetGender();

        //LOG_INFO_F("SOMEONE FINSIHED SUB %d, %d\n", _subclinical_duration, subclinical_timer);
        subclinical_timer = UNINIT_TIMER;
        ProbabilityNumber p2 = 0;
        ProbabilityNumber carrier_prob = 0;
        int agebin = int(floor(age/10));
        if (agebin>=GallstoneDataLength)
        {
            agebin=GallstoneDataLength-1;
        }
        if( sex == Gender::FEMALE )
        {
            p2=FemaleGallstones[agebin];
            carrier_prob = IndividualHumanTyphoidConfig::typhoid_carrier_probability;
        } 
        else // if (sex==0)
        {
            p2=MaleGallstones[agebin];
            carrier_prob = IndividualHumanTyphoidConfig::typhoid_carrier_probability;
        } 

        ProbabilityNumber prob = float(p2)*float(carrier_prob);
        LOG_VALID_F( "Deciding whether individual goes chronic (vs recovered) based on probability of %f.\n", float(prob) );
        if( parent->GetRng()->SmartDraw( prob ) )
        {
            chronic_timer = _chronic_duration;
            LOG_DEBUG_F( "Individual %d age %f, sex %s, just went chronic (from subclinical) with new timer %d based on gallstone probability of %f and carrier probability of %f.\n",
                         parent->GetSuid().data, age, ( sex==0 ? "Male" : "Female" ), chronic_timer, float(p2), float(carrier_prob)
                       );
            state_to_report = CHRONIC_STATE_LABEL;
        }
        else
        {
            LOG_VALID_F( "Individual %d age %f, sex %s, just recovered (from subclinical).\n",
                         parent->GetSuid().data, age, ( sex==0 ? "Male" : "Female" )
                       );
            state_to_report = SUSCEPT_STATE_LABEL;
        }
    }

    void InfectionTyphoid::Update(float dt, ISusceptibilityContext* _immunity)
    {
        auto age = dynamic_cast<IIndividualHuman*>(parent)->GetAge() / DAYSPERYEAR;
        LOG_DEBUG_F("%d age %f INFECTED! prepat=%d,acute=%d,subclin=%d,chronic=%d\n", parent->GetSuid().data, age, (int) prepatent_timer, acute_timer, subclinical_timer,chronic_timer);
        /*if( prepatent_timer.IsDead() == false )
        {
            LOG_VALID_F( "Decrementing prepat timer (%f) for individual %d.\n", float( prepatent_timer ), parent->GetSuid().data );
            prepatent_timer.Decrement( dt );
        }*/
        if( prepatent_timer > UNINIT_TIMER)
        {
            LOG_VALID_F( "Decrementing prepat timer (%f) for individual %d.\n", float( prepatent_timer ), parent->GetSuid().data );
            prepatent_timer -= dt;
            if (UNINIT_TIMER<prepatent_timer && prepatent_timer<=0)
            {
                handlePrepatentExpiry();
            }
        }
        else if (subclinical_timer > UNINIT_TIMER)
        { // asymptomatic infection
            //              LOG_INFO_F("is subclinical dur %d, %d, %d\n", _subclinical_duration, subclinical_timer, dt);
            state_to_report=SUBCLINICAL_STATE_LABEL;
            subclinical_timer -= dt;
            if (UNINIT_TIMER<subclinical_timer && subclinical_timer<=0)
            {
                handleSubclinicalExpiry();
            }
        }
        else if (acute_timer > UNINIT_TIMER)
        {
            // acute infection
            state_to_report = ACUTE_STATE_LABEL;
            acute_timer -= dt;
            treatment_timer -= dt;
            int acute_day = int(_acute_duration) - int(acute_timer) + 1; // 1-based counting

            if( (acute_day == treatment_day)  && parent->GetRng()->SmartDraw( treatmentprobability ) )
            {       //if they seek treatment and don't die, we are assuming they have a probability of becoming a carrier (chloramphenicol treatment does not prevent carriage)
                // so they either get treatment or die?
                if( parent->GetRng()->SmartDraw( CFRH ) )
                {
                    isDead = true;
                    state_to_report = DEAD_STATE_LABEL;
                    acute_timer = UNINIT_TIMER;
                }
                else
                {
                    treatment_multiplier = 1;
                    LOG_VALID_F( "Individual ID: %d, State: Acute, GetTreatment: True.\n", parent->GetSuid().data );
                }
            }

            if (acute_timer<= 0)
            {
                handleAcuteExpiry();
            }
        }

        //Assume all acute individuals seek treatment since this is Mike's "reported" fraction
        //Some fraction are treated effectively, some become chronic carriers, some die, some remain infectious


        else if (chronic_timer > UNINIT_TIMER)
        {
            state_to_report=CHRONIC_STATE_LABEL;
            chronic_timer -= dt;
            if (chronic_timer_2 <= DAYSPERYEAR)
                chronic_timer_2 += dt;
//            LOG_INFO_F("Chronic_timer = %d, chronic_timer_2 = %d\n", chronic_timer, chronic_timer_2);
            if (UNINIT_TIMER< chronic_timer && chronic_timer<=0)
                chronic_timer = UNINIT_TIMER;
        }

        if (last_state_reported==state_to_report)
        {
            // typhoid state is the same as before
            state_changed = false;
        }
        else
        {
            // typhoid state changed
            last_state_reported=state_to_report;
            state_changed = true;
        }
        LOG_DEBUG_F( "state_to_report for individual %d = %s\n", parent->GetSuid().data, state_to_report.c_str() );

        if( state_to_report == SUSCEPT_STATE_LABEL && state_changed ) // && GetInfections().size() > 0 )
        {
            Clear();
        }
        else if( state_to_report == DEAD_STATE_LABEL && state_changed )
        {    
            LOG_INFO_F( "[Update] Somebody died from their infection.\n" );
        }
        return;
    }


    float InfectionTyphoid::GetInfectiousness() const
    {
        float infectiousness = 0.0f;
        float base_infectiousness = IndividualHumanTyphoidConfig::typhoid_acute_infectiousness;
        auto irt = dynamic_cast<IDrugVaccineInterventionEffects*>(parent->GetInterventionsContext())->GetInterventionReducedTransmit();
        if (acute_timer>0)
        {
            infectiousness = treatment_multiplier*base_infectiousness*irt;
            LOG_VALID_F( "ACUTE infectiousness calculated as %f\n", infectiousness );
        }
        else if (prepatent_timer>0)
        {
            infectiousness = base_infectiousness*IndividualHumanTyphoidConfig::typhoid_prepatent_relative_infectiousness*irt;
            LOG_VALID_F( "PREPATENT infectiousness calculated as %f\n", infectiousness );
        }
        else if (subclinical_timer>0)
        {
            infectiousness = base_infectiousness*IndividualHumanTyphoidConfig::typhoid_subclinical_relative_infectiousness*irt;
            LOG_VALID_F( "SUBCLINICAL infectiousness calculated as %f\n", infectiousness );
        }
        else if (chronic_timer>0)
        {
            infectiousness = base_infectiousness*IndividualHumanTyphoidConfig::typhoid_chronic_relative_infectiousness*irt;
            float SimDay = dynamic_cast<IIndividualHuman*>(parent)->GetParent()->GetTime().time;
            float SimYear = (SimDay / DAYSPERYEAR) + 1875; // bad magic number, but not sure what name to give it
            LOG_VALID_F( "CHRONIC infectiousness calculated as %f\n", infectiousness );
        }
        return infectiousness;
    }

    void InfectionTyphoid::Clear()
    {
        LOG_DEBUG_F( "Infection cleared.\n" );
        StateChange = InfectionStateChange::Cleared;
    }

    REGISTER_SERIALIZABLE(InfectionTyphoid);

    void InfectionTyphoid::serialize(IArchive& ar, InfectionTyphoid* obj)
    {
        InfectionTyphoid& infection = *obj;
        ar.labelElement("prepatent_timer") & infection.prepatent_timer;
        ar.labelElement("prepatent_duration") & infection._prepatent_duration;
        ar.labelElement("acute_timer") & infection.acute_timer;
        ar.labelElement("treatment_timer") & infection.treatment_timer;
        ar.labelElement("acute_duration") & infection._acute_duration;
        ar.labelElement("subclinical_timer") & infection.subclinical_timer;
        ar.labelElement("subclinical_duration") & infection._subclinical_duration;
        ar.labelElement("chronic_timer") & infection.chronic_timer;
        //ar.labelElement("chronic_duration") & infection._chronic_duration;
        ar.labelElement("state_to_report") & infection.state_to_report;
    }
}

#endif // ENABLE_TYPHOID
