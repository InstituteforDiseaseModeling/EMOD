/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "TransmissionGroupsBase.h"
#include "IContagionPopulation.h"
#include "Types.h"

using namespace std;

namespace Kernel
{
    class RANDOMBASE;
    typedef map<uint32_t, float> SubstrainMap_t;
    typedef vector<SubstrainMap_t> GroupSubstrainMap_t;
    typedef vector<GroupSubstrainMap_t> AntigenGroupSubstrainMap_t;

    class StrainAwareTransmissionGroups : protected TransmissionGroupsBase
    {
    public:
        StrainAwareTransmissionGroups( RANDOMBASE* prng );
        virtual ~StrainAwareTransmissionGroups() {}

    protected:

        RANDOMBASE* pRNG;
        PropertyToValuesMap_t propertyToValuesMap;
        ScalingMatrix_t scalingMatrix;
        float contagionDecayRate;
        float populationSize;
        vector<float> populationSizeByGroup;

        // Function names are lower case to differentiate from externally visible methods.
        void addPropertyValueListToPropertyToValueMap( const string& property, const PropertyValueList_t& values );
        void buildScalingMatrix( void );
        void allocateAccumulators( NaturalNumber numberOfStrains, NaturalNumber numberOfSubstrains );

        inline int getGroupCount() { return scalingMatrix.size(); }

        int antigenCount;
        int substrainCount;
        bool normalizeByTotalPopulation;
        vector<bool> antigenWasShed;                // Contagion of this antigenId was shed this cycle.
        vector<set<unsigned int>> substrainWasShed; // Contagion of this antigenId/substrainId was shed this cycle.

        vector<ContagionAccumulator_t> newlyDepositedContagionByAntigenAndGroup;       // All antigen (substrains summed) shed this timestep
        vector<ContagionAccumulator_t> currentContagionByAntigenAndSourceGroup;        // All antigen (substrains summed) current contagion (by contagion source)
        vector<ContagionAccumulator_t> currentContagionByAntigenAndDestinationGroup;   // All antigen (substrains summed) current contagion (by contagion destination)
        vector<ContagionAccumulator_t> forceOfInfectionByAntigenAndGroup;              // All antigen (substrains summed) force of infection (current contagion normalized)
        AntigenGroupSubstrainMap_t newContagionByAntigenGroupAndSubstrain;
        AntigenGroupSubstrainMap_t currentContagionByAntigenSourceGroupAndSubstrain;
        AntigenGroupSubstrainMap_t currentContagionByAntigenDestinationGroupAndSubstrain;
        AntigenGroupSubstrainMap_t forceOfInfectionByAntigenGroupAndSubstrain;

        std::string tag;

        class SubstrainPopulationImpl : IContagionPopulation
        {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()
        public: 
            SubstrainPopulationImpl(RANDOMBASE* prng, int _antigenId, float _quantity, const SubstrainMap_t& _substrainDistribution);
                
        private:
            // IContagionPopulation implementation
            RANDOMBASE* pRNG;
            virtual AntigenId GetAntigenID() const override;
            virtual AntigenId GetGeneticID() const override;
            virtual void SetAntigenID(int antigenID) override {}
            virtual void SetGeneticID(int geneticID) override {}
            virtual float GetTotalContagion() const override;
            virtual void ResolveInfectingStrain( IStrainIdentity* strainId ) const override;

            int antigenId;
            float contagionQuantity;
            const SubstrainMap_t substrainDistribution;
        };

    private:

        // ITransmissionGroups
        virtual void AddProperty(const string& property, const PropertyValueList_t& values, const ScalingMatrix_t& scalingMatrix) override;
        virtual void Build(float contagionDecayRate, int numberOfStrains = 1, int numberOfSubstrains = 1) override;
        virtual void GetGroupMembershipForProperties(const tProperties& properties, TransmissionGroupMembership_t& membershipOut ) const override;
        virtual void UpdatePopulationSize(const TransmissionGroupMembership_t& transmissionGroupMembership, float size_changes, float mc_weight) override;
        virtual void DepositContagion(const IStrainIdentity& strain, float amount, TransmissionGroupMembership_t transmissionGroupMembership) override;
        virtual void ExposeToContagion(IInfectable* candidate, TransmissionGroupMembership_t transmissionGroupMembership, float deltaTee, TransmissionRoute::Enum tx_route) const override;
        virtual void CorrectInfectivityByGroup(float infectivityCorrection, TransmissionGroupMembership_t transmissionGroupMembership) override;
        virtual void EndUpdate(float infectivityMultiplier = 1.0f, float InfectivityAddition = 0.0f ) override;
        virtual float GetContagionByProperty( const IPKeyValue& property_value ) override;

        virtual void UseTotalPopulationForNormalization() override { normalizeByTotalPopulation = true; }
        virtual void UseGroupPopulationForNormalization() override { normalizeByTotalPopulation = false; }

        virtual void SetTag( const std::string& tag ) override   { this->tag = tag; }
        virtual const std::string& GetTag( void ) const override { return tag; }

        virtual float GetTotalContagion( void ) override;                                           // Return total contagion.
        virtual float GetTotalContagionForGroup( TransmissionGroupMembership_t group ) override;    // Return total contagion for given membership.
// NOTYET        virtual float GetTotalContagionForProperties( const IPKeyValueContainer& property_value ) override;             // Return total contagion on for given properties (maps to membership).
    };
}
