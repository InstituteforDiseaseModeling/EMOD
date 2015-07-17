/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once
#include "IdmApi.h"
#include "ISupports.h"
#include "suids.hpp"
#include "RANDOM.h"
#include "IdmDateTime.h"
#include "IInfectable.h"
#include "TransmissionGroupMembership.h"
#include "ITransmissionGroups.h"

namespace Kernel
{
    class  MigrationInfo;
    struct NodeDemographics;
    struct NodeDemographicsDistribution;
    class  Climate;
    struct INodeEventContext;
    struct IIndividualHuman;

    struct IDMAPI INodeContext : public ISupports // information and services related to the context in the simulation environment provided exposed by a node for its contained objects
    {
        // TODO/OPTION:
        // could have an PostMigratingIndividual interface too if individuals will call back to do migration....

        // DMB 9-9-2014 May need to consider implementing the equality operator for ISupports
        virtual bool operator==( const INodeContext& rThat ) const
        {
            throw NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "INodeContect::operator==() needs to be implemented by subclass.");
        } ;

        virtual bool operator!=( const INodeContext& rThat ) const
        { 
            return !(*this == rThat);
        } ;

        //individual can get an id of their parent to compare against, for instance, their home node id
        virtual suids::suid GetSuid() const = 0;

        virtual suids::suid GetNextInfectionSuid() = 0;
        virtual ::RANDOMBASE* GetRng() = 0; 

        // heterogeneous intra-node transmission
        virtual void ExposeIndividual(IInfectable* candidate, const TransmissionGroupMembership_t* individual, float dt) = 0;
        virtual void DepositFromIndividual(StrainIdentity* strain_IDs, float contagion_quantity, const TransmissionGroupMembership_t* individual) = 0;
        virtual void GetGroupMembershipForIndividual(RouteList_t& route, tProperties* properties, TransmissionGroupMembership_t* membershipOut ) = 0;
        virtual void UpdateTransmissionGroupPopulation(const TransmissionGroupMembership_t* membership, float size_changes,float mc_weight) = 0;
        virtual float GetTotalContagion(const TransmissionGroupMembership_t* membership) = 0;
        virtual RouteList_t& GetTransmissionRoutes( ) = 0;
        
        // Discrete HINT contagion
        virtual act_prob_vec_t DiscreteGetTotalContagion(const TransmissionGroupMembership_t* membership) = 0;

        virtual const MigrationInfo* GetMigrationInfo() const = 0;
        virtual const NodeDemographics* GetDemographics() const = 0;
        virtual const NodeDemographicsDistribution* GetDemographicsDistribution(std::string) const = 0;

        // reporting interfaces
        virtual IdmDateTime GetTime()          const = 0;
        virtual float       GetInfected()      const = 0;
        virtual float       GetStatPop()       const = 0;
        virtual float       GetBirths()        const = 0;
        virtual float       GetCampaignCost()  const = 0;
        virtual float       GetInfectivity()   const = 0;
        virtual float       GetInfectionRate() const = 0;
        virtual float       GetSusceptDynamicScaling() const = 0;
        virtual const Climate* GetLocalWeather() const = 0;
        virtual long int GetPossibleMothers() const = 0;

        // This method will ONLY be used for reporting by input node ID, don't use it elsewhere!
        virtual int GetExternalID() const = 0;

        typedef std::function<void(IIndividualHuman*)> callback_t;
        virtual void RegisterNewInfectionObserver(void* id, INodeContext::callback_t observer) = 0;
        virtual void UnregisterNewInfectionObserver(void* id) = 0;

        // for interventions
        virtual INodeEventContext* GetEventContext() = 0;
        typedef std::map< std::string, std::multimap< float, std::string > > tDistrib;
        virtual const tDistrib& GetIndividualPropertyDistributions() const = 0;
        virtual void checkValidIPValue( const std::string& key, const std::string& to_value ) = 0;

        //Verify that the user entered in set of property key/value pairs which are included in the demographics file
        virtual void VerifyPropertyDefined( const std::string& rKey, const std::string& rVal ) const = 0;
    };
}

