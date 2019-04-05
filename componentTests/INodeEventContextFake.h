/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "ISimulationContext.h"
#include "NodeEventContext.h"
#include "IIndividualHuman.h"
#include "IIndividualHumanContext.h"
#include "EventTrigger.h"
#include "RANDOM.h"

using namespace Kernel;

class INodeEventContextFake : public INodeEventContext,
                              public IIndividualEventBroadcaster,
                              public INodeInterventionConsumer,
                              public ICampaignCostObserver
{
public:
    INodeEventContextFake()
        : INodeEventContext()
        , m_NodeContext(nullptr)
        , m_ID()
        , m_TriggeredEvent()
        , m_IdmDateTime()
        , m_HumanList()
        , m_ObserversMap()
        , m_Rng()
    {
    }

    ~INodeEventContextFake()
    {
        for( auto human : m_HumanList )
        {
            delete human;
        }
        m_HumanList.clear();
    }

    void Initialize()
    {
        m_ObserversMap.resize( EventTriggerFactory::GetInstance()->GetNumEventTriggers() );
    }

    void Add( IIndividualHumanContext* human )
    {
        m_HumanList.push_back( human );
    }

    // ---------------------
    // --- ISupport Methods
    // ---------------------
    virtual QueryResult QueryInterface(iid_t iid, void **ppvObject)
    {
        *ppvObject = nullptr ;
        if ( iid == GET_IID(INodeEventContext)) 
            *ppvObject = static_cast<INodeEventContext*>(this);
        else if ( iid == GET_IID( IIndividualEventBroadcaster ))
            *ppvObject = static_cast<IIndividualEventBroadcaster*>(this);
        else if ( iid == GET_IID(ICampaignCostObserver)) 
            *ppvObject = static_cast<ICampaignCostObserver*>(this);
        else if ( iid == GET_IID(INodeInterventionConsumer)) 
            *ppvObject = static_cast<INodeInterventionConsumer*>(this);

        if( *ppvObject != nullptr )
        {
            return QueryResult::s_OK ;
        }
        else
            return QueryResult::e_NOINTERFACE ;
    }

    virtual int32_t AddRef()  { return 10 ; }
    virtual int32_t Release() { return 10 ; }


    virtual void notifyCampaignExpenseIncurred( float expenseIncurred, const IIndividualHumanEventContext * pIndiv ) {};
    virtual void notifyCampaignEventOccurred( ISupports * pDistributedIntervention,
                                              ISupports * pDistributor, 
                                              IIndividualHumanContext * pDistributeeIndividual ) {};

    // -----------------------------------------------
    // --- IIndividualEventBroadcaster Methods
    // -----------------------------------------------
    virtual void RegisterObserver( IIndividualEventObserver* pIEO, const EventTrigger &trigger )
    {
        m_ObserversMap[ trigger.GetIndex() ].push_back( pIEO );
    }

    virtual void UnregisterObserver( IIndividualEventObserver* pIEO, const EventTrigger &trigger )
    {
        //m_Observers.erase( pIEO );
    }

    virtual void TriggerObservers( IIndividualHumanEventContext* pIndiv, const EventTrigger &trigger )
    {
        m_TriggeredEvent = trigger ;

        for( auto p_ieo : m_ObserversMap[ trigger.GetIndex() ] )
        {
            p_ieo->notifyOnEvent( pIndiv, trigger );
        }
    }


    // ------------------------------
    // --- INodeEventContext Methods
    // ------------------------------
    virtual IIndividualEventBroadcaster* GetIndividualEventBroadcaster()
    {
        return this;
    }

    virtual const IdmDateTime& GetTime() const
    {
        return m_IdmDateTime ;
    }

    virtual const suids::suid & GetId() const
    {
        release_assert( m_NodeContext );
        return m_ID;
    }

    virtual INodeContext* GetNodeContext()
    {
        release_assert( m_NodeContext );
        return m_NodeContext;
    }

    virtual void SetContextTo(INodeContext* context)
    {
        m_NodeContext = context;
        if( m_NodeContext != nullptr )
        {
            m_ID = m_NodeContext->GetSuid();
        }
    }

    virtual void VisitIndividuals(individual_visit_function_t func)
    {
        for( auto p_human : m_HumanList )
        {
            func( p_human->GetEventContext() );
        }
    }

    virtual int VisitIndividuals(IVisitIndividual* pIndividualVisitImpl)
    { 
        float cost = 0.0;
        int count = 0;
        for( auto p_human : m_HumanList )
        {
            if( pIndividualVisitImpl->visitIndividualCallback( p_human->GetEventContext(), cost, nullptr ) )
            {
                ++count;
            }
        }
        return count;
    }

    virtual const NodeDemographics& GetDemographics() { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    //virtual float GetYear() const = 0;

    // to update any node-owned interventions
    virtual void UpdateInterventions(float = 0.0f) { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
        
    // methods to install hooks for arrival/departure/birth/etc
    virtual void RegisterTravelDistributionSource(  ITravelLinkedDistributionSource *tles, TravelEventType type) { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual void UnregisterTravelDistributionSource(ITravelLinkedDistributionSource *tles, TravelEventType type) { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }

    virtual void PurgeExisting( const std::string& iv_name )
    {
    }

    virtual std::list<INodeDistributableIntervention*> GetInterventionsByType(const std::string& type_name)         { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
       
    virtual bool IsInPolygon(float* vertex_coords, int numcoords) { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual bool IsInPolygon( const json::Array &poly )           { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual bool IsInExternalIdSet( const std::list<ExternalNodeId_t>& nodelist ) { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }

    virtual RANDOMBASE* GetRng()
    {
        return &m_Rng;
    }

    virtual int GetIndividualHumanCount() const { return m_HumanList.size(); }
    virtual ExternalNodeId_t GetExternalId()  const { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }

    // --------------------------------------
    // --- INodeInterventionConsumer methods
    // --------------------------------------
    virtual bool GiveIntervention( INodeDistributableIntervention * pIV )
    {
        return true;
    }

    // -----------------
    // --- Other Methods
    // -----------------
    EventTrigger GetTriggeredEvent() const { return m_TriggeredEvent ; }
    void ClearTriggeredEvent() { m_TriggeredEvent = EventTrigger(); }

    void SetTime( const IdmDateTime& rTime )
    {
        m_IdmDateTime = rTime ;
    }

    IIndividualHumanContext* GetIndividualById( int humanId )
    {
        for( auto human : m_HumanList )
        {
            if( human->GetSuid().data == humanId )
            {
                return human;
            }
        }
        return nullptr;
    }

private:
    INodeContext* m_NodeContext;
    suids::suid m_ID;
    EventTrigger m_TriggeredEvent ;
    IdmDateTime m_IdmDateTime ;
    std::vector<IIndividualHumanContext*> m_HumanList;
    std::vector<std::vector<IIndividualEventObserver*> > m_ObserversMap;
    PSEUDO_DES m_Rng;
};
