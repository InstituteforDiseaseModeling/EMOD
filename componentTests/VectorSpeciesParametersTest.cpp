
#include "stdafx.h"
#include "UnitTest++.h"
#include "componentTests.h"
#include "VectorSpeciesParameters.h"

using namespace Kernel;

SUITE( VectorSpeciesParametersTest )
{
    struct VspFixture
    {
        VspFixture()
        {
            Environment::Finalize();
            Environment::setLogger( new SimpleLogger( Logger::tLevel::WARNING ) );
            Environment::setSimulationConfig( nullptr );

            JsonConfigurable::ClearMissingParameters();
            JsonConfigurable::_useDefaults = false;
            JsonConfigurable::_track_missing = false;
        }

        ~VspFixture()
        {
            Environment::Finalize();
            JsonConfigurable::ClearMissingParameters();
            JsonConfigurable::_useDefaults = true;
            JsonConfigurable::_track_missing = true;
        }

        void TestHelper_ConfigureException( int lineNumber, const std::string& rFilename, const std::string& rExpMsg )
        {
            unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( rFilename.c_str() ) );

            VectorSpeciesCollection collection;
            try
            {
                collection.ConfigureFromJsonAndKey( p_config.get(), "Vector_Species_Params" );
                collection.CheckConfiguration();

                CHECK_LN( false, lineNumber ); // should not get here
            }
            catch( DetailedException& re )
            {
                std::string msg = re.GetMsg();
                if( msg.find( rExpMsg ) == string::npos )
                {
                    PrintDebug( rExpMsg );
                    PrintDebug( msg );
                    CHECK_LN( false, lineNumber );
                }
            }
        }
    };

    TEST_FIXTURE( VspFixture, TestTooManySpecies )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorSpeciesParametersTest/TestTooManySpecies.json",
                                       "7 (>6) species is not allowed in 'Vector_Species_Params'.\nPlease reduce the number of species you have to the maximum of 6" );
    }

    TEST_FIXTURE( VspFixture, TestDuplicateNames )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorSpeciesParametersTest/TestDuplicateNames.json",
                                       "Duplicate vector species name.\nThe names of the species in 'Vector_Species_Params' must be unique.\nThe following names are defined:\nspecies_1\nspecies_2\nspecies_1" );
    }

    TEST_FIXTURE( VspFixture, TestEmptyName )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorSpeciesParametersTest/TestEmptyName.json",
                                       "The 'Name' of the vector species must be defined and cannot be empty." );
    }

    TEST_FIXTURE( VspFixture, TestLarvalHabitatTypesDictionary )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorSpeciesParametersTest/TestLarvalHabitatTypesDictionary.json",
                                       "While trying to parse json data for param/key >>> Habitats <<< in otherwise valid json segment... \n{\n\t\"TEMPORARY_RAINFALL\" : 1.125e+10\n}\nCaught exception msg below: \nExpected ARRAY of OBJECTs" );
    }

    TEST_FIXTURE( VspFixture, TestLarvalHabitatTypesElementNotObject )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorSpeciesParametersTest/TestLarvalHabitatTypesElementNotObject.json",
                                       "While trying to parse json data for param/key >>> Habitats[0] <<< in otherwise valid json segment... \n[\n\t1.125e+10\n]\nCaught exception msg below: \nExpected ARRAY of OBJECTs" );
    }

    TEST_FIXTURE( VspFixture, TestLarvalHabitatTypesEmpty )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorSpeciesParametersTest/TestLarvalHabitatTypesEmpty.json",
                                       "'Habitats' for vector species = 'species_1' cannot be empty.\nPlease define at least one habitat." );
    }

    TEST_FIXTURE( VspFixture, TestLarvalHabitatTypesNoVectorHabitatType )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorSpeciesParametersTest/TestLarvalHabitatTypesNoVectorHabitatType.json",
                                       "'Habitat_Type' does not exist in 'Habitats[1]'.\nIt has the following JSON:\n{\n\t\"...Habitat_Type\" : \"CONSTANT\",\n\t\"Max_Larval_Capacity\" : 1.125e+10\n}" );
    }

    TEST_FIXTURE( VspFixture, TestLarvalHabitatTypesEmptyElement )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorSpeciesParametersTest/TestLarvalHabitatTypesEmptyElement.json",
                                       "Found zero elements in JSON for 'Habitats[1]' in <testdata/VectorSpeciesParametersTest/TestLarvalHabitatTypesEmptyElement.json>." );
    }

    TEST_FIXTURE( VspFixture, TestLarvalHabitatTypesAllHabitats )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorSpeciesParametersTest/TestLarvalHabitatTypesAllHabitats.json",
                                       "Invalid 'Habitat_Type' = 'ALL_HABITATS' in 'Habitats[1]' in <testdata/VectorSpeciesParametersTest/TestLarvalHabitatTypesAllHabitats.json>." );
    }

    TEST_FIXTURE( VspFixture, TestLarvalHabitatTypesDuplicates )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorSpeciesParametersTest/TestLarvalHabitatTypesDuplicates.json",
                                       "Duplicate 'Habitat_Type' = 'TEMPORARY_RAINFALL'.\nOnly one habitat type per species is allowed." );
    }
}
