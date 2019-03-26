/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "Drugs.h"
#include "RANDOM.h"
#include "Sigmoid.h"
#include "SimulationEnums.h"        // Just for PKPDModel parameter (!)
#include "IIndividualHumanContext.h"
#include "DistributionFactory.h"

SETUP_LOGGING( "GenericDrug" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(GenericDrug)
        HANDLE_INTERFACE(IConfigurable)
        HANDLE_INTERFACE(IDistributableIntervention)
        HANDLE_INTERFACE(IDrug)
        HANDLE_INTERFACE(IBaseIntervention)
        HANDLE_ISUPPORTS_VIA(IDistributableIntervention)
    END_QUERY_INTERFACE_BODY(GenericDrug)

#ifndef INTERVENTIONS_AS_DLLS
    //IMPLEMENT_FACTORY_REGISTERED(GenericDrug)
#endif
    bool
    GenericDrug::Configure(
        const Configuration * inputJson
    )
    {
        initConfigTypeMap("Primary_Decay_Time_Constant", &fast_decay_time_constant, DRUG_Primary_Decay_Time_Constant_DESC_TEXT, 0, 999999);
        initConfigTypeMap("Remaining_Doses", &remaining_doses, DRUG_Remaining_Doses_DESC_TEXT, -1, 999999);
        initConfigTypeMap("Dose_Interval", &time_between_doses, DRUG_Dose_Interval_DESC_TEXT, 0.0f, 99999.0f, 1.0f);
        initConfigTypeMap("Cost_To_Consumer", &cost_per_unit, DRUG_Cost_To_Consumer_DESC_TEXT, 0, 99999);
        initConfigTypeMap("Fraction_Defaulters", &fraction_defaulters, DRUG_Fraction_Defaulters_DESC_TEXT, 0.0f, 1.0f, 0.0f);

        initConfig( "Durability_Profile", durability_time_profile, inputJson, MetadataDescriptor::Enum("Durability_Profile", DRUG_Durability_Profile_DESC_TEXT, MDD_ENUM_ARGS(PKPDModel)) );
        if (durability_time_profile == PKPDModel::CONCENTRATION_VERSUS_TIME || JsonConfigurable::_dryrun)
        {
            initConfigTypeMap("Secondary_Decay_Time_Constant", &slow_decay_time_constant, DRUG_Secondary_Decay_Time_Constant_DESC_TEXT, 0, 999999);
            initConfigTypeMap("Drug_CMax", &Cmax, DRUG_Drug_CMax_DESC_TEXT, 0, 10000);
            initConfigTypeMap("Drug_Vd", &Vd, DRUG_Drug_Vd_DESC_TEXT, 0, 10000);
            initConfigTypeMap("Drug_PKPD_C50", &drug_c50, DRUG_Drug_PKPD_C50_DESC_TEXT, 0, 5000);
        }

        bool configured = BaseIntervention::Configure( inputJson );

        PkPdParameterValidation();

        return configured;
    }

    void GenericDrug::PkPdParameterValidation()
    {
        if ( durability_time_profile == PKPDModel::CONCENTRATION_VERSUS_TIME )
        {
            // Validate that long-decay mode (slow_decay_time_constant) is longer than short-decay mode (fast_decay_time_constant)
            // N.B. In the case that they are equal, Vd is ignored and the system reverts to a single-compartment PkPd model.

            if ( slow_decay_time_constant < fast_decay_time_constant )
            {
                //throw IncoherentInitializationException( __FILE__, __LINE__, __FUNCTION__, "slow_decay_time_constant", slow_decay_time_constant, "fast_decay_time_constant", fast_decay_time_constant );
                throw InitializationException( __FILE__, __LINE__, __FUNCTION__, "Value of drug \'slow_decay_time_constant\' must be greater or equal to \'fast_decay_time_constant\'." );
            }

            // Validate that (slow_decay_time_constant/Vd) is greater than fast_decay_time_constant.
            // Or else, the input parameters do not make sense as a solution to the two-compartment PkPd model
            // with the concentration in the central compartment as the sum of two exponentials with the two eigenvalues
            // being the specified decay times.

            if ( (slow_decay_time_constant != fast_decay_time_constant) && (slow_decay_time_constant / Vd) <= fast_decay_time_constant )
            {
                throw InitializationException( __FILE__, __LINE__, __FUNCTION__, "Ratio of drug value \'slow_decay_time_constant\' over \'Drug_Vd\' must be greater than \'primary_decay_constant\'.  Otherwise, the parameters do not make sense as a solution to the two-compartment PkPd model, with the concentration in the central compartment as the sum of two exponentials with the two eigenvalues being the specified decay times:\n\n                     _______________________                     __________________________\n    Cmax(t=0) --->  /  Central compartment  \\  ---- k_CP --->   /  Peripheral compartment  \\\n                    \\_______________________/  <--- k_PC ----   \\__________________________/\n                               |\n                             k_out\n                               |\n                               v" );
            }

            //                     _______________________                     __________________________
            //    Cmax(t=0) --->  /  Central compartment  \  ---- k_CP --->   /  Peripheral compartment  \
            //                    \_______________________/  <--- k_PC ----   \__________________________/
            //                               |
            //                             k_out
            //                               |
            //                               v
        }
    }

    GenericDrug::GenericDrug( const std::string& rDefaultName )
        : BaseIntervention()
        , drug_name( rDefaultName )
        , durability_time_profile(PKPDModel::FIXED_DURATION_CONSTANT_EFFECT)
        , fast_decay_time_constant(0)
        , slow_decay_time_constant(0)
        , dosing_timer(0)
        , remaining_doses(1)
        , time_between_doses(0)
        , fast_component(0)
        , slow_component(0)
        , start_concentration(0)
        , end_concentration(0)
        , current_concentration( 0 )
        , current_efficacy(0)
        , current_reducedacquire(0)  // NOTE: malaria drug has specific killing effects, TB drugs have inactivation + cure rates
        , current_reducedtransmit(0) //   "    "
        , pk_rate_mod(1.0)           // homogeneous by default
        , Cmax(0)
        , Vd(0)
        , drug_c50(0)
        , fraction_defaulters(0)
        , p_uniform_distribution( DistributionFactory::CreateDistribution( DistributionFunction::UNIFORM_DISTRIBUTION ) )
    {
    }

    GenericDrug::GenericDrug( const GenericDrug& rMaster )
        : BaseIntervention( rMaster )
        , drug_name( rMaster.drug_name )
        , durability_time_profile(rMaster.durability_time_profile)
        , fast_decay_time_constant(rMaster.fast_decay_time_constant)
        , slow_decay_time_constant(rMaster.slow_decay_time_constant)
        , dosing_timer(rMaster.dosing_timer)
        , remaining_doses(rMaster.remaining_doses)
        , time_between_doses(rMaster.time_between_doses)
        , fast_component(rMaster.fast_component)
        , slow_component(rMaster.slow_component)
        , start_concentration(rMaster.start_concentration)
        , end_concentration(rMaster.end_concentration)
        , current_concentration( rMaster.current_concentration )
        , current_efficacy(rMaster.current_efficacy)
        , current_reducedacquire(rMaster.current_reducedacquire)  // NOTE: malaria drug has specific killing effects, TB drugs have inactivation + cure rates
        , current_reducedtransmit(rMaster.current_reducedtransmit) //   "    "
        , pk_rate_mod(rMaster.pk_rate_mod)           // homogeneous by default
        , Cmax(rMaster.Cmax)
        , Vd(rMaster.Vd)
        , drug_c50(rMaster.drug_c50)
        , fraction_defaulters(rMaster.fraction_defaulters)
        , p_uniform_distribution( DistributionFactory::CreateDistribution( DistributionFunction::UNIFORM_DISTRIBUTION ) )
    {
    }

    GenericDrug::~GenericDrug()
    {
        delete p_uniform_distribution;
    }

    int
    GenericDrug::AddRef()
    {
        return BaseIntervention::AddRef();
    }

    int
    GenericDrug::Release()
    {
        return BaseIntervention::Release();
    }

    void
    GenericDrug::ConfigureDrugTreatment( IIndividualHumanInterventionsContext * ivc )
    { 
    }

    const std::string&
    GenericDrug::GetDrugName() const
    {
        return drug_name;
    }

    float
    GenericDrug::GetDrugCurrentConcentration() const
    {
        return current_concentration;
    }

    float
    GenericDrug::GetDrugCurrentEfficacy() const
    {
        return current_efficacy;
    }

    float
    GenericDrug::GetDrugReducedAcquire()  const
    {
        return current_efficacy * current_reducedacquire;
    }

    float
    GenericDrug::GetDrugReducedTransmit() const
    {
        LOG_DEBUG_F("GetDrugReducedTransmit is efficacy (%f) * transmission-reduction factor (%f) = %f \n ",  current_efficacy, current_reducedtransmit, current_efficacy*current_reducedtransmit);
        return current_efficacy * current_reducedtransmit;
    }

    bool GenericDrug::Distribute( IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pICCO )
    {
        // shouldn't really need to do this specially since ConfigureDrugTreatment()
        // takes a context, but this will limit code changes.
        SetContextTo( context->GetParent() );

        ConfigureDrugTreatment( context );

        return BaseIntervention::Distribute( context, pICCO );
    }


    // I think we can get rid of this altogether
    void
    GenericDrug::Update(float dt)
    {
        if( !BaseIntervention::UpdateIndividualsInterventionStatus() ) return;

        switch (durability_time_profile)
        {
            case PKPDModel::FIXED_DURATION_CONSTANT_EFFECT:
                SimpleUpdate(dt);
                break;
            case PKPDModel::CONCENTRATION_VERSUS_TIME:
                UpdateWithPkPd(dt);
                break;
            default:
                throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__, "durability_time_profile", durability_time_profile, PKPDModel::pairs::lookup_key(durability_time_profile) );
        }
        if( !expired )
        {
            ApplyEffects();
        }
    }

    void GenericDrug::ResetForNextDose(float dt)
    {
        dosing_timer = time_between_doses;
        remaining_doses--;

        if (remaining_doses != 0 && time_between_doses < dt)
        {
            std::ostringstream oss;
            oss << "Time to next dose (" << time_between_doses << ") is shorter than the time-step, dt (" << dt << ")" << std::endl;
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, oss.str().c_str() );
        }
    }

    void GenericDrug::SimpleUpdate(float dt)
    {
        // Do everything based on fast_component ignoring slow_component
        LOG_DEBUG_F("Drug compartment = %0.2f\n", fast_component);

        // Time Decay
        if ( fast_component > 0 ) { fast_component -= dt; }

        // New Doses - remaining_doses = -1 implies infinite doses
        if ( remaining_doses != 0 )
        {
            dosing_timer -= dt;
            LOG_DEBUG_F("Remaining doses = %d, Dosing timer = %0.3f\n", remaining_doses, dosing_timer);
            if ( (dosing_timer <= 0) && IsTakingDose( dt ) )
            {
                // DJK: Remove or fix fraction_defaulters.  It only makes sense with remaining_doses=1 <ERAD-1854>
                if( parent->GetRng()->SmartDraw( fraction_defaulters ) )
                {
                    // Assume uniformly distributed dropout times, cf. Fig 3 from Kruk 2008 Trop Med Int Health 13:703
                    p_uniform_distribution->SetParameters( 1, fast_decay_time_constant, 0.0 );
                    fast_component = p_uniform_distribution->Calculate( parent->GetRng() );

                    LOG_DEBUG_F("Individual dropout time = %0.2f\n", fast_component);
                }
                else
                {
                    fast_component = fast_decay_time_constant;
                }
                ResetForNextDose(dt);
                LOG_DEBUG_F("Distributed next dose: remaining doses = %d, Drug compartment = %0.2f, Dosing timer = %0.3f\n", remaining_doses, fast_component, dosing_timer);
            }
        }

        // Efficacy
        if ( fast_component > 0 )
        {
            current_efficacy = 1.0;
            current_concentration = 1.0;
        }
        else
        {
            LOG_DEBUG("Duration of drug effectiveness is finished.\n");
            current_efficacy = 0;
            current_concentration = 0.0;
            if (remaining_doses == 0) // remaining_doses = -1 implies infinite doses
            {
                LOG_DEBUG("Effectiveness of last dose is finished. Expiring drug.\n");
                Expire();
            }
        }
    }

    void GenericDrug::UpdateWithPkPd(float dt)
    {
        // New Doses - remaining_doses = -1 implies infinite doeses
        if ( remaining_doses != 0 )
        {
            dosing_timer -= dt;
            LOG_DEBUG_F("Remaining doses = %d, Dosing timer = %0.3f\n", remaining_doses, dosing_timer);
            if ( (dosing_timer <= 0) && IsTakingDose( dt ) )
            {
                float slow_component_fraction = 0;
                if ( fast_decay_time_constant == slow_decay_time_constant )
                {
                    // DJK: Should be easier to configure first order pkpd (see <ERAD-1853>)
                    slow_component_fraction = 0;
                }
                else
                {
                    slow_component_fraction = (slow_decay_time_constant/Vd - fast_decay_time_constant) /
                                       (slow_decay_time_constant - fast_decay_time_constant);
                    LOG_DEBUG_F("fast_component_fraction = %0.2f for Tfast=%0.2f, Tslow=%0.2f, Vd=%0.2f\n", 1-slow_component_fraction, fast_decay_time_constant, slow_decay_time_constant, Vd);
                }

                // DJK: Why not Cmax - Cmin? <ERAD-1855>
                fast_component += Cmax*(1-slow_component_fraction);         // Central 1=primary=fast, 2=secondary=slow
                slow_component += Cmax*slow_component_fraction;

                ResetForNextDose(dt);
                LOG_DEBUG_F("Distributed next dose: remaining doses = %d, Drug components (fast,slow) = (%0.2f, %0.2f), Dosing timer = %0.3f\n", remaining_doses, fast_component, slow_component, dosing_timer);
            }
        }

        // Time Decay: cache start-of-time-step concentration; then calculate exponential decay
        LOG_DEBUG_F("Start-of-timestep components (fast,slow) = (%0.2f, %0.2f)\n", fast_component, slow_component);
        start_concentration = fast_component + slow_component;
        if ( fast_component > 0 || slow_component > 0 )
        {
            if ( fast_decay_time_constant > 0 && fast_component > 0)
            {
                fast_component *= exp(-dt/fast_decay_time_constant);
            }
            if ( slow_decay_time_constant > 0 && slow_component > 0)
            {
                slow_component *= exp(-dt/slow_decay_time_constant);
            }
        }
        LOG_DEBUG_F("End-of-timestep compartments = (%0.2f, %0.2f)\n", fast_component, slow_component);
        end_concentration = fast_component + slow_component;

        current_concentration = end_concentration;

        current_efficacy = CalculateEfficacy( drug_c50, start_concentration, end_concentration );

        // TODO: should drugs with efficacy below some threshold be:
        //       (1) zeroed out to avoid continuing to do the calculations above ad infinitum (where is killing-effect and/or resistance-pressure negligible?)
        //       (2) removed from the InterventionsContainer (potentially some reporting is relying on the object for treatment history)
    }

    float GenericDrug::CalculateEfficacy( float c50, float startConcentation, float endConcentration )
    {
        float efficacy = current_efficacy;
        if( durability_time_profile == PKPDModel::CONCENTRATION_VERSUS_TIME )
        {
            if( endConcentration > 0 )
            {
                // Efficacy: approximate average efficacy over the dt step
                // DJK: pkpd should be separate from efficacy! <ERAD-1853>
                float start_efficacy = Sigmoid::basic_sigmoid( c50, startConcentation );
                float end_efficacy   = Sigmoid::basic_sigmoid( c50, endConcentration  );
                efficacy = 0.5 * (start_efficacy + end_efficacy);
                LOG_DEBUG_F( "Drug efficacy = %0.2f  (Conc,Eff) at start = (%0.2f,%0.2f)  at end = (%0.2f,%0.2f)\n", current_efficacy, startConcentation, start_efficacy, endConcentration, end_efficacy );
            }
        }
        return efficacy;
    }

    // no-op
    void GenericDrug::ApplyEffects()
    {
    }

    void GenericDrug::Expire()
    {
        expired = true;
    }

    REGISTER_SERIALIZABLE(GenericDrug);

    void GenericDrug::serialize(IArchive& ar, GenericDrug* obj)
    {
        BaseIntervention::serialize( ar, obj );
        GenericDrug& drug = *obj;
        ar.labelElement("drug_name") & drug.drug_name;
        ar.labelElement("durability_time_profile") & (uint32_t&)drug.durability_time_profile;
        ar.labelElement("fast_decay_time_constant") & drug.fast_decay_time_constant;
        ar.labelElement("slow_decay_time_constant") & drug.slow_decay_time_constant;
        ar.labelElement("dosing_timer") & drug.dosing_timer;
        ar.labelElement("remaining_doses") & drug.remaining_doses;
        ar.labelElement("time_between_doses") & drug.time_between_doses;
        ar.labelElement("fast_component") & drug.fast_component;
        ar.labelElement("slow_component") & drug.slow_component;
        ar.labelElement("start_concentration") & drug.start_concentration;
        ar.labelElement("end_concentration") & drug.end_concentration;
        ar.labelElement("current_concentration") & drug.current_concentration;
        ar.labelElement("current_efficacy") & drug.current_efficacy;
        ar.labelElement("current_reducedacquire") & drug.current_reducedacquire;
        ar.labelElement("current_reducedtransmit") & drug.current_reducedtransmit;
        ar.labelElement("pk_rate_mod") & drug.pk_rate_mod;
        ar.labelElement("Cmax") & drug.Cmax;
        ar.labelElement("Vd") & drug.Vd;
        ar.labelElement("drug_c50") & drug.drug_c50;
        ar.labelElement("fraction_defaulters") & drug.fraction_defaulters;
    }
}
