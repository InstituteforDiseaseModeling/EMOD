/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"
#include "PairFormationParametersImpl.h"
#include "Exceptions.h"
#include "Common.h"
#include "NoCrtWarnings.h"

#include "Log.h"
static const char * _module = "PairFormationParametersImpl";

namespace Kernel {

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

    float PairFormationParametersImpl::BasePairFormationRate() const
    {
        return base_pair_formation_rate;
    }

    IPairFormationParameters* PairFormationParametersImpl::CreateParameters( RelationshipType::Enum relType,
                                                                             const Configuration* pConfig,
                                                                             float base_rate, 
                                                                             float rate_ratio_male, 
                                                                             float rate_ratio_female )
    {
        PairFormationParametersImpl* newParameters = _new_ PairFormationParametersImpl( relType, base_rate, rate_ratio_male, rate_ratio_female );

        newParameters->Configure(pConfig);

        return newParameters;
    }

    PairFormationParametersImpl::PairFormationParametersImpl( RelationshipType::Enum relType,
                                                              float base_rate, 
                                                              float rate_ratio_male, 
                                                              float rate_ratio_female )
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
        , base_pair_formation_rate(base_rate)
    {
        rate_ratio[Gender::MALE] = rate_ratio_male;
        rate_ratio[Gender::FEMALE] = rate_ratio_female;

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
        initConfigTypeMap("Number_Age_Bins_Male",           &male_age_bin_count,     Number_Age_Bins_Male_DESC_TEXT,           1,    1000,    1    );
        initConfigTypeMap("Number_Age_Bins_Female",         &female_age_bin_count,   Number_Age_Bins_Female_DESC_TEXT,         1,    1000,    1    );
        initConfigTypeMap("Age_of_First_Bin_Edge_Male",     &initial_male_age,       Age_of_First_Bin_Edge_Male_DESC_TEXT,     0,     100,    1    );
        initConfigTypeMap("Age_of_First_Bin_Edge_Female",   &initial_female_age,     Age_of_First_Bin_Edge_Female_DESC_TEXT,   0,     100,    1    );
        initConfigTypeMap("Years_Between_Bin_Edges_Male",   &male_age_increment,     Years_Between_Bin_Edges_Male_DESC_TEXT,   0.1f,  100.0f, 1.0f );
        initConfigTypeMap("Years_Between_Bin_Edges_Female", &female_age_increment,   Years_Between_Bin_Edges_Female_DESC_TEXT, 0.1f,  100.0f, 1.0f );
        initConfigTypeMap("Joint_Probabilities",            &joint_probabilities,    Joint_Probabilities_DESC_TEXT,            0.0, FLT_MAX, 0.0f ); 

        bool ret = false;
        
        try
        {
            bool prev_use_defaults = JsonConfigurable::_useDefaults ;
            JsonConfigurable::_useDefaults = false ;

            ret = JsonConfigurable::Configure( inputJson );

            JsonConfigurable::_useDefaults = prev_use_defaults ;
        }
        catch( DetailedException& e )
        {
            std::stringstream ss ;
            ss << e.GetMsg() << "\n" << "Was reading values for " << RelationshipType::pairs::lookup_key( rel_type ) << "." ;
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
        catch( json::Exception& e )
        {
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
}
