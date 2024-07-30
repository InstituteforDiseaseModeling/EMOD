
#pragma once

#include <map>

#include "BaseTextReport.h"
#include "ReportMalaria.h"
#include "ReportFilter.h"

#ifndef _REPORT_DLL
#include "ReportFactory.h"
#endif

namespace Kernel
{
    class ReportMalariaFiltered : public ReportMalaria
    {
#ifndef _REPORT_DLL
        DECLARE_FACTORY_REGISTERED( ReportFactory, ReportMalariaFiltered, IReport )
#endif
    public:
        ReportMalariaFiltered();
        virtual ~ReportMalariaFiltered();

        // ISupports
        virtual QueryResult QueryInterface( iid_t iid, void** pinstance ) override;
        virtual int32_t AddRef() override { return ReportMalaria::AddRef(); }
        virtual int32_t Release() override { return ReportMalaria::Release(); }

        // ReportMalaria
        virtual bool Configure( const Configuration* ) override;
        virtual void Initialize( unsigned int nrmSize ) override;
        virtual void CheckForValidNodeIDs( const std::vector<ExternalNodeId_t>& nodeIds_demographics ) override;

        virtual void UpdateEventRegistration( float currentTime, 
                                              float dt, 
                                              std::vector<INodeEventContext*>& rNodeEventContextList,
                                              ISimulationEventContext* pSimEventContext ) override;
        virtual void BeginTimestep() override;
        virtual void EndTimestep( float currentTime, float dt ) override;
        virtual void LogIndividualData( IIndividualHuman* individual ) override;
        virtual bool IsCollectingIndividualData( float currentTime, float dt ) const override;
        virtual void LogNodeData( INodeContext* pNC ) override;
        virtual void postProcessAccumulatedData() override;
        virtual bool notifyOnEvent( IIndividualHumanEventContext *pEntity, const EventTrigger& trigger ) override;
        virtual std::string GetReportName() const override;

        virtual const char* GetParameterNameForHasInterventions() const override;
        virtual const char* GetDescTextForHasInterventions() const override;
        virtual const char* GetDependsOnForHasInterventions() const override;
        virtual const char* GetParameterNameForHasIP() const override;
        virtual const char* GetDescTextForHasIP() const override;
        virtual const char* GetDependsOnForHasIP() const override;
        virtual const char* GetParameterNameForIncludePregancies() const override;
        virtual const char* GetDescTextForIncludePregancies() const override;
        virtual const char* GetDependsOnForIncludePregancies() const override;
        virtual const char* GetParameterNameFor30DayAvg() const override;
        virtual const char* GetDescTextFor30DayAvg() const override;
        virtual const char* GetDependsOnFor30DayAvg() const override;

        virtual void populateSummaryDataUnitsMap( std::map<std::string, std::string> &units_map ) override;

    protected:
        virtual void BeginTimestepOther() {};
        virtual void LogIndividualDataOther( IIndividualHuman* individual ) {};
        virtual void LogNodeDataOther( INodeContext* pNC ) {};
        virtual void EndTimestepOther( float currentTime, float dt ) {};
        virtual void notifyOnEventOther( IIndividualHumanEventContext *pEntity, const EventTrigger& trigger ) {};
        virtual void postProcessAccumulatedDataOther() {};
        virtual void RemoveUnwantedChannels() {};

    private:
        ChannelID stat_pop_age_range_id;
        ReportFilter m_ReportFilter;
        bool m_IsValidDay;
        bool m_HasValidNode;
        bool m_HasCollectedData;
    };
}
