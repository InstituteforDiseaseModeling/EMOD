/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"
#include "VectorSpeciesParameters.h"

#include "NodeVector.h"  // just for the NodeFlags :(
#include "Vector.h"
#include "Exceptions.h"
#include <algorithm> // for transform
#include <cctype> // for tolower

static const char * _module = "VectorSpeciesParameters";

std::map< std::string, const Kernel::VectorSpeciesParameters* > Kernel::VectorSpeciesParameters::_vspMap;

namespace Kernel
{
    VectorSpeciesParameters::VectorSpeciesParameters() :
        aquaticarrhenius1(DEFAULT_AQUATIC_ARRHENIUS1),
        aquaticarrhenius2(DEFAULT_AQUATIC_ARRHENIUS2),
        infectedarrhenius1(DEFAULT_INFECTED_ARRHENIUS1),
        infectedarrhenius2(DEFAULT_INFECTED_ARRHENIUS2),
        immatureduration(DEFAULT_IMMATURE_DURATION),
        daysbetweenfeeds(DEFAULT_DAYS_BETWEEN_FEEDS),
        anthropophily(DEFAULT_ANTHROPOPHILY),
        eggbatchsize(DEFAULT_EGGBATCH_SIZE),
        infectedeggbatchmod(DEFAULT_INFECTED_EGGBATCH_MODIFIER),
        infectiousmortalitymod(DEFAULT_INFECTIOUS_MORTALITY_MODIFIER),
        aquaticmortalityrate(DEFAULT_AQUATIC_MORTALITY_RATE),
        adultlifeexpectancy(DEFAULT_ADULT_LIFE_EXPECTANCY),
        transmissionmod(DEFAULT_TRANSMISSION_MODIFIER),
        acquiremod(DEFAULT_ACQUIRE_MODIFIER),
        infectioushfmortmod(DEFAULT_INFECTIOUS_HUMAN_FEEDING_MORTALITY_MODIFIER),
        indoor_feeding(1.0),
        feedingrate(0.0),
        adultmortality(0.0),
        immaturerate(0.0)
    {
    }

    VectorSpeciesParameters::~VectorSpeciesParameters()
    {
    }

    VectorSpeciesParameters* VectorSpeciesParameters::CreateVectorSpeciesParameters(const std::string& vector_species_name)
    {
        LOG_DEBUG( "CreateVectorSpeciesParameters\n" );
        VectorSpeciesParameters* params = _new_ VectorSpeciesParameters();
        params->Initialize(vector_species_name);
        if( !JsonConfigurable::_dryrun )
        {
            params->Configure( Configuration::CopyFromElement( (*EnvPtr->Config)["Vector_Species_Params"][vector_species_name.c_str()] ) );
        }

        _vspMap[ vector_species_name ] = params;
        LOG_DEBUG( "END CreateVectorSpeciesParameters\n" );
        return params;
    }

    bool
    VectorSpeciesParameters::Configure(
        const ::Configuration *config
    )
    {
        LOG_DEBUG( "Configure\n" );
        initVectorConfig( ("Habitat_Type"), habitat_type, config, MetadataDescriptor::VectorOfEnum("Habitat_Type", Habitat_Type_DESC_TEXT, MDD_ENUM_ARGS(VectorHabitatType)) );
        initConfigTypeMap( ( "Required_Habitat_Factor" ), &habitat_param, Required_Habitat_Factor_DESC_TEXT, 0.0f, 1E15f, 1.25E10f );
        initConfigTypeMap( ( "Aquatic_Arrhenius_1" ), &aquaticarrhenius1, Aquatic_Arrhenius_1_DESC_TEXT, 0.0f, 1E15f, 8.42E10f );
        initConfigTypeMap( ( "Aquatic_Arrhenius_2" ), &aquaticarrhenius2, Aquatic_Arrhenius_2_DESC_TEXT, 0.0f, 1E15f, 8328 );
        initConfigTypeMap( ( "Infected_Arrhenius_1" ), &infectedarrhenius1, Infected_Arrhenius_1_DESC_TEXT, 0.0f, 1E15f, 1.17E11f );
        initConfigTypeMap( ( "Infected_Arrhenius_2" ), &infectedarrhenius2, Infected_Arrhenius_2_DESC_TEXT, 0.0f, 1E15f, 8.34E3f );
        initConfigTypeMap( ( "Immature_Duration" ), &immatureduration, Immature_Duration_DESC_TEXT, 0.0f, 730.0f, 2.0f );
        initConfigTypeMap( ( "Days_Between_Feeds" ), &daysbetweenfeeds, Days_Between_Feeds_DESC_TEXT, 0.0f, 730.0f, 3.0f );
        initConfigTypeMap( ( "Anthropophily" ), &anthropophily, Anthropophily_DESC_TEXT, 0.0f, 1.0f, 1.0f );
        initConfigTypeMap( ( "Egg_Batch_Size" ), &eggbatchsize, Egg_Batch_Size_DESC_TEXT, 0.0f, 10000.0f, 100.0f );
        initConfigTypeMap( ( "Infected_Egg_Batch_Factor" ), &infectedeggbatchmod, Infected_Egg_Batch_Factor_DESC_TEXT, 0.0f, 10.0f, 0.8f );
        initConfigTypeMap( ( "Aquatic_Mortality_Rate" ), &aquaticmortalityrate, Aquatic_Mortality_Rate_DESC_TEXT, 0.0f, 1.0f, 0.1f );
        initConfigTypeMap( ( "Adult_Life_Expectancy" ), &adultlifeexpectancy, Adult_Life_Expectancy_DESC_TEXT, 0.0f, 730.0f, 10.0f );
        initConfigTypeMap( ( "Transmission_Rate" ), &transmissionmod, Transmission_Rate_DESC_TEXT, 0.0f, 1.0f, 0.5f );
        initConfigTypeMap( ( "Acquire_Modifier" ), &acquiremod, Acquire_Modifier_DESC_TEXT, 0.0f, 1.0f, 1.0f );
        initConfigTypeMap( ( "Infectious_Human_Feed_Mortality_Factor" ), &infectioushfmortmod, Infectious_Human_Feed_Mortality_Factor_DESC_TEXT, 0.0f, 1000.0f, 1.5f );
        initConfigTypeMap( ( "Indoor_Feeding_Fraction" ), &indoor_feeding, Indoor_Feeding_Fraction_DESC_TEXT, 0.0f, 1.0f, 1.0f );

        bool ret =  JsonConfigurable::Configure( config );
        feedingrate    = 1.0f / daysbetweenfeeds;
        adultmortality = 1.0f / adultlifeexpectancy;
        immaturerate   = 1.0f / immatureduration;

        // Some verification of valid "Habitat_Type"and "Required_Habitat_Factor"
        if( !JsonConfigurable::_dryrun )
        {
            if ( habitat_type.size() != habitat_param.size() )
            {
                throw GeneralConfigurationException(__FILE__, __LINE__, __FUNCTION__, "'Required_Habitat_Factor' and 'Habitat_Type' must have the same number of entries." );
            }

            if ( habitat_type.empty() && !JsonConfigurable::_dryrun )
            {
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, "There must be at least one entry in the 'Habitat_Type' array per species." );
            }

            if ( std::set<VectorHabitatType::Enum>( habitat_type.begin(), habitat_type.end() ).size() != habitat_type.size() )
            {
                throw GeneralConfigurationException(__FILE__, __LINE__, __FUNCTION__, "Duplicate entries of the same 'Habitat_Type' are not allowed for a single species." );
            }
        }

        // Would like to do this, but SimulationConfig not set yet.
        // Instead it's done in VectorPopulation::SetupLarvalHabitat
        //LOG_DEBUG_F( "Multiply by x_templarvalhabitat: %x\n", GET_CONFIGURABLE(SimulationConfig) );
        //if( GET_CONFIGURABLE(SimulationConfig) )
        //{
        //    habitat_param *= GET_CONFIGURABLE(SimulationConfig)->x_templarvalhabitat;
        //}

        return ret;
    }

    QueryResult
    VectorSpeciesParameters::QueryInterface(
        iid_t iid, void **ppvObject
    )
    {
        throw NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__ );
    }

    void VectorSpeciesParameters::Initialize(const std::string& vector_species_name)
    {
        LOG_DEBUG_F( "VectorSpeciesParameters::Initialize: species = %s\n", vector_species_name.c_str() );
        _species = vector_species_name;
    }
}

#if USE_BOOST_SERIALIZATION
namespace Kernel
{
    template< typename Archive >
    void serialize(Archive & ar, VectorSpeciesParameters& pars, const unsigned int file_version)
    {
        // Register derived types - N/A

        // Serialize fields
        ar  &  pars.habitat_type
            &  pars.habitat_param
            &  pars.aquaticarrhenius1
            &  pars.aquaticarrhenius2
            &  pars.infectedarrhenius1
            &  pars.infectedarrhenius2
            &  pars.immatureduration
            &  pars.daysbetweenfeeds
            &  pars.anthropophily
            &  pars.eggbatchsize
            &  pars.infectedeggbatchmod
            &  pars.infectiousmortalitymod
            &  pars.aquaticmortalityrate
            &  pars.adultlifeexpectancy
            &  pars.transmissionmod
            &  pars.acquiremod
            &  pars.infectioushfmortmod
            &  pars.indoor_feeding
            &  pars.feedingrate
            &  pars.adultmortality
            &  pars.immaturerate;

        // Serialize base class - N/A
    }

    template void serialize(boost::archive::binary_iarchive & ar, VectorSpeciesParameters&, const unsigned int file_version);
    template void serialize(boost::archive::binary_oarchive & ar, VectorSpeciesParameters&, const unsigned int file_version);
    template void serialize(boost::mpi::packed_skeleton_iarchive& ar, VectorSpeciesParameters&, const unsigned int file_version);
    template void serialize(boost::mpi::packed_skeleton_oarchive& ar, VectorSpeciesParameters&, const unsigned int file_version);
    template void serialize(boost::mpi::packed_oarchive& ar, VectorSpeciesParameters&, const unsigned int file_version);
    template void serialize(boost::mpi::packed_iarchive& ar, VectorSpeciesParameters&, const unsigned int file_version);
    template void serialize(boost::mpi::detail::content_oarchive& ar, VectorSpeciesParameters&, const unsigned int file_version);
    template void serialize(boost::mpi::detail::mpi_datatype_oarchive& ar, VectorSpeciesParameters&, const unsigned int file_version);
}
#endif
