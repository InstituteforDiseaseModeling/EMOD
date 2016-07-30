/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#ifdef WIN32
#include "windows.h"
#endif

#include "SimulationConfig.h"

// These headers are included here mostly for 
// many constants defines which eventually in SS4 will be
// moved back to where they belong so that we don't have to 
// include all these header files
#include "Common.h"
#include "Debug.h"
#include "Malaria.h"
#include "SusceptibilityMalaria.h"
#include "Vector.h"
#include "Configure.h"

#include "InterventionEnums.h" // for MalariaDrugTypes

#include "MalariaDrugTypeParameters.h"
#include "TBDrugTypeParameters.h" //for TBDrugTypes

#include "Log.h"
#include "Sugar.h"
#ifdef ENABLE_POLIO
#include "PolioDefs.h"
#endif
#include <set>

#include "MathFunctions.h"

static const char* _module = "SimulationConfig";

namespace Kernel
{

ISimulationConfigFactory * SimulationConfigFactory::_instance = nullptr;
static const char* tb_drug_placeholder_string = "<tb_drug_name_goes_here>";
static const char* malaria_drug_placeholder_string = "<malaria_drug_name_goes_here>";

ISimulationConfigFactory * SimulationConfigFactory::getInstance()
{
    if( _instance == nullptr )
    {
        _instance = new SimulationConfigFactory();
    }
    return _instance;
}

SimulationConfig* SimulationConfigFactory::CreateInstance(const Configuration * config)
{
    SimulationConfig *SimConfig =  _new_ SimulationConfig();
    release_assert(SimConfig);
    if (SimConfig)
    {           
        if (SimConfig->Configure(config))
        {
            SimConfig->AddRef();
        }
        else
        {
            SimConfig->Release();
            SimConfig = nullptr;
        }
    }
    return SimConfig;
}

void SimulationConfigFactory::Register(string classname, instantiator_function_t _if)
{  
    getRegisteredClasses()[classname] = _if;
}

support_spec_map_t& SimulationConfigFactory::getRegisteredClasses()
{ 
    static support_spec_map_t registered_classes; 
    return registered_classes; 
}

IMPLEMENT_FACTORY_REGISTERED(SimulationConfig)

BEGIN_QUERY_INTERFACE_BODY(SimulationConfig)
     HANDLE_INTERFACE(IConfigurable)
END_QUERY_INTERFACE_BODY(SimulationConfig)

SimulationConfig::~SimulationConfig()
{
}

bool SimulationConfig::Configure(const Configuration * inputJson)
{
    LOG_DEBUG( "Configure\n" );

    release_assert(inputJson);

    m_jsonConfig = inputJson;


    // Generic enums

    initConfig( "Age_Initialization_Distribution_Type", age_initialization_distribution_type, inputJson, MetadataDescriptor::Enum(Age_Initialization_Distribution_Type_DESC_TEXT, Age_Initialization_Distribution_Type_DESC_TEXT, MDD_ENUM_ARGS(DistributionType)) );     // 'global'

    initConfig( "Migration_Model", migration_structure, inputJson, MetadataDescriptor::Enum(Migration_Model_DESC_TEXT, Migration_Model_DESC_TEXT, MDD_ENUM_ARGS(MigrationStructure)) ); // 'global'
    initConfig( "Population_Scale_Type", population_scaling, inputJson, MetadataDescriptor::Enum(Population_Scale_Type_DESC_TEXT, Population_Scale_Type_DESC_TEXT, MDD_ENUM_ARGS(PopulationScaling)) ); // node only (move)
    initConfig( "Simulation_Type", sim_type, inputJson, MetadataDescriptor::Enum(Simulation_Type_DESC_TEXT, Simulation_Type_DESC_TEXT, MDD_ENUM_ARGS(SimType)) ); // simulation only (???move)
    initConfig( "Death_Rate_Dependence", vital_death_dependence, inputJson, MetadataDescriptor::Enum(Death_Rate_Dependence_DESC_TEXT, Death_Rate_Dependence_DESC_TEXT, MDD_ENUM_ARGS(VitalDeathDependence)), "Enable_Vital_Dynamics" ); // node only (move)
    LOG_DEBUG_F( "Death_Rate_Dependence configured as %s\n", VitalDeathDependence::pairs::lookup_key( vital_death_dependence ) );
    initConfig( "Susceptibility_Scale_Type", susceptibility_scaling, inputJson, MetadataDescriptor::Enum("susceptibility_scaling", Susceptibility_Scale_Type_DESC_TEXT, MDD_ENUM_ARGS(SusceptibilityScaling)) ); // Can be node-level or individual susceptibility-level

    //vector enums
    if (sim_type == SimType::VECTOR_SIM || sim_type == SimType::MALARIA_SIM)
    {
        initConfig( "Vector_Sampling_Type", vector_sampling_type, inputJson, MetadataDescriptor::Enum(Vector_Sampling_Type_DESC_TEXT, Vector_Sampling_Type_DESC_TEXT, MDD_ENUM_ARGS(VectorSamplingType)) ); // node (vector) only
        initConfig( "Egg_Hatch_Delay_Distribution", egg_hatch_delay_dist, inputJson, MetadataDescriptor::Enum(Egg_Hatch_Delay_Distribution_DESC_TEXT, Egg_Hatch_Delay_Distribution_DESC_TEXT, MDD_ENUM_ARGS(EggHatchDelayDist)) ); // vector pop only
        initConfig( "Egg_Saturation_At_Oviposition", egg_saturation, inputJson, MetadataDescriptor::Enum(Egg_Saturation_At_Oviposition_DESC_TEXT, Egg_Saturation_At_Oviposition_DESC_TEXT, MDD_ENUM_ARGS(EggSaturation)) ); // vector pop only
        initConfig( "Larval_Density_Dependence", larval_density_dependence, inputJson, MetadataDescriptor::Enum(Larval_Density_Dependence_DESC_TEXT, Larval_Density_Dependence_DESC_TEXT, MDD_ENUM_ARGS(LarvalDensityDependence)) ); // vector pop only
        initConfig( "Vector_Sugar_Feeding_Frequency", vector_sugar_feeding, inputJson, MetadataDescriptor::Enum(Vector_Sugar_Feeding_Frequency_DESC_TEXT, Vector_Sugar_Feeding_Frequency_DESC_TEXT, MDD_ENUM_ARGS(VectorSugarFeeding)) ); // vector pop individual only
        initConfig( "Vector_Larval_Rainfall_Mortality", vector_larval_rainfall_mortality, inputJson, MetadataDescriptor::Enum(Vector_Larval_Rainfall_Mortality_DESC_TEXT, Vector_Larval_Rainfall_Mortality_DESC_TEXT, MDD_ENUM_ARGS(VectorRainfallMortality)) );
        // get the larval density dependence parameters
        if( larval_density_dependence == LarvalDensityDependence::GRADUAL_INSTAR_SPECIFIC || 
            larval_density_dependence == LarvalDensityDependence::LARVAL_AGE_DENSITY_DEPENDENT_MORTALITY_ONLY  || 
            JsonConfigurable::_dryrun )
        {
            initConfigTypeMap( "Larval_Density_Mortality_Scalar", &larvalDensityMortalityScalar, Larval_Density_Mortality_Scalar_DESC_TEXT, 0.01f, 1000.0f, 10.0f);
            initConfigTypeMap( "Larval_Density_Mortality_Offset", &larvalDensityMortalityOffset, Larval_Density_Mortality_Offset_DESC_TEXT, 0.0001f, 1000.0f, 0.1f);
        }
        initConfig( "HEG_Model", heg_model, inputJson, MetadataDescriptor::Enum(HEG_Model_DESC_TEXT, HEG_Model_DESC_TEXT, MDD_ENUM_ARGS(HEGModel)) );
        // get the larval density dependence parameters
        if(heg_model != HEGModel::OFF  || JsonConfigurable::_dryrun )
        {
            initConfigTypeMap( "HEG_Homing_Rate", &HEGhomingRate, HEG_Homing_Rate_DESC_TEXT, 0, 1, 0 );
            initConfigTypeMap( "HEG_Fecundity_Limiting", &HEGfecundityLimiting, HEG_Fecundity_Limiting_DESC_TEXT, 0, 1, 0 );
        }
    }

    // malaria enums
    if (sim_type == SimType::MALARIA_SIM)
    {
        initConfig( "PKPD_Model", PKPD_model, inputJson, MetadataDescriptor::Enum(PKPD_Model_DESC_TEXT, PKPD_Model_DESC_TEXT, MDD_ENUM_ARGS(PKPDModel)) ); // special case: intervention (anti-malarial drug) only
    }

#ifdef ENABLE_POLIO
    // polio enums
    if (sim_type == SimType::POLIO_SIM)
    {
        initConfig( "Evolution_Polio_Clock_Type", evolution_polio_clock_type, inputJson, MetadataDescriptor::Enum(Evolution_Polio_Clock_Type_DESC_TEXT, Evolution_Polio_Clock_Type_DESC_TEXT, MDD_ENUM_ARGS(EvolutionPolioClockType)) ); // infection (polio) only
        initConfig( "VDPV_Virulence_Model_Type", VDPV_virulence_model_type, inputJson, MetadataDescriptor::Enum(VDPV_Virulence_Model_Type_DESC_TEXT, VDPV_Virulence_Model_Type_DESC_TEXT, MDD_ENUM_ARGS(VDPVVirulenceModelType)) ); // susceptibility polio only
        if ((susceptibility_scaling == SusceptibilityScaling::LOG_LINEAR_FUNCTION_OF_TIME) || JsonConfigurable::_dryrun)
        {
            initConfigTypeMap( "Susceptibility_Scaling_Rate", &susceptibility_scaling_rate, Susceptibility_Scaling_Rate_DESC_TEXT, 0.0f, FLT_MAX, 0.0f );
        }
    }
#endif
    // susceptibility scaling enum
    if( ( susceptibility_scaling == SusceptibilityScaling::LOG_LINEAR_FUNCTION_OF_TIME ) || ( susceptibility_scaling == SusceptibilityScaling::LINEAR_FUNCTION_OF_AGE) )
    {
        initConfigTypeMap( "Susceptibility_Scaling_Rate", &susceptibility_scaling_rate, Susceptibility_Scaling_Rate_DESC_TEXT, 0.0f, FLT_MAX, 0.0f );
    }
    if( susceptibility_scaling == SusceptibilityScaling::LINEAR_FUNCTION_OF_AGE )
    {
        initConfigTypeMap( "Susceptibility_Scaling_Age0_Intercept", &susceptibility_scaling_intercept, Susceptibility_Scaling_Intercept_DESC_TEXT, 0.0f, 1.0f, 0.0f ); 
    }

    // Generic parameters
    // Controller/high level stuff
    //initConfigTypeMap( "Serialization_Test_Cycles", &serialization_test_cycles, Serialization_Test_Cycles_DESC_TEXT, 0, 100, 1 ); // 'global' (sorta)

#ifdef ENABLE_PYTHON
    //initConfigTypeMap( "Python_Script_Path", &python_script_path, Path_to_Python_Scripts_DESC_TEXT ); // nodedemographics only
#endif
    initConfigTypeMap( "Simulation_Duration", &Sim_Duration, Simulation_Duration_DESC_TEXT, 0.0f, 1000000.0f, 1.0f ); // 'global'? (controller only)
    initConfigTypeMap( "Simulation_Timestep", &Sim_Tstep, Simulation_Timestep_DESC_TEXT, 0.0f, 1000000.0f, 1.0f ); // 'global'? (controller only)
    initConfigTypeMap( "Start_Time", &starttime, Start_Time_DESC_TEXT, 0.0f, 1000000.0f, 1.0f ); // simulation only (actually Climate also!)
    initConfigTypeMap( "Config_Name", &ConfigName, Config_Name_DESC_TEXT );

    bool demographics_builtin = false;
    initConfigTypeMap( "Enable_Demographics_Builtin", &demographics_builtin, Enable_Demographics_Initial_DESC_TEXT, false ); // 'global' (3 files)
    initConfigTypeMap( "Default_Geography_Initial_Node_Population", &default_node_population, Default_Geography_Initial_Node_Population_DESC_TEXT, 0, 1e6, 1000, "Enable_Demographics_Builtin");
    initConfigTypeMap( "Default_Geography_Torus_Size", &default_torus_size, Default_Geography_Torus_Size_DESC_TEXT, 3, 100, 10, "Enable_Demographics_Builtin");

    initConfigTypeMap( "Enable_Vital_Dynamics", &vital_dynamics, Enable_Vital_Dynamics_DESC_TEXT, true );
    initConfigTypeMap( "Enable_Disease_Mortality", &vital_disease_mortality, Enable_Disease_Mortality_DESC_TEXT, true );

    initConfigTypeMap( "Enable_Heterogeneous_Intranode_Transmission", &heterogeneous_intranode_transmission_enabled, Enable_Heterogeneous_Intranode_Transmission_DESC_TEXT, false ); // generic

    initConfigTypeMap( "Enable_Interventions", &interventions, Enable_Interventions_DESC_TEXT, false );

    initConfigTypeMap( "Number_Basestrains", &number_basestrains, Number_Basestrains_DESC_TEXT, 1, 10, 1);
    initConfigTypeMap( "Number_Substrains", &number_substrains, Number_Substrains_DESC_TEXT, 1, 16777216, 256 );

    initConfigTypeMap( "Maternal_Transmission_Probability", &prob_maternal_transmission, Maternal_Transmission_Probability_DESC_TEXT, 0.0f, 1.0f,    0.0f, "Enable_Maternal_Transmission"  );

    initConfig( "Immunity_Initialization_Distribution_Type", immunity_initialization_distribution_type, inputJson, MetadataDescriptor::Enum("immunity_initialization_distribution_type", Immunity_Initialization_Distribution_Type_DESC_TEXT, MDD_ENUM_ARGS(DistributionType)), "Enable_Immunity" ); // polio and malaria

    initConfigTypeMap( "Node_Grid_Size", &node_grid_size, Node_Grid_Size_DESC_TEXT, 0.004167f, 90.0f, 0.004167f );

    // Vector parameters
    if (sim_type == SimType::VECTOR_SIM || sim_type == SimType::MALARIA_SIM)
    {

        initConfigTypeMap( "Enable_Vector_Aging", &vector_aging, VECTOR_Enable_Aging_DESC_TEXT, false );
        //initConfigTypeMap( "Enable_Vector_Species_Habitat_Competition", &enable_vector_species_habitat_competition, VECTOR_Enable_Vector_Species_Habitat_Competition, false );
        initConfigTypeMap( "Enable_Temperature_Dependent_Feeding_Cycle", &temperature_dependent_feeding_cycle, VECTOR_Enable_Temperature_Dependent_Feeding_Cycle_DESC_TEXT, false );
        initConfigTypeMap( "Mean_Egg_Hatch_Delay", &meanEggHatchDelay, Mean_Egg_Hatch_Delay_DESC_TEXT, 0, 120, 0 );
        initConfigTypeMap( "Wolbachia_Mortality_Modification", &WolbachiaMortalityModification, Wolbachia_Mortality_Modification_DESC_TEXT, 0, 100, 1 );
        initConfigTypeMap( "Wolbachia_Infection_Modification", &WolbachiaInfectionModification, Wolbachia_Infection_Modification_DESC_TEXT, 0, 100, 1 );

        // get the c50 value if there is rainfall mortality
        if((vector_larval_rainfall_mortality != VectorRainfallMortality::NONE) || JsonConfigurable::_dryrun )
        {
            initConfigTypeMap( "Larval_Rainfall_Mortality_Threshold", &larval_rainfall_mortality_threshold, Larval_Rainfall_Mortality_Threshold_DESC_TEXT, 0.01f, 1000.0f, 100.0f);
        }

        initConfigTypeMap( "Human_Feeding_Mortality", &human_feeding_mortality, Human_Feeding_Mortality_DESC_TEXT, 0.0f, 1.0f, 0.1f );
    }

    // Malaria parameters
    if (sim_type == SimType::MALARIA_SIM)
    {
        initConfigTypeMap( "Falciparum_MSP_Variants", &falciparumMSPVars, Falciparum_MSP_Variants_DESC_TEXT, 0, 1e3, DEFAULT_MSP_VARIANTS ); // malaria
        initConfigTypeMap( "Falciparum_Nonspecific_Types", &falciparumNonSpecTypes, Falciparum_Nonspecific_Types_DESC_TEXT, 0, 1e3, DEFAULT_NONSPECIFIC_TYPES ); // malaria
        initConfigTypeMap( "Falciparum_PfEMP1_Variants", &falciparumPfEMP1Vars, Falciparum_PfEMP1_Variants_DESC_TEXT, 0, 1e5, DEFAULT_PFEMP1_VARIANTS ); // malaria

        initConfigTypeMap( "Fever_Detection_Threshold", &feverDetectionThreshold, Fever_Detection_Threshold_DESC_TEXT, 0.5f, 5.0f, 1.0f );

        initConfigTypeMap( "Parasite_Smear_Sensitivity", &parasiteSmearSensitivity, Parasite_Smear_Sensitivity_DESC_TEXT, 0.0001f, 100.0f, 0.1f ); // malaria
        initConfigTypeMap( "New_Diagnostic_Sensitivity", &newDiagnosticSensitivity, New_Diagnostic_Sensitivity_DESC_TEXT, 0.0001f, 100000.0f, 0.01f ); // malaria
    }

#ifdef ENABLE_POLIO
    // Polio parameters
    if (sim_type == SimType::POLIO_SIM)
    {
        initConfigTypeMap( "Max_Rand_Standard_Deviations", &MaxRandStdDev, Max_Rand_Standard_Deviations_DESC_TEXT, 0.0f, 10.0f, 2.0f);

        initConfigTypeMap( "Specific_Infectivity_WPV1",   &PVinf0[0], Specific_Infectivity_WPV1_DESC_TEXT, 0.0f, 1.0e+3f, 0.004f );
        initConfigTypeMap( "Specific_Infectivity_WPV2",   &PVinf0[1], Specific_Infectivity_WPV2_DESC_TEXT, 0.0f, 1.0e+3f, 0.004f );
        initConfigTypeMap( "Specific_Infectivity_WPV3",   &PVinf0[2], Specific_Infectivity_WPV3_DESC_TEXT, 0.0f, 1.0e+3f, 0.004f );
        initConfigTypeMap( "Specific_Infectivity_Sabin1", &PVinf0[3], Specific_Infectivity_Sabin1_DESC_TEXT, 0.0f, 1.0e+3f, 3.02e-005f );
        initConfigTypeMap( "Specific_Infectivity_Sabin2", &PVinf0[4], Specific_Infectivity_Sabin2_DESC_TEXT, 0.0f, 1.0e+3f, 0.00135f );
        initConfigTypeMap( "Specific_Infectivity_Sabin3", &PVinf0[5], Specific_Infectivity_Sabin3_DESC_TEXT, 0.0f, 1.0e+3f, 0.000115f );
        initConfigTypeMap( "Viral_Interference_WPV1",   &viral_interference[0], Viral_Interference_WPV1_DESC_TEXT, 0.0f, 1.0f, 0.0653f );
        initConfigTypeMap( "Viral_Interference_WPV2",   &viral_interference[1], Viral_Interference_WPV2_DESC_TEXT, 0.0f, 1.0f, 0.278f );
        initConfigTypeMap( "Viral_Interference_WPV3",   &viral_interference[2], Viral_Interference_WPV3_DESC_TEXT, 0.0f, 1.0f, 0.263f );
        initConfigTypeMap( "Viral_Interference_Sabin1", &viral_interference[3], Viral_Interference_Sabin1_DESC_TEXT, 0.0f, 1.0f, 0.0653f );
        initConfigTypeMap( "Viral_Interference_Sabin2", &viral_interference[4], Viral_Interference_Sabin2_DESC_TEXT, 0.0f, 1.0f, 0.278f );
        initConfigTypeMap( "Viral_Interference_Sabin3", &viral_interference[5], Viral_Interference_Sabin3_DESC_TEXT, 0.0f, 1.0f, 0.263f );


        //this scaling factor is for vaccines, so set the scaling of wild virus to 1.
        vaccine_take_multiplier[0] = 1.0f;
        vaccine_take_multiplier[1] = 1.0f;
        vaccine_take_multiplier[2] = 1.0f;
        initConfigTypeMap("Vaccine_Take_Scaling_Sabin1", &vaccine_take_multiplier[3], Vaccine_Take_Multiplier_Sabin1_DESC_TEXT, 0.0f, 1.0f, 1.0f);
        initConfigTypeMap("Vaccine_Take_Scaling_Sabin2", &vaccine_take_multiplier[4], Vaccine_Take_Multiplier_Sabin2_DESC_TEXT, 0.0f, 1.0f, 0.278f);
        initConfigTypeMap("Vaccine_Take_Scaling_Sabin3", &vaccine_take_multiplier[5], Vaccine_Take_Multiplier_Sabin3_DESC_TEXT, 0.0f, 1.0f, 0.263f);

        initConfigTypeMap( "Mucosal_Immunogenicity_IPV", &mucosalImmIPV, Mucosal_Immunogenicity_IPV_DESC_TEXT, 0.0f, 100.0f, 3.0f );
        initConfigTypeMap("Mucosal_Immunogenicity_IPV_OPVExposed", &mucosalImmIPVOPVExposed, Mucosal_Immunogenicity_IPV_OPVEXPOSED_DESC_TEXT, 0.0f, 100.0f, 3.0f);
        initConfigTypeMap( "Neutralization_Time_Tau",    &TauNAb, Neutralization_Time_Tau_DESC_TEXT, 0.0f, 1.0f, 0.04f );

        initConfigTypeMap( "Paralysis_Base_Rate_WPV1",   &paralysis_base_rate[0], Paralysis_Base_Rate_WPV1_DESC_TEXT, 0.0f, 1.0f, 0.05f );
        initConfigTypeMap( "Paralysis_Base_Rate_WPV2",   &paralysis_base_rate[1], Paralysis_Base_Rate_WPV2_DESC_TEXT, 0.0f, 1.0f, 0.00033f );
        initConfigTypeMap( "Paralysis_Base_Rate_WPV3",   &paralysis_base_rate[2], Paralysis_Base_Rate_WPV3_DESC_TEXT, 0.0f, 1.0f, 0.001f );
        initConfigTypeMap( "Paralysis_Base_Rate_Sabin1", &paralysis_base_rate[3], Paralysis_Base_Rate_Sabin1_DESC_TEXT, 0.0f, 1.0f, 0.005f );
        initConfigTypeMap( "Paralysis_Base_Rate_Sabin2", &paralysis_base_rate[4], Paralysis_Base_Rate_Sabin2_DESC_TEXT, 0.0f, 1.0f, 0.00033f );
        initConfigTypeMap( "Paralysis_Base_Rate_Sabin3", &paralysis_base_rate[5], Paralysis_Base_Rate_Sabin3_DESC_TEXT, 0.0f, 1.0f, 0.001f );

        initConfigTypeMap( "Incubation_Disease_Mu",      &Incubation_Disease_Mu, Incubation_Disease_Mu_DESC_TEXT, 0.0f, 100.0f, 2.3893f );
        initConfigTypeMap( "Incubation_Disease_Sigma",   &Incubation_Disease_Sigma, Incubation_Disease_Sigma_DESC_TEXT, 0.0f, 100.0f, 0.4558f );
        
        initConfigTypeMap( "Boost_Log2_NAb_OPV1", &boostLog2NAb_OPV[0], Boost_Log2_NAb_OPV1_DESC_TEXT, 0.0f, 10.0f, 1.0f );
        initConfigTypeMap( "Boost_Log2_NAb_OPV2", &boostLog2NAb_OPV[1], Boost_Log2_NAb_OPV2_DESC_TEXT, 0.0f, 10.0f, 1.0f );
        initConfigTypeMap( "Boost_Log2_NAb_OPV3", &boostLog2NAb_OPV[2], Boost_Log2_NAb_OPV3_DESC_TEXT, 0.0f, 10.0f, 1.0f );
        initConfigTypeMap( "Boost_Log2_NAb_IPV1", &boostLog2NAb_IPV[0], Boost_Log2_NAb_IPV1_DESC_TEXT, 0.0f, 10.0f, 1.0f );
        initConfigTypeMap( "Boost_Log2_NAb_IPV2", &boostLog2NAb_IPV[1], Boost_Log2_NAb_IPV2_DESC_TEXT, 0.0f, 10.0f, 1.0f );
        initConfigTypeMap( "Boost_Log2_NAb_IPV3", &boostLog2NAb_IPV[2], Boost_Log2_NAb_IPV3_DESC_TEXT, 0.0f, 10.0f, 1.0f );

        initConfigTypeMap( "Max_Log2_NAb_OPV1", &maxLog2NAb_OPV[0], Max_Log2_NAb_OPV1_DESC_TEXT, 0.0f, FLT_MAX, 2.0f );
        initConfigTypeMap( "Max_Log2_NAb_OPV2", &maxLog2NAb_OPV[1], Max_Log2_NAb_OPV2_DESC_TEXT, 0.0f, FLT_MAX, 2.0f );
        initConfigTypeMap( "Max_Log2_NAb_OPV3", &maxLog2NAb_OPV[2], Max_Log2_NAb_OPV3_DESC_TEXT, 0.0f, FLT_MAX, 2.0f );
        initConfigTypeMap( "Max_Log2_NAb_IPV1", &maxLog2NAb_IPV[0], Max_Log2_NAb_IPV1_DESC_TEXT, 0.0f, FLT_MAX, 4.0f );
        initConfigTypeMap( "Max_Log2_NAb_IPV2", &maxLog2NAb_IPV[1], Max_Log2_NAb_IPV2_DESC_TEXT, 0.0f, FLT_MAX, 4.0f );
        initConfigTypeMap( "Max_Log2_NAb_IPV3", &maxLog2NAb_IPV[2], Max_Log2_NAb_IPV3_DESC_TEXT, 0.0f, FLT_MAX, 4.0f );

        initConfigTypeMap( "Boost_Log2_NAb_Stddev_OPV1", &boostLog2NAb_stddev_OPV[0], Boost_Log2_NAb_Stddev_OPV1_DESC_TEXT, 0.0f, 10.0f, 1.5f );
        initConfigTypeMap( "Boost_Log2_NAb_Stddev_OPV2", &boostLog2NAb_stddev_OPV[1], Boost_Log2_NAb_Stddev_OPV2_DESC_TEXT, 0.0f, 10.0f, 1.5f );
        initConfigTypeMap( "Boost_Log2_NAb_Stddev_OPV3", &boostLog2NAb_stddev_OPV[2], Boost_Log2_NAb_Stddev_OPV3_DESC_TEXT, 0.0f, 10.0f, 1.5f );
        initConfigTypeMap( "Boost_Log2_NAb_Stddev_IPV1", &boostLog2NAb_stddev_IPV[0], Boost_Log2_NAb_Stddev_IPV1_DESC_TEXT, 0.0f, 1.0f, 1.0f );
        initConfigTypeMap( "Boost_Log2_NAb_Stddev_IPV2", &boostLog2NAb_stddev_IPV[1], Boost_Log2_NAb_Stddev_IPV2_DESC_TEXT, 0.0f, 1.0f, 1.0f );
        initConfigTypeMap( "Boost_Log2_NAb_Stddev_IPV3", &boostLog2NAb_stddev_IPV[2], Boost_Log2_NAb_Stddev_IPV3_DESC_TEXT, 0.0f, 1.0f, 1.0f );

        initConfigTypeMap( "Prime_Log2_NAb_OPV1", &primeLog2NAb_OPV[0], Prime_Log2_NAb_OPV1_DESC_TEXT, 0.0f, 10.0f, 2.0f );
        initConfigTypeMap( "Prime_Log2_NAb_OPV2", &primeLog2NAb_OPV[1], Prime_Log2_NAb_OPV2_DESC_TEXT, 0.0f, 10.0f, 2.0f );
        initConfigTypeMap( "Prime_Log2_NAb_OPV3", &primeLog2NAb_OPV[2], Prime_Log2_NAb_OPV3_DESC_TEXT, 0.0f, 10.0f, 2.0f );
        initConfigTypeMap( "Prime_Log2_NAb_IPV1", &primeLog2NAb_IPV[0], Prime_Log2_NAb_IPV1_DESC_TEXT, 0.0f, 10.0f, 4.0f );
        initConfigTypeMap( "Prime_Log2_NAb_IPV2", &primeLog2NAb_IPV[1], Prime_Log2_NAb_IPV2_DESC_TEXT, 0.0f, 10.0f, 4.0f );
        initConfigTypeMap( "Prime_Log2_NAb_IPV3", &primeLog2NAb_IPV[2], Prime_Log2_NAb_IPV3_DESC_TEXT, 0.0f, 10.0f, 4.0f );

        initConfigTypeMap( "Prime_Log2_NAb_Stddev_OPV1", &primeLog2NAb_stddev_OPV[0], Prime_Log2_NAb_Stddev_OPV1_DESC_TEXT, 0.0f, 10.0f, 1.0f );
        initConfigTypeMap( "Prime_Log2_NAb_Stddev_OPV2", &primeLog2NAb_stddev_OPV[1], Prime_Log2_NAb_Stddev_OPV2_DESC_TEXT, 0.0f, 10.0f, 1.0f );
        initConfigTypeMap( "Prime_Log2_NAb_Stddev_OPV3", &primeLog2NAb_stddev_OPV[2], Prime_Log2_NAb_Stddev_OPV3_DESC_TEXT, 0.0f, 10.0f, 1.0f );
        initConfigTypeMap( "Prime_Log2_NAb_Stddev_IPV1", &primeLog2NAb_stddev_IPV[0], Prime_Log2_NAb_Stddev_IPV1_DESC_TEXT, 0.0f, 10.0f, 1.0f );
        initConfigTypeMap( "Prime_Log2_NAb_Stddev_IPV2", &primeLog2NAb_stddev_IPV[1], Prime_Log2_NAb_Stddev_IPV2_DESC_TEXT, 0.0f, 10.0f, 1.0f );
        initConfigTypeMap( "Prime_Log2_NAb_Stddev_IPV3", &primeLog2NAb_stddev_IPV[2], Prime_Log2_NAb_Stddev_IPV3_DESC_TEXT, 0.0f, 10.0f, 1.0f );

        initConfigTypeMap( "Shed_Fecal_MaxLog10_Peak_Titer",         &shedFecalMaxLog10PeakTiter, Shed_Fecal_MaxLog10_Peak_Titer_DESC_TEXT, 0.0f, 10.0f, 5.0f );
        initConfigTypeMap( "Shed_Fecal_MaxLog10_Peak_Titer_Stddev",  &shedFecalMaxLog10PeakTiter_Stddev, Shed_Fecal_MaxLog10_Peak_Titer_Stddev_DESC_TEXT, 0.0f, 10.0f, 1.0f );
        initConfigTypeMap( "Shed_Fecal_Titer_Block_Log2NAb",         &shedFecalTiterBlockLog2NAb, Shed_Fecal_Titer_Block_Log2NAb_DESC_TEXT, 1.0f, 100.0f, 15.0f );
        initConfigTypeMap( "Shed_Fecal_Titer_Profile_Mu",            &shedFecalTiterProfile_mu, Shed_Fecal_Titer_Profile_Mu_DESC_TEXT, 0.0f, 10.0f, 3.0f );
        initConfigTypeMap( "Shed_Fecal_Titer_Profile_Sigma",         &shedFecalTiterProfile_sigma, Shed_Fecal_Titer_Profile_Sigma_DESC_TEXT, 0.0f, 10.0f, 1.0f );
        initConfigTypeMap( "Shed_Fecal_MaxLn_Duration",              &shedFecalMaxLnDuration, Shed_Fecal_MaxLn_Duration_DESC_TEXT, 0.0f, 100.0f, 4.0f );
        initConfigTypeMap( "Shed_Fecal_MaxLn_Duration_Stddev",       &shedFecalMaxLnDuration_Stddev, Shed_Fecal_MaxLn_Duration_Stddev_DESC_TEXT, 0.0f, 10.0f, 1.0f );
        initConfigTypeMap( "Shed_Fecal_Duration_Block_Log2NAb",      &shedFecalDurationBlockLog2NAb, Shed_Fecal_Duration_Block_Log2NAb_DESC_TEXT, 1.0f, 100.0f, 15.0f );
        initConfigTypeMap( "Shed_Oral_MaxLog10_Peak_Titer",         &shedOralMaxLog10PeakTiter, Shed_Oral_MaxLog10_Peak_Titer_DESC_TEXT, 0.0f, 10.0f, 5.0f );
        initConfigTypeMap( "Shed_Oral_MaxLog10_Peak_Titer_Stddev",  &shedOralMaxLog10PeakTiter_Stddev, Shed_Oral_MaxLog10_Peak_Titer_Stddev_DESC_TEXT, 0.0f, 10.0f, 1.0f );
        initConfigTypeMap( "Shed_Oral_Titer_Block_Log2NAb",         &shedOralTiterBlockLog2NAb, Shed_Oral_Titer_Block_Log2NAb_DESC_TEXT, 1.0f, 100.0f, 15.0f );
        initConfigTypeMap( "Shed_Oral_Titer_Profile_Mu",            &shedOralTiterProfile_mu, Shed_Oral_Titer_Profile_Mu_DESC_TEXT, 0.0f, 10.0f, 3.0f );
        initConfigTypeMap( "Shed_Oral_Titer_Profile_Sigma",         &shedOralTiterProfile_sigma, Shed_Oral_Titer_Profile_Sigma_DESC_TEXT, 0.0f, 10.0f, 1.0f );
        initConfigTypeMap( "Shed_Oral_MaxLn_Duration",              &shedOralMaxLnDuration, Shed_Oral_MaxLn_Duration_DESC_TEXT, 0.0f, 10.0f, 4.0f );
        initConfigTypeMap( "Shed_Oral_MaxLn_Duration_Stddev",       &shedOralMaxLnDuration_Stddev, Shed_Oral_MaxLn_Duration_Stddev_DESC_TEXT, 0.0f, 10.0f, 1.0f );
        initConfigTypeMap( "Shed_Oral_Duration_Block_Log2NAb",      &shedOralDurationBlockLog2NAb, Shed_Oral_Duration_Block_Log2NAb_DESC_TEXT, 1.0f, 100.0f, 15.0f );

        initConfigTypeMap( "Maternal_log2NAb_mean", &maternal_log2NAb_mean, Maternal_log2NAb_mean_DESC_TEXT,  0.0f, 18.0f, 6.0f );
        initConfigTypeMap( "Maternal_log2NAb_std",  &maternal_log2NAb_std,  Maternal_log2NAb_std_DESC_TEXT,   0.0f, 18.0f, 3.0f );
        initConfigTypeMap( "Maternal_Ab_Halflife",  &maternalAbHalfLife,    Maternal_Ab_Halflife_DESC_TEXT,   0.0f, 1000.0f, 22.0f );

        initConfigTypeMap( "Vaccine_Titer_tOPV1", &vaccine_titer_tOPV[0], Vaccine_Titer_tOPV1_DESC_TEXT, 0.0f, 100000000.0f, 1000000.0f );
        initConfigTypeMap( "Vaccine_Titer_tOPV2", &vaccine_titer_tOPV[1], Vaccine_Titer_tOPV2_DESC_TEXT, 0.0f, 100000000.0f, 100000.0f );
        initConfigTypeMap( "Vaccine_Titer_tOPV3", &vaccine_titer_tOPV[2], Vaccine_Titer_tOPV3_DESC_TEXT, 0.0f, 100000000.0f, 630000.0f );
        initConfigTypeMap( "Vaccine_Titer_bOPV1", &vaccine_titer_bOPV[0], Vaccine_Titer_bOPV1_DESC_TEXT, 0.0f, 100000000.0f, 1000000.0f );
        initConfigTypeMap( "Vaccine_Titer_bOPV3", &vaccine_titer_bOPV[1], Vaccine_Titer_bOPV3_DESC_TEXT, 0.0f, 100000000.0f, 630000.0f );
        initConfigTypeMap( "Vaccine_Titer_mOPV1", &vaccine_titer_mOPV[0], Vaccine_Titer_mOPV1_DESC_TEXT, 0.0f, 100000000.0f, 1000000.0f );
        initConfigTypeMap( "Vaccine_Titer_mOPV2", &vaccine_titer_mOPV[1], Vaccine_Titer_mOPV2_DESC_TEXT, 0.0f, 100000000.0f, 100000.0f );
        initConfigTypeMap( "Vaccine_Titer_mOPV3", &vaccine_titer_mOPV[2], Vaccine_Titer_mOPV3_DESC_TEXT, 0.0f, 100000000.0f, 1000000.0f );

        initConfigTypeMap( "Vaccine_Dantigen_IPV1", &vaccine_Dantigen_IPV[0], Vaccine_Dantigen_IPV1_DESC_TEXT, 0.0f, 1000.0f, 40.0f );
        initConfigTypeMap( "Vaccine_Dantigen_IPV2", &vaccine_Dantigen_IPV[1], Vaccine_Dantigen_IPV2_DESC_TEXT, 0.0f, 1000.0f, 8.0f );
        initConfigTypeMap( "Vaccine_Dantigen_IPV3", &vaccine_Dantigen_IPV[2], Vaccine_Dantigen_IPV3_DESC_TEXT, 0.0f, 1000.0f, 32.0f );
         
        initConfigTypeMap( "Reversion_Steps_Sabin1", &reversionSteps_cVDPV[0], Reversion_Steps_Sabin1_DESC_TEXT, 0, 8, 6 );
        initConfigTypeMap( "Reversion_Steps_Sabin2", &reversionSteps_cVDPV[1], Reversion_Steps_Sabin2_DESC_TEXT, 0, 8, 2 );
        initConfigTypeMap( "Reversion_Steps_Sabin3", &reversionSteps_cVDPV[2], Reversion_Steps_Sabin3_DESC_TEXT, 0, 8, 3 );

        initConfigTypeMap( "Excrement_Load", &excrement_load, Excrement_Load_DESC_TEXT, 0.0f, 1000.0f, 300.0f );
    }
#endif

    if( sim_type == SimType::VECTOR_SIM || sim_type == SimType::MALARIA_SIM )
    {
        initConfigTypeMap( "Temporary_Habitat_Decay_Factor", &tempHabitatDecayScalar, Temporary_Habitat_Decay_Factor_DESC_TEXT, 0.001f, 100.0f, 0.05f );
        initConfigTypeMap( "Semipermanent_Habitat_Decay_Rate", &semipermanentHabitatDecayRate, Semipermanent_Habitat_Decay_Rate_DESC_TEXT, 0.0001f, 100.0f, 0.01f );
        initConfigTypeMap( "Rainfall_In_mm_To_Fill_Swamp", &mmRainfallToFillSwamp, Rainfall_In_mm_To_Fill_Swamp_DESC_TEXT, 1.0f, 10000.0f, 1000.0f );

        // This is a key parameter for the mosquito ecology and can vary quite a lot
        initConfigTypeMap( "x_Temporary_Larval_Habitat", &x_templarvalhabitat, x_Temporary_Larval_Habitat_DESC_TEXT, 0.0f, 10000.0f, 1.0f ); // should this be renamed vector_weight?
        vector_species_names.value_source = "Vector_Species_Params.*";
        initConfigTypeMap( "Vector_Species_Names", &vector_species_names, Vector_Species_Names_DESC_TEXT );
        if( JsonConfigurable::_dryrun )
        {
#ifndef DISABLE_VECTOR
#if !defined(_DLLS_)
            // for the schema
            std::string arab( "vector_species_name_goes_here" );
            VectorSpeciesParameters * vsp = VectorSpeciesParameters::CreateVectorSpeciesParameters( arab );
            vsp->Configure( inputJson );
            vspMap[ arab ] = vsp;
#endif
#endif
        }
    }

#ifdef ENABLE_TB
    if( sim_type == SimType::TB_SIM )
    {
        tb_drug_names_for_this_sim.value_source = "TB_Drug_Params.*";
        initConfigTypeMap( "TB_Drug_Types_For_This_Sim", &tb_drug_names_for_this_sim, TB_Drug_Types_For_This_Sim_DESC_TEXT );
        if( JsonConfigurable::_dryrun )
        {
#if !defined(_DLLS_)
            // for the schema
            std::string tb_drug_names_array_str( tb_drug_placeholder_string );
            TBDrugTypeParameters * tbdtp = TBDrugTypeParameters::CreateTBDrugTypeParameters( tb_drug_names_array_str );
            tbdtp->Configure( inputJson );
            TBDrugMap[ tb_drug_names_array_str ] = tbdtp;
#endif
        }
    }
#endif

#ifdef ENABLE_TB
#ifdef ENABLE_TBHIV
    if( sim_type == SimType::TBHIV_SIM)
    {
        initConfigTypeMap("Enable_Coinfection_Incidence", &coinfection_incidence,Enable_Coinfection_Incidence_DESC_TEXT, false);
        initConfigTypeMap("Enable_Coinfection_Mortality", &enable_coinfection_mortality, Enable_Coinfection_Mortality_DESC_TEXT, false);
    }
#endif // TBHIV
#endif // TB

#ifndef DISABLE_MALARIA
    if( sim_type == SimType::MALARIA_SIM )
    {
        // for schema?
        if( JsonConfigurable::_dryrun )
        {
            std::string drug_name( malaria_drug_placeholder_string );
            MalariaDrugTypeParameters * mdtp = MalariaDrugTypeParameters::CreateMalariaDrugTypeParameters( drug_name );
            mdtp->Configure( inputJson );
            MalariaDrugMap[ drug_name ] = mdtp;
        }
    }
#endif

    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // !!! See below after JsonConfigure::Configure() is called.  !!!
    // !!! Below this value is updated with the built-in events   !!!
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    initConfigTypeMap( "Listed_Events", &listed_events, Listed_Events_DESC_TEXT);

#ifdef ENABLE_POLIO
#if !defined(_DLLS_)

    string tmpSubstrainInfectivityString[3];
    string tmpSiteRatesStrings[3];

    if( sim_type == SimType::POLIO_SIM )
    {
        initConfigTypeMap( "Substrain_Relative_Infectivity_String_VDPV1", &tmpSubstrainInfectivityString[0], Substrain_Relative_Infectivity_String_VDPV1_DESC_TEXT );
        initConfigTypeMap( "Substrain_Relative_Infectivity_String_VDPV2", &tmpSubstrainInfectivityString[1], Substrain_Relative_Infectivity_String_VDPV2_DESC_TEXT );
        initConfigTypeMap( "Substrain_Relative_Infectivity_String_VDPV3", &tmpSubstrainInfectivityString[2], Substrain_Relative_Infectivity_String_VDPV3_DESC_TEXT );

        initConfigTypeMap( "Sabin1_Site_Reversion_Rates", &tmpSiteRatesStrings[0], Sabin1_Site_Reversion_Rates_DESC_TEXT );
        initConfigTypeMap( "Sabin2_Site_Reversion_Rates", &tmpSiteRatesStrings[1], Sabin2_Site_Reversion_Rates_DESC_TEXT );
        initConfigTypeMap( "Sabin3_Site_Reversion_Rates", &tmpSiteRatesStrings[2], Sabin3_Site_Reversion_Rates_DESC_TEXT );

        vaccine_strains[0] = StrainIdentity(PolioVirusTypes::VRPV1,0); // sets antigenID for VRPV strains and genome to zero (fully Sabin), note: literal definition of "vaccine-related poliovirus"
        vaccine_strains[1] = StrainIdentity(PolioVirusTypes::VRPV2,0); // sets antigenID for VRPV strains and genome to zero (fully Sabin), note: literal definition of "vaccine-related poliovirus"
        vaccine_strains[2] = StrainIdentity(PolioVirusTypes::VRPV3,0); // sets antigenID for VRPV strains and genome to zero (fully Sabin), note: literal definition of "vaccine-related poliovirus"

        initConfigTypeMap( "Vaccine_Genome_OPV1", &vaccine_genome_OPV1, Vaccine_Genome_OPV1_DESC_TEXT, 0, 1023, 1 );
        initConfigTypeMap( "Vaccine_Genome_OPV2", &vaccine_genome_OPV2, Vaccine_Genome_OPV2_DESC_TEXT, 0, 1023, 1 );
        initConfigTypeMap( "Vaccine_Genome_OPV3", &vaccine_genome_OPV3, Vaccine_Genome_OPV3_DESC_TEXT, 0, 1023, 1 );
    }
#endif
#endif

#ifndef DISABLE_STI
    if( sim_type == SimType::STI_SIM ||
        sim_type == SimType::HIV_SIM 
      )
    {
        // DJK TODO: These parameters should be owned by Relationship
        initConfigTypeMap( "Enable_Coital_Dilution", &enable_coital_dilution, Enable_Coital_Dilution_DESC_TEXT, true );
        initConfigTypeMap( "Coital_Dilution_Factor_2_Partners", &coital_dilution_2_partners, Coital_Dilution_Factor_2_Partners_DESC_TEXT, FLT_EPSILON, 1.0f, 1.0f );
        initConfigTypeMap( "Coital_Dilution_Factor_3_Partners", &coital_dilution_3_partners, Coital_Dilution_Factor_3_Partners_DESC_TEXT, FLT_EPSILON, 1.0f, 1.0f);
        initConfigTypeMap( "Coital_Dilution_Factor_4_Plus_Partners", &coital_dilution_4_plus_partners, Coital_Dilution_Factor_4_Plus_Partners_DESC_TEXT, FLT_EPSILON, 1.0f, 1.0f );
    }

#ifndef DISABLE_HIV
    if( sim_type == SimType::HIV_SIM )
    {
        initConfigTypeMap( "Maternal_Transmission_ART_Multiplier", &maternal_transmission_ART_multiplier, Maternal_Transmission_ART_Multiplier_DESC_TEXT, 0.0f, 1.0f, 0.1f );

        initConfigTypeMap( "Days_Between_Symptomatic_And_Death_Weibull_Scale", &days_between_symptomatic_and_death_lambda, Days_Between_Symptomatic_And_Death_Weibull_Scale_DESC_TEXT, 1, 3650.0f, 183.0f );   // Constrain away from 0 for use as a rate
        initConfigTypeMap( "Days_Between_Symptomatic_And_Death_Weibull_Heterogeneity", &days_between_symptomatic_and_death_inv_kappa, Days_Between_Symptomatic_And_Death_Weibull_Heterogeneity_DESC_TEXT, 0.1f, 10.0f, 1.0f );   // Constrain away from 0 for use as a rate
    }
#endif // DISABLE_HIV
#endif // DISABLE_STI

    LOG_DEBUG_F( "Calling main Configure..., use_defaults = %d\n", JsonConfigurable::_useDefaults );
    bool ret = JsonConfigurable::Configure( inputJson );
    demographics_initial = !demographics_builtin;

#ifndef DISABLE_VECTOR
        for (const auto& vector_species_name : vector_species_names)
        {
            // vspMap only in SimConfig now. No more static map in VSP.
            vspMap[ vector_species_name ] = VectorSpeciesParameters::CreateVectorSpeciesParameters(vector_species_name);
        }
#endif


    if( JsonConfigurable::_dryrun == true )
    {
        return true;
    }

    for( int i = 0 ; i < IndividualEventTriggerType::pairs::count()-2 ; i++ )
    {
        auto trigger = IndividualEventTriggerType::pairs::lookup_key( i );
        if( trigger != nullptr )
        {
            listed_events.insert( trigger );
        }
    }

    if( sim_type == SimType::VECTOR_SIM || sim_type == SimType::MALARIA_SIM )
    {
        if( vector_species_names.empty() )
        {
            LOG_WARN("The simulation is being run without any mosquitoes!  Unless this was intentional, please specify the name of one or more vector species in the 'Vector_Species_Names' array and their associated vector species parameters.\n\n                     ,-.\n         `._        /  |        ,\n            `--._  ,   '    _,-'\n     _       __  `.|  / ,--'\n      `-._,-'  `-. \\ : /\n           ,--.-.-`'.'.-.,_-\n         _ `--'-'-;.'.'-'`--\n     _,-' `-.__,-' / : \\\n                _,'|  \\ `--._\n           _,--'   '   .     `-.\n         ,'         \\  |        `\n                     `-'\n\n");
        }

    }

#ifdef ENABLE_TB
//#if !defined(_DLLS_)
    //this section needs to be below the JsonConfigurable::Configure
    if( sim_type == SimType::TB_SIM )
    {    
        if( tb_drug_names_for_this_sim.empty() )
        {
            LOG_INFO("No custom drugs in config file!\n");
        }

        LOG_DEBUG_F("Reading in drugs \n");
        for (const auto& tb_drug_name : tb_drug_names_for_this_sim)
        {
            LOG_DEBUG_F("Reading in drug %s \n", tb_drug_name.c_str());
            auto * tbdtp = TBDrugTypeParameters::CreateTBDrugTypeParameters(tb_drug_name);
            release_assert( tbdtp  );
            TBDrugMap[ tb_drug_name ] = tbdtp;
        }
    }
//#endif
#endif

    if ( sim_type == SimType::MALARIA_SIM )
    {
#ifndef DISABLE_MALARIA
        // for each key in Malaria_Drug_Params, create/configure MalariaDrugTypeParameters object and add to static map
        try
        {
            json::Object mdp = (*EnvPtr->Config)["Malaria_Drug_Params"].As<Object>();
            json::Object::const_iterator itMdp;
            for (itMdp = mdp.Begin(); itMdp != mdp.End(); itMdp++)
            {
                std::string drug_name( itMdp->name );
                auto * mdtp = MalariaDrugTypeParameters::CreateMalariaDrugTypeParameters( drug_name );
                release_assert( mdtp );
                MalariaDrugMap[ drug_name ] = mdtp;
            }
        }
        catch(json::Exception &e)
        {
            // Exception casting Malaria_Drug_Params to json::Object
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, e.what() ); 
        }
#endif
    }


    lloffset = 0.5f * node_grid_size;

    //if (vector_sampling_type == VectorSamplingType::TRACK_ALL_VECTORS) mosquito_weight = 1;

    // a little malaria range checking
    if( sim_type == SimType::MALARIA_SIM )
    {
        larvalconstant0 = (float)(LARVAL_CONSTANT_FACTOR * lloffset * lloffset * ONE_POINT_TWO * (float)x_templarvalhabitat);
    }

#ifdef ENABLE_POLIO
#if !defined(_DLLS_)
    if( sim_type == SimType::POLIO_SIM )
    {
        std::vector<float> dummyvect(number_substrains, 1.0f);
        substrainRelativeInfectivity.resize(3, dummyvect);

#ifndef WIN32
    #define sscanf_s sscanf
#endif

        // reversion rates for Sabin attenuating sites
        for(int i=0; i<3; i++)
        {
            int nDataRead = 0;
            int ptrPosition;
            const char * readStr = tmpSiteRatesStrings[i].c_str();
            float data;
            size_t maximum_index = 1 + (int)( log((float)number_substrains) / log(2.0f) );
            vector<float> tmp_sabin_rates;
            tmp_sabin_rates.resize(maximum_index);

            while (sscanf_s(readStr, "%g%n", &data,  &ptrPosition) == 1)
            {
                if (nDataRead < maximum_index)
                    tmp_sabin_rates[nDataRead] = data;
                readStr += ptrPosition;
                nDataRead++;
            }

            if (nDataRead > maximum_index)
            {
                throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, "nDataRead(reversion rates for Sabin attenuating sites)", nDataRead, "maximum_index", maximum_index );
            }

            tmp_sabin_rates.resize(nDataRead);

            switch (i)
            {
            case 0: Sabin1_Site_Rates = tmp_sabin_rates; break;
            case 1: Sabin2_Site_Rates = tmp_sabin_rates; break;
            case 2: Sabin3_Site_Rates = tmp_sabin_rates; break;
            }

        }


        // vaccine infectivity by genotype
        for(int i=0; i<3; i++)
        {
            int nDataRead = 0;
            int ptrPosition;
            const char * readStr = tmpSubstrainInfectivityString[i].c_str();
            float data;
            size_t maximum_index = substrainRelativeInfectivity[i].size();
            while (sscanf_s(readStr, "%g%n", &data,  &ptrPosition) == 1)
            {
                if (nDataRead < maximum_index)
                    substrainRelativeInfectivity[i][nDataRead] = data;
                readStr += ptrPosition;
                nDataRead++;
            }

            if (nDataRead != number_substrains)
            {
                throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, "nDataRead", nDataRead, "number_substrains", number_substrains );
            }
        }
        decayRatePassiveImmunity = log(2.0f) / maternalAbHalfLife; // (days) decayrate=log(2.0)/halflife; Ogra 1968, Warren 1964, Dexiang1956, Plotkin 1959

        vaccine_strains[0].SetGeneticID(vaccine_genome_OPV1); // user-configuration of Sabin genome in OPV, default is zero
        vaccine_strains[1].SetGeneticID(vaccine_genome_OPV2); // user-configuration of Sabin genome in OPV, default is zero
        vaccine_strains[2].SetGeneticID(vaccine_genome_OPV3); // user-configuration of Sabin genome in OPV, default is zero
    }
#endif // end of _DLLS_
#endif // POLIO

    // Return list of all missing parameters
    return ret;
}

QuickBuilder SimulationConfig::GetSchema()
{
    LOG_DEBUG( "GetSchema\n" );
    json::Object * job = new json::Object( jsonSchemaBase );
    json::QuickBuilder retJson( *job );
#if !defined(_DLLS_)
#ifndef DISABLE_VECTOR
    for (auto& entry : vspMap)
    {
        json::QuickBuilder foo = json::QuickBuilder( vspMap[ entry.first ]->GetSchema() );
        const std::string& species_key = entry.first;
        retJson["Vector_Species_Params"][ species_key ] = foo.As<Object>();
    }
#endif
#ifndef DISABLE_MALARIA
    if( MalariaDrugMap.count( malaria_drug_placeholder_string ) > 0 )
    {
        retJson["Malaria_Drug_Params"][ malaria_drug_placeholder_string ] = MalariaDrugMap[ malaria_drug_placeholder_string ]->GetSchema().As<Object>();
    }
#endif
#ifdef ENABLE_TB
    if( TBDrugMap.count( tb_drug_placeholder_string ) > 0 )
    {
        retJson["TB_Drug_Params"][ tb_drug_placeholder_string ] = TBDrugMap[ tb_drug_placeholder_string ]->GetSchema().As<Object>();
    }
#endif
#endif
    return retJson;
}

SimulationConfig::SimulationConfig()
    : age_initialization_distribution_type(DistributionType::DISTRIBUTION_OFF)
    , vital_death_dependence(VitalDeathDependence::NONDISEASE_MORTALITY_OFF)
    , egg_hatch_delay_dist(EggHatchDelayDist::NO_DELAY)
    , egg_saturation(EggSaturation::NO_SATURATION)
    , evolution_polio_clock_type(EvolutionPolioClockType::POLIO_EVOCLOCK_NONE)
    , larval_density_dependence(LarvalDensityDependence::NO_DENSITY_DEPENDENCE)
    , load_balancing(LoadBalancingScheme::STATIC)
    , migration_structure(MigrationStructure::NO_MIGRATION)
    , PKPD_model(PKPDModel::FIXED_DURATION_CONSTANT_EFFECT)
    , population_scaling(PopulationScaling::USE_INPUT_FILE)
    , susceptibility_scaling(SusceptibilityScaling::CONSTANT_SUSCEPTIBILITY)
    , sim_type(SimType::GENERIC_SIM)
    , VDPV_virulence_model_type(VDPVVirulenceModelType::POLIO_VDPV_NONVIRULENT)
    , vector_sugar_feeding(VectorSugarFeeding::VECTOR_SUGAR_FEEDING_NONE)
    , vector_larval_rainfall_mortality(VectorRainfallMortality::NONE)
    , heg_model(HEGModel::OFF)
    , susceptibility_scaling_rate(-42.0f)
    , susceptibility_scaling_intercept(-42.0f)
    , vector_aging(false)
    , temperature_dependent_feeding_cycle(false)
    , meanEggHatchDelay(0.0f)
    , WolbachiaMortalityModification(0.0f)
    , WolbachiaInfectionModification(0.0f)
    , HEGhomingRate(0.0f)
    , HEGfecundityLimiting(0.0f)
    , human_feeding_mortality(DEFAULT_HUMAN_FEEDING_MORTALITY)
    , immunity_initialization_distribution_type( DistributionType::DISTRIBUTION_OFF )
    , parasiteSmearSensitivity(-42.0f)
    , newDiagnosticSensitivity(-42.0f)
    , falciparumMSPVars(0)
    , falciparumNonSpecTypes(0)
    , falciparumPfEMP1Vars(0)
    , larvalconstant0(0.0f)
    , tempHabitatDecayScalar(0.0f)
    , semipermanentHabitatDecayRate(0.0f)
    , mmRainfallToFillSwamp(1.0f)
    , larval_rainfall_mortality_threshold(1.0f)
    , larvalDensityMortalityScalar(10.0f)
    , larvalDensityMortalityOffset(0.1f)
    , demographics_initial(false)
    , default_torus_size( 10 )
    , default_node_population( 1000 )
    , lloffset(0)
    , coinfection_incidence(false)
    , enable_coinfection_mortality(false)
    , vital_dynamics(false)
    , vital_disease_mortality(false)
    , interventions(false)
    , feverDetectionThreshold(-42.0f)
    , airtemperature(-42.0f)
    , landtemperature(-42.0f)
    , rainfall(-42.0f)
    , humidity(-42.0f)
    , airtemperature_offset(-42.0f)
    , landtemperature_offset(-42.0f)
    , rainfall_scale_factor(-42.0f)
    , humidity_scale_factor(-42.0f)
    , airtemperature_variance(-42.0f)
    , landtemperature_variance(-42.0f)
    , rainfall_variance(false)
    , humidity_variance(-42.0f)
#ifdef ENABLE_POLIO
    , TauNAb(-42.0f)
//    , PVinf0( { -42.0f, -42.0f, -42.0f, -42.0f, -42.0f, -42.0f } )
//    , viral_interference( { -42.0f, -42.0f, -42.0f, -42.0f, -42.0f, -42.0f } )
    , mucosalImmIPV(-42.0f)
    , mucosalImmIPVOPVExposed(-42.0f)
//    , paralysis_base_rate( { -42.0f, -42.0f, -42.0f, -42.0f, -42.0f, -42.0f } )
//    , boostLog2NAb_OPV( { -42.0f, -42.0f, -42.0f } )
//    , boostLog2NAb_IPV( { -42.0f, -42.0f, -42.0f } )
//    , maxLog2NAb_OPV( { -42.0f, -42.0f, -42.0f } )
//    , maxLog2NAb_IPV( { -42.0f, -42.0f, -42.0f } )
//    , boostLog2NAb_stddev_OPV( { -42.0f, -42.0f, -42.0f } )
//    , boostLog2NAb_stddev_IPV( { -42.0f, -42.0f, -42.0f } )
//    , primeLog2NAb_OPV( { -42.0f, -42.0f, -42.0f } )
//    , primeLog2NAb_IPV( { -42.0f, -42.0f, -42.0f } )
//    , primeLog2NAb_stddev_OPV( { -42.0f, -42.0f, -42.0f } )
//    , primeLog2NAb_stddev_IPV( { -42.0f, -42.0f, -42.0f } )
    , decayRatePassiveImmunity(-42.0f)
    , maternal_log2NAb_mean(-42.0f)
    , maternal_log2NAb_std(-42.0f)
//    , vaccine_titer_tOPV( { -42.0f, -42.0f, -42.0f } )
//    , vaccine_titer_bOPV( { -42.0f, -42.0f } )
//    , vaccine_titer_mOPV( { -42.0f, -42.0f, -42.0f } )
//    , vaccine_Dantigen_IPV( { -42.0f, -42.0f, -42.0f } )
    , Incubation_Disease_Mu(-42.0f)
    , Incubation_Disease_Sigma(-42.0f)
#endif
    , number_basestrains(1)
    , number_substrains(0)
#ifdef ENABLE_POLIO
//    , reversionSteps_cVDPV( { -42.0f, -42.0f, -42.0f } )
    , excrement_load(-42.0f)
    , MaxRandStdDev(-42.0f)

    , shedFecalMaxLog10PeakTiter(-42.0f)
    , shedFecalMaxLog10PeakTiter_Stddev(-42.0f)
    , shedFecalTiterBlockLog2NAb(-42.0f)
    , shedFecalTiterProfile_mu(-42.0f)
    , shedFecalTiterProfile_sigma(-42.0f)
    , shedFecalMaxLnDuration(-42.0f)
    , shedFecalMaxLnDuration_Stddev(-42.0f)
    , shedFecalDurationBlockLog2NAb(-42.0f)
    , shedOralMaxLog10PeakTiter(-42.0f)
    , shedOralMaxLog10PeakTiter_Stddev(-42.0f)
    , shedOralTiterBlockLog2NAb(-42.0f)
    , shedOralTiterProfile_mu(-42.0f)
    , shedOralTiterProfile_sigma(-42.0f)
    , shedOralMaxLnDuration(-42.0f)
    , shedOralMaxLnDuration_Stddev(-42.0f)
    , shedOralDurationBlockLog2NAb(-42.0f)
#endif

    , heterogeneous_intranode_transmission_enabled(false)
    , Sim_Duration(-42.0f)
    , Sim_Tstep(-42.0f)
    , starttime(0.0f)
    , node_grid_size(0.0f)
    , Run_Number(-42)
    , branch_duration(-1)
    , branch_end_state(-1)
    , branch_start_state(-1)
    , burnin_period(-1)
    , serialization_test_cycles(0)
    , maternalAbHalfLife(-42.0f)
    , x_templarvalhabitat(1.0f)
#ifdef ENABLE_POLIO
    , vaccine_genome_OPV1(1)
    , vaccine_genome_OPV2(1)
    , vaccine_genome_OPV3(1)
#endif
#ifdef ENABLE_TB
    , tb_drug_names_for_this_sim()
    , TBDrugMap()
#endif
#ifdef ENABLE_TBHIV
    , cd4_count_at_beginning_of_hiv_infection(0.0f)
    , cd4_count_at_end_of_hiv_infection(0.0f)
#endif
#if !defined(_DLLS_)
//    , vaccine_strains( { 0, 0, 0 } )
#endif
#ifdef ENABLE_POLIO
    , substrainRelativeInfectivity()
    , Sabin1_Site_Rates()
    , Sabin2_Site_Rates()
    , Sabin3_Site_Rates()
#endif
    , vector_species_names()
    , ConfigName("UNSPECIFIED")
    , airmig_filename("UNSPECIFIED")
    , campaign_filename("UNSPECIFIED")
    , climate_airtemperature_filename("UNSPECIFIED")
    , climate_koppen_filename("UNSPECIFIED")
    , climate_landtemperature_filename("UNSPECIFIED")
    , climate_rainfall_filename("UNSPECIFIED")
    , climate_relativehumidity_filename("UNSPECIFIED")
    , loadbalance_filename("UNSPECIFIED")
    , localmig_filename("UNSPECIFIED")
    , regionmig_filename("UNSPECIFIED")
    , seamig_filename("UNSPECIFIED")
#ifdef ENABLE_PYTHON
    , python_script_path("UNSPECIFIED")
#endif
    , vspMap()
    , MalariaDrugMap()
    , m_jsonConfig(nullptr)
#ifndef DISABLE_STI
    , enable_coital_dilution(true)
    , coital_dilution_2_partners(1)
    , coital_dilution_3_partners(1)
    , coital_dilution_4_plus_partners(1)

#ifndef DISABLE_HIV
    , prob_maternal_transmission(1.0f)
    , days_between_symptomatic_and_death_lambda(183.0f)
    , days_between_symptomatic_and_death_inv_kappa(1.0f)
    , maternal_transmission_ART_multiplier(1.0f)
#endif // DISABLE_HIV
#endif // DISABLE_STI
{
#ifdef ENABLE_POLIO
    ZERO_ARRAY(PVinf0);
    ZERO_ARRAY(viral_interference);
    ZERO_ARRAY(vaccine_take_multiplier);
    ZERO_ARRAY(paralysis_base_rate);
    ZERO_ARRAY(boostLog2NAb_OPV);
    ZERO_ARRAY(boostLog2NAb_IPV);
    ZERO_ARRAY(maxLog2NAb_OPV);
    ZERO_ARRAY(maxLog2NAb_IPV);
    ZERO_ARRAY(boostLog2NAb_stddev_OPV);
    ZERO_ARRAY(boostLog2NAb_stddev_IPV);
    ZERO_ARRAY(primeLog2NAb_OPV);
    ZERO_ARRAY(primeLog2NAb_IPV);
    ZERO_ARRAY(primeLog2NAb_stddev_OPV);
    ZERO_ARRAY(primeLog2NAb_stddev_IPV);
    ZERO_ARRAY(vaccine_titer_tOPV);
    ZERO_ARRAY(vaccine_titer_bOPV);
    ZERO_ARRAY(vaccine_titer_mOPV);
    ZERO_ARRAY(vaccine_Dantigen_IPV);
    ZERO_ARRAY(reversionSteps_cVDPV);
#if !defined(_DLLS_)
    ZERO_ARRAY(vaccine_strains);
#endif
#endif
}

}
