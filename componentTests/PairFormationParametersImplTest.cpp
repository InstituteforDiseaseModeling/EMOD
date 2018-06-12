/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include <iostream>
#include <fstream>
#include <memory> // unique_ptr
#include "UnitTest++.h"
#include "PairFormationParametersImpl.h"
#include "common.h"

using namespace std; 
using namespace Kernel; 

float Joint_Probabilities[20][20] =
{
    { 0.045567f, 0.035956f, 0.019793f, 0.007568f, 0.002328f, 0.000551f, 0.000059f, 0.000018f, 0.000004f, 0.000001f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f },
    { 0.044738f, 0.036352f, 0.021958f, 0.010123f, 0.003782f, 0.001001f, 0.000184f, 0.000073f, 0.000022f, 0.000004f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f },
    { 0.041749f, 0.037081f, 0.028094f, 0.017609f, 0.008170f, 0.002469f, 0.000642f, 0.000267f, 0.000090f, 0.000021f, 0.000002f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f },
    { 0.034883f, 0.034782f, 0.032721f, 0.025031f, 0.013417f, 0.004969f, 0.001732f, 0.000707f, 0.000251f, 0.000079f, 0.000018f, 0.000004f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f },
    { 0.022317f, 0.025265f, 0.028532f, 0.025516f, 0.016489f, 0.008321f, 0.003780f, 0.001533f, 0.000572f, 0.000213f, 0.000062f, 0.000012f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f },
    { 0.009200f, 0.013361f, 0.019412f, 0.020918f, 0.016811f, 0.011010f, 0.005957f, 0.002611f, 0.001029f, 0.000369f, 0.000102f, 0.000020f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f },
    { 0.002873f, 0.006137f, 0.011340f, 0.014349f, 0.013829f, 0.010947f, 0.007060f, 0.003720f, 0.001602f, 0.000506f, 0.000122f, 0.000031f, 0.000004f, 0.000001f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f },
    { 0.001060f, 0.002528f, 0.005183f, 0.007796f, 0.009413f, 0.009220f, 0.007328f, 0.004767f, 0.002392f, 0.000882f, 0.000305f, 0.000121f, 0.000038f, 0.000008f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f },
    { 0.000282f, 0.000827f, 0.002074f, 0.004090f, 0.006305f, 0.007459f, 0.007090f, 0.005534f, 0.003443f, 0.001782f, 0.000873f, 0.000393f, 0.000140f, 0.000032f, 0.000002f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f },
    { 0.000148f, 0.000373f, 0.001016f, 0.002362f, 0.004162f, 0.005612f, 0.006176f, 0.005583f, 0.004120f, 0.002630f, 0.001522f, 0.000769f, 0.000319f, 0.000097f, 0.000021f, 0.000004f, 0.000000f, 0.000000f, 0.000000f, 0.000000f },
    { 0.000077f, 0.000166f, 0.000484f, 0.001276f, 0.002467f, 0.003661f, 0.004442f, 0.004384f, 0.003574f, 0.002614f, 0.001800f, 0.001118f, 0.000585f, 0.000245f, 0.000083f, 0.000020f, 0.000002f, 0.000000f, 0.000000f, 0.000000f },
    { 0.000096f, 0.000144f, 0.000302f, 0.000669f, 0.001252f, 0.001963f, 0.002577f, 0.002736f, 0.002445f, 0.002074f, 0.001750f, 0.001355f, 0.000895f, 0.000496f, 0.000222f, 0.000071f, 0.000017f, 0.000003f, 0.000000f, 0.000000f },
    { 0.000219f, 0.000202f, 0.000201f, 0.000291f, 0.000540f, 0.001013f, 0.001555f, 0.001784f, 0.001668f, 0.001522f, 0.001453f, 0.001358f, 0.001170f, 0.000855f, 0.000475f, 0.000195f, 0.000063f, 0.000013f, 0.000000f, 0.000000f },
    { 0.000320f, 0.000250f, 0.000144f, 0.000097f, 0.000205f, 0.000567f, 0.001037f, 0.001230f, 0.001110f, 0.000997f, 0.001033f, 0.001178f, 0.001296f, 0.001150f, 0.000755f, 0.000381f, 0.000150f, 0.000037f, 0.000004f, 0.000001f },
    { 0.000227f, 0.000170f, 0.000077f, 0.000015f, 0.000051f, 0.000253f, 0.000535f, 0.000648f, 0.000582f, 0.000560f, 0.000678f, 0.000905f, 0.001125f, 0.001143f, 0.000924f, 0.000598f, 0.000283f, 0.000086f, 0.000021f, 0.000010f },
    { 0.000062f, 0.000046f, 0.000021f, 0.000004f, 0.000014f, 0.000069f, 0.000146f, 0.000177f, 0.000176f, 0.000240f, 0.000395f, 0.000600f, 0.000804f, 0.000943f, 0.000951f, 0.000756f, 0.000428f, 0.000183f, 0.000088f, 0.000063f },
    { 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000015f, 0.000076f, 0.000194f, 0.000361f, 0.000551f, 0.000720f, 0.000808f, 0.000735f, 0.000536f, 0.000355f, 0.000253f, 0.000212f },
    { 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000004f, 0.000021f, 0.000070f, 0.000186f, 0.000351f, 0.000496f, 0.000586f, 0.000621f, 0.000607f, 0.000555f, 0.000489f, 0.000451f },
    { 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000015f, 0.000077f, 0.000184f, 0.000305f, 0.000424f, 0.000543f, 0.000649f, 0.000717f, 0.000746f, 0.000754f },
    { 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000005f, 0.000024f, 0.000074f, 0.000178f, 0.000331f, 0.000500f, 0.000666f, 0.000820f, 0.000946f, 0.001010f }
};

float Marginal_Values_Male[20] =
{ 0.106221022f, 0.112291608f, 0.129345664f, 0.141122147f, 0.125943781f, 0.095731405f, 0.068874377f, 0.048474471f, 0.03829826f, 0.033158396f, 0.025640441f, 0.018108241f, 0.013844015f, 0.011341512f, 0.008443928f, 0.005855951f, 0.004573834f, 0.004213891f, 0.004192048f, 0.004325008f };

float Marginal_Values_Female[20] =
{ 0.193569281f, 0.183903068f, 0.162735791f, 0.130789233f, 0.094245099f, 0.065611152f, 0.047770731f, 0.033973252f, 0.021937497f, 0.013857311f, 0.0098761f, 0.008083985f, 0.007157062f, 0.006333658f, 0.005301316f, 0.004201545f, 0.003229985f, 0.002629764f, 0.002418927f, 0.002375241f };

float Joint_Probabilities_M[20][20] =
{
    { 0.001014f, 0.000773f, 0.000377f, 0.000102f, 0.000016f, 0.000003f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f },
    { 0.001467f, 0.001149f, 0.000618f, 0.000222f, 0.000062f, 0.000012f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f },
    { 0.002110f, 0.001740f, 0.001116f, 0.000633f, 0.000393f, 0.000235f, 0.000098f, 0.000020f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f },
    { 0.002212f, 0.002054f, 0.001796f, 0.001620f, 0.001472f, 0.001076f, 0.000489f, 0.000098f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f },
    { 0.001684f, 0.001953f, 0.002461f, 0.003028f, 0.003301f, 0.002735f, 0.001514f, 0.000551f, 0.000155f, 0.000031f, 0.000001f, 0.000005f, 0.000009f, 0.000009f, 0.000005f, 0.000001f, 0.000000f, 0.000000f, 0.000000f, 0.000000f },
    { 0.001040f, 0.001617f, 0.002774f, 0.004255f, 0.005423f, 0.005206f, 0.003661f, 0.001972f, 0.000775f, 0.000155f, 0.000005f, 0.000024f, 0.000047f, 0.000047f, 0.000024f, 0.000005f, 0.000000f, 0.000000f, 0.000000f, 0.000000f },
    { 0.000569f, 0.001200f, 0.002613f, 0.004826f, 0.007094f, 0.007849f, 0.006726f, 0.004557f, 0.002234f, 0.000711f, 0.000183f, 0.000092f, 0.000101f, 0.000097f, 0.000050f, 0.000013f, 0.000002f, 0.000000f, 0.000000f, 0.000000f },
    { 0.000339f, 0.000830f, 0.002079f, 0.004396f, 0.007285f, 0.009393f, 0.009818f, 0.008106f, 0.004974f, 0.002315f, 0.000877f, 0.000269f, 0.000126f, 0.000108f, 0.000063f, 0.000025f, 0.000008f, 0.000002f, 0.000000f, 0.000000f },
    { 0.000293f, 0.000570f, 0.001407f, 0.003244f, 0.006069f, 0.009393f, 0.011954f, 0.011712f, 0.008762f, 0.005333f, 0.002674f, 0.001056f, 0.000371f, 0.000138f, 0.000062f, 0.000038f, 0.000016f, 0.000003f, 0.000000f, 0.000000f },
    { 0.000253f, 0.000419f, 0.000970f, 0.002268f, 0.004615f, 0.008232f, 0.012061f, 0.013764f, 0.012690f, 0.009880f, 0.006410f, 0.003391f, 0.001375f, 0.000352f, 0.000070f, 0.000039f, 0.000016f, 0.000003f, 0.000000f, 0.000000f },
    { 0.000159f, 0.000333f, 0.000802f, 0.001727f, 0.003427f, 0.006363f, 0.010138f, 0.013505f, 0.015301f, 0.014457f, 0.011223f, 0.007178f, 0.003560f, 0.001263f, 0.000358f, 0.000087f, 0.000009f, 0.000002f, 0.000000f, 0.000000f },
    { 0.000075f, 0.000230f, 0.000598f, 0.001230f, 0.002345f, 0.004316f, 0.007261f, 0.011034f, 0.014582f, 0.015942f, 0.014559f, 0.011288f, 0.007231f, 0.003733f, 0.001443f, 0.000304f, 0.000009f, 0.000002f, 0.000000f, 0.000000f },
    { 0.000027f, 0.000102f, 0.000300f, 0.000684f, 0.001367f, 0.002505f, 0.004362f, 0.007309f, 0.010903f, 0.013882f, 0.015276f, 0.014404f, 0.011428f, 0.007354f, 0.003388f, 0.000935f, 0.000171f, 0.000046f, 0.000007f, 0.000002f },
    { 0.000005f, 0.000020f, 0.000090f, 0.000286f, 0.000658f, 0.001226f, 0.002235f, 0.004148f, 0.007018f, 0.010488f, 0.013644f, 0.015020f, 0.013925f, 0.010445f, 0.005801f, 0.002345f, 0.000792f, 0.000218f, 0.000037f, 0.000010f },
    { 0.000000f, 0.000000f, 0.000020f, 0.000102f, 0.000273f, 0.000549f, 0.001102f, 0.002260f, 0.004239f, 0.007138f, 0.010432f, 0.012966f, 0.013724f, 0.011826f, 0.007974f, 0.004455f, 0.002233f, 0.000958f, 0.000324f, 0.000092f },
    { 0.000000f, 0.000000f, 0.000013f, 0.000063f, 0.000163f, 0.000314f, 0.000611f, 0.001249f, 0.002433f, 0.004383f, 0.006909f, 0.009408f, 0.011056f, 0.010813f, 0.008887f, 0.006737f, 0.004916f, 0.003067f, 0.001322f, 0.000382f },
    { 0.000000f, 0.000000f, 0.000018f, 0.000088f, 0.000203f, 0.000312f, 0.000455f, 0.000764f, 0.001380f, 0.002449f, 0.003925f, 0.005568f, 0.007040f, 0.007909f, 0.008149f, 0.008144f, 0.007639f, 0.005738f, 0.002924f, 0.001208f },
    { 0.000000f, 0.000000f, 0.000018f, 0.000088f, 0.000198f, 0.000288f, 0.000365f, 0.000502f, 0.000770f, 0.001238f, 0.001911f, 0.002730f, 0.003684f, 0.004800f, 0.006002f, 0.007105f, 0.007580f, 0.006558f, 0.004455f, 0.003100f },
    { 0.000000f, 0.000000f, 0.000009f, 0.000044f, 0.000105f, 0.000175f, 0.000244f, 0.000316f, 0.000417f, 0.000601f, 0.000889f, 0.001276f, 0.001787f, 0.002479f, 0.003318f, 0.004175f, 0.004909f, 0.005365f, 0.005540f, 0.005586f },
    { 0.000000f, 0.000000f, 0.000002f, 0.000012f, 0.000038f, 0.000096f, 0.000173f, 0.000228f, 0.000270f, 0.000355f, 0.000509f, 0.000729f, 0.001011f, 0.001347f, 0.001743f, 0.002217f, 0.002961f, 0.004339f, 0.006055f, 0.007062f }
};

float Marginal_Values_Male_M[20] =
{ 0.002282033f, 0.003525417f, 0.006336762f, 0.010802956f, 0.017420354f, 0.026994907f, 0.038866474f, 0.050946769f, 0.063013083f, 0.076708279f, 0.089775292f, 0.096057126f, 0.094329372f, 0.088296215f, 0.080562269f, 0.072631579f, 0.063830021f, 0.051325277f, 0.037186657f, 0.029109158f };

float Marginal_Values_Female_M[20] =
{0.011232398f, 0.012973135f, 0.018057525f, 0.028880455f, 0.044449216f, 0.06019974f, 0.073171877f, 0.081988415f, 0.086790173f, 0.089241985f, 0.089310896f, 0.085293119f, 0.076375712f, 0.06263857f, 0.047275542f, 0.036577449f, 0.031220413f, 0.026266853f, 0.020637172f, 0.017419355f };


SUITE(PairFormationParametersImplTest)
{
    TEST(TestReadingDataTransitory)
    {
        unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/PairFormationParametersTest/TransitoryParameters.json" ) );

        unique_ptr<IPairFormationParameters> from_data( PairFormationParametersImpl::CreateParameters( RelationshipType::TRANSITORY, p_config.get() ) );

        IdmDateTime current_time;
        CHECK_EQUAL( 0.0013699f, from_data->FormationRate( current_time, 1.0 ) );

        CHECK_EQUAL(    20, from_data->GetMaleAgeBinCount()    );
        CHECK_EQUAL(    20, from_data->GetFemaleAgeBinCount()  );
        CHECK_EQUAL( 17.5f, from_data->GetInitialMaleAge()     );
        CHECK_EQUAL( 17.5f, from_data->GetInitialFemaleAge()   );
        CHECK_EQUAL(  2.5f, from_data->GetMaleAgeIncrement()   );
        CHECK_EQUAL(  2.5f, from_data->GetFemaleAgeIncrement() );

        for( int row = 0 ; row < 20 ; row++ )
        {
            float cum = 0.0 ;
            for( int col = 0 ; col < 20 ; col++ )
            {
                cum += Joint_Probabilities[row][col] ;
            }
            for( int col = 0 ; col < 20 ; col++ )
            {
                Joint_Probabilities[row][col] /= cum ;
            }
        }

        for( int i = 0 ; i < 20 ; i++ )
        {
            CHECK_ARRAY_EQUAL( Joint_Probabilities[i], from_data->JointProbabilityTable()[i], 20 );
        }

        CHECK_ARRAY_CLOSE( Marginal_Values_Male,   from_data->MarginalValues().at(Gender::MALE  ), 20, 0.000001 );
        CHECK_ARRAY_CLOSE( Marginal_Values_Female, from_data->MarginalValues().at(Gender::FEMALE), 20, 0.000001 );
    }

    TEST(TestReadingDataMarital)
    {
        unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/PairFormationParametersTest/MaritalParameters.json" ) );

        unique_ptr<IPairFormationParameters> from_data( PairFormationParametersImpl::CreateParameters( RelationshipType::MARITAL, p_config.get() ) );

        IdmDateTime current_time;
        CHECK_EQUAL( 0.0000914427f, from_data->FormationRate( current_time, 1.0 ) );

        CHECK_EQUAL(    20, from_data->GetMaleAgeBinCount()    );
        CHECK_EQUAL(    20, from_data->GetFemaleAgeBinCount()  );
        CHECK_EQUAL( 17.5f, from_data->GetInitialMaleAge()     );
        CHECK_EQUAL( 17.5f, from_data->GetInitialFemaleAge()   );
        CHECK_EQUAL(  2.5f, from_data->GetMaleAgeIncrement()   );
        CHECK_EQUAL(  2.5f, from_data->GetFemaleAgeIncrement() );

        for( int row = 0 ; row < 20 ; row++ )
        {
            float cum = 0.0 ;
            for( int col = 0 ; col < 20 ; col++ )
            {
                cum += Joint_Probabilities_M[row][col] ;
            }
            for( int col = 0 ; col < 20 ; col++ )
            {
                Joint_Probabilities_M[row][col] /= cum ;
            }
        }

        for( int i = 0 ; i < 20 ; i++ )
        {
            CHECK_ARRAY_EQUAL( Joint_Probabilities_M[i], from_data->JointProbabilityTable()[i], 20 );
        }

        CHECK_ARRAY_CLOSE( Marginal_Values_Male_M,   from_data->MarginalValues().at(Gender::MALE  ), 20, 0.00001 );
        CHECK_ARRAY_CLOSE( Marginal_Values_Female_M, from_data->MarginalValues().at(Gender::FEMALE), 20, 0.00001 );
    }

    TEST(TestSigmoid)
    {
        unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/PairFormationParametersTest/TestSigmoid.json" ) );

        unique_ptr<IPairFormationParameters> from_data( PairFormationParametersImpl::CreateParameters( RelationshipType::MARITAL, p_config.get() ) );

        IdmDateTime current_time;
        current_time.time = 1990.0 * DAYSPERYEAR;
        CHECK_CLOSE( 0.0692, from_data->FormationRate( current_time, 1.0 ), 0.0001 );
        current_time.time = 1995.0 * DAYSPERYEAR;
        CHECK_CLOSE( 0.4505, from_data->FormationRate( current_time, 1.0 ), 0.0001 );
        current_time.time = 2000.0 * DAYSPERYEAR;
        CHECK_CLOSE( 0.8318, from_data->FormationRate( current_time, 1.0 ), 0.0001 );
    }

    TEST(TestInterpolatedValueMap)
    {
        unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/PairFormationParametersTest/TestInterpolatedValueMap.json" ) );

        unique_ptr<IPairFormationParameters> from_data( PairFormationParametersImpl::CreateParameters( RelationshipType::MARITAL, p_config.get() ) );

        IdmDateTime current_time;
        current_time.time = 1990.0 * DAYSPERYEAR;
        CHECK_CLOSE( 0.5, from_data->FormationRate( current_time, 1.0 ), 0.0001 );
        current_time.time = 2000.0 * DAYSPERYEAR;
        CHECK_CLOSE( 0.8, from_data->FormationRate( current_time, 1.0 ), 0.0001 );
        current_time.time = 2010 * DAYSPERYEAR;
        CHECK_CLOSE( 0.6, from_data->FormationRate( current_time, 1.0 ), 0.0001 );
    }

    TEST(TestBins)
    {
        unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/PairFormationParametersTest/TransitoryParameters.json" ) );

        unique_ptr<IPairFormationParameters> from_data( PairFormationParametersImpl::CreateParameters( RelationshipType::TRANSITORY, p_config.get() ) );

        CHECK_EQUAL(    20, from_data->GetMaleAgeBinCount()    );
        CHECK_EQUAL(    20, from_data->GetFemaleAgeBinCount()  );
        CHECK_EQUAL( 17.5f, from_data->GetInitialMaleAge()     );
        CHECK_EQUAL( 17.5f, from_data->GetInitialFemaleAge()   );
        CHECK_EQUAL(  2.5f, from_data->GetMaleAgeIncrement()   );
        CHECK_EQUAL(  2.5f, from_data->GetFemaleAgeIncrement() );

        const map<int, vector<float>>& r_age_bins = from_data->GetAgeBins();

        CHECK_EQUAL( 2, r_age_bins.size() );
        CHECK_EQUAL( 20, r_age_bins.at(Gender::MALE  ).size() );
        CHECK_EQUAL( 20, r_age_bins.at(Gender::FEMALE).size() );

        CHECK_ARRAY_EQUAL( r_age_bins.at(Gender::MALE), r_age_bins.at(Gender::FEMALE), (int)r_age_bins.at(Gender::FEMALE).size() );

        CHECK_EQUAL( 17.5f, r_age_bins.at(Gender::FEMALE)[0] );
        CHECK_EQUAL( 17.5f + 19.0f*2.5f, r_age_bins.at(Gender::FEMALE)[ r_age_bins.at(Gender::FEMALE).size() - 1 ] );

        CHECK_EQUAL(  0, from_data->BinIndexForAgeAndSex( float( 0*365), Gender::MALE ) );
        CHECK_EQUAL(  0, from_data->BinIndexForAgeAndSex( float( 5*365), Gender::MALE ) );
        CHECK_EQUAL(  0, from_data->BinIndexForAgeAndSex( float(10*365), Gender::MALE ) );
        CHECK_EQUAL(  0, from_data->BinIndexForAgeAndSex( float(15*365), Gender::MALE ) );
        CHECK_EQUAL(  2, from_data->BinIndexForAgeAndSex( float(20*365), Gender::MALE ) );
        CHECK_EQUAL(  4, from_data->BinIndexForAgeAndSex( float(25*365), Gender::MALE ) );
        CHECK_EQUAL(  6, from_data->BinIndexForAgeAndSex( float(30*365), Gender::MALE ) );
        CHECK_EQUAL(  8, from_data->BinIndexForAgeAndSex( float(35*365), Gender::MALE ) );
        CHECK_EQUAL( 10, from_data->BinIndexForAgeAndSex( float(40*365), Gender::MALE ) );
        CHECK_EQUAL( 12, from_data->BinIndexForAgeAndSex( float(45*365), Gender::MALE ) );
        CHECK_EQUAL( 14, from_data->BinIndexForAgeAndSex( float(50*365), Gender::MALE ) );
        CHECK_EQUAL( 16, from_data->BinIndexForAgeAndSex( float(55*365), Gender::MALE ) );
        CHECK_EQUAL( 18, from_data->BinIndexForAgeAndSex( float(60*365), Gender::MALE ) );
        CHECK_EQUAL( 19, from_data->BinIndexForAgeAndSex( float(65*365), Gender::MALE ) );
        CHECK_EQUAL( 19, from_data->BinIndexForAgeAndSex( float(70*365), Gender::MALE ) );
    }

    void TestHelper_Exception( int lineNumber, const std::string& rFilename, const std::string& rExpMsg )
    {
        try
        {
            unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( rFilename ) );

            unique_ptr<IPairFormationParameters> from_data( PairFormationParametersImpl::CreateParameters( RelationshipType::TRANSITORY, p_config.get() ) );

            CHECK_LN( false, lineNumber ); // should not get here
        }
        catch( DetailedException& re )
        {
            std::string msg = re.GetMsg();
            if( msg.find( rExpMsg ) == string::npos )
            {
                PrintDebug( msg );
                CHECK_LN( false, lineNumber );
            }
        }
    }

    TEST(TestBadMaleBinCount)
    {
        TestHelper_Exception( __LINE__, "testdata/PairFormationParametersTest/TestBadMaleBinCount.json",
            "Configuration variable Number_Age_Bins_Male with value 0 out of range: less than 1.\nWas reading values for TRANSITORY." ) ;
    }

    TEST(TestBadFemaleBinCount)
    {
        TestHelper_Exception( __LINE__, "testdata/PairFormationParametersTest/TestBadFemaleBinCount.json",
            "Configuration variable Number_Age_Bins_Female with value 9999 out of range: greater than 1000.\nWas reading values for TRANSITORY." ) ;
    }

    TEST(TestMissingFemaleBinCount)
    {
        TestHelper_Exception( __LINE__, "testdata/PairFormationParametersTest/TestMissingFemaleBinCount.json",
            "Parameter 'Number_Age_Bins_Female of PairFormationParametersImpl' not found in input file 'testdata/PairFormationParametersTest/TestMissingFemaleBinCount.json'.\n\nWas reading values for TRANSITORY." ) ;
    }

    TEST(TestBadMarginalProbabilityValue)
    {
        TestHelper_Exception( __LINE__, "testdata/PairFormationParametersTest/TestBadMarginalProbabilityValue.json",
            "Configuration variable Joint_Probabilities with value -99.0078 out of range: less than 0.\nWas reading values for TRANSITORY." ) ;
    }

    TEST(TestMissingMarginalProbabilityValuesInThirdRow)
    {
        TestHelper_Exception( __LINE__, "testdata/PairFormationParametersTest/TestMissingMarginalProbabilityValuesInThirdRow.json",
             "The TRANSITORY:Joint_Probabilities matrix row 3 has 6 columns when it should have one for each female bin (Number_Age_Bins_Female=20)" );
    }

    TEST(TestMissingMarginalProbabilityRows)
    {
        TestHelper_Exception( __LINE__, "testdata/PairFormationParametersTest/TestMissingMarginalProbabilityRows.json",
             "The TRANSITORY:Joint_Probabilities matrix has 10 rows when it should have one row for each male bin (Number_Age_Bins_Male=20)" );
   }
}