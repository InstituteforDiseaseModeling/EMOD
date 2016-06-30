/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "Contexts.h"
#include "NodeEventContext.h"
#include "IIndividualHuman.h"

using namespace Kernel;

class INodeEventContextFake : public INodeEventContext,
                              public INodeTriggeredInterventionConsumer
{
public:
    INodeEventContextFake()
        : INodeEventContext()
        , m_TriggeredEvent( IndividualEventTriggerType::NoTrigger )
        , m_IdmDateTime()
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
        else if ( iid == GET_IID(INodeTriggeredInterventionConsumer)) 
            *ppvObject = static_cast<INodeTriggeredInterventionConsumer*>(this);

        if( *ppvObject != nullptr )
        {
            return QueryResult::s_OK ;
        }
        else
            return QueryResult::e_NOINTERFACE ;
    }

    virtual int32_t AddRef()  { return 10 ; }
    virtual int32_t Release() { return 10 ; }

    // -----------------------------------------------
    // --- INodeTriggeredInterventionConsumer Methods
    // -----------------------------------------------
    virtual void RegisterNodeEventObserver(   IIndividualEventObserver* NodeEventObserver, const IndividualEventTriggerType::Enum &trigger    ) {}
    virtual void UnregisterNodeEventObserver( IIndividualEventObserver* NodeEventObserver, const IndividualEventTriggerType::Enum &trigger    ) {}

    virtual void TriggerNodeEventObservers( IIndividualHumanEventContext* pIndiv, const IndividualEventTriggerType::Enum &StateChange)
    {
        m_TriggeredEvent = StateChange ;
    }

    virtual void RegisterNodeEventObserverByString( IIndividualEventObserver *pIEO, const std::string &trigger ) {};
    virtual void UnregisterNodeEventObserverByString( IIndividualEventObserver *pIEO, const std::string &trigger ) {};
    virtual void TriggerNodeEventObserversByString( IIndividualHumanEventContext *ihec, const std::string &trigger )
    {
        m_TriggeredEvent = (IndividualEventTriggerType::Enum)IndividualEventTriggerType::pairs::lookup_value( trigger.c_str() );
    };

    // ------------------------------
    // --- INodeEventContext Methods
    // ------------------------------
    virtual IdmDateTime GetTime() const
    {
        return m_IdmDateTime ;
    }

    virtual void VisitIndividuals(individual_visit_function_t func)
    {
        for( auto human : m_HumanList)
        {
            func(human->GetEventContext());
        }
    }

    virtual int VisitIndividuals(IVisitIndividual* pIndividualVisitImpl, int limit = -1) { throw std::exception("The method or operation is not implemented."); }

    virtual const NodeDemographics& GetDemographics() { throw std::exception("The method or operation is not implemented."); }

    virtual bool GetUrban() const       { throw std::exception("The method or operation is not implemented."); }
    //virtual float GetYear() const = 0;

    // to update any node-owned interventions
    virtual void UpdateInterventions(float = 0.0f) { throw std::exception("The method or operation is not implemented."); }
        
    // methods to install hooks for arrival/departure/birth/etc
    virtual void RegisterTravelDistributionSource(  ITravelLinkedDistributionSource *tles, TravelEventType type) { throw std::exception("The method or operation is not implemented."); }
    virtual void UnregisterTravelDistributionSource(ITravelLinkedDistributionSource *tles, TravelEventType type) { throw std::exception("The method or operation is not implemented."); }

    virtual const suids::suid & GetId() const                { throw std::exception("The method or operation is not implemented."); }
    virtual void SetContextTo(INodeContext* context)         { throw std::exception("The method or operation is not implemented."); }
    virtual void PurgeExisting( const std::string& iv_name ) { throw std::exception("The method or operation is not implemented."); }

    virtual std::list<INodeDistributableIntervention*> GetInterventionsByType(const std::string& type_name)         { throw std::exception("The method or operation is not implemented."); }
       
    virtual bool IsInPolygon(float* vertex_coords, int numcoords) { throw std::exception("The method or operation is not implemented."); }
    virtual bool IsInPolygon( const json::Array &poly )           { throw std::exception("The method or operation is not implemented."); }
    virtual bool IsInExternalIdSet( const tNodeIdList& nodelist ) { throw std::exception("The method or operation is not implemented."); }

    virtual ::RANDOMBASE* GetRng()         { throw std::exception("The method or operation is not implemented."); }
    virtual INodeContext* GetNodeContext() { throw std::exception("The method or operation is not implemented."); }

    virtual int GetIndividualHumanCount() const { return m_HumanList.size(); }
    virtual ExternalNodeId_t GetExternalId()  const { throw std::exception("The method or operation is not implemented."); }

    // -----------------
    // --- Other Methods
    // -----------------
    IndividualEventTriggerType::Enum GetTriggeredEvent() const { return m_TriggeredEvent ; }
    void ClearTriggeredEvent() { m_TriggeredEvent = IndividualEventTriggerType::NoTrigger; }

    void SetTime( const IdmDateTime& rTime )
    {
        m_IdmDateTime = rTime ;
    }
private:
    IndividualEventTriggerType::Enum m_TriggeredEvent ;
    IdmDateTime m_IdmDateTime ;
    std::vector<IIndividualHumanContext*> m_HumanList;
};