/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

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
        virtual AntigenId GetAntigenId( void ) const; // { return 0; }
        virtual float GetTotalContagion( void ) const { return 0; }
        virtual void ResolveInfectingStrain( StrainIdentity* strainId ) const { }
        virtual NaturalNumber GetInfectorID( void ) const;

        DiscreteContagionPopulation( act_prob_vec_t &payload, NaturalNumber depositor_id )
        {
            probs = payload;
            _infector = depositor_id;
        }
        virtual act_prob_vec_t GetProbabilities() const
        {
            return probs;
        }
        protected:
        act_prob_vec_t probs;
        AntigenId _antigen;
        NaturalNumber _infector;
    };

    class INodeSTI;
    class RelationshipGroups: public TransmissionGroupsBase
    {
        public:
            RelationshipGroups();

            virtual void AddProperty(const string& property, const PropertyValueList_t& values, const ScalingMatrix_t& scalingMatrix, const string& route);
            virtual void AddPropertyValuesToValueToIndexMap( const string& propertyName, const PropertyValueList_t& valueSet, int currentMatrixSize );
            virtual void Build(const RouteToContagionDecayMap_t& contagionDecayRatesByRoute, int numberOfStrains, int numberOfSubstrains = 1);
            virtual void GetGroupMembershipForProperties(const RouteList_t& route, const tProperties* properties, TransmissionGroupMembership_t* membershipOut ) const;
            virtual void UpdatePopulationSize(const TransmissionGroupMembership_t* transmissionGroupMembership, float size_changes, float mc_weight);
            virtual void DepositContagion(const StrainIdentity* strain, float amount, const TransmissionGroupMembership_t* poolMembership);
            virtual void ExposeToContagion(IInfectable* candidate, const TransmissionGroupMembership_t* poolMembership, float deltaTee) const;
            virtual void EndUpdate(float infectivityCorrection);
            virtual void SetParent( INodeSTI* parent );
            
            virtual void CorrectInfectivityByGroup(float infectivityCorrection, const TransmissionGroupMembership_t* transmissionGroupMembership);
            virtual float GetTotalContagion(const TransmissionGroupMembership_t* transmissionGroupMembership);

            virtual act_prob_vec_t DiscreteGetTotalContagion(const TransmissionGroupMembership_t* transmissionGroupMembership) { return infectionRate[0]; }

        protected:
        
            typedef vector<act_prob_vec_t> DiscreteContagionAccumulator_t;

            void BuildRouteScalingMatrices( void );
            void StoreRouteDecayValues( const RouteToContagionDecayMap_t& contagionDecayRatesByRoute );

            map< int, std::string > poolIndexToRelationshipReverseMap;

            PropertyToValuesMap_t propertyNameToValuesMap;
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
