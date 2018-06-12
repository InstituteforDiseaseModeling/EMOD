/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "VectorSpeciesParameters.h"
#include "VectorParameters.h"
#include "Exceptions.h"

SETUP_LOGGING( "VectorSpeciesParameters" )

namespace Kernel
{
    void
    LarvalHabitatParams::ConfigureFromJsonAndKey(
        const Configuration* inputJson,
        const std::string& key
    )
    {
        LOG_DEBUG_F( "Configuring larval habitats from config.json\n" );
        // we have a map of enum keys to floats
        try {
            const auto& tvcs_jo = json_cast<const json::Object&>( (*inputJson)[key] );
            for( auto data = tvcs_jo.Begin();
                    data != tvcs_jo.End();
                    ++data )
            {
                auto tvcs = inputJson->As< json::Object >()[ key ];

                auto habitat_type_string = data->name;
                VectorHabitatType::Enum habitat_type = (VectorHabitatType::Enum) VectorHabitatType::pairs::lookup_value( habitat_type_string.c_str() );
                if( habitat_type == -1 )
                {
                    std::stringstream msg;
                    msg << habitat_type_string 
                        << " is not a valid VectorHabitatType.";
                    throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
                }
                Configuration* json_copy = Configuration::CopyFromElement( tvcs, inputJson->GetDataLocation() );
                habitat_map.insert( std::make_pair( habitat_type, json_copy ) );
            }
            LOG_DEBUG_F( "Found %d larval habitats\n", habitat_map.size() );
        }
        catch( const json::Exception & )
        {
            throw Kernel::JsonTypeConfigurationException( __FILE__, __LINE__, __FUNCTION__, key.c_str(), (*inputJson), "Expected OBJECT" );
        }
    }

    json::QuickBuilder
    LarvalHabitatParams::GetSchema()
    {
        json::QuickBuilder schema( GetSchemaBase() );
        auto tn = JsonConfigurable::_typename_label();
        auto ts = JsonConfigurable::_typeschema_label();
        schema[ tn ] = json::String( "idmType:LarvalHabitats" );
#if 0
        schema[ts] = json::Array();
        schema[ts][0] = json::Object();
        schema[ts][0]["Low"] = json::Object();
        schema[ts][0]["Low"][ "type" ] = json::String( "float" );
        schema[ts][0]["Low"][ "min" ] = json::Number( 0 );
        schema[ts][0]["Low"][ "max" ] = json::Number( 1000.0 );
        schema[ts][0]["Low"][ "description" ] = json::String( HIV_Age_Diagnostic_Low_DESC_TEXT );
        schema[ts][0]["High"] = json::Object();
        schema[ts][0]["High"][ "type" ] = json::String( "float" );
        schema[ts][0]["High"][ "min" ] = json::Number( 0 );
        schema[ts][0]["High"][ "max" ] = json::Number( 1000.0 );
        schema[ts][0]["High"][ "description" ] = json::String( HIV_Age_Diagnostic_High_DESC_TEXT );
        schema[ts][0]["Event"] = json::Object();
        schema[ts][0]["Event"][ "type" ] = json::String( "String" );
        schema[ts][0]["Event"][ "description" ] = json::String( HIV_Age_Diagnostic_Event_Name_DESC_TEXT );
#endif
        return schema;
    }

    VectorSpeciesParameters::VectorSpeciesParameters() :
        aquaticarrhenius1(DEFAULT_AQUATIC_ARRHENIUS1),
        aquaticarrhenius2(DEFAULT_AQUATIC_ARRHENIUS2),
        infectedarrhenius1(DEFAULT_INFECTED_ARRHENIUS1),
        infectedarrhenius2(DEFAULT_INFECTED_ARRHENIUS2),
        cyclearrhenius1(DEFAULT_CYCLE_ARRHENIUS1),
        cyclearrhenius2(DEFAULT_CYCLE_ARRHENIUS2),
        cyclearrheniusreductionfactor(1.0),
        immatureduration(DEFAULT_IMMATURE_DURATION),
        daysbetweenfeeds(DEFAULT_DAYS_BETWEEN_FEEDS),
        anthropophily(DEFAULT_ANTHROPOPHILY),
        eggbatchsize(DEFAULT_EGGBATCH_SIZE),
        infectedeggbatchmod(DEFAULT_INFECTED_EGGBATCH_MODIFIER),
        eggsurvivalrate(DEFAULT_EGG_SURVIVAL_RATE),
        infectiousmortalitymod(DEFAULT_INFECTIOUS_MORTALITY_MODIFIER),
        aquaticmortalityrate(DEFAULT_AQUATIC_MORTALITY_RATE),
        adultlifeexpectancy(DEFAULT_ADULT_LIFE_EXPECTANCY),
        transmissionmod(DEFAULT_TRANSMISSION_MODIFIER),
        acquiremod(DEFAULT_ACQUIRE_MODIFIER),
        infectioushfmortmod(DEFAULT_INFECTIOUS_HUMAN_FEEDING_MORTALITY_MODIFIER),
        indoor_feeding(1.0),
        nighttime_feeding(1.0),
        adultmortality(0.0),
        immaturerate(0.0)
    {
    }

    VectorSpeciesParameters::~VectorSpeciesParameters()
    {
    }

    VectorSpeciesParameters* VectorSpeciesParameters::CreateVectorSpeciesParameters( const Configuration* inputJson, 
                                                                                     const std::string& vector_species_name )
    {
        LOG_DEBUG( "CreateVectorSpeciesParameters\n" );

        // ------------------------------------------------------------------------------------------------------------
        // --- We have to pass this VectorParameter value in because we can't access it quite yet via SimulationConfig
        // --- We are dependent upon an outside variable so we are passing it in.  We make it static
        // --- so that we don't create a new instance of the variable for every instance of VSP.
        // ------------------------------------------------------------------------------------------------------------

        VectorSpeciesParameters* params = _new_ VectorSpeciesParameters();
        params->Initialize(vector_species_name);
        if( !JsonConfigurable::_dryrun )
        {
            auto tmp_config = Configuration::CopyFromElement( (*inputJson)["Vector_Species_Params"][vector_species_name.c_str()], inputJson->GetDataLocation() );
            params->Configure( tmp_config );
            delete tmp_config;
            tmp_config = nullptr;
        }
        LOG_DEBUG( "END CreateVectorSpeciesParameters\n" );
        return params;
    }

    bool
    VectorSpeciesParameters::Configure(
        const ::Configuration *config
    )
    {
        LOG_DEBUG( "Configure\n" );
        initConfigComplexType( "Larval_Habitat_Types", &habitat_params,  Larval_Habitat_Types_DESC_TEXT );
        initConfigTypeMap( ( "Aquatic_Arrhenius_1" ), &aquaticarrhenius1, Aquatic_Arrhenius_1_DESC_TEXT, 0.0f, 1E15f, 8.42E10f );
        initConfigTypeMap( ( "Aquatic_Arrhenius_2" ), &aquaticarrhenius2, Aquatic_Arrhenius_2_DESC_TEXT, 0.0f, 1E15f, 8328 );
        initConfigTypeMap( ( "Infected_Arrhenius_1" ), &infectedarrhenius1, Infected_Arrhenius_1_DESC_TEXT, 0.0f, 1E15f, 1.17E11f );
        initConfigTypeMap( ( "Infected_Arrhenius_2" ), &infectedarrhenius2, Infected_Arrhenius_2_DESC_TEXT, 0.0f, 1E15f, 8.34E3f );
        
        initConfigTypeMap( ("Cycle_Arrhenius_1"               ), &cyclearrhenius1,               Cycle_Arrhenius_1_DESC_TEXT,                0.0f, 1E15f, 4.09E10f, "Temperature_Dependent_Feeding_Cycle", "ARRHENIUS_DEPENDENCE" );
        initConfigTypeMap( ("Cycle_Arrhenius_2"               ), &cyclearrhenius2,               Cycle_Arrhenius_2_DESC_TEXT,                0.0f, 1E15f, 7.74E3f,  "Temperature_Dependent_Feeding_Cycle", "ARRHENIUS_DEPENDENCE" );
        initConfigTypeMap( ("Cycle_Arrhenius_Reduction_Factor"), &cyclearrheniusreductionfactor, Cycle_Arrhenius_Reduction_Factor_DESC_TEXT, 0.0f, 1.0f, 1.0f,      "Temperature_Dependent_Feeding_Cycle", "ARRHENIUS_DEPENDENCE" );

        initConfigTypeMap( ( "Immature_Duration" ), &immatureduration, Immature_Duration_DESC_TEXT, 0.0f, 730.0f, 2.0f );
        initConfigTypeMap( ( "Days_Between_Feeds" ), &daysbetweenfeeds, Days_Between_Feeds_DESC_TEXT, 1.0f, 730.0f, 3.0f, "Temperature_Dependent_Feeding_Cycle", "NO_TEMPERATURE_DEPENDENCE,BOUNDED_DEPENDENCE" );
        initConfigTypeMap( ( "Anthropophily" ), &anthropophily, Anthropophily_DESC_TEXT, 0.0f, 1.0f, 1.0f );
        initConfigTypeMap( ( "Egg_Batch_Size" ), &eggbatchsize, Egg_Batch_Size_DESC_TEXT, 0.0f, 10000.0f, 100.0f );
        initConfigTypeMap( ( "Infected_Egg_Batch_Factor" ), &infectedeggbatchmod, Infected_Egg_Batch_Factor_DESC_TEXT, 0.0f, 10.0f, 0.8f );
        // Below is not used yet. Not going to add param to all vector configs until it's used.
        //initConfigTypeMap( ( "Egg_Survival_Rate" ), &eggsurvivalrate, Egg_Survival_Rate_DESC_TEXT, 0.0f, 1.0f, 0.99f );
        initConfigTypeMap( ( "Aquatic_Mortality_Rate" ), &aquaticmortalityrate, Aquatic_Mortality_Rate_DESC_TEXT, 0.0f, 1.0f, 0.1f );
        initConfigTypeMap( ( "Adult_Life_Expectancy" ), &adultlifeexpectancy, Adult_Life_Expectancy_DESC_TEXT, 0.0f, 730.0f, 10.0f );
        initConfigTypeMap( ( "Transmission_Rate" ), &transmissionmod, Transmission_Rate_DESC_TEXT, 0.0f, 1.0f, 0.5f );
        initConfigTypeMap( ( "Acquire_Modifier" ), &acquiremod, Acquire_Modifier_DESC_TEXT, 0.0f, 1.0f, 1.0f );
        initConfigTypeMap( ( "Infectious_Human_Feed_Mortality_Factor" ), &infectioushfmortmod, Infectious_Human_Feed_Mortality_Factor_DESC_TEXT, 0.0f, 1000.0f, 1.5f );
        initConfigTypeMap( ( "Indoor_Feeding_Fraction" ), &indoor_feeding, Indoor_Feeding_Fraction_DESC_TEXT, 0.0f, 1.0f, 1.0f );
        initConfigTypeMap( ( "Nighttime_Feeding_Fraction" ), &nighttime_feeding, Nighttime_Feeding_Fraction_DESC_TEXT, 0.0f, 1.0f, 1.0f );

        bool ret = false;
        try {
            ret =  JsonConfigurable::Configure( config );
        }
        catch( DetailedException& exc )
        {
            LOG_WARN_F( "Please note that the specification of larval habitats in the config.json recently changed. Up to now you had to input matching Habitat_Type and Required_Habitat_Factor (per vector species). Now they are input together in a single Larval_Habitat_Types parameter.\n" );
            throw exc;
        }

        adultmortality = 1.0f / adultlifeexpectancy;
        immaturerate   = 1.0f / immatureduration;

        return ret;
    }

    QueryResult
    VectorSpeciesParameters::QueryInterface(
        iid_t iid, void **ppvObject
    )
    {
        throw NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "Should not get here" );
    }

    void VectorSpeciesParameters::Initialize(const std::string& vector_species_name)
    {
        LOG_DEBUG_F( "VectorSpeciesParameters::Initialize: species = %s\n", vector_species_name.c_str() );
        _species = vector_species_name;
    }

    void VectorSpeciesParameters::serialize( IArchive& ar, VectorSpeciesParameters*& parameters )
    {
        if( !ar.IsWriter() )
        {
            parameters = new VectorSpeciesParameters();
        }

        ar.startObject();
        //ar.labelElement("habitat_paramss") & parameters->habitat_params;
        ar.labelElement( "aquaticarrhenius1" ) & parameters->aquaticarrhenius1;
        ar.labelElement( "aquaticarrhenius2" ) & parameters->aquaticarrhenius2;
        ar.labelElement( "infectedarrhenius1" ) & parameters->infectedarrhenius1;
        ar.labelElement( "infectedarrhenius2" ) & parameters->infectedarrhenius2;
        ar.labelElement( "cyclearrhenius1" ) & parameters->cyclearrhenius1;
        ar.labelElement( "cyclearrhenius2" ) & parameters->cyclearrhenius2;
        ar.labelElement( "cyclearrheniusreductionfactor" ) & parameters->cyclearrheniusreductionfactor;
        ar.labelElement( "immatureduration" ) & parameters->immatureduration;
        ar.labelElement( "daysbetweenfeeds" ) & parameters->daysbetweenfeeds;
        ar.labelElement( "anthropophily" ) & parameters->anthropophily;
        ar.labelElement( "eggbatchsize" ) & parameters->eggbatchsize;
        ar.labelElement( "infectedeggbatchmod" ) & parameters->infectedeggbatchmod;
        ar.labelElement( "eggsurvivalrate" ) & parameters->eggsurvivalrate;
        ar.labelElement( "infectiousmortalitymod" ) & parameters->infectiousmortalitymod;
        ar.labelElement( "aquaticmortalityrate" ) & parameters->aquaticmortalityrate;
        ar.labelElement( "adultlifeexpectancy" ) & parameters->adultlifeexpectancy;
        ar.labelElement( "transmissionmod" ) & parameters->transmissionmod;
        ar.labelElement( "acquiremod" ) & parameters->acquiremod;
        ar.labelElement( "infectioushfmortmod" ) & parameters->infectioushfmortmod;
        ar.labelElement( "indoor_feeding" ) & parameters->indoor_feeding;
        ar.labelElement( "nighttime_feeding" ) & parameters->nighttime_feeding;
        ar.labelElement( "adultmortality" ) & parameters->adultmortality;
        ar.labelElement( "immaturerate" ) & parameters->immaturerate;
        ar.endObject();
    }
}



