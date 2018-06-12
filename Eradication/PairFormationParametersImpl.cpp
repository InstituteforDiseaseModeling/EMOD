/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "PairFormationParametersImpl.h"
#include "Exceptions.h"
#include "Common.h"
#include "NoCrtWarnings.h"

#include "Log.h"

SETUP_LOGGING( "PairFormationParametersImpl" )

namespace Kernel 
{
    BEGIN_QUERY_INTERFACE_BODY(PairFormationParametersImpl)
    END_QUERY_INTERFACE_BODY(PairFormationParametersImpl)

    RelationshipType::Enum PairFormationParametersImpl::GetRelationshipType() const
    {
        return rel_type ;
    }

    int PairFormationParametersImpl::GetMaleAgeBinCount() const
    {
        return male_age_bin_count;
    }

    float PairFormationParametersImpl::GetInitialMaleAge() const
    {
        return initial_male_age;
    }

    float PairFormationParametersImpl::GetMaleAgeIncrement() const
    {
        return male_age_increment;
    }

    int PairFormationParametersImpl::GetFemaleAgeBinCount() const
    {
        return female_age_bin_count;
    }

    float PairFormationParametersImpl::GetInitialFemaleAge() const
    {
        return initial_female_age;
    }

    float PairFormationParametersImpl::GetFemaleAgeIncrement() const
    {
        return female_age_increment;
    }

    float PairFormationParametersImpl::GetRateRatio(Gender::Enum gender) const
    {
        return rate_ratio[gender];
    }

    const map<int, vector<float>>& PairFormationParametersImpl::GetAgeBins() const
    {
        return age_bins;
    }

    const int PairFormationParametersImpl::BinIndexForAgeAndSex( float age_in_days, int sex ) const
    {
        float age_in_years = age_in_days / DAYSPERYEAR;

        // TODO - is this the best solution?
        if (age_in_years < age_bins.at(sex)[0])
        {
            LOG_DEBUG_F( "%s: age %f is off the start of the bins (lower limit %f) \n", __FUNCTION__, age_in_years, age_bins.at(sex)[0]);
            return 0;
        }

        int index = 0;
        for (float limit : age_bins.at(sex))
        {
            if( age_in_years < limit)
            {
                return index ;
            }
            index++;
        }

        return age_bins.at(sex).size() - 1;
    }

    const vector<vector<float>>& PairFormationParametersImpl::JointProbabilityTable() const
    {
        return joint_probabilities;
    }

    const vector<vector<float>>& PairFormationParametersImpl::CumulativeJointProbabilityTable() const
    {
        return cumulative_joint_probabilities;
    }

    const map<int, vector<float>>& PairFormationParametersImpl::MarginalValues() const
    {
        return marginal_values;
    }

    const vector<vector<float>>& PairFormationParametersImpl::AperpPseudoInverse() const
    {
        throw NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "AperpPseudoInverse() is not supported." );
    }

    const vector<vector<float>>& PairFormationParametersImpl::OrthogonalBasisForATranspose() const
    {
        throw NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "OrthogonalBasisForATranspose() is not supported." );
    }

    const vector<float>& PairFormationParametersImpl::SingularValues() const
    {
        throw NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "SingularValues() is not supported." );
    }

    float PairFormationParametersImpl::FormationRate( const IdmDateTime& rCurrentTime, float dt ) const
    {
        float formation_rate = 0.0 ;
        switch( formation_rate_type )
        {
            case FormationRateType::CONSTANT:
                formation_rate = formation_rate_constant ;
                break;
            case FormationRateType::SIGMOID_VARIABLE_WIDTH_HEIGHT:
                formation_rate = formation_rate_sigmoid.variableWidthAndHeightSigmoid( rCurrentTime.Year() );
                break;
            case FormationRateType::INTERPOLATED_VALUES:
                formation_rate = formation_rate_value_map.getValueLinearInterpolation( rCurrentTime.Year(), 0.0f );
                break;
            default:
                throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__, "Formation_Rate_Type", formation_rate_type, FormationRateType::pairs::lookup_key( formation_rate_type ) );
        }
        return formation_rate;
    }

    float PairFormationParametersImpl::UpdatePeriod() const
    {
        return update_period;
    }

    IPairFormationParameters* PairFormationParametersImpl::CreateParameters( RelationshipType::Enum relType,
                                                                             const Configuration* pConfig )
    {
        PairFormationParametersImpl* newParameters = _new_ PairFormationParametersImpl( relType );

        newParameters->Configure(pConfig);

        return newParameters;
    }

    PairFormationParametersImpl::PairFormationParametersImpl()
        : rel_type(RelationshipType::TRANSITORY)
        , male_age_bin_count(0)
        , initial_male_age(0.0f)
        , male_age_increment(1.0f)
        , female_age_bin_count(0)
        , initial_female_age(0.0f)
        , female_age_increment(1.0f)
        , age_bins()
        , joint_probabilities()
        , cumulative_joint_probabilities()
        , marginal_values()
        , update_period(0.0f)
        , formation_rate_type(FormationRateType::CONSTANT)
        , formation_rate_constant(0.0f)
        , formation_rate_sigmoid()
        , formation_rate_value_map()
    {
    }

    PairFormationParametersImpl::PairFormationParametersImpl( RelationshipType::Enum relType )
        : rel_type( relType )
        , male_age_bin_count(0)
        , initial_male_age(0.0f)
        , male_age_increment(1.0f)
        , female_age_bin_count(0)
        , initial_female_age(0.0f)
        , female_age_increment(1.0f)
        , age_bins()
        , joint_probabilities()
        , cumulative_joint_probabilities()
        , marginal_values()
        , update_period(0.0f)
        , formation_rate_type(FormationRateType::CONSTANT)
        , formation_rate_constant(0.0f)
        , formation_rate_sigmoid()
        , formation_rate_value_map()
    {
        marginal_values[ Gender::MALE   ] = std::vector<float>() ;
        marginal_values[ Gender::FEMALE ] = std::vector<float>() ;
        // Real work done in Initialize()
    }

    PairFormationParametersImpl::~PairFormationParametersImpl()
    {
        // Nothing to do here, at the moment.
    }

    bool PairFormationParametersImpl::Configure( const Configuration* inputJson )
    {
        initConfig( "Formation_Rate_Type", 
                    formation_rate_type,
                    inputJson, 
                    MetadataDescriptor::Enum("Formation_Rate_Type", "TBD"/*Formation_Rate_Type_DESC_TEXT*/, MDD_ENUM_ARGS( FormationRateType ) ) );

        if( formation_rate_type == FormationRateType::CONSTANT )
        {
            initConfigTypeMap( "Formation_Rate_Constant", &formation_rate_constant, "TBD"/*Formation_Rate_DESC_TEXT*/, 0, 1, 0.001f );
        }
        else if( formation_rate_type == FormationRateType::SIGMOID_VARIABLE_WIDTH_HEIGHT )
        {
            initConfigTypeMap( "Formation_Rate_Sigmoid", &formation_rate_sigmoid, "TBD"/*Formation_Rate_Sigmoid_DESC_TEXT*/ );
        }
        else if( formation_rate_type == FormationRateType::INTERPOLATED_VALUES )
        {
            initConfigComplexType("Formation_Rate_Interpolated_Values", &formation_rate_value_map, "TBD"/*Formation_Rate_Interpolated_Values_DESC_TEXT*/ );
        }
        else
        {
            throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__, "Formation_Rate_Type", formation_rate_type, FormationRateType::pairs::lookup_key( formation_rate_type ) );
        }

        initConfigTypeMap( "Extra_Relational_Rate_Ratio_Male",   &rate_ratio[Gender::MALE  ], Extra_Relational_Rate_Ratio_Male_DESC_TEXT,   1.0, FLT_MAX, 1.0f );
        initConfigTypeMap( "Extra_Relational_Rate_Ratio_Female", &rate_ratio[Gender::FEMALE], Extra_Relational_Rate_Ratio_Female_DESC_TEXT, 1.0, FLT_MAX, 1.0f );

        initConfigTypeMap( "Update_Period",                  &update_period,            "TBD"/*Update_Period_DESC_TEXT*/,         0,    FLT_MAX,    0      );
        initConfigTypeMap( "Number_Age_Bins_Male",           &male_age_bin_count,       Number_Age_Bins_Male_DESC_TEXT,           1,       1000,    1      );
        initConfigTypeMap( "Number_Age_Bins_Female",         &female_age_bin_count,     Number_Age_Bins_Female_DESC_TEXT,         1,       1000,    1      );
        initConfigTypeMap( "Age_of_First_Bin_Edge_Male",     &initial_male_age,         Age_of_First_Bin_Edge_Male_DESC_TEXT,     0,        100,    1      );
        initConfigTypeMap( "Age_of_First_Bin_Edge_Female",   &initial_female_age,       Age_of_First_Bin_Edge_Female_DESC_TEXT,   0,        100,    1      );
        initConfigTypeMap( "Years_Between_Bin_Edges_Male",   &male_age_increment,       Years_Between_Bin_Edges_Male_DESC_TEXT,   0.1f,     100.0f, 1.0f   );
        initConfigTypeMap( "Years_Between_Bin_Edges_Female", &female_age_increment,     Years_Between_Bin_Edges_Female_DESC_TEXT, 0.1f,     100.0f, 1.0f   );
        initConfigTypeMap( "Joint_Probabilities",            &joint_probabilities,      Joint_Probabilities_DESC_TEXT,            0.0,  FLT_MAX,    0.0f   ); 

        bool ret = false;
        bool prev_use_defaults = JsonConfigurable::_useDefaults ;
        bool reset_track_missing = JsonConfigurable::_track_missing;
        JsonConfigurable::_track_missing = false;
        JsonConfigurable::_useDefaults = false ;
        
        try
        {

            ret = JsonConfigurable::Configure( inputJson );

            JsonConfigurable::_useDefaults = prev_use_defaults ;
            JsonConfigurable::_track_missing = reset_track_missing;
        }
        catch( DetailedException& e )
        {
            JsonConfigurable::_useDefaults = prev_use_defaults ;
            JsonConfigurable::_track_missing = reset_track_missing;

            std::stringstream ss ;
            ss << e.GetMsg() << "\n" << "Was reading values for " << RelationshipType::pairs::lookup_key( rel_type ) << "." ;
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
        catch( json::Exception& e )
        {
            JsonConfigurable::_useDefaults = prev_use_defaults ;
            JsonConfigurable::_track_missing = reset_track_missing;

            std::stringstream ss ;
            ss << e.what() << "\n" << "Was reading values for " << RelationshipType::pairs::lookup_key( rel_type ) << "." ;
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }

        if( ret )
        {
            CheckArraySizes();

            InitializeAgeBins( Gender::MALE,   male_age_bin_count,   initial_male_age,   male_age_increment   );
            InitializeAgeBins( Gender::FEMALE, female_age_bin_count, initial_female_age, female_age_increment );

            InitializeMarginalValues();
            InitializeCumulativeProbabilities();
        }
        return ret ;
    }

    void PairFormationParametersImpl::CheckArraySizes()
    {
        if( joint_probabilities.size() != male_age_bin_count )
        {
            std::stringstream ss ;
            ss << "The " << RelationshipType::pairs::lookup_key( rel_type ) << ":Joint_Probabilities matrix has "
               << joint_probabilities.size() << " rows when it should have one row for each male bin (Number_Age_Bins_Male=" << male_age_bin_count << ")" ;
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
        for( int male_bin_index = 0 ; male_bin_index < male_age_bin_count ; male_bin_index++ )
        {
            if( joint_probabilities[ male_bin_index ].size() != female_age_bin_count )
            {
                std::stringstream ss ;
                ss << "The " << RelationshipType::pairs::lookup_key( rel_type ) << ":Joint_Probabilities matrix row " << (male_bin_index+1) << " has "
                   << joint_probabilities[ male_bin_index ].size() << " columns when it should have one for each female bin (Number_Age_Bins_Female=" << female_age_bin_count << ")" ;
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }
        }
    }


    void PairFormationParametersImpl::InitializeAgeBins( Gender::Enum mof, 
                                                         int bin_count, 
                                                         float initial_age, 
                                                         float increment )
    {
        age_bins[mof].resize(bin_count);
        float limit = initial_age;
        for (int iBin = 0; iBin < bin_count; iBin++)
        {
            age_bins[mof][iBin] = limit;
            limit += increment;
        }
    }

    void PairFormationParametersImpl::InitializeCumulativeProbabilities()
    {
        release_assert( joint_probabilities.size()    == male_age_bin_count   );
        release_assert( joint_probabilities[0].size() == female_age_bin_count );

        cumulative_joint_probabilities.resize(male_age_bin_count);
        for (auto& vec : cumulative_joint_probabilities)
        {
            vec.resize(female_age_bin_count);
        }

        float cumulative = 0.0f;
        for( int row = 0; row < male_age_bin_count; row++)
        {
            cumulative = 0.0f;
            for( int col = 0; col < female_age_bin_count; col++)
            {
                cumulative += joint_probabilities[row][col];
                cumulative_joint_probabilities[row][col] = cumulative;
            }

            for( int col = 0; col < female_age_bin_count; col++)
            {
                if (cumulative > 0.0f)
                {
                    joint_probabilities[row][col] /= cumulative;
                    cumulative_joint_probabilities[row][col] /= cumulative;
                }
                else
                {
                    cumulative_joint_probabilities[row][col] = 1.0f;
                }
            }
        }
    }

    void PairFormationParametersImpl::InitializeMarginalValues()
    {
        for( int row = 0; row < male_age_bin_count; row++)
        {
            float cumulative = 0.0f;
            for( int col = 0; col < female_age_bin_count; col++)
            {
                cumulative += joint_probabilities[row][col];
            }
            marginal_values[ Gender::MALE ].push_back( cumulative ) ;
        }
        Normalize( marginal_values[ Gender::MALE ] );

        for( int col = 0; col < female_age_bin_count; col++)
        {
            float cumulative = 0.0f;
            for( int row = 0; row < male_age_bin_count; row++)
            {
                cumulative += joint_probabilities[row][col];
            }
            marginal_values[ Gender::FEMALE ].push_back( cumulative ) ;
        }
        Normalize( marginal_values[ Gender::FEMALE ] );
    }

    void PairFormationParametersImpl::Normalize( std::vector<float>& rArray )
    {
        float total = 0.0 ;
        for( int i = 0 ; i < rArray.size() ; i++ )
        {
            total += rArray[i] ;
        }
        if( total != 0.0 )
        {
            for( int i = 0 ; i < rArray.size() ; i++ )
            {
                rArray[i] = rArray[i] / total ;
            }
        }
    }

    REGISTER_SERIALIZABLE(PairFormationParametersImpl);

    void PairFormationParametersImpl::serialize(IArchive& ar, PairFormationParametersImpl* obj)
    {
        PairFormationParametersImpl& parameters = *obj;
        ar.labelElement("rel_type"                      ) & (uint32_t&)parameters.rel_type;
        ar.labelElement("male_age_bin_count"            ) & parameters.male_age_bin_count;
        ar.labelElement("initial_male_age"              ) & parameters.initial_male_age;
        ar.labelElement("male_age_increment"            ) & parameters.male_age_increment;
        ar.labelElement("female_age_bin_count"          ) & parameters.female_age_bin_count;
        ar.labelElement("initial_female_age"            ) & parameters.initial_female_age;
        ar.labelElement("female_age_increment"          ) & parameters.female_age_increment;
        ar.labelElement("rate_ratio"                    ); ar.serialize( parameters.rate_ratio, Gender::COUNT );
        ar.labelElement("age_bins"                      ) & parameters.age_bins;
        ar.labelElement("joint_probabilities"           ) & parameters.joint_probabilities;
        ar.labelElement("cumulative_joint_probabilities") & parameters.cumulative_joint_probabilities;
        ar.labelElement("marginal_values"               ) & parameters.marginal_values;
        ar.labelElement("update_period"                 ) & parameters.update_period;
        ar.labelElement("formation_rate_type"           ) & (uint32_t&)parameters.formation_rate_type;
        ar.labelElement("formation_rate_constant"       ) & parameters.formation_rate_constant;
        ar.labelElement("formation_rate_sigmoid"        ) & parameters.formation_rate_sigmoid;
        ar.labelElement("formation_rate_value_map"      ) & (std::map<float,float>&)parameters.formation_rate_value_map;
    }
}
