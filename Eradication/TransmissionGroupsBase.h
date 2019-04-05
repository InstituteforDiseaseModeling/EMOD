/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

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
    typedef map<const string, PropertyValueList_t> PropertyToValuesMap_t;
    typedef map<const string, int> ValueToIndexMap_t;
    typedef map<const string, ValueToIndexMap_t> PropertyValueToIndexMap_t;
    typedef vector<float> ContagionAccumulator_t;
    typedef map<const string, ScalingMatrix_t> PropertyToMatrixMap_t;

    class TransmissionGroupsBase : public ITransmissionGroups
    {
    protected:
        TransmissionGroupsBase();

        PropertyToMatrixMap_t propertyNameToMatrixMap;

        PropertyValueToIndexMap_t propertyValueToIndexMap;      // Used to determine group membership

        // Common implementation
        void checkForDuplicatePropertyName( const string& property ) const;
        void checkForValidValueListSize( const PropertyValueList_t& values ) const;
        void checkForValidScalingMatrixSize( const ScalingMatrix_t& scalingMatrix, const PropertyValueList_t& values ) const;
        void addScalingMatrixToPropertyToMatrixMap( const string& property, const ScalingMatrix_t& scalingMatrix );
        void CheckForValidStrainListSize( const StrainIdentitySet_t& strains ) const;
        virtual void addPropertyValuesToValueToIndexMap( const string& propertyName, const PropertyValueList_t& valueSet, int currentMatrixSize );
        void getGroupIndicesForProperty( const IPKeyValue& property_value, const PropertyToValuesMap_t& propertyNameToValuesMap, std::vector<size_t>& indices );

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
        inline float vectorDotProduct(const vector<float>& vectorOne, const vector<float>& vectorTwo)
        {
            size_t vectorSize = min(vectorOne.size(), vectorTwo.size());
            float result = 0.0f;
            const float* pOne = vectorOne.data();
            const float* pTwo = vectorTwo.data();
            for (size_t iElement = 0; iElement < vectorSize; ++iElement)
            {
                result += *pOne++ * *pTwo++;
            }

            return result;
        }

        inline void vectorElementAdd(vector<float>& a, vector<float>& b)
        {
            size_t count = min(a.size(), b.size());
            float* aData = a.data();
            const float* bData = b.data();
            for (size_t index = 0; index < count; ++index)
            {
                *aData += *bData;
                ++aData;
                ++bData;
            }
        }

        inline void vectorElementDivide(vector<float>& numerator, vector<float>& denominator)
        {
            size_t count = min(numerator.size(), denominator.size());
            float* pNumerator = numerator.data();
            const float* pDenominator = denominator.data();
            for (size_t index = 0; index < count; ++index)
            {
                if (*pDenominator != 0.0f)
                {
                    *pNumerator /= *pDenominator;
                }
                else
                {
                    *pNumerator = 0.0f;
                }
                ++pNumerator;
                ++pDenominator;
            }
        }

        inline void vectorScalarAdd(ContagionAccumulator_t& vector, float scalar)
        {
            for (float& entry : vector)
            {
                entry += scalar;
            }
        }

        inline void vectorScalarMultiply(ContagionAccumulator_t& vector, float scalar)
        {
           for (float& entry : vector)
           {
               entry *= scalar;
           }
        }

    private:

        // ITransmissionGroups implementation
        virtual void AddProperty(const string& property, const PropertyValueList_t& values, const ScalingMatrix_t& scalingMatrix) {}
        virtual void Build(float contagionDecayRate, int numberOfAntigens = 1, int numberOfSubstrains = 1) {}
        virtual void GetGroupMembershipForProperties(const tProperties& properties) const { }
        virtual void DepositContagion(const IStrainIdentity& strain, float amount, TransmissionGroupMembership_t transmissionGroupMembership) {}
        virtual void ExposeToContagion(IInfectable* candidate, TransmissionGroupMembership_t transmissionGroupMembership, float deltaTee, TransmissionRoute::Enum tx_route = TransmissionRoute::TRANSMISSIONROUTE_CONTACT) const {}
        virtual void CorrectInfectivityByGroup(float infectivityCorrection, TransmissionGroupMembership_t transmissionGroupMembership) {}
        virtual void EndUpdate(float infectivityCorrection, float infectivityAddition) {}

        virtual act_prob_vec_t DiscreteGetTotalContagion( void );
   };
}
