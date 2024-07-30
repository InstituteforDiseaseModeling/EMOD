
#include "stdafx.h"
#include "DrugModel.h"
#include "RANDOM.h"
#include "MathFunctions.h"
#include "Sigmoid.h"
#include "IIndividualHumanContext.h"
#include "DistributionFactory.h"

SETUP_LOGGING( "DrugModel" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY( DrugModel )
        HANDLE_INTERFACE( IDrug )
        HANDLE_ISUPPORTS_VIA(ISerializable)
    END_QUERY_INTERFACE_BODY( DrugModel )

    DrugModel::DrugModel( const std::string& rDefaultName )
        : drug_name( rDefaultName )
        , durability_time_profile( PKPDModel::FIXED_DURATION_CONSTANT_EFFECT )
        , fast_decay_time_constant( 0 )
        , slow_decay_time_constant( 0 )
        , fast_component( 0 )
        , slow_component( 0 )
        , start_concentration( 0 )
        , end_concentration( 0 )
        , current_concentration( 0 )
        , current_efficacy( 0 )
        , current_reducedacquire( 0 )  // NOTE: malaria drug has specific killing effects, TB drugs have inactivation + cure rates
        , current_reducedtransmit( 0 ) //   "    "
        , pk_rate_mod( 1.0 )           // homogeneous by default
        , Cmax( 0 )
        , Vd( 0 )
        , drug_c50( 0 )
        , fraction_defaulters( 0 )
        , p_uniform_distribution( DistributionFactory::CreateDistribution( DistributionFunction::UNIFORM_DISTRIBUTION ) )
    {
    }

    DrugModel::DrugModel( const DrugModel& rMaster )
        : drug_name( rMaster.drug_name )
        , durability_time_profile( rMaster.durability_time_profile )
        , fast_decay_time_constant( rMaster.fast_decay_time_constant )
        , slow_decay_time_constant( rMaster.slow_decay_time_constant )
        , fast_component( rMaster.fast_component )
        , slow_component( rMaster.slow_component )
        , start_concentration( rMaster.start_concentration )
        , end_concentration( rMaster.end_concentration )
        , current_concentration( rMaster.current_concentration )
        , current_efficacy( rMaster.current_efficacy )
        , current_reducedacquire( rMaster.current_reducedacquire )
        , current_reducedtransmit( rMaster.current_reducedtransmit )
        , pk_rate_mod( rMaster.pk_rate_mod )
        , Cmax( rMaster.Cmax )
        , Vd( rMaster.Vd )
        , drug_c50( rMaster.drug_c50 )
        , fraction_defaulters( rMaster.fraction_defaulters )
        , p_uniform_distribution( rMaster.p_uniform_distribution->Clone() )
    {
    }
    
    DrugModel::~DrugModel()
    {
        delete p_uniform_distribution;
    }

    DrugModel* DrugModel::Clone()
    {
        return new DrugModel( *this );
    }

    void DrugModel::PkPdParameterValidation()
    {
        if( durability_time_profile == PKPDModel::CONCENTRATION_VERSUS_TIME )
        {
            // Validate that long-decay mode (slow_decay_time_constant) is longer than short-decay mode (fast_decay_time_constant)
            // N.B. In the case that they are equal, Vd is ignored and the system reverts to a single-compartment PkPd model.

            if( slow_decay_time_constant < fast_decay_time_constant )
            {
                throw InitializationException( __FILE__, __LINE__, __FUNCTION__, "Value of drug \'slow_decay_time_constant\' must be greater or equal to \'fast_decay_time_constant\'." );
            }

            // Validate that (slow_decay_time_constant/Vd) is greater than fast_decay_time_constant.
            // Or else, the input parameters do not make sense as a solution to the two-compartment PkPd model
            // with the concentration in the central compartment as the sum of two exponentials with the two eigenvalues
            // being the specified decay times.

            if( (slow_decay_time_constant != fast_decay_time_constant) && (slow_decay_time_constant / Vd) <= fast_decay_time_constant )
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

    void DrugModel::ConfigureDrugTreatment( IIndividualHumanInterventionsContext * ivc )
    {
    }

    const std::string& DrugModel::GetDrugName() const
    {
        return drug_name;
    }

    int DrugModel::GetNumRemainingDoses() const
    {
        release_assert( false ); // shouldn't be called
        return 0;
    }

    float DrugModel::GetDrugCurrentConcentration() const
    {
        return current_concentration;
    }

    float DrugModel::GetDrugCurrentEfficacy() const
    {
        return current_efficacy;
    }

    float DrugModel::GetDrugReducedAcquire()  const
    {
        return current_efficacy * current_reducedacquire;
    }

    float DrugModel::GetDrugReducedTransmit() const
    {
        LOG_DEBUG_F( "GetDrugReducedTransmit is efficacy (%f) * transmission-reduction factor (%f) = %f \n ", current_efficacy, current_reducedtransmit, current_efficacy*current_reducedtransmit );
        return current_efficacy * current_reducedtransmit;
    }

    void DrugModel::SetDrugName( const std::string& rDrugName )
    {
        drug_name = rDrugName;
    }

    void DrugModel::SetFastDecayTimeConstant( float fastDecay )
    {
        fast_decay_time_constant = fastDecay;
    }


    void DrugModel::SetDrugCurrentEfficacy( float efficacy )
    {
        current_efficacy = efficacy;
    }

    void DrugModel::SetDrugReducedAcquire( float reducedAcquire )
    {
        current_reducedacquire = reducedAcquire;
    }

    void DrugModel::SetDrugReducedTransmit( float reducedTransmit )
    {
        current_reducedtransmit = reducedTransmit;
    }

    void DrugModel::TakeDose( float dt, RANDOMBASE* pRNG )
    {
        switch( durability_time_profile )
        {
            case PKPDModel::FIXED_DURATION_CONSTANT_EFFECT:
                TakeDoseSimple( dt, pRNG );
                break;

            case PKPDModel::CONCENTRATION_VERSUS_TIME:
                TakeDoseWithPkPd( dt, pRNG );
                break;

            default:
                throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__, "durability_time_profile", durability_time_profile, PKPDModel::pairs::lookup_key( durability_time_profile ) );
        }
    }

    void DrugModel::TakeDoseSimple( float dt, RANDOMBASE* pRNG )
    {
        // DJK: Remove or fix fraction_defaulters.  It only makes sense with remaining_doses=1 <ERAD-1854>
        if( pRNG->SmartDraw( fraction_defaulters ) )
        {
            // Assume uniformly distributed dropout times, cf. Fig 3 from Kruk 2008 Trop Med Int Health 13:703
            p_uniform_distribution->SetParameters( 1.0f, fast_decay_time_constant, 0.0f );
            fast_component = p_uniform_distribution->Calculate( pRNG );
            LOG_DEBUG_F( "Individual dropout time = %0.2f\n", fast_component );
        }
        else
        {
            fast_component = fast_decay_time_constant;
        }
        LOG_DEBUG_F( "Distributed next dose: Drug compartment = %0.2f\n", fast_component );

        // add dt because it will be subtracted on decay.
        fast_component += dt;
    }

    void DrugModel::TakeDoseWithPkPd( float dt, RANDOMBASE* pRNG )
    {
        float slow_component_fraction = 0;
        if( fast_decay_time_constant == slow_decay_time_constant )
        {
            // DJK: Should be easier to configure first order pkpd (see <ERAD-1853>)
            slow_component_fraction = 0;
        }
        else
        {
            slow_component_fraction = (slow_decay_time_constant / Vd - fast_decay_time_constant) /
                (slow_decay_time_constant - fast_decay_time_constant);
            LOG_DEBUG_F( "fast_component_fraction = %0.2f for Tfast=%0.2f, Tslow=%0.2f, Vd=%0.2f\n", 1 - slow_component_fraction, fast_decay_time_constant, slow_decay_time_constant, Vd );
        }

        // DJK: Why not Cmax - Cmin? <ERAD-1855>
        fast_component += Cmax*(1 - slow_component_fraction);         // Central 1=primary=fast, 2=secondary=slow
        slow_component += Cmax*slow_component_fraction;

        LOG_DEBUG_F( "Distributed next dose: Drug components (fast,slow) = (%0.2f, %0.2f)\n", fast_component, slow_component );
    }

    void DrugModel::DecayAndUpdateEfficacy( float dt )
    {
        switch( durability_time_profile )
        {
            case PKPDModel::FIXED_DURATION_CONSTANT_EFFECT:
                DecayAndUpdateEfficacySimple( dt );
                break;

            case PKPDModel::CONCENTRATION_VERSUS_TIME:
                DecayAndUpdateEfficacyWithPkPd( dt );
                break;

            default:
                throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__, "durability_time_profile", durability_time_profile, PKPDModel::pairs::lookup_key( durability_time_profile ) );
        }
    }

    void DrugModel::DecayAndUpdateEfficacySimple( float dt )
    {
        // Time Decay
        if( fast_component > 0 )
        {
            fast_component -= dt;
        }

        // Efficacy
        if( fast_component > 0 )
        {
            current_efficacy = 1.0;
            current_concentration = 1.0;
        }
        else
        {
            LOG_DEBUG( "Duration of drug effectiveness is finished.\n" );
            current_efficacy = 0;
            current_concentration = 0.0;
        }
    }

    void DrugModel::DecayAndUpdateEfficacyWithPkPd( float dt )
    {
        // Time Decay: cache start-of-time-step concentration; then calculate exponential decay
        LOG_DEBUG_F( "Start-of-timestep components (fast,slow) = (%0.2f, %0.2f)\n", fast_component, slow_component );
        start_concentration = fast_component + slow_component;
        if( fast_component > 0 || slow_component > 0 )
        {
            if( fast_decay_time_constant > 0 && fast_component > 0 )
            {
                fast_component *= exp( -dt / fast_decay_time_constant );
            }
            if( slow_decay_time_constant > 0 && slow_component > 0 )
            {
                slow_component *= exp( -dt / slow_decay_time_constant );
            }
        }

        LOG_DEBUG_F( "End-of-timestep compartments = (%0.2f, %0.2f)\n", fast_component, slow_component );
        end_concentration = fast_component + slow_component;

        current_concentration = end_concentration;

        current_efficacy = CalculateEfficacy( drug_c50, start_concentration, end_concentration );

        // TODO: should drugs with efficacy below some threshold be:
        //       (1) zeroed out to avoid continuing to do the calculations above ad infinitum (where is killing-effect and/or resistance-pressure negligible?)
        //       (2) removed from the InterventionsContainer (potentially some reporting is relying on the object for treatment history)
    }

    float DrugModel::CalculateEfficacy( float c50, float startConcentration, float endConcentration )
    {
        float efficacy = current_efficacy;
        if( durability_time_profile == PKPDModel::CONCENTRATION_VERSUS_TIME )
        {
            if( endConcentration > 0 )
            {
                // Efficacy: approximate average efficacy over the dt step
                // DJK: pkpd should be separate from efficacy! <ERAD-1853>
                float start_efficacy = Sigmoid::basic_sigmoid( c50, startConcentration );
                float end_efficacy = Sigmoid::basic_sigmoid( c50, endConcentration );
                efficacy = 0.5 * (start_efficacy + end_efficacy);
                LOG_DEBUG_F( "Drug efficacy = %0.2f  (Conc,Eff) at start = (%0.2f,%0.2f)  at end = (%0.2f,%0.2f)\n", current_efficacy, startConcentration, start_efficacy, endConcentration, end_efficacy );
            }
        }
        return efficacy;
    }

    REGISTER_SERIALIZABLE( DrugModel );

    void DrugModel::serialize( IArchive& ar, DrugModel* obj )
    {
        DrugModel& drug = *obj;
        ar.labelElement( "drug_name" ) & drug.drug_name;
        ar.labelElement( "durability_time_profile" ) & (uint32_t&)drug.durability_time_profile;
        ar.labelElement( "fast_decay_time_constant" ) & drug.fast_decay_time_constant;
        ar.labelElement( "slow_decay_time_constant" ) & drug.slow_decay_time_constant;
        ar.labelElement( "fast_component" ) & drug.fast_component;
        ar.labelElement( "slow_component" ) & drug.slow_component;
        ar.labelElement( "start_concentration" ) & drug.start_concentration;
        ar.labelElement( "end_concentration" ) & drug.end_concentration;
        ar.labelElement( "current_concentration" ) & drug.current_concentration;
        ar.labelElement( "current_efficacy" ) & drug.current_efficacy;
        ar.labelElement( "current_reducedacquire" ) & drug.current_reducedacquire;
        ar.labelElement( "current_reducedtransmit" ) & drug.current_reducedtransmit;
        ar.labelElement( "pk_rate_mod" ) & drug.pk_rate_mod;
        ar.labelElement( "Cmax" ) & drug.Cmax;
        ar.labelElement( "Vd" ) & drug.Vd;
        ar.labelElement( "drug_c50" ) & drug.drug_c50;
        ar.labelElement( "fraction_defaulters" ) & drug.fraction_defaulters;
        ar.labelElement( "p_uniform_distribution" ) & drug.p_uniform_distribution;
    }
}
