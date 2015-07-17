/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include "IdmApi.h"
#include <string>
#include <list>
#include <map>
#include <functional>

#include "BoostLibWrapper.h"

#include "ISupports.h"
#include "Configuration.h"
#include "FactorySupport.h"
#include "Configure.h"

#include "SimulationEnums.h"
#include "VectorEnums.h"   // remove dependency when egg, larva, and vector enums are configured outside this class
#include "InterventionEnums.h"
#include "StrainIdentity.h"
#include "VectorSpeciesParameters.h"
#ifdef ENABLE_POLIO
#include "PolioDefs.h"
#endif

#ifdef ENABLE_TB
#include "TBDrugTypeParameters.h"
#endif

#ifndef DISABLE_HIV
#include "IRelationship.h"
#endif

// This macro, GET_CONFIGURABLE(Category), is the convenient and fast way to access the SimulationConfig parameters 
// within monolithic executable Eradication.exe.
// To access the same SimulationConfig instance from EModule (Dlls/so), use the IGlobalContext method
// GetSimulationConfigObj() via emodule's context QI's return for IGlobalContext interface
#define GET_CONFIGURABLE(Category)  ((Kernel::Category*)(Environment::get##Category()))

using namespace std;

namespace Kernel
{
    class MalariaDrugTypeParameters; 
    class SimulationConfig;

    class ISimulationConfigFactory
    {
    public:
        virtual void Register(string classname, instantiator_function_t _if) = 0;
    };            

    class IDMAPI SimulationConfigFactory : public ISimulationConfigFactory
    {
    public:
        static ISimulationConfigFactory * getInstance();

        static SimulationConfig* CreateInstance(const Configuration * config);
        void Register(string classname, instantiator_function_t _if);

    protected:
        static support_spec_map_t& getRegisteredClasses();

    private:
        static ISimulationConfigFactory * _instance;
    };


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SimulationConfig Class Layout Special Notes 
// Note 1: 
// All non-primitive objects on the stack (like STL or user-defined type) have to be at the very last 
// for the primitive objects in the SimulationConfig object crossing the DLL/EModule boundary
// without affecting the memory image and therefore its member values
// Note 2:
// All the non-primitive static objects will not be able to cross DLL/EModule boundary
// so their values will be different on two sides of memory space even for the same SimulationConfig object
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    class IDMAPI SimulationConfig : public JsonConfigurable
    {

        DECLARE_FACTORY_REGISTERED(SimulationConfigFactory, SimulationConfig, IConfigurable)

    public:
        DECLARE_CONFIGURED(SimulationConfig)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()  
        DECLARE_QUERY_INTERFACE()

#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        JsonConfigurable::tDynamicStringSet listed_events;
#pragma warning( pop )
        // Enum type name                                    Enum variable name                                name in config.json
        //
        DistributionType::Enum                               age_initialization_distribution_type;             // Age_Initialization_Distribution_Type
        EggHatchDelayDist::Enum                              egg_hatch_delay_dist;                             // eggHatchDelayDist
        EggSaturation::Enum                                  egg_saturation;                                   // eggSaturation
        EvolutionPolioClockType::Enum                        evolution_polio_clock_type;                       // evolution_polio_clock_type

        LarvalDensityDependence::Enum                        larval_density_dependence;                        // larvalDensityDependence
        LoadBalancingScheme::Enum                            load_balancing;                                   // LOAD_BALANCING_SCHEME

        MigrationStructure::Enum                             migration_structure;                              // MIGRATION_STRUCTURE
        PKPDModel::Enum                                      PKPD_model;                                       // PKPD_MODEL

        PopulationScaling::Enum                              population_scaling;                               // POPULATION_SCALING

        SusceptibilityScaling::Enum                          susceptibility_scaling;                           // Susceptibility_Scale_Type
        SimType::Enum                                        sim_type;                                         // Sim_Type
        VDPVVirulenceModelType::Enum                         VDPV_virulence_model_type;                        // VDPV_virulence_model_type

        VectorSamplingType::Enum                             vector_sampling_type;                             // Vector_Sampling_Type
        VectorSugarFeeding::Enum                             vector_sugar_feeding;                             // vectorSugarFeeding
        VectorRainfallMortality::Enum                        vector_larval_rainfall_mortality;                 // vectorLarvalRainfallMortality
        HEGModel::Enum                                       heg_model;                                        //HEGModel
        VitalBirthDependence::Enum                           vital_birth_dependence;                           // Vital_Birth_Dependence
        VitalDeathDependence::Enum                           vital_death_dependence;                           // Vital_Death_Dependence

        float susceptibility_scaling_rate;     // Susceptibility_Scaling_Rate, only for Susceptibility_Scale_Type = *_FUNCTION_OF_TIME

        bool demographic_tracking;              // from Simulation base class

        bool  vector_aging;                     // From SimulationVector

        bool  temperature_dependent_feeding_cycle;
        float meanEggHatchDelay;
        float WolbachiaMortalityModification;
        float WolbachiaInfectionModification;
        float HEGhomingRate;
        float HEGfecundityLimiting;
        float human_feeding_mortality;

        bool enable_immunity_initialization_distribution;   // toggles/flags

        // suscept malaria (TODO: these can probably move to SusceptibilityMalariaConfig as well, as long as the parasiteSmearSensitivity usage in IndividualHumanMalaria can be addressed)
        float parasiteSmearSensitivity;
        float newDiagnosticSensitivity;

        // antigen population
        int falciparumMSPVars;
        int falciparumNonSpecTypes;
        int falciparumPfEMP1Vars;

        // node vector
        float larvalconstant0;

        // allows configuration of larval habitat decay time constants
        float tempHabitatDecayScalar;
        float semipermanentHabitatDecayRate;
        float mmRainfallToFillSwamp;
        float larval_rainfall_mortality_threshold;

        // allows configuration of larval density dependence
        float larvalDensityMortalityScalar;
        float larvalDensityMortalityOffset;

        bool demographics_initial;

        float lloffset; // half the size of a grid edge in degrees, set by SetFlags()

        bool coinfection_incidence;
        bool enable_coinfection_mortality;

        // parameters for individual
        bool vital_dynamics;
        bool vital_disease_mortality;
        
        int infection_updates_per_tstep;
        bool interventions;

        float feverDetectionThreshold;

        // constant climate params
        float          airtemperature;
        float          landtemperature;
        float          rainfall;
        float          humidity;

        // scale params
        float          airtemperature_offset;
        float          landtemperature_offset;
        float          rainfall_scale_factor;
        float          humidity_scale_factor;

        // stochasticity params
        float          airtemperature_variance;
        float          landtemperature_variance;
        bool           rainfall_variance;
        float          humidity_variance;

#ifdef ENABLE_POLIO
        // SusceptibilityPolio
        // probability of no infection (1-p0)^events is approximated by Poisson, exp(-p0*events)
        float TauNAb; // (days) neutralization time constant
        float PVinf0[N_POLIO_VIRUS_TYPES]; // (dimensionless) probability of infection from single virion
        float viral_interference[N_POLIO_VIRUS_TYPES]; // (dimensionless) probability that infection will prevent infection of any heterologous serotype
        float vaccine_take_multiplier[N_POLIO_VIRUS_TYPES]; //counts for the host factors of vaccine take
        float mucosalImmIPV; // (dimensionless, OPV mucosal / IPV mucosal) relative mucosal immunogenicity
        float mucosalImmIPVOPVExposed; // (dimensionless, OPV mucosal / IPV mucosal) relative mucosal immunogenicity on OPV-exposed individuals
        float paralysis_base_rate[N_POLIO_VIRUS_TYPES]; // (dimensionless) probability of a fully susceptible individual to be paralyzed by infection, for WPV and fully reverted cVDPV, assuming age = ? years
        float boostLog2NAb_OPV[N_POLIO_SEROTYPES]; // (dimensionless) multiplication of antibody titer in response to challenge infection
        float boostLog2NAb_IPV[N_POLIO_SEROTYPES]; // (dimensionless) multiplication of antibody titer in response to challenge infection
        float maxLog2NAb_OPV[N_POLIO_SEROTYPES]; // (dimensionless) multiplication of antibody titer in response to challenge infection
        float maxLog2NAb_IPV[N_POLIO_SEROTYPES]; // (dimensionless) multiplication of antibody titer in response to challenge infection
        float boostLog2NAb_stddev_OPV[N_POLIO_SEROTYPES]; // (dimensionless) multiplication of antibody titer in response to challenge infection
        float boostLog2NAb_stddev_IPV[N_POLIO_SEROTYPES]; // (dimensionless) multiplication of antibody titer in response to challenge infection
        float primeLog2NAb_OPV[N_POLIO_SEROTYPES]; // (log2 reciprocal titer) antibody level at first infection
        float primeLog2NAb_IPV[N_POLIO_SEROTYPES]; // (log2 reciprocal titer) antibody level at first infection
        float primeLog2NAb_stddev_OPV[N_POLIO_SEROTYPES]; // (log2 reciprocal titer) standard deviation antibody level at first infection
        float primeLog2NAb_stddev_IPV[N_POLIO_SEROTYPES]; // (log2 reciprocal titer) standard deviation antibody level at first infection
        float decayRatePassiveImmunity; // (1/days) exponential decay rate
        float maternal_log2NAb_mean;
        float maternal_log2NAb_std;
        float vaccine_titer_tOPV[N_POLIO_SEROTYPES]; // (TCID50) viral dose of each serotype
        float vaccine_titer_bOPV[2]; // (TCID50) viral dose of each serotype
        float vaccine_titer_mOPV[N_POLIO_SEROTYPES]; // (TCID50) viral dose of each serotype
        float vaccine_Dantigen_IPV[N_POLIO_SEROTYPES]; // (D-antigen units) antigen content of each serotype
        float Incubation_Disease_Mu; // paralysis incubation period, lognormal parameter mu
        float Incubation_Disease_Sigma; // paralysis incubation period, lognormal parameter sigma
#endif

        int number_basestrains; // poliovirus types WPV1-3 VRPV1-3
        int number_substrains; // genetic variants
#ifdef ENABLE_POLIO
        int reversionSteps_cVDPV[N_POLIO_SEROTYPES]; // (bits) number of mutation steps to revert from Sabin to cVDPV, must be <= number_substrains
        float excrement_load; // (grams/day) feces
        float MaxRandStdDev; // (dimensionless) limit to effect of a random normal

        float shedFecalMaxLog10PeakTiter; // (log10 TCID50)
        float shedFecalMaxLog10PeakTiter_Stddev; // (log10 TCID50)
        float shedFecalTiterBlockLog2NAb; // (log2 NAb)
        float shedFecalTiterProfile_mu; // (Ln time)
        float shedFecalTiterProfile_sigma; // (Ln time)
        float shedFecalMaxLnDuration; // (Ln time)
        float shedFecalMaxLnDuration_Stddev; // (Ln time)
        float shedFecalDurationBlockLog2NAb; // (log2 Nab)
        float shedOralMaxLog10PeakTiter; // (log10 TCID50)
        float shedOralMaxLog10PeakTiter_Stddev; // (log10 TCID50)
        float shedOralTiterBlockLog2NAb; // (log2 NAb)
        float shedOralTiterProfile_mu; // (Ln time)
        float shedOralTiterProfile_sigma; // (Ln time)
        float shedOralMaxLnDuration; // (Ln time)
        float shedOralMaxLnDuration_Stddev; // (Ln time)
        float shedOralDurationBlockLog2NAb; // (log2 NAb)
#endif

        // flag for heterogeneity in mixing (true) or uniform mixing (false)
        bool heterogeneous_intranode_transmission_enabled;

        float Sim_Duration;
        float Sim_Tstep;
        float starttime;
        float node_grid_size;
        int Run_Number;

#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details

        int branch_duration;
        int branch_end_state;
        int branch_start_state;

        int burnin_period;
        int serialization_test_cycles;

        float maternalAbHalfLife;
        double x_templarvalhabitat;
#ifdef ENABLE_POLIO
        int vaccine_genome_OPV1;
        int vaccine_genome_OPV2;
        int vaccine_genome_OPV3;
#endif
#ifdef ENABLE_TB
        JsonConfigurable::tDynamicStringSet tb_drug_names_for_this_sim;
        std::map< std::string, TBDrugTypeParameters * > TBDrugMap;
#endif
        // STI: these will all move to IndividualHumanSTIConfig soon.
        float shortTermRelationshipLength;
        float concurrentRelationshipLength;
        float prob_super_spreader;
        bool  enable_coital_dilution;


        std::map< RelationshipType::Enum, float > coital_act_rate;

        float coital_dilution_2_partners;
        float coital_dilution_3_partners;
        float coital_dilution_4_plus_partners;

        float maritalRel_inv_kappa;
        float maritalRel_lambda;
        float informalRel_inv_kappa;
        float informalRel_lambda;
        float transitoryRel_inv_kappa;
        float transitoryRel_lambda;

        bool  Enable_cd4_dep_prog;
        int num_cd4_time_steps;
        float cd4_time_step;
        float cd4_count_at_beginning_of_hiv_infection;
        float cd4_count_at_end_of_hiv_infection;

        float days_between_symptomatic_and_death_lambda;
        float days_between_symptomatic_and_death_inv_kappa;

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Important Note: 
        // All static STL objects have to be at the very last 
        // for the SimulationConfig object crossing the DLL/EModule boundary
        // without affecting the memory layout and therefore its member values
        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#if !defined(_DLLS_)
        StrainIdentity vaccine_strains[3]; // (StrainIdentity) strainIDs for vaccine virus out of the vial, types 1-3 Sabin strains
#endif
        std::vector<std::vector<float> >   substrainRelativeInfectivity;

#ifdef ENABLE_POLIO
        std::vector<float> Sabin1_Site_Rates;
        std::vector<float> Sabin2_Site_Rates;
        std::vector<float> Sabin3_Site_Rates;

#endif

        JsonConfigurable::tDynamicStringSet vector_species_names;

        std::string ConfigName;
        std::string airmig_filename;
        std::string campaign_filename;
        std::string climate_airtemperature_filename;
        std::string climate_koppen_filename;
        std::string climate_landtemperature_filename;
        std::string climate_rainfall_filename;
        std::string climate_relativehumidity_filename;
        std::string loadbalance_filename;
        std::string localmig_filename;
        std::string regionmig_filename;
        std::string seamig_filename;
        std::string TerminationPredicate;
#ifdef ENABLE_PYTHON
        std::string python_script_path;
#endif
#pragma warning( pop )

        float prob_maternal_transmission;
        float maternal_transmission_ART_multiplier;

        ///////////////////////////////////////////
        SimulationConfig();
        virtual ~SimulationConfig();

        const Configuration* GetJsonConfigObj() const { return m_jsonConfig; }


#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
    protected:
        std::map< std::string, VectorSpeciesParameters * > vspMap;
        std::map< std::string, MalariaDrugTypeParameters * > MalariaDrugMap;
#pragma warning( pop )

    private: // for serialization to work

        const Configuration* m_jsonConfig;

#if USE_BOOST_SERIALIZATION
        friend class boost::serialization::access;    
        template<class Archive>
        friend void serialize(Archive & ar, SimulationConfig& configs, const unsigned int /* file_version */);
        FORCE_POLYMORPHIC()
#endif
    };
}
