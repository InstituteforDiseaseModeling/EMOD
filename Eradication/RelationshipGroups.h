/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

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
            RelationshipGroups( RANDOMBASE * prng );

            // ITransmissionGroups - invalid methods
            virtual void AddProperty( const string& property, const PropertyValueList_t& values, const ScalingMatrix_t& scalingMatrix );
            virtual void GetGroupMembershipForProperties( const std::map<std::string, uint32_t>& properties, std::map< int, TransmissionGroupMembership_t> & membershipOut ) const;
            virtual void GetGroupMembershipForProperties( const tProperties& properties, TransmissionGroupMembership_t& membershipOut ) const override;

            // ITransmissionGroups - implemented methods
            virtual void Build(float contagionDecayRate, int numberOfStrains, int numberOfSubstrains = 1);
            virtual void UpdatePopulationSize(const TransmissionGroupMembership_t& transmissionGroupMembership, float size_changes, float mc_weight);
            virtual void DepositContagion(const IStrainIdentity& strain, float amount, TransmissionGroupMembership_t poolMembership);
            virtual void ExposeToContagion(IInfectable* candidate, TransmissionGroupMembership_t poolMembership, float deltaTee, TransmissionRoute::Enum tx_route = TransmissionRoute::TRANSMISSIONROUTE_CONTACT) const;
            virtual void EndUpdate(float infectivityMultiplier = 1.0f, float infectivityAddition = 0.0f );

            virtual void CorrectInfectivityByGroup(float infectivityCorrection, TransmissionGroupMembership_t transmissionGroupMembership);
            virtual float GetTotalContagion( void ) override { return nanf("NAN"); }
            virtual act_prob_vec_t DiscreteGetTotalContagion( void ) { return infectionRate[0]; }
            virtual float GetContagionByProperty( const IPKeyValue& property_value ) override;

            // Special RelationGroups methods so we can use RelationshipID instead of strings with the ID in them
            virtual void SetParent( INodeSTI* parent );
            virtual void AddProperty( const string& property, const std::list<uint32_t>& values, const ScalingMatrix_t& scalingMatrix );
            virtual void addPropertyValuesToValueToIndexMap( const string& propertyName, const std::list<uint32_t>& valueSet, int currentMatrixSize );


        protected:
        
            typedef vector<act_prob_vec_t> DiscreteContagionAccumulator_t;

            void BuildRouteScalingMatrices( void );

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

            static std::string tag;

        private:
            INodeSTI * m_parent;

            virtual void UseTotalPopulationForNormalization( void ) { return; } // N/A
            virtual void UseGroupPopulationForNormalization( void ) { return; } // N/A

            virtual void SetTag( const std::string& tag )   { return; };
            virtual const std::string& GetTag( void ) const { return tag; };

            virtual float GetTotalContagionForGroup( TransmissionGroupMembership_t group ) override { return nanf("NAN"); }
// NOTYET        virtual float GetTotalContagionForProperties( const IPKeyValueContainer& property_value ) override { return nanf("NAN"); }

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
