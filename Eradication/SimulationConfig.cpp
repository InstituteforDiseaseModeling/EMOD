/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

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
#include "Configure.h"
#include "InterventionEnums.h"
#include "Log.h"
#include "Sugar.h"
#include "NoCrtWarnings.h"

#ifndef DISABLE_VECTOR
#include "VectorParameters.h"
#include "VectorSpeciesParameters.h"
#endif

#ifndef DISABLE_MALARIA
#include "GenomeMarkers.h"
#include "MalariaParameters.h"
#include "MalariaDrugTypeParameters.h"
#endif

#ifdef ENABLE_POLIO
#include "PolioParameters.h"
#endif

#ifdef ENABLE_TBHIV
#include "TBHIVDrugTypeParameters.h"
#include "TBHIVParameters.h"
#endif

SETUP_LOGGING( "SimulationConfig" )

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

SimulationConfig* SimulationConfigFactory::CreateInstance(Configuration * config)
{
    SimulationConfig *SimConfig =  _new_ SimulationConfig();
    release_assert(SimConfig);
    if (SimConfig)
    {           
        SimConfig->SetFixedParameters(config);
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

void SimulationConfig::SetFixedParameters(Configuration * inputJson)
{
    SimType::Enum sim_type;
    initConfig("Simulation_Type", sim_type, inputJson, MetadataDescriptor::Enum(Simulation_Type_DESC_TEXT, Simulation_Type_DESC_TEXT, MDD_ENUM_ARGS(SimType))); // simulation only (???move)

    switch (sim_type)
    {
    case SimType::MALARIA_SIM:
        inputJson->Add("Enable_Immunity", 1);
        inputJson->Add("Enable_Immune_Decay", 1);
        inputJson->Add("Enable_Maternal_Protection", 0 );
        break;
    case SimType::HIV_SIM:
        inputJson->Add("Enable_Disease_Mortality", 1);
        inputJson->Add("Enable_Immunity", 1);   //There is no HIV immunity. Switch is used to enable ART.
        inputJson->Add("Enable_Immune_Decay", 0);   //must exist because Enable_Immunity: 1
        inputJson->Add("Enable_Immunity_Distribution", 0); //must exist because Enable_Immunity: 1
        inputJson->Add("Enable_Maternal_Infection_Transmission", 1);
        inputJson->Add("Enable_Vital_Dynamics", 1);
        break;
    case SimType::TYPHOID_SIM:
        inputJson->Add("Enable_Maternal_Infection_Transmission", 0);  //must exist because fixed-off and depends on Enable_Birth
        break;
    case SimType::DENGUE_SIM:
        inputJson->Add("Enable_Immunity", 1);
        inputJson->Add("Enable_Immune_Decay", 1);
        inputJson->Add("Enable_Maternal_Infection_Transmission", 1);
        break;
    case SimType::POLIO_SIM:
        inputJson->Add("Enable_Immunity", 1);
        inputJson->Add("Enable_Immune_Decay", 1);
        inputJson->Add("Enable_Immunity_Distribution", 1);
        inputJson->Add("Enable_Superinfection", 1);
        inputJson->Add("Enable_Disease_Mortality", 1);
        inputJson->Add("Enable_Maternal_Infection_Transmission", 0); //must exist because fixed-off and depends on Enable_Birth
        break;
    }
}

bool SimulationConfig::Configure(const Configuration * inputJson)
{
    LOG_DEBUG( "Configure\n" );

    release_assert(inputJson);

    m_jsonConfig = inputJson;

    // --------------------------------------------------
    // --- Define the parameters to be read from the JSON
    // --------------------------------------------------

    // ------
    // Generic Parameters
    // ------
    initConfig( "Simulation_Type", sim_type,  inputJson, MetadataDescriptor::Enum(Simulation_Type_DESC_TEXT, Simulation_Type_DESC_TEXT, MDD_ENUM_ARGS(SimType)) ); // simulation only (???move)

    initConfigTypeMap( "Simulation_Duration", &Sim_Duration, Simulation_Duration_DESC_TEXT, 0.0f, 1000000.0f, 1.0f ); // 'global'? (controller only)
    initConfigTypeMap( "Simulation_Timestep", &Sim_Tstep,    Simulation_Timestep_DESC_TEXT, 0.0f, 1000000.0f, 1.0f ); // 'global'? (controller only)
    initConfigTypeMap( "Start_Time",          &starttime,    Start_Time_DESC_TEXT,          0.0f, 1000000.0f, 1.0f ); // simulation only (actually Climate also!)
    initConfigTypeMap( "Config_Name",         &ConfigName,   Config_Name_DESC_TEXT );

    bool demographics_builtin = false;
    initConfigTypeMap( "Enable_Demographics_Builtin",               &demographics_builtin,    Enable_Demographics_Builtin_DESC_TEXT, false ); // 'global' (3 files)
    initConfigTypeMap( "Default_Geography_Initial_Node_Population", &default_node_population, Default_Geography_Initial_Node_Population_DESC_TEXT, 0, 1e6, 1000, "Enable_Demographics_Builtin");
    initConfigTypeMap( "Default_Geography_Torus_Size",              &default_torus_size,      Default_Geography_Torus_Size_DESC_TEXT,              3, 100,   10, "Enable_Demographics_Builtin");
    initConfigTypeMap( "Node_Grid_Size", &node_grid_size, Node_Grid_Size_DESC_TEXT, 0.004167f, 90.0f, 0.004167f );

    initConfig( "Migration_Model",       migration_structure,    inputJson, MetadataDescriptor::Enum(Migration_Model_DESC_TEXT,       Migration_Model_DESC_TEXT,       MDD_ENUM_ARGS(MigrationStructure)) ); // 'global'

    initConfigTypeMap( "Enable_Interventions", &interventions, Enable_Interventions_DESC_TEXT, false );

    initConfigTypeMap( "Enable_Heterogeneous_Intranode_Transmission", &heterogeneous_intranode_transmission_enabled, Enable_Heterogeneous_Intranode_Transmission_DESC_TEXT, false ); // generic

    initConfigTypeMap( "Number_Basestrains", &number_basestrains, Number_Basestrains_DESC_TEXT, 1,       10,   1 );

    if( this->sim_type != SimType::MALARIA_SIM )
    {
        initConfigTypeMap( "Number_Substrains", &number_substrains, Number_Substrains_DESC_TEXT, 1, 16777216, 256 );
    }

    //vector enums
    if (sim_type == SimType::VECTOR_SIM || sim_type == SimType::MALARIA_SIM || sim_type == SimType::DENGUE_SIM )
    {
        VectorInitConfig( inputJson );

        if (sim_type == SimType::MALARIA_SIM)
        {
            MalariaInitConfig( inputJson );
        }
    }
    else if (sim_type == SimType::POLIO_SIM)
    {
        PolioInitConfig( inputJson );
    }
    else if( sim_type == SimType::TBHIV_SIM)
    {
        TBHIVInitConfig( inputJson );
    }

    // --------------------------------------
    // --- Read the parameters from the JSON
    // --------------------------------------
    LOG_DEBUG_F( "Calling main Configure..., use_defaults = %d\n", JsonConfigurable::_useDefaults );

    bool ret = JsonConfigurable::Configure( inputJson );
    demographics_initial = !demographics_builtin;

    // ---------------------------------------
    // --- Return if just generating schema
    // ---------------------------------------
    if( JsonConfigurable::_dryrun == true )
    {
        static vector<int32_t> serialization_time_steps;
        static string serialized_population_path;
        static vector<string> serialized_population_filenames;

        initConfigTypeMap( "Serialization_Time_Steps",        &serialization_time_steps,        Serialization_Time_Steps_DESC_TEXT, 0, INT_MAX, 0 );
        initConfigTypeMap( "Serialized_Population_Path",      &serialized_population_path,      Serialized_Population_Path_DESC_TEXT, "." );
        initConfigTypeMap( "Serialized_Population_Filenames", &serialized_population_filenames, Serialized_Population_Filenames_DESC_TEXT );

        return true;
    }

    // ------------------------------------------------------------------
    // --- Update, check, and read other parameters given the input data
    // ------------------------------------------------------------------
    lloffset = 0.5f * node_grid_size;

    if (sim_type == SimType::VECTOR_SIM || sim_type == SimType::MALARIA_SIM || sim_type == SimType::DENGUE_SIM )
    {
        VectorCheckConfig( inputJson );
        if ( sim_type == SimType::MALARIA_SIM )
        {
            MalariaCheckConfig( inputJson );
        }
    }
    else if( sim_type == SimType::POLIO_SIM )
    {
        PolioCheckConfig( inputJson );
    }
    else if( sim_type == SimType::TBHIV_SIM )
    {
        TBHIVCheckConfig( inputJson );
    }

    return ret;
}

QuickBuilder SimulationConfig::GetSchema()
{
    LOG_DEBUG( "GetSchema\n" );
    json::Object * job = new json::Object( GetSchemaBase() );
    json::QuickBuilder retJson( *job );

#if !defined(_DLLS_)
    VectorAddSchema( retJson );
    MalariaAddSchema( retJson );
    PolioAddSchema( retJson );
    TBHIVAddSchema( retJson );
#endif

    return retJson;
}

SimulationConfig::SimulationConfig()
    : migration_structure(MigrationStructure::NO_MIGRATION) 
    , sim_type(SimType::GENERIC_SIM) 
    , demographics_initial(false)
    , default_torus_size( 10 )
    , default_node_population( 1000 )
    , lloffset(0) 
    , interventions(false)
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
    , rainfall_variance_enabled(false)
    , humidity_variance(-42.0f)
    , number_basestrains(1)
    , number_substrains(0)
    , heterogeneous_intranode_transmission_enabled(false)
    , Sim_Duration(-42.0f)
    , Sim_Tstep(-42.0f)
    , starttime(0.0f)
    , node_grid_size(0.0f)
    , Run_Number(-42)
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
    , m_jsonConfig(nullptr)
    , vector_params(nullptr)
    , malaria_params(nullptr)
    , tb_params(nullptr)
    , polio_params(nullptr)
    , tbhiv_params(nullptr)
    , tb_drug_names_for_this_sim()
{
#ifndef DISABLE_VECTOR
    vector_params = new VectorParameters();
#endif
#ifndef DISABLE_MALARIA
    malaria_params = new MalariaParameters();
    malaria_params->pGenomeMarkers = new GenomeMarkers();
#endif
#ifdef ENABLE_POLIO
    polio_params = new PolioParameters();
#endif
#ifdef ENABLE_TBHIV
    tbhiv_params = new TBHIVParameters();
#endif
}

// ----------------------------------------------------------------------------
// --- VectorParameters
// ----------------------------------------------------------------------------

void SimulationConfig::VectorInitConfig( const Configuration* inputJson )
{
#ifndef DISABLE_VECTOR
    initConfig( "Vector_Sampling_Type",             vector_params->vector_sampling_type,             inputJson, MetadataDescriptor::Enum(Vector_Sampling_Type_DESC_TEXT,             Vector_Sampling_Type_DESC_TEXT,             MDD_ENUM_ARGS(VectorSamplingType))      ); // node (vector) only
    initConfig( "Egg_Hatch_Delay_Distribution",     vector_params->egg_hatch_delay_dist,             inputJson, MetadataDescriptor::Enum(Egg_Hatch_Delay_Distribution_DESC_TEXT,     Egg_Hatch_Delay_Distribution_DESC_TEXT,     MDD_ENUM_ARGS(EggHatchDelayDist))       ); // vector pop only
    initConfig( "Egg_Saturation_At_Oviposition",    vector_params->egg_saturation,                   inputJson, MetadataDescriptor::Enum(Egg_Saturation_At_Oviposition_DESC_TEXT,    Egg_Saturation_At_Oviposition_DESC_TEXT,    MDD_ENUM_ARGS(EggSaturation))           ); // vector pop only
    initConfig( "Larval_Density_Dependence",        vector_params->larval_density_dependence,        inputJson, MetadataDescriptor::Enum(Larval_Density_Dependence_DESC_TEXT,        Larval_Density_Dependence_DESC_TEXT,        MDD_ENUM_ARGS(LarvalDensityDependence)) ); // vector pop only
    initConfig( "Egg_Hatch_Density_Dependence",     vector_params->egg_hatch_density_dependence,     inputJson, MetadataDescriptor::Enum(Egg_Hatch_Density_Dependence_DESC_TEXT,     Egg_Hatch_Density_Dependence_DESC_TEXT,     MDD_ENUM_ARGS(EggHatchDensityDependence)) ); // vector pop only

    initConfig( "Vector_Sugar_Feeding_Frequency",   vector_params->vector_sugar_feeding,             inputJson, MetadataDescriptor::Enum(Vector_Sugar_Feeding_Frequency_DESC_TEXT,   Vector_Sugar_Feeding_Frequency_DESC_TEXT,   MDD_ENUM_ARGS(VectorSugarFeeding)), "Vector_Sampling_Type", "TRACK_ALL_VECTORS,SAMPLE_IND_VECTORS"      ); // vector pop individual only
    initConfig( "Vector_Larval_Rainfall_Mortality", vector_params->vector_larval_rainfall_mortality, inputJson, MetadataDescriptor::Enum(Vector_Larval_Rainfall_Mortality_DESC_TEXT, Vector_Larval_Rainfall_Mortality_DESC_TEXT, MDD_ENUM_ARGS(VectorRainfallMortality)) );
    // get the c50 value if there is rainfall mortality
    initConfigTypeMap( "Larval_Rainfall_Mortality_Threshold", &(vector_params->larval_rainfall_mortality_threshold), Larval_Rainfall_Mortality_Threshold_DESC_TEXT, 0.01f, 1000.0f, 100.0f, "Vector_Larval_Rainfall_Mortality", "SIGMOID,SIGMOID_HABITAT_SHIFTING" );

    //Q added stuff
    initConfigTypeMap( "Enable_Temperature_Dependent_Egg_Hatching", &(vector_params->temperature_dependent_egg_hatching), Enable_Temperature_Dependent_Egg_Hatching_DESC_TEXT, false );
    initConfigTypeMap( "Egg_Arrhenius1", &(vector_params->eggarrhenius1), Egg_Arrhenius1_DESC_TEXT, 0.0f, 10000000000.0f, 61599956.864f, "Enable_Temperature_Dependent_Egg_Hatching" );
    initConfigTypeMap( "Egg_Arrhenius2", &(vector_params->eggarrhenius2), Egg_Arrhenius2_DESC_TEXT, 0.0f, 10000000000.0f, 5754.033f, "Enable_Temperature_Dependent_Egg_Hatching" );
    initConfigTypeMap( "Enable_Egg_Mortality", &(vector_params->egg_mortality), Enable_Egg_Mortality_DESC_TEXT, false ); // not hooked up yet
    initConfigTypeMap( "Enable_Drought_Egg_Hatch_Delay", &(vector_params->delayed_hatching_when_habitat_dries_up), Enable_Drought_Egg_Hatch_Delay_DESC_TEXT, false );
    initConfigTypeMap( "Drought_Egg_Hatch_Delay", &(vector_params->droughtEggHatchDelay), Drought_Egg_Hatch_Delay_DESC_TEXT, 0.0f, 1.0f, 0.33f, "Enable_Drought_Egg_Hatch_Delay" );

    // get the larval density dependence parameters 
    initConfigTypeMap( "Larval_Density_Mortality_Scalar", &(vector_params->larvalDensityMortalityScalar), Larval_Density_Mortality_Scalar_DESC_TEXT,  0.01f, 1000.0f, 10.0f, "Larval_Density_Dependence", "GRADUAL_INSTAR_SPECIFIC,LARVAL_AGE_DENSITY_DEPENDENT_MORTALITY_ONLY" );
    initConfigTypeMap( "Larval_Density_Mortality_Offset", &(vector_params->larvalDensityMortalityOffset), Larval_Density_Mortality_Offset_DESC_TEXT, 0.0001f, 1000.0f, 0.1f, "Larval_Density_Dependence", "GRADUAL_INSTAR_SPECIFIC,LARVAL_AGE_DENSITY_DEPENDENT_MORTALITY_ONLY" );

    initConfig( "HEG_Model", vector_params->heg_model, inputJson, MetadataDescriptor::Enum(HEG_Model_DESC_TEXT, HEG_Model_DESC_TEXT, MDD_ENUM_ARGS(HEGModel)) );
    // get the larval density dependence parameters
    initConfigTypeMap( "HEG_Homing_Rate",        &(vector_params->HEGhomingRate),        HEG_Homing_Rate_DESC_TEXT,        0, 1, 0, "HEG_Model", "GERMLINE_HOMING,EGG_HOMING,DUAL_GERMLINE_HOMING,DRIVING_Y" );
    initConfigTypeMap( "HEG_Fecundity_Limiting", &(vector_params->HEGfecundityLimiting), HEG_Fecundity_Limiting_DESC_TEXT, 0, 1, 0, "HEG_Model", "GERMLINE_HOMING,EGG_HOMING,DUAL_GERMLINE_HOMING,DRIVING_Y" );

    //initConfigTypeMap( "Enable_Vector_Species_Habitat_Competition", &enable_vector_species_habitat_competition, VECTOR_Enable_Vector_Species_Habitat_Competition, false );

    initConfigTypeMap( "Enable_Vector_Aging",                 &(vector_params->vector_aging),                        Enable_Vector_Aging_DESC_TEXT, false );
    initConfig( "Temperature_Dependent_Feeding_Cycle", vector_params->temperature_dependent_feeding_cycle, inputJson, MetadataDescriptor::Enum(Temperature_Dependent_Feeding_Cycle_DESC_TEXT, Temperature_Dependent_Feeding_Cycle_DESC_TEXT, MDD_ENUM_ARGS(TemperatureDependentFeedingCycle)) ); // vector pop only

    initConfigTypeMap( "Mean_Egg_Hatch_Delay",                       &(vector_params->meanEggHatchDelay),                   Mean_Egg_Hatch_Delay_DESC_TEXT, 0, 120, 0 );
    initConfigTypeMap( "Wolbachia_Mortality_Modification",           &(vector_params->WolbachiaMortalityModification),      Wolbachia_Mortality_Modification_DESC_TEXT, 0, 100, 1 );
    initConfigTypeMap( "Wolbachia_Infection_Modification",           &(vector_params->WolbachiaInfectionModification),      Wolbachia_Infection_Modification_DESC_TEXT, 0, 100, 1 );

    initConfigTypeMap( "Human_Feeding_Mortality", &(vector_params->human_feeding_mortality), Human_Feeding_Mortality_DESC_TEXT, 0.0f, 1.0f, 0.1f );

    initConfigTypeMap( "Temporary_Habitat_Decay_Factor",   &(vector_params->tempHabitatDecayScalar),        Temporary_Habitat_Decay_Factor_DESC_TEXT,   0.001f,    100.0f, 0.05f );
    initConfigTypeMap( "Semipermanent_Habitat_Decay_Rate", &(vector_params->semipermanentHabitatDecayRate), Semipermanent_Habitat_Decay_Rate_DESC_TEXT, 0.0001f,   100.0f, 0.01f );
    initConfigTypeMap( "Rainfall_In_mm_To_Fill_Swamp",     &(vector_params->mmRainfallToFillSwamp),         Rainfall_In_mm_To_Fill_Swamp_DESC_TEXT,        1.0f, 10000.0f, 1000.0f );

    // This is a key parameter for the mosquito ecology and can vary quite a lot
    initConfigTypeMap( "x_Temporary_Larval_Habitat", &(vector_params->x_tempLarvalHabitat), x_Temporary_Larval_Habitat_DESC_TEXT, 0.0f, 10000.0f, 1.0f ); // should this be renamed vector_weight?

    vector_params->vector_species_names.value_source = "Vector_Species_Params.*";
    initConfigTypeMap( "Vector_Species_Names", &(vector_params->vector_species_names), Vector_Species_Names_DESC_TEXT );

    if( JsonConfigurable::_dryrun )
    {
#if !defined(_DLLS_)
        // for the schema
        std::string arab( "vector_species_name_goes_here" );
        VectorSpeciesParameters * vsp = VectorSpeciesParameters::CreateVectorSpeciesParameters( inputJson, arab );
        vsp->Configure( inputJson );
        vector_params->vspMap[ arab ] = vsp;
#endif
    }

#endif // DISABLE_VECTOR
}

void SimulationConfig::VectorCheckConfig( const Configuration* inputJson )
{
#ifndef DISABLE_VECTOR
    if( Sim_Tstep != 1.0f )
    {
        // There has not been sufficient testing for vector related simulations when dt != 1.
        throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, "Vector-based simulations assume that the Simulation_Timestep = 1" );
    }

    if( vector_params->vector_species_names.empty() )
    {
        LOG_WARN("The simulation is being run without any mosquitoes!  Unless this was intentional, please specify the name of one or more vector species in the 'Vector_Species_Names' array and their associated vector species parameters.\n\n                     ,-.\n         `._        /  |        ,\n            `--._  ,   '    _,-'\n     _       __  `.|  / ,--'\n      `-._,-'  `-. \\ : /\n           ,--.-.-`'.'.-.,_-\n         _ `--'-'-;.'.'-'`--\n     _,-' `-.__,-' / : \\\n                _,'|  \\ `--._\n           _,--'   '   .     `-.\n         ,'         \\  |        `\n                     `-'\n\n");
    }
    for (const auto& vector_species_name : vector_params->vector_species_names)
    {
        // vspMap only in SimConfig now. No more static map in VSP.
        vector_params->vspMap[ vector_species_name ] = VectorSpeciesParameters::CreateVectorSpeciesParameters( inputJson,
                                                                                                               vector_species_name );
    }
#endif // DISABLE_VECTOR
}

void SimulationConfig::VectorAddSchema( json::QuickBuilder& retJson )
{
#ifndef DISABLE_VECTOR
    for (auto& entry : vector_params->vspMap)
    {
        json::QuickBuilder foo = json::QuickBuilder( vector_params->vspMap[ entry.first ]->GetSchema() );
        const std::string& species_key = entry.first;
        retJson["Vector_Species_Params"][ species_key ] = foo.As<Object>();
    }
#endif // DISABLE_VECTOR
}

// ----------------------------------------------------------------------------
// --- MalariaParameters
// ----------------------------------------------------------------------------

void SimulationConfig::MalariaInitConfig( const Configuration* inputJson )
{
#ifndef DISABLE_MALARIA
    initConfig( "PKPD_Model", malaria_params->PKPD_model, inputJson, MetadataDescriptor::Enum(PKPD_Model_DESC_TEXT, PKPD_Model_DESC_TEXT, MDD_ENUM_ARGS(PKPDModel)) ); // special case: intervention (anti-malarial drug) only

    initConfigTypeMap( "Falciparum_MSP_Variants",      &(malaria_params->falciparumMSPVars),       Falciparum_MSP_Variants_DESC_TEXT,      0, 1e3, DEFAULT_MSP_VARIANTS ); // malaria
    initConfigTypeMap( "Falciparum_Nonspecific_Types", &(malaria_params->falciparumNonSpecTypes),  Falciparum_Nonspecific_Types_DESC_TEXT, 0, 1e3, DEFAULT_NONSPECIFIC_TYPES ); // malaria
    initConfigTypeMap( "Falciparum_PfEMP1_Variants",   &(malaria_params->falciparumPfEMP1Vars),    Falciparum_PfEMP1_Variants_DESC_TEXT,   0, 1e5, DEFAULT_PFEMP1_VARIANTS ); // malaria
    initConfigTypeMap( "Fever_Detection_Threshold",    &(malaria_params->feverDetectionThreshold), Fever_Detection_Threshold_DESC_TEXT,    0.5f, 5.0f, 1.0f );

    initConfigTypeMap( "Parasite_Smear_Sensitivity", &(malaria_params->parasiteSmearSensitivity), Parasite_Smear_Sensitivity_DESC_TEXT, 0.0001f, 100.0f, 0.1f ); // malaria
    initConfigTypeMap( "New_Diagnostic_Sensitivity", &(malaria_params->newDiagnosticSensitivity), New_Diagnostic_Sensitivity_DESC_TEXT, 0.0001f, 100000.0f, 0.01f ); // malaria

    initConfigTypeMap( "Genome_Markers", &(malaria_params->genome_marker_names), Genome_Markers_DESC_TEXT );

    // for schema?
    if( JsonConfigurable::_dryrun )
    {
        std::string drug_name( malaria_drug_placeholder_string );
        MalariaDrugTypeParameters * mdtp = MalariaDrugTypeParameters::CreateMalariaDrugTypeParameters( inputJson, drug_name, *(malaria_params->pGenomeMarkers) );
        mdtp->Configure( inputJson );
        malaria_params->MalariaDrugMap[ drug_name ] = mdtp;
    }
#endif //DISABLE_MALARIA
}

void SimulationConfig::MalariaCheckConfig( const Configuration* inputJson )
{
#ifndef DISABLE_MALARIA
    // for each key in Malaria_Drug_Params, create/configure MalariaDrugTypeParameters object and add to static map
    try
    {
        if( (malaria_params->genome_marker_names.size() > 0) &&
            ((vector_params->vector_sampling_type == VectorSamplingType::VECTOR_COMPARTMENTS_NUMBER) ||
             (vector_params->vector_sampling_type == VectorSamplingType::VECTOR_COMPARTMENTS_PERCENT)) )
        {
            throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                    "Vector_Sampling_Type", VectorSamplingType::pairs::lookup_key( vector_params->vector_sampling_type ),
                                                    "Genome_Markers", "<not empty>",
                                                    "Genome_Markers can only be used with individual vectors (i.e. TRACK_ALL_VECTORS or SAMPLE_IND_VECTORS)." );
        }

        // number_substrains is not configured from the JSON like the other diseases
        number_substrains = malaria_params->pGenomeMarkers->Initialize( malaria_params->genome_marker_names );

        json::Object mdp = (*EnvPtr->Config)["Malaria_Drug_Params"].As<Object>();
        json::Object::const_iterator itMdp;
        for (itMdp = mdp.Begin(); itMdp != mdp.End(); ++itMdp)
        {
            std::string drug_name( itMdp->name );
            auto * mdtp = MalariaDrugTypeParameters::CreateMalariaDrugTypeParameters( inputJson, drug_name, *(malaria_params->pGenomeMarkers) );
            release_assert( mdtp );
            malaria_params->MalariaDrugMap[ drug_name ] = mdtp;
        }
    }
    catch(json::Exception &e)
    {
        // Exception casting Malaria_Drug_Params to json::Object
        throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, e.what() ); 
    }
#endif //DISABLE_MALARIA
}

void SimulationConfig::MalariaAddSchema( json::QuickBuilder& retJson )
{
#ifndef DISABLE_MALARIA
    if( malaria_params->MalariaDrugMap.count( malaria_drug_placeholder_string ) > 0 )
    {
        retJson["Malaria_Drug_Params"][ malaria_drug_placeholder_string ] = malaria_params->MalariaDrugMap[ malaria_drug_placeholder_string ]->GetSchema().As<Object>();
    }
#endif // DISABLE_MALARIA
}


// ----------------------------------------------------------------------------
// --- PolioParameters
// ----------------------------------------------------------------------------

void SimulationConfig::PolioInitConfig( const Configuration* inputJson )
{
#ifdef ENABLE_POLIO

    initConfig( "Evolution_Polio_Clock_Type", polio_params->evolution_polio_clock_type, inputJson, MetadataDescriptor::Enum(Evolution_Polio_Clock_Type_DESC_TEXT, Evolution_Polio_Clock_Type_DESC_TEXT, MDD_ENUM_ARGS(EvolutionPolioClockType)) ); // infection (polio) only
    initConfig( "VDPV_Virulence_Model_Type",  polio_params->VDPV_virulence_model_type,  inputJson, MetadataDescriptor::Enum(VDPV_Virulence_Model_Type_DESC_TEXT,  VDPV_Virulence_Model_Type_DESC_TEXT,  MDD_ENUM_ARGS(VDPVVirulenceModelType))  ); // susceptibility polio only
    initConfigTypeMap( "Max_Rand_Standard_Deviations", &(polio_params->MaxRandStdDev), Max_Rand_Standard_Deviations_DESC_TEXT, 0.0f, 10.0f, 2.0f);

    initConfigTypeMap( "Specific_Infectivity_WPV1",   &(polio_params->PVinf0[0]), Specific_Infectivity_WPV1_DESC_TEXT,   0.0f, 1.0e+3f, 0.004f );
    initConfigTypeMap( "Specific_Infectivity_WPV2",   &(polio_params->PVinf0[1]), Specific_Infectivity_WPV2_DESC_TEXT,   0.0f, 1.0e+3f, 0.004f );
    initConfigTypeMap( "Specific_Infectivity_WPV3",   &(polio_params->PVinf0[2]), Specific_Infectivity_WPV3_DESC_TEXT,   0.0f, 1.0e+3f, 0.004f );
    initConfigTypeMap( "Specific_Infectivity_Sabin1", &(polio_params->PVinf0[3]), Specific_Infectivity_Sabin1_DESC_TEXT, 0.0f, 1.0e+3f, 3.02e-005f );
    initConfigTypeMap( "Specific_Infectivity_Sabin2", &(polio_params->PVinf0[4]), Specific_Infectivity_Sabin2_DESC_TEXT, 0.0f, 1.0e+3f, 0.00135f );
    initConfigTypeMap( "Specific_Infectivity_Sabin3", &(polio_params->PVinf0[5]), Specific_Infectivity_Sabin3_DESC_TEXT, 0.0f, 1.0e+3f, 0.000115f );

    initConfigTypeMap( "Viral_Interference_WPV1",     &(polio_params->viral_interference[0]), Viral_Interference_WPV1_DESC_TEXT,   0.0f, 1.0f, 0.0653f );
    initConfigTypeMap( "Viral_Interference_WPV2",     &(polio_params->viral_interference[1]), Viral_Interference_WPV2_DESC_TEXT,   0.0f, 1.0f, 0.278f );
    initConfigTypeMap( "Viral_Interference_WPV3",     &(polio_params->viral_interference[2]), Viral_Interference_WPV3_DESC_TEXT,   0.0f, 1.0f, 0.263f );
    initConfigTypeMap( "Viral_Interference_Sabin1",   &(polio_params->viral_interference[3]), Viral_Interference_Sabin1_DESC_TEXT, 0.0f, 1.0f, 0.0653f );
    initConfigTypeMap( "Viral_Interference_Sabin2",   &(polio_params->viral_interference[4]), Viral_Interference_Sabin2_DESC_TEXT, 0.0f, 1.0f, 0.278f );
    initConfigTypeMap( "Viral_Interference_Sabin3",   &(polio_params->viral_interference[5]), Viral_Interference_Sabin3_DESC_TEXT, 0.0f, 1.0f, 0.263f );


    //this scaling factor is for vaccines, so set the scaling of wild virus to 1.
    polio_params->vaccine_take_multiplier[0] = 1.0f;
    polio_params->vaccine_take_multiplier[1] = 1.0f;
    polio_params->vaccine_take_multiplier[2] = 1.0f;
    initConfigTypeMap("Vaccine_Take_Scaling_Sabin1", &(polio_params->vaccine_take_multiplier[3]), Vaccine_Take_Multiplier_Sabin1_DESC_TEXT, 0.0f, 1.0f, 1.0f);
    initConfigTypeMap("Vaccine_Take_Scaling_Sabin2", &(polio_params->vaccine_take_multiplier[4]), Vaccine_Take_Multiplier_Sabin2_DESC_TEXT, 0.0f, 1.0f, 0.278f);
    initConfigTypeMap("Vaccine_Take_Scaling_Sabin3", &(polio_params->vaccine_take_multiplier[5]), Vaccine_Take_Multiplier_Sabin3_DESC_TEXT, 0.0f, 1.0f, 0.263f);

    initConfigTypeMap( "Mucosal_Immunogenicity_IPV",            &(polio_params->mucosalImmIPV),           Mucosal_Immunogenicity_IPV_DESC_TEXT, 0.0f, 100.0f, 3.0f );
    initConfigTypeMap( "Mucosal_Immunogenicity_IPV_OPVExposed", &(polio_params->mucosalImmIPVOPVExposed), Mucosal_Immunogenicity_IPV_OPVEXPOSED_DESC_TEXT, 0.0f, 100.0f, 3.0f);
    initConfigTypeMap( "Neutralization_Time_Tau",               &(polio_params->TauNAb),                  Neutralization_Time_Tau_DESC_TEXT, 0.0f, 1.0f, 0.04f );

    initConfigTypeMap( "Paralysis_Base_Rate_WPV1",   &(polio_params->paralysis_base_rate[0]), Paralysis_Base_Rate_WPV1_DESC_TEXT,   0.0f, 1.0f, 0.05f );
    initConfigTypeMap( "Paralysis_Base_Rate_WPV2",   &(polio_params->paralysis_base_rate[1]), Paralysis_Base_Rate_WPV2_DESC_TEXT,   0.0f, 1.0f, 0.00033f );
    initConfigTypeMap( "Paralysis_Base_Rate_WPV3",   &(polio_params->paralysis_base_rate[2]), Paralysis_Base_Rate_WPV3_DESC_TEXT,   0.0f, 1.0f, 0.001f );
    initConfigTypeMap( "Paralysis_Base_Rate_Sabin1", &(polio_params->paralysis_base_rate[3]), Paralysis_Base_Rate_Sabin1_DESC_TEXT, 0.0f, 1.0f, 0.005f );
    initConfigTypeMap( "Paralysis_Base_Rate_Sabin2", &(polio_params->paralysis_base_rate[4]), Paralysis_Base_Rate_Sabin2_DESC_TEXT, 0.0f, 1.0f, 0.00033f );
    initConfigTypeMap( "Paralysis_Base_Rate_Sabin3", &(polio_params->paralysis_base_rate[5]), Paralysis_Base_Rate_Sabin3_DESC_TEXT, 0.0f, 1.0f, 0.001f );

    initConfigTypeMap( "Incubation_Disease_Mu",      &(polio_params->Incubation_Disease_Mu),    Incubation_Disease_Mu_DESC_TEXT,    0.0f, 100.0f, 2.3893f );
    initConfigTypeMap( "Incubation_Disease_Sigma",   &(polio_params->Incubation_Disease_Sigma), Incubation_Disease_Sigma_DESC_TEXT, 0.0f, 100.0f, 0.4558f );
        
    initConfigTypeMap( "Boost_Log2_NAb_OPV1", &(polio_params->boostLog2NAb_OPV[0]), Boost_Log2_NAb_OPV1_DESC_TEXT, 0.0f, 10.0f, 1.0f );
    initConfigTypeMap( "Boost_Log2_NAb_OPV2", &(polio_params->boostLog2NAb_OPV[1]), Boost_Log2_NAb_OPV2_DESC_TEXT, 0.0f, 10.0f, 1.0f );
    initConfigTypeMap( "Boost_Log2_NAb_OPV3", &(polio_params->boostLog2NAb_OPV[2]), Boost_Log2_NAb_OPV3_DESC_TEXT, 0.0f, 10.0f, 1.0f );
    initConfigTypeMap( "Boost_Log2_NAb_IPV1", &(polio_params->boostLog2NAb_IPV[0]), Boost_Log2_NAb_IPV1_DESC_TEXT, 0.0f, 10.0f, 1.0f );
    initConfigTypeMap( "Boost_Log2_NAb_IPV2", &(polio_params->boostLog2NAb_IPV[1]), Boost_Log2_NAb_IPV2_DESC_TEXT, 0.0f, 10.0f, 1.0f );
    initConfigTypeMap( "Boost_Log2_NAb_IPV3", &(polio_params->boostLog2NAb_IPV[2]), Boost_Log2_NAb_IPV3_DESC_TEXT, 0.0f, 10.0f, 1.0f );

    initConfigTypeMap( "Max_Log2_NAb_OPV1", &(polio_params->maxLog2NAb_OPV[0]), Max_Log2_NAb_OPV1_DESC_TEXT, 0.0f, FLT_MAX, 2.0f );
    initConfigTypeMap( "Max_Log2_NAb_OPV2", &(polio_params->maxLog2NAb_OPV[1]), Max_Log2_NAb_OPV2_DESC_TEXT, 0.0f, FLT_MAX, 2.0f );
    initConfigTypeMap( "Max_Log2_NAb_OPV3", &(polio_params->maxLog2NAb_OPV[2]), Max_Log2_NAb_OPV3_DESC_TEXT, 0.0f, FLT_MAX, 2.0f );
    initConfigTypeMap( "Max_Log2_NAb_IPV1", &(polio_params->maxLog2NAb_IPV[0]), Max_Log2_NAb_IPV1_DESC_TEXT, 0.0f, FLT_MAX, 4.0f );
    initConfigTypeMap( "Max_Log2_NAb_IPV2", &(polio_params->maxLog2NAb_IPV[1]), Max_Log2_NAb_IPV2_DESC_TEXT, 0.0f, FLT_MAX, 4.0f );
    initConfigTypeMap( "Max_Log2_NAb_IPV3", &(polio_params->maxLog2NAb_IPV[2]), Max_Log2_NAb_IPV3_DESC_TEXT, 0.0f, FLT_MAX, 4.0f );

    initConfigTypeMap( "Boost_Log2_NAb_Stddev_OPV1", &(polio_params->boostLog2NAb_stddev_OPV[0]), Boost_Log2_NAb_Stddev_OPV1_DESC_TEXT, 0.0f, 10.0f, 1.5f );
    initConfigTypeMap( "Boost_Log2_NAb_Stddev_OPV2", &(polio_params->boostLog2NAb_stddev_OPV[1]), Boost_Log2_NAb_Stddev_OPV2_DESC_TEXT, 0.0f, 10.0f, 1.5f );
    initConfigTypeMap( "Boost_Log2_NAb_Stddev_OPV3", &(polio_params->boostLog2NAb_stddev_OPV[2]), Boost_Log2_NAb_Stddev_OPV3_DESC_TEXT, 0.0f, 10.0f, 1.5f );
    initConfigTypeMap( "Boost_Log2_NAb_Stddev_IPV1", &(polio_params->boostLog2NAb_stddev_IPV[0]), Boost_Log2_NAb_Stddev_IPV1_DESC_TEXT, 0.0f, 1.0f, 1.0f );
    initConfigTypeMap( "Boost_Log2_NAb_Stddev_IPV2", &(polio_params->boostLog2NAb_stddev_IPV[1]), Boost_Log2_NAb_Stddev_IPV2_DESC_TEXT, 0.0f, 1.0f, 1.0f );
    initConfigTypeMap( "Boost_Log2_NAb_Stddev_IPV3", &(polio_params->boostLog2NAb_stddev_IPV[2]), Boost_Log2_NAb_Stddev_IPV3_DESC_TEXT, 0.0f, 1.0f, 1.0f );

    initConfigTypeMap( "Prime_Log2_NAb_OPV1", &(polio_params->primeLog2NAb_OPV[0]), Prime_Log2_NAb_OPV1_DESC_TEXT, 0.0f, 10.0f, 2.0f );
    initConfigTypeMap( "Prime_Log2_NAb_OPV2", &(polio_params->primeLog2NAb_OPV[1]), Prime_Log2_NAb_OPV2_DESC_TEXT, 0.0f, 10.0f, 2.0f );
    initConfigTypeMap( "Prime_Log2_NAb_OPV3", &(polio_params->primeLog2NAb_OPV[2]), Prime_Log2_NAb_OPV3_DESC_TEXT, 0.0f, 10.0f, 2.0f );
    initConfigTypeMap( "Prime_Log2_NAb_IPV1", &(polio_params->primeLog2NAb_IPV[0]), Prime_Log2_NAb_IPV1_DESC_TEXT, 0.0f, 10.0f, 4.0f );
    initConfigTypeMap( "Prime_Log2_NAb_IPV2", &(polio_params->primeLog2NAb_IPV[1]), Prime_Log2_NAb_IPV2_DESC_TEXT, 0.0f, 10.0f, 4.0f );
    initConfigTypeMap( "Prime_Log2_NAb_IPV3", &(polio_params->primeLog2NAb_IPV[2]), Prime_Log2_NAb_IPV3_DESC_TEXT, 0.0f, 10.0f, 4.0f );

    initConfigTypeMap( "Prime_Log2_NAb_Stddev_OPV1", &(polio_params->primeLog2NAb_stddev_OPV[0]), Prime_Log2_NAb_Stddev_OPV1_DESC_TEXT, 0.0f, 10.0f, 1.0f );
    initConfigTypeMap( "Prime_Log2_NAb_Stddev_OPV2", &(polio_params->primeLog2NAb_stddev_OPV[1]), Prime_Log2_NAb_Stddev_OPV2_DESC_TEXT, 0.0f, 10.0f, 1.0f );
    initConfigTypeMap( "Prime_Log2_NAb_Stddev_OPV3", &(polio_params->primeLog2NAb_stddev_OPV[2]), Prime_Log2_NAb_Stddev_OPV3_DESC_TEXT, 0.0f, 10.0f, 1.0f );
    initConfigTypeMap( "Prime_Log2_NAb_Stddev_IPV1", &(polio_params->primeLog2NAb_stddev_IPV[0]), Prime_Log2_NAb_Stddev_IPV1_DESC_TEXT, 0.0f, 10.0f, 1.0f );
    initConfigTypeMap( "Prime_Log2_NAb_Stddev_IPV2", &(polio_params->primeLog2NAb_stddev_IPV[1]), Prime_Log2_NAb_Stddev_IPV2_DESC_TEXT, 0.0f, 10.0f, 1.0f );
    initConfigTypeMap( "Prime_Log2_NAb_Stddev_IPV3", &(polio_params->primeLog2NAb_stddev_IPV[2]), Prime_Log2_NAb_Stddev_IPV3_DESC_TEXT, 0.0f, 10.0f, 1.0f );

    initConfigTypeMap( "Shed_Fecal_MaxLog10_Peak_Titer",         &(polio_params->shedFecalMaxLog10PeakTiter),        Shed_Fecal_MaxLog10_Peak_Titer_DESC_TEXT,        0.0f, 10.0f, 5.0f );
    initConfigTypeMap( "Shed_Fecal_MaxLog10_Peak_Titer_Stddev",  &(polio_params->shedFecalMaxLog10PeakTiter_Stddev), Shed_Fecal_MaxLog10_Peak_Titer_Stddev_DESC_TEXT, 0.0f, 10.0f, 1.0f );
    initConfigTypeMap( "Shed_Fecal_Titer_Block_Log2NAb",         &(polio_params->shedFecalTiterBlockLog2NAb),        Shed_Fecal_Titer_Block_Log2NAb_DESC_TEXT,        1.0f, 100.0f, 15.0f );
    initConfigTypeMap( "Shed_Fecal_Titer_Profile_Mu",            &(polio_params->shedFecalTiterProfile_mu),          Shed_Fecal_Titer_Profile_Mu_DESC_TEXT,           0.0f, 10.0f, 3.0f );
    initConfigTypeMap( "Shed_Fecal_Titer_Profile_Sigma",         &(polio_params->shedFecalTiterProfile_sigma),       Shed_Fecal_Titer_Profile_Sigma_DESC_TEXT,        0.0f, 10.0f, 1.0f );
    initConfigTypeMap( "Shed_Fecal_MaxLn_Duration",              &(polio_params->shedFecalMaxLnDuration),            Shed_Fecal_MaxLn_Duration_DESC_TEXT,             0.0f, 100.0f, 4.0f );
    initConfigTypeMap( "Shed_Fecal_MaxLn_Duration_Stddev",       &(polio_params->shedFecalMaxLnDuration_Stddev),     Shed_Fecal_MaxLn_Duration_Stddev_DESC_TEXT,      0.0f, 10.0f, 1.0f );
    initConfigTypeMap( "Shed_Fecal_Duration_Block_Log2NAb",      &(polio_params->shedFecalDurationBlockLog2NAb),     Shed_Fecal_Duration_Block_Log2NAb_DESC_TEXT,     1.0f, 100.0f, 15.0f );
    initConfigTypeMap( "Shed_Oral_MaxLog10_Peak_Titer",          &(polio_params->shedOralMaxLog10PeakTiter),         Shed_Oral_MaxLog10_Peak_Titer_DESC_TEXT,         0.0f, 10.0f, 5.0f );
    initConfigTypeMap( "Shed_Oral_MaxLog10_Peak_Titer_Stddev",   &(polio_params->shedOralMaxLog10PeakTiter_Stddev),  Shed_Oral_MaxLog10_Peak_Titer_Stddev_DESC_TEXT,  0.0f, 10.0f, 1.0f );
    initConfigTypeMap( "Shed_Oral_Titer_Block_Log2NAb",          &(polio_params->shedOralTiterBlockLog2NAb),         Shed_Oral_Titer_Block_Log2NAb_DESC_TEXT,         1.0f, 100.0f, 15.0f );
    initConfigTypeMap( "Shed_Oral_Titer_Profile_Mu",             &(polio_params->shedOralTiterProfile_mu),           Shed_Oral_Titer_Profile_Mu_DESC_TEXT,            0.0f, 10.0f, 3.0f );
    initConfigTypeMap( "Shed_Oral_Titer_Profile_Sigma",          &(polio_params->shedOralTiterProfile_sigma),        Shed_Oral_Titer_Profile_Sigma_DESC_TEXT,         0.0f, 10.0f, 1.0f );
    initConfigTypeMap( "Shed_Oral_MaxLn_Duration",               &(polio_params->shedOralMaxLnDuration),             Shed_Oral_MaxLn_Duration_DESC_TEXT,              0.0f, 10.0f, 4.0f );
    initConfigTypeMap( "Shed_Oral_MaxLn_Duration_Stddev",        &(polio_params->shedOralMaxLnDuration_Stddev),      Shed_Oral_MaxLn_Duration_Stddev_DESC_TEXT,       0.0f, 10.0f, 1.0f );
    initConfigTypeMap( "Shed_Oral_Duration_Block_Log2NAb",       &(polio_params->shedOralDurationBlockLog2NAb),      Shed_Oral_Duration_Block_Log2NAb_DESC_TEXT,      1.0f, 100.0f, 15.0f );

    initConfigTypeMap( "Maternal_log2NAb_mean", &(polio_params->maternal_log2NAb_mean), Maternal_log2NAb_mean_DESC_TEXT,  0.0f, 18.0f, 6.0f );
    initConfigTypeMap( "Maternal_log2NAb_std",  &(polio_params->maternal_log2NAb_std),  Maternal_log2NAb_std_DESC_TEXT,   0.0f, 18.0f, 3.0f );
    initConfigTypeMap( "Maternal_Ab_Halflife",  &(polio_params->maternalAbHalfLife),    Maternal_Ab_Halflife_DESC_TEXT,   0.0f, 1000.0f, 22.0f );

    initConfigTypeMap( "Vaccine_Titer_tOPV1", &(polio_params->vaccine_titer_tOPV[0]), Vaccine_Titer_tOPV1_DESC_TEXT, 0.0f, 100000000.0f, 1000000.0f );
    initConfigTypeMap( "Vaccine_Titer_tOPV2", &(polio_params->vaccine_titer_tOPV[1]), Vaccine_Titer_tOPV2_DESC_TEXT, 0.0f, 100000000.0f, 100000.0f );
    initConfigTypeMap( "Vaccine_Titer_tOPV3", &(polio_params->vaccine_titer_tOPV[2]), Vaccine_Titer_tOPV3_DESC_TEXT, 0.0f, 100000000.0f, 630000.0f );
    initConfigTypeMap( "Vaccine_Titer_bOPV1", &(polio_params->vaccine_titer_bOPV[0]), Vaccine_Titer_bOPV1_DESC_TEXT, 0.0f, 100000000.0f, 1000000.0f );
    initConfigTypeMap( "Vaccine_Titer_bOPV3", &(polio_params->vaccine_titer_bOPV[1]), Vaccine_Titer_bOPV3_DESC_TEXT, 0.0f, 100000000.0f, 630000.0f );
    initConfigTypeMap( "Vaccine_Titer_mOPV1", &(polio_params->vaccine_titer_mOPV[0]), Vaccine_Titer_mOPV1_DESC_TEXT, 0.0f, 100000000.0f, 1000000.0f );
    initConfigTypeMap( "Vaccine_Titer_mOPV2", &(polio_params->vaccine_titer_mOPV[1]), Vaccine_Titer_mOPV2_DESC_TEXT, 0.0f, 100000000.0f, 100000.0f );
    initConfigTypeMap( "Vaccine_Titer_mOPV3", &(polio_params->vaccine_titer_mOPV[2]), Vaccine_Titer_mOPV3_DESC_TEXT, 0.0f, 100000000.0f, 1000000.0f );

    initConfigTypeMap( "Vaccine_Dantigen_IPV1", &(polio_params->vaccine_Dantigen_IPV[0]), Vaccine_Dantigen_IPV1_DESC_TEXT, 0.0f, 1000.0f, 40.0f );
    initConfigTypeMap( "Vaccine_Dantigen_IPV2", &(polio_params->vaccine_Dantigen_IPV[1]), Vaccine_Dantigen_IPV2_DESC_TEXT, 0.0f, 1000.0f, 8.0f );
    initConfigTypeMap( "Vaccine_Dantigen_IPV3", &(polio_params->vaccine_Dantigen_IPV[2]), Vaccine_Dantigen_IPV3_DESC_TEXT, 0.0f, 1000.0f, 32.0f );
         
    initConfigTypeMap( "Reversion_Steps_Sabin1", &(polio_params->reversionSteps_cVDPV[0]), Reversion_Steps_Sabin1_DESC_TEXT, 0, 8, 6 );
    initConfigTypeMap( "Reversion_Steps_Sabin2", &(polio_params->reversionSteps_cVDPV[1]), Reversion_Steps_Sabin2_DESC_TEXT, 0, 8, 2 );
    initConfigTypeMap( "Reversion_Steps_Sabin3", &(polio_params->reversionSteps_cVDPV[2]), Reversion_Steps_Sabin3_DESC_TEXT, 0, 8, 3 );

    initConfigTypeMap( "Excrement_Load", &(polio_params->excrement_load), Excrement_Load_DESC_TEXT, 0.0f, 1000.0f, 300.0f );

    initConfigTypeMap( "Substrain_Relative_Infectivity_String_VDPV1", &tmpSubstrainInfectivityString[0], Substrain_Relative_Infectivity_String_VDPV1_DESC_TEXT );
    initConfigTypeMap( "Substrain_Relative_Infectivity_String_VDPV2", &tmpSubstrainInfectivityString[1], Substrain_Relative_Infectivity_String_VDPV2_DESC_TEXT );
    initConfigTypeMap( "Substrain_Relative_Infectivity_String_VDPV3", &tmpSubstrainInfectivityString[2], Substrain_Relative_Infectivity_String_VDPV3_DESC_TEXT );

    initConfigTypeMap( "Sabin1_Site_Reversion_Rates", &tmpSiteRatesStrings[0], Sabin1_Site_Reversion_Rates_DESC_TEXT );
    initConfigTypeMap( "Sabin2_Site_Reversion_Rates", &tmpSiteRatesStrings[1], Sabin2_Site_Reversion_Rates_DESC_TEXT );
    initConfigTypeMap( "Sabin3_Site_Reversion_Rates", &tmpSiteRatesStrings[2], Sabin3_Site_Reversion_Rates_DESC_TEXT );

    initConfigTypeMap( "Vaccine_Genome_OPV1", &(polio_params->vaccine_genome_OPV1), Vaccine_Genome_OPV1_DESC_TEXT, 0, 1023, 1 );
    initConfigTypeMap( "Vaccine_Genome_OPV2", &(polio_params->vaccine_genome_OPV2), Vaccine_Genome_OPV2_DESC_TEXT, 0, 1023, 1 );
    initConfigTypeMap( "Vaccine_Genome_OPV3", &(polio_params->vaccine_genome_OPV3), Vaccine_Genome_OPV3_DESC_TEXT, 0, 1023, 1 );

    polio_params->vaccine_strains[0] = new StrainIdentity(PolioVirusTypes::VRPV1,0); // sets antigenID for VRPV strains and genome to zero (fully Sabin), note: literal definition of "vaccine-related poliovirus"
    polio_params->vaccine_strains[1] = new StrainIdentity(PolioVirusTypes::VRPV2,0); // sets antigenID for VRPV strains and genome to zero (fully Sabin), note: literal definition of "vaccine-related poliovirus"
    polio_params->vaccine_strains[2] = new StrainIdentity(PolioVirusTypes::VRPV3,0); // sets antigenID for VRPV strains and genome to zero (fully Sabin), note: literal definition of "vaccine-related poliovirus"

    polio_params->vaccine_strains[0]->SetGeneticID( polio_params->vaccine_genome_OPV1 ); // user-configuration of Sabin genome in OPV, default is zero
    polio_params->vaccine_strains[1]->SetGeneticID( polio_params->vaccine_genome_OPV2 ); // user-configuration of Sabin genome in OPV, default is zero
    polio_params->vaccine_strains[2]->SetGeneticID( polio_params->vaccine_genome_OPV3 ); // user-configuration of Sabin genome in OPV, default is zero
#endif // ENABLE_POLIO
}

void SimulationConfig::PolioCheckConfig( const Configuration* inputJson )
{
#ifdef ENABLE_POLIO
    std::vector<float> dummyvect(number_substrains, 1.0f);
    polio_params->substrainRelativeInfectivity.resize(3, dummyvect);

    // reversion rates for Sabin attenuating sites
    for(int i=0; i<3; ++i)
    {
        int nDataRead = 0;
        int ptrPosition;
        const char * readStr = tmpSiteRatesStrings[i].c_str();
        float data;
        size_t maximum_index = 1 + int(log(float(number_substrains)) / log(2.0f));
        vector<float> tmp_sabin_rates;
        tmp_sabin_rates.resize(maximum_index);

        while (sscanf_s(readStr, "%g%n", &data,  &ptrPosition) == 1)
        {
            if (nDataRead < maximum_index)
                tmp_sabin_rates[nDataRead] = data;
            readStr += ptrPosition;
            ++nDataRead;
        }

        if (nDataRead > maximum_index)
        {
            throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, "nDataRead(reversion rates for Sabin attenuating sites)", nDataRead, "maximum_index", int(maximum_index) );
        }

        tmp_sabin_rates.resize(nDataRead);

        switch (i)
        {
        case 0: polio_params->Sabin1_Site_Rates = tmp_sabin_rates; break;
        case 1: polio_params->Sabin2_Site_Rates = tmp_sabin_rates; break;
        case 2: polio_params->Sabin3_Site_Rates = tmp_sabin_rates; break;
        }
    }


    // vaccine infectivity by genotype
    for(int i=0; i<3; ++i)
    {
        int nDataRead = 0;
        int ptrPosition;
        const char * readStr = tmpSubstrainInfectivityString[i].c_str();
        float data;
        size_t maximum_index = polio_params->substrainRelativeInfectivity[i].size();
        while (sscanf_s(readStr, "%g%n", &data,  &ptrPosition) == 1)
        {
            if (nDataRead < maximum_index)
            {
                polio_params->substrainRelativeInfectivity[i][nDataRead] = data;
            }
            readStr += ptrPosition;
            ++nDataRead;
        }

        if (nDataRead != number_substrains)
        {
            throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, "nDataRead", nDataRead, "number_substrains", number_substrains );
        }
    }
    polio_params->decayRatePassiveImmunity = log(2.0f) / polio_params->maternalAbHalfLife; // (days) decayrate=log(2.0)/halflife; Ogra 1968, Warren 1964, Dexiang1956, Plotkin 1959

    polio_params->vaccine_strains[0]->SetGeneticID( polio_params->vaccine_genome_OPV1 ); // user-configuration of Sabin genome in OPV, default is zero
    polio_params->vaccine_strains[1]->SetGeneticID( polio_params->vaccine_genome_OPV2 ); // user-configuration of Sabin genome in OPV, default is zero
    polio_params->vaccine_strains[2]->SetGeneticID( polio_params->vaccine_genome_OPV3 ); // user-configuration of Sabin genome in OPV, default is zero
#endif // ENABLE_POLIO
}

void SimulationConfig::PolioAddSchema( json::QuickBuilder& retJson )
{
}

// ----------------------------------------------------------------------------
// --- TBHIVParameters
// ----------------------------------------------------------------------------

void SimulationConfig::TBHIVInitConfig( const Configuration* inputJson )
{
#ifdef ENABLE_TBHIV
    //TBHIV
    initConfigTypeMap( "TBHIV_Drug_Types", &tb_drug_names_for_this_sim, TB_Drug_Types_For_This_Sim_DESC_TEXT );   
    
    tb_drug_names_for_this_sim.value_source = "TBHIV_Drug_Params.*";

    if (JsonConfigurable::_dryrun)
    {
#if !defined(_DLLS_)
        // for the schema
        std::string tb_drug_names_array_str(tb_drug_placeholder_string);
        TBHIVDrugTypeParameters * tbdtp = TBHIVDrugTypeParameters::CreateTBHIVDrugTypeParameters(inputJson, tb_drug_names_array_str);
        tbdtp->Configure(inputJson);
        tbhiv_params->TBHIVDrugMap[tb_drug_names_array_str] = tbdtp;
#endif
    }
#endif // ENABLE_TBHIV
}

void SimulationConfig::TBHIVCheckConfig( const Configuration* inputJson )
{
#ifdef ENABLE_TBHIV

    if (tb_drug_names_for_this_sim.empty())
    {
        LOG_INFO("No custom drugs in config file!\n");
    }

    LOG_DEBUG_F("Reading in drugs \n");
    for (const auto& tb_drug_name : tb_drug_names_for_this_sim)
    {
        LOG_DEBUG_F("Reading in drug %s \n", tb_drug_name.c_str());
        auto * tbdtp = TBHIVDrugTypeParameters::CreateTBHIVDrugTypeParameters(inputJson, tb_drug_name);
        release_assert(tbdtp);
        tbhiv_params->TBHIVDrugMap[tb_drug_name] = tbdtp;
    }
#endif // ENABLE_TBHIV
}

void SimulationConfig::TBHIVAddSchema( json::QuickBuilder& retJson )
{
#ifdef ENABLE_TBHIV
    if (tbhiv_params->TBHIVDrugMap.count(tb_drug_placeholder_string) > 0)
    {
        retJson["TBHIV_Drug_Params"][tb_drug_placeholder_string] = tbhiv_params->TBHIVDrugMap[tb_drug_placeholder_string]->GetSchema().As<Object>();
    }
#endif // ENABLE_TBHIV 
}
}
