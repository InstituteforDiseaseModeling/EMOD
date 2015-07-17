/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include "MultiRouteTransmissionGroups.h"
#include "IContagionPopulation.h"

using namespace std;

namespace Kernel
{
    class StrainAwareTransmissionGroups : protected MultiRouteTransmissionGroups
    {
    public:
        StrainAwareTransmissionGroups();

    protected:
        // Implementation details
        int antigenCount;
        int substrainCount;
        int routeCount;
        vector<bool> antigenWasShed;                 // Contagion of this antigenId was shed this cycle.
        vector<set<unsigned int>> substrainWasShed; // Contagion of this antigenId/substrainId was shed this cycle.

        vector<vector<ContagionAccumulator_t>> newInfectivityByAntigenRouteGroup;    // All antigen (substrains summed) shed
        vector<vector<ContagionAccumulator_t>> sumInfectivityByAntigenRouteGroup;    // All antigen (substrains summed) exposure
        vector<vector<ContagionAccumulator_t>> forceOfInfectionByAntigenRouteGroup;  // All antigen (substrains summed) force of infection
        typedef map<unsigned int, float> SubstrainMap_t;
        typedef vector<vector<SubstrainMap_t>> RouteGroupSubstrainMap_t;
        typedef vector<RouteGroupSubstrainMap_t> AntigenRouteGroupSubstrainMap_t;
        AntigenRouteGroupSubstrainMap_t newInfectivityByAntigenRouteGroupSubstrain;
        AntigenRouteGroupSubstrainMap_t sumInfectivityByAntigenRouteGroupSubstrain;

        class SubstrainPopulationImpl : IContagionPopulation
        {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()
        public:
            SubstrainPopulationImpl(int _antigenId, float _quantity, const vector<const SubstrainMap_t*>& _substrainDistributions)
                : antigenId(_antigenId)
                , contagionQuantity(_quantity)
                , substrainDistributions(_substrainDistributions)
            {}

        private:
            // IContagionPopulation implementation
            virtual AntigenId GetAntigenId( void ) const;
            virtual float GetTotalContagion( void ) const;
            virtual void ResolveInfectingStrain( StrainIdentity* strainId ) const;

            int antigenId;
            float contagionQuantity;
            const vector<const SubstrainMap_t*>& substrainDistributions;
        };

    private:

        // ITransmissionGroups implementation
        // Same as MultiRouteTransmissionGroups
        // virtual void AddProperty(const string& property, const PropertyValueList_t& values, const ScalingMatrix_t& scalingMatrix, const string& route);
        virtual void Build(const RouteToContagionDecayMap_t& contagionDecayRatesByRoute, int numberOfStrains, int numberOfSubstrains);
        // Same as MultiRouteTransmissionGroups
        // virtual const TransmissionGroupMembership_t* GetGroupMembershipForProperties(const tProperties* properties) const;
        // virtual void UpdatePopulationSize(const TransmissionGroupMembership_t* transmissionGroupMembership, float size_changes, float mc_weight);
        virtual void DepositContagion(const StrainIdentity* strain, float amount, const TransmissionGroupMembership_t* transmissionGroupMembership);
        virtual void ExposeToContagion(IInfectable* candidate, const TransmissionGroupMembership_t* transmissionGroupMembership, float deltaTee) const;
        virtual float GetTotalContagion(const TransmissionGroupMembership_t* transmissionGroupMembership);
        virtual void CorrectInfectivityByGroup(float infectivityCorrection, const TransmissionGroupMembership_t* transmissionGroupMembership);
        virtual void EndUpdate(float infectivityCorrection);

        /********************************************************************/

        void AllocateAccumulators( int routeCount, int numberOfStrains, int numberOfSubstrains );
    };
}
