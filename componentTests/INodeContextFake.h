/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include "Contexts.h"

using namespace Kernel;

struct INodeContextFake : public INodeContext
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

    virtual bool operator==( const INodeContext& rThat ) const
    {
        const INodeContextFake* pThat = dynamic_cast<const INodeContextFake*>(&rThat) ;
        if( pThat == nullptr ) return false ;

        if( this->m_suid.data != pThat->m_suid.data ) return false ;

        return true ;
    }

    virtual suids::suid GetSuid() const
    {
        return m_suid ;
    }

    virtual suids::suid GetNextInfectionSuid()
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual ::RANDOMBASE* GetRng()
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual void ExposeIndividual( IInfectable* candidate, const TransmissionGroupMembership_t* individual, float dt )
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual void DepositFromIndividual( StrainIdentity* strain_IDs, float contagion_quantity, const TransmissionGroupMembership_t* individual )
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual void GetGroupMembershipForIndividual( RouteList_t& route, tProperties* properties, TransmissionGroupMembership_t* membershipOut )
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual void UpdateTransmissionGroupPopulation( const TransmissionGroupMembership_t* membership, float size_changes,float mc_weight )
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual float GetTotalContagion( const TransmissionGroupMembership_t* membership )
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual RouteList_t& GetTransmissionRoutes()
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual const MigrationInfo* GetMigrationInfo() const
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual const NodeDemographics* GetDemographics() const
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual const NodeDemographicsDistribution* GetDemographicsDistribution( std::string ) const
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual IdmDateTime GetTime() const
    {
        IdmDateTime time;
        return time ;
    }

    virtual float GetInfected() const
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual float GetStatPop() const
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual float GetBirths() const
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual float GetCampaignCost() const
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual float GetInfectivity() const
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual float GetInfectionRate() const
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual float GetSusceptDynamicScaling() const
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual const Climate* GetLocalWeather() const
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual INodeEventContext* GetEventContext()
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual const tDistrib& GetIndividualPropertyDistributions() const
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual void checkValidIPValue( const std::string& key, const std::string& to_value )
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual QueryResult QueryInterface( iid_t iid, void** pinstance )
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual int32_t AddRef()
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual int32_t Release()
    {
        throw std::exception("The method or operation is not implemented.");
    }

    act_prob_vec_t DiscreteGetTotalContagion(const TransmissionGroupMembership_t* membership)
    {
        throw std::exception("The method or operation is not implemented.");
    }

    void RegisterNewInfectionObserver(void* id, INodeContext::callback_t observer)
    {
        throw std::exception("The method or operation is not implemented.");
    }

    void UnregisterNewInfectionObserver(void* id)
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual void VerifyPropertyDefined( const std::string& rKey, const std::string& rVal ) const
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual long int GetPossibleMothers() const
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual int GetExternalID() const
    {
        return m_suid.data ;
    }
};