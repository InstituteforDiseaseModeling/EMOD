/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "ITransmissionGroups.h"
#include "TransmissionGroupMembership.h"
#include "IContagionPopulation.h"
#include "Types.h"  // For act_prob_vec_t and act_prob_t

namespace Kernel
{
    typedef map<const string, ScalingMatrix_t> PropertyToMatrixMap_t;

    class TransmissionGroupsBase : public ITransmissionGroups
    {
    protected:
        TransmissionGroupsBase();

        typedef map<const string, PropertyValueList_t> PropertyToValuesMap_t;

        PropertyToMatrixMap_t propertyNameToMatrixMap;

        typedef map<const string, int> ValueToIndexMap_t;
        typedef map<const string, ValueToIndexMap_t> PropertyValueToIndexMap_t;
        PropertyValueToIndexMap_t propertyValueToIndexMap;      // Used to determine group membership

        typedef vector<float> ContagionAccumulator_t;

        // Common implementation
        void CheckForDuplicatePropertyName( const string& property ) const;
        void CheckForValidValueListSize( const PropertyValueList_t& values ) const;
        void CheckForValidScalingMatrixSize( const ScalingMatrix_t& scalingMatrix, const PropertyValueList_t& values ) const;
        void AddScalingMatrixToPropertyToMatrixMap( const string& property, const ScalingMatrix_t& scalingMatrix );
        void CheckForValidStrainListSize( const StrainIdentitySet_t& strains ) const;
        virtual void AddPropertyValuesToValueToIndexMap( const string& propertyName, const PropertyValueList_t& valueSet, int currentMatrixSize );

    public:
        static void InitializeCumulativeMatrix( ScalingMatrix_t& cumulativeMatrix );
        static void AggregatePropertyMatrixWithCumulativeMatrix( const ScalingMatrix_t& propertyMatrix, ScalingMatrix_t& cumulativeMatrix );

        class ContagionPopulationImpl : public IContagionPopulation
        {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()
        public:
            ContagionPopulationImpl(float quantity) : contagionQuantity(quantity) {}
            ContagionPopulationImpl( IStrainIdentity * strain, float quantity );

        private:
            // IContagionPopulation implementation
            virtual AntigenId GetAntigenID( void ) const override;
            virtual AntigenId GetGeneticID( void ) const override; // Hmm.... not sure about this
            virtual void SetAntigenID(int in_antigenID) override {}; // Hmm.... not sure about this
            virtual void SetGeneticID(int in_geneticID) override {}; // Hmm.... not sure about this
            virtual float GetTotalContagion( void ) const override;
            virtual void ResolveInfectingStrain( IStrainIdentity* strainId ) const override;

            float contagionQuantity;
            int antigenId;
        };

    protected:
        // Utility function(s)
        inline float VectorDotProduct(const vector<float>& vectorOne, const vector<float>& vectorTwo)
        {
            int vectorSize = min(vectorOne.size(), vectorTwo.size());
            float result = 0.0f;
            const float* pOne = vectorOne.data();
            const float* pTwo = vectorTwo.data();
            for (int iElement = 0; iElement < vectorSize; iElement++)
            {
                result += *pOne++ * *pTwo++;
            }

            return result;
        }

        inline void VectorScalarMultiplyInPlace(ContagionAccumulator_t& vector, float scalar)
        {
            int vectorSize = vector.size();
            float* pElement = vector.data();
            for (int iElement = 0; iElement < vectorSize; iElement++)
            {
                *pElement++ *= scalar;
            }
        }

    private:

        // ITransmissionGroups implementation
        virtual void AddProperty(const string& property, const PropertyValueList_t& values, const ScalingMatrix_t& scalingMatrix, const string& route) {}
        virtual void Build(const RouteToContagionDecayMap_t& contagionDecayRatesByRoute, int numberOfAntigens = 1, int numberOfSubstrains = 1) {}
        virtual void GetGroupMembershipForProperties(const RouteList_t& route, const tProperties* properties) const { }
        virtual void DepositContagion(const IStrainIdentity& strain, float amount, const TransmissionGroupMembership_t* transmissionGroupMembership) {}
        virtual void ExposeToContagion(IInfectable* candidate, const TransmissionGroupMembership_t* transmissionGroupMembership, float deltaTee) const {}
        virtual void CorrectInfectivityByGroup(float infectivityCorrection, const TransmissionGroupMembership_t* transmissionGroupMembership) {}
        virtual void IncrementWeightedPopulation(const TransmissionGroupMembership_t* transmissionGroupMembership, float weight) {}
        virtual void EndUpdate(float infectivityCorrection) {}

        virtual act_prob_vec_t DiscreteGetTotalContagion(const TransmissionGroupMembership_t* transmissionGroupMembership);
   };
}
