/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "TransmissionGroupsBase.h"

namespace Kernel {
    class IContagionProbabilities: public ISupports
    {
        public:
        virtual act_prob_vec_t GetProbabilities() const = 0;
        virtual NaturalNumber GetInfectorID( void ) const = 0;
    };

    class DiscreteContagionPopulation : public IContagionPopulation, public IContagionProbabilities
    {
        DECLARE_QUERY_INTERFACE()
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
    public:
        DiscreteContagionPopulation( act_prob_vec_t &payload, NaturalNumber depositor_id )
        {
            probs = payload;
            _infector = depositor_id;
        }
        virtual ~DiscreteContagionPopulation() {}

        // IContagionPopulation
        virtual int GetAntigenID( void ) const override;
        virtual int GetGeneticID( void ) const override { return -1; }
        virtual void SetAntigenID(int in_antigenID) override {}
        virtual void SetGeneticID(int in_geneticID) override {}
        virtual float GetTotalContagion( void ) const override { return 0; }
        virtual void ResolveInfectingStrain( IStrainIdentity* strainId ) const override { }

        // IContagionProbabilities
        virtual NaturalNumber GetInfectorID( void ) const override;

        virtual act_prob_vec_t GetProbabilities() const override
        {
            return probs;
        }

    protected:
        act_prob_vec_t probs;
        AntigenId _antigen;
        NaturalNumber _infector;
    };

    class INodeSTI;
    class RelationshipGroups: public ITransmissionGroups
    {
        public:
            RelationshipGroups();

            // ITransmissionGroups - invalid methods
            virtual void AddProperty( const string& property, const PropertyValueList_t& values, const ScalingMatrix_t& scalingMatrix, const string& route );
            virtual void GetGroupMembershipForProperties( const RouteList_t& route, const tProperties* properties, TransmissionGroupMembership_t* membershipOut ) const;
            virtual void IncrementWeightedPopulation(const TransmissionGroupMembership_t* transmissionGroupMembership, float weight);

            // ITransmissionGroups - implemented methods
            virtual void Build(const RouteToContagionDecayMap_t& contagionDecayRatesByRoute, int numberOfStrains, int numberOfSubstrains = 1);
            virtual void UpdatePopulationSize(const TransmissionGroupMembership_t* transmissionGroupMembership, float size_changes, float mc_weight);
            virtual void DepositContagion(const IStrainIdentity& strain, float amount, const TransmissionGroupMembership_t* poolMembership);
            virtual void ExposeToContagion(IInfectable* candidate, const TransmissionGroupMembership_t* poolMembership, float deltaTee) const;
            virtual void EndUpdate(float infectivityCorrection);
            virtual void CorrectInfectivityByGroup(float infectivityCorrection, const TransmissionGroupMembership_t* transmissionGroupMembership);
            virtual float GetTotalContagion(const TransmissionGroupMembership_t* transmissionGroupMembership);
            virtual act_prob_vec_t DiscreteGetTotalContagion(const TransmissionGroupMembership_t* transmissionGroupMembership) { return infectionRate[0]; }

            // Special RelationGroups methods so we can use RelationshipID instead of strings with the ID in them
            virtual void SetParent( INodeSTI* parent );
            virtual void AddProperty( const string& property, const std::list<uint32_t>& values, const ScalingMatrix_t& scalingMatrix, const string& route );
            virtual void AddPropertyValuesToValueToIndexMap( const string& propertyName, const std::list<uint32_t>& valueSet, int currentMatrixSize );
            virtual void GetGroupMembershipForProperties( const RouteList_t& route, const std::map<std::string, uint32_t>* properties, TransmissionGroupMembership_t* membershipOut ) const;

        protected:
        
            typedef vector<act_prob_vec_t> DiscreteContagionAccumulator_t;

            void BuildRouteScalingMatrices( void );
            void StoreRouteDecayValues( const RouteToContagionDecayMap_t& contagionDecayRatesByRoute );

            // index to RelationshipID
            map< int, uint32_t > poolIndexToRelationshipReverseMap;

            // Relationship name to RelationshipID
            map<const string, list<uint32_t>> propertyNameToValuesMap;

            typedef map<uint32_t, int> RgValueToIndexMap_t; // RelationshipID to index
            typedef map<string, RgValueToIndexMap_t> RgPropertyValueToIndexMap_t; // RelationshipName to map
            RgPropertyValueToIndexMap_t rg_propertyValueToIndexMap;      // Used to determine group membership

            PropertyToMatrixMap_t rg_propertyNameToMatrixMap;

            unsigned int max_index;
            ScalingMatrix_t scalingMatrix;
            string transmissionRouteName;
            float contagionDecayRate;
            DiscreteContagionAccumulator_t shedContagion;
            DiscreteContagionAccumulator_t currentContagion;
            DiscreteContagionAccumulator_t infectionRate;
            std::map< GroupIndex, int > infectors;
            float populationSize;

        private:
            INodeSTI * m_parent;

            static void LogActivity( unsigned int _action, unsigned int _actor, unsigned int _index, float _amount );
            struct LogEntry {
                unsigned int action;
                unsigned int actor;
                unsigned int index;
                float amount;
            };
#define GRP_LOG_COUNT    0x100000   // 2^20 entries
            static unsigned int _head;
            static unsigned int _tail;
            static LogEntry _log[GRP_LOG_COUNT];
    };
}
