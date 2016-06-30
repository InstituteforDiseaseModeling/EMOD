/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "Contexts.h"

using namespace Kernel;

struct INodeContextFake : INodeContext
{
private:
    suids::suid m_suid ;

public:
    INodeContextFake()
    : m_suid()
    {
        m_suid.data = 1 ;
    }

    INodeContextFake( const suids::suid& rSuid )
    : m_suid()
    {
        m_suid = rSuid ;
    }

    virtual bool operator==( const INodeContext& rThat ) const override
    {
        const INodeContextFake* pThat = dynamic_cast<const INodeContextFake*>(&rThat) ;
        if( pThat == nullptr ) return false ;

        if( this->m_suid.data != pThat->m_suid.data ) return false ;

        return true ;
    }

    virtual ISimulationContext* GetParent() override
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual suids::suid GetSuid() const override
    {
        return m_suid ;
    }

    virtual suids::suid GetNextInfectionSuid() override
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual ::RANDOMBASE* GetRng() override
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual void ExposeIndividual( IInfectable* candidate, const TransmissionGroupMembership_t* individual, float dt ) override
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual void DepositFromIndividual( StrainIdentity* strain_IDs, float contagion_quantity, const TransmissionGroupMembership_t* individual ) override
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual void GetGroupMembershipForIndividual( const RouteList_t& route, tProperties* properties, TransmissionGroupMembership_t* membershipOut ) override
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual void UpdateTransmissionGroupPopulation( const TransmissionGroupMembership_t* membership, float size_changes,float mc_weight ) override
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual float GetTotalContagion( const TransmissionGroupMembership_t* membership ) override
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual RouteList_t& GetTransmissionRoutes() const override
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual float getSinusoidalCorrection(float sinusoidal_amplitude, float sinusoidal_phase) const override
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual float getBoxcarCorrection(float boxcar_amplitude, float boxcar_start_time, float boxcar_end_time) const override
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual IMigrationInfo* GetMigrationInfo() override
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual const NodeDemographics* GetDemographics() const override
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual const NodeDemographicsDistribution* GetDemographicsDistribution( std::string ) const override
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual IdmDateTime GetTime() const override
    {
        IdmDateTime time;
        return time ;
    }

    virtual float GetInfected() const override
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual float GetStatPop() const override
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual float GetBirths() const override
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual float GetCampaignCost() const override
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual float GetInfectivity() const override
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual float GetInfectionRate() const override
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual float GetSusceptDynamicScaling() const override
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual const Climate* GetLocalWeather() const override
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual INodeEventContext* GetEventContext() override
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual const tPropertiesDistrib& GetIndividualPropertyDistributions() const override
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual void checkValidIPValue( const std::string& key, const std::string& to_value ) override
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual void AddEventsFromOtherNodes( const std::vector<std::string>& rEventNameList ) override
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual QueryResult QueryInterface( iid_t iid, void** pinstance ) override
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual int32_t AddRef() override
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual int32_t Release() override
    {
        throw std::exception("The method or operation is not implemented.");
    }

    act_prob_vec_t DiscreteGetTotalContagion(const TransmissionGroupMembership_t* membership) override
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual void VerifyPropertyDefined( const std::string& rKey, const std::string& rVal ) const override
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual long int GetPossibleMothers() const override
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual bool IsEveryoneHome() const override
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual void SetWaitingForFamilyTrip( suids::suid migrationDestination, MigrationType::Enum migrationType, float timeUntilTrip, float timeAtDestination, bool isDestinationNewHome ) override
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual float GetMeanAgeInfection() const override
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual std::vector<bool> GetMigrationTypeEnabledFromDemographics() const override
    {
        std::vector<bool> enabled_list;

        enabled_list.push_back( true );
        enabled_list.push_back( true );
        enabled_list.push_back( true );
        enabled_list.push_back( true );
        enabled_list.push_back( true );

        return enabled_list;
    }

    virtual ExternalNodeId_t GetExternalID() const override
    {
        return m_suid.data ;
    }

    virtual float GetLatitudeDegrees() override
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual float GetLongitudeDegrees() override
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual void SetupMigration( IMigrationInfoFactory * migration_factory, 
                                 MigrationStructure::Enum ms,
                                 const boost::bimap<ExternalNodeId_t, suids::suid>& rNodeIdSuidMap ) override
    { 
        throw std::exception("The method is not implemented.");
    }

    virtual void SetContextTo(ISimulationContext*)                                                             override { throw std::exception("The method is not implemented."); }
    virtual void SetMonteCarloParameters(float indsamplerate =.05, int nummininf = 0)                          override { throw std::exception("The method is not implemented."); }
    virtual void SetParameters(NodeDemographicsFactory *demographics_factory, ClimateFactory *climate_factory) override { throw std::exception("The method is not implemented."); }
    virtual void PopulateFromDemographics()                                                                    override { throw std::exception("The method is not implemented."); }
    virtual void Update(float)                                                                                 override { throw std::exception("The method is not implemented."); }
    virtual IIndividualHuman* processImmigratingIndividual(IIndividualHuman*)                                  override { throw std::exception("The method is not implemented."); }

    virtual float GetBasePopulationScaleFactor() const
    {
        return 1.0;
    }
};