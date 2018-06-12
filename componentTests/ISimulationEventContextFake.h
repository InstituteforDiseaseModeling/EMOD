/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "Contexts.h"
#include "SimulationEventContext.h"

using namespace Kernel;

class ISimulationEventContextFake : public ISimulationEventContext
{
public:
    ISimulationEventContextFake()
        : ISimulationEventContext()
        , m_IdmDateTime()
    {
    }

    ~ISimulationEventContextFake()
    {
    }

    // ---------------------
    // --- ISupport Methods
    // ---------------------
    virtual QueryResult QueryInterface(iid_t iid, void **ppvObject)
    {
        *ppvObject = nullptr ;
        if ( iid == GET_IID(ISimulationEventContext)) 
            *ppvObject = static_cast<ISimulationEventContext*>(this);

        if( *ppvObject != nullptr )
        {
            return QueryResult::s_OK ;
        }
        else
            return QueryResult::e_NOINTERFACE ;
    }

    virtual int32_t AddRef()  { return 10 ; }
    virtual int32_t Release() { return 10 ; }

    // ------------------------------------
    // --- ISimulationEventContext Methods 
    // ------------------------------------

    INodeEventContext* GetNodeEventContext( suids::suid node_id )
    {
        return m_Map.at( node_id );
    }

    void VisitNodes(node_visit_function_t func)                 { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    void RegisterEventCoordinator(IEventCoordinator* iec)       { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    int GetSimulationTimestep() const                           { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    RANDOMBASE* GetRng()                                        { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }

    IdmDateTime GetSimulationTime() const
    {
        return m_IdmDateTime;
    }

    // -----------------
    // --- Other Methods
    // -----------------
    void SetTime( const IdmDateTime& rTime )
    {
        m_IdmDateTime = rTime ;
    }

    void AddNode( INodeEventContext* pNEC )
    {
        m_Map.insert( std::make_pair( pNEC->GetId(), pNEC ) );
    }

private:
    IdmDateTime m_IdmDateTime ;
    std::map<suids::suid,INodeEventContext*> m_Map;
};
