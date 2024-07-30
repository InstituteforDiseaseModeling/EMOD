
#pragma once

#include "BaseTextReport.h"
#include "ReportFactory.h"

namespace Kernel
{
    struct ISimulationContext;

    // ReportSimulationStats collects data on the simulation to help understand the performance
    // of the simulation.  This will include data on the system such as memory and duration as
    // well as number of people, interventions, and infections.
    class ReportSimulationStats : public BaseTextReport
    {
        DECLARE_FACTORY_REGISTERED( ReportFactory, ReportSimulationStats, IReport )
    public:
        ReportSimulationStats();
        virtual ~ReportSimulationStats();

        // ISupports
        virtual QueryResult QueryInterface( iid_t iid, void** pinstance ) override;
        virtual int32_t AddRef() override { return BaseTextReport::AddRef(); }
        virtual int32_t Release() override { return BaseTextReport::Release(); }

        // IReport
        virtual bool IsCollectingIndividualData( float currentTime, float dt ) const { return true; };
        virtual void LogIndividualData( IIndividualHuman* individual ) override;
        virtual void LogNodeData( INodeContext* pNC ) override;
        virtual void EndTimestep( float currentTime, float dt );

    protected:
        // BaseTextReport
        virtual std::string GetHeader() const override ;

        virtual void ClearData();
        virtual std::string GetOtherData();

        ISimulationContext* m_pSim;

        time_t m_InitialTimeSim;
        time_t m_InitialTimeStep;
        uint64_t m_NumNodes;
        uint64_t m_NumPeople;
        uint64_t m_NumInterventionsPersisted;
        uint64_t m_NumInterventionsAdded;
        uint64_t m_NumInfections;
        uint64_t m_NumEventsTriggered;
        uint64_t m_NumEventsObserved;
    };
}
