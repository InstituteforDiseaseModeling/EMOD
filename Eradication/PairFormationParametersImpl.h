/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "SimulationEnums.h"
#include "IPairFormationParameters.h"
#include "Configure.h"
#include "EnumSupport.h"
#include "InterpolatedValueMap.h"
#include "Sigmoid.h"

namespace Kernel {

    ENUM_DEFINE(FormationRateType,
        ENUM_VALUE_SPEC(CONSTANT                     , 0)
        ENUM_VALUE_SPEC(SIGMOID_VARIABLE_WIDTH_HEIGHT, 1)
        ENUM_VALUE_SPEC(INTERPOLATED_VALUES          , 2))


    class IDMAPI PairFormationParametersImpl : public IPairFormationParameters,
                                               public JsonConfigurable
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING();
        DECLARE_QUERY_INTERFACE();
    public:
        static IPairFormationParameters* CreateParameters( RelationshipType::Enum relType,
                                                           const Configuration* pConfig );

        virtual RelationshipType::Enum GetRelationshipType() const override;

        virtual int GetMaleAgeBinCount() const override;
        virtual float GetInitialMaleAge() const override;
        virtual float GetMaleAgeIncrement() const override;

        virtual int GetFemaleAgeBinCount() const override;
        virtual float GetInitialFemaleAge() const override;
        virtual float GetFemaleAgeIncrement() const override;

        virtual float GetRateRatio(Gender::Enum gender) const override;

        virtual const map<int, vector<float>>& GetAgeBins() const override;
        virtual const int BinIndexForAgeAndSex( float age_in_days, int sex ) const override;

        virtual const vector<vector<float>>& JointProbabilityTable() const override;
        virtual const vector<vector<float>>& CumulativeJointProbabilityTable() const override;

        virtual const map<int, vector<float>>& MarginalValues() const override;

        virtual const vector<vector<float>>& AperpPseudoInverse() const override;
        virtual const vector<vector<float>>& OrthogonalBasisForATranspose() const override;
        virtual const vector<float>& SingularValues() const override;

        virtual float FormationRate( const IdmDateTime& rCurrentTime, float dt ) const override;
        virtual float UpdatePeriod() const override;

        // ---------------------
        // --- ISupport Methods
        // ---------------------
        virtual bool Configure( const Configuration* inputJson ) override;

    protected:
        PairFormationParametersImpl();
        PairFormationParametersImpl( RelationshipType::Enum relType );

        virtual ~PairFormationParametersImpl();
        void Initialize(const string& filename);

        void InitializeAgeBins( Kernel::Gender::Enum mof, 
                                int bin_count, 
                                float initial_age, 
                                float increment );

        void InitializeCumulativeProbabilities();
        void InitializeMarginalValues();
        void CheckArraySizes();
        void Normalize( std::vector<float>& rArray );

        RelationshipType::Enum rel_type ;

        int male_age_bin_count;
        float initial_male_age;
        float male_age_increment;

        int female_age_bin_count;
        float initial_female_age;
        float female_age_increment;

#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        float rate_ratio[Gender::COUNT];

        map<int, vector<float>> age_bins;

        vector<vector<float>> joint_probabilities;
        vector<vector<float>> cumulative_joint_probabilities;

        map<int, vector<float>> marginal_values;

        float update_period;
        FormationRateType::Enum formation_rate_type;
        float                   formation_rate_constant;
        InterpolatedValueMap    formation_rate_value_map;
        Sigmoid                 formation_rate_sigmoid;

        DECLARE_SERIALIZABLE(PairFormationParametersImpl);
#pragma warning( pop )
    };
}
