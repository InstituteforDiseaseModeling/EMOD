/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include "SimulationEnums.h"
#include "IPairFormationParameters.h"
#include "Configure.h"

namespace Kernel {

    class IDMAPI PairFormationParametersImpl : public IPairFormationParameters,
                                               public JsonConfigurable
    {

    public:
        static IPairFormationParameters* CreateParameters( RelationshipType::Enum relType,
                                                           const Configuration* pConfig,
                                                           float base_rate, 
                                                           float rate_ratio_male,
                                                           float rate_ratio_female );

        virtual RelationshipType::Enum GetRelationshipType() const ;

        virtual int GetMaleAgeBinCount() const;
        virtual float GetInitialMaleAge() const;
        virtual float GetMaleAgeIncrement() const;

        virtual int GetFemaleAgeBinCount() const;
        virtual float GetInitialFemaleAge() const;
        virtual float GetFemaleAgeIncrement() const;

        virtual float GetRateRatio(Gender::Enum gender) const;

        virtual const map<int, vector<float>>& GetAgeBins() const;
        virtual const int BinIndexForAgeAndSex( float age_in_days, int sex ) const;

        virtual const vector<vector<float>>& JointProbabilityTable() const;
        virtual const vector<vector<float>>& CumulativeJointProbabilityTable() const;

        virtual const map<int, vector<float>>& MarginalValues() const;

        virtual const vector<vector<float>>& AperpPseudoInverse() const;
        virtual const vector<vector<float>>& OrthogonalBasisForATranspose() const;
        virtual const vector<float>& SingularValues() const;

        virtual float BasePairFormationRate() const;

        // ---------------------
        // --- ISupport Methods
        // ---------------------
        virtual bool Configure( const Configuration* inputJson );
        virtual Kernel::QueryResult QueryInterface(Kernel::iid_t iid, void **ppvObject) { return Kernel::e_NOINTERFACE; }
        virtual int32_t AddRef()  { return -1 ; }
        virtual int32_t Release() { return -1 ; }

    protected:
        PairFormationParametersImpl( RelationshipType::Enum relType, float base_rate, float rate_ratio_male, float rate_ratio_female );

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

        float base_pair_formation_rate;
#pragma warning( pop )
    };
}
