
#pragma once

#include "IReport.h"
#include "NodeSet.h"
#include "Interventions.h" //IIndividualEventObserver
#include "EventTrigger.h"
#include "ReportFilter.h"

namespace Kernel
{
    // BaseEventReport is the base class for a group of reports that collect
    // event oriented data.  This report is light weight and does NOT specify
    // what data is collected or how it is written out.  However, it does implement
    // the base code for registering and unregistering for event notification.
    class IDMAPI BaseEventReport : public IReport, 
                                   public IIndividualEventObserver
    {
    public:

        BaseEventReport( const std::string& rReportName, bool useHumanMinMaxAge, bool useHumanOther );
        virtual ~BaseEventReport();

        // -----------------------
        // --- IReport Methods
        // -----------------------
        virtual bool Configure( const Configuration* ) override;

        virtual std::string GetReportName() const override;
        virtual void Initialize( unsigned int nrmSize ) override;

        virtual void CheckForValidNodeIDs(const std::vector<ExternalNodeId_t>& nodeIds_demographics) override;
        virtual void UpdateEventRegistration( float currentTime, 
                                              float dt, 
                                              std::vector<INodeEventContext*>& rNodeEventContextList,
                                              ISimulationEventContext* pSimEventContext ) override;
        virtual void BeginTimestep() override;
        virtual bool IsCollectingIndividualData( float currentTime, float dt ) const override;
        virtual void LogIndividualData( Kernel::IIndividualHuman* individual ) override;
        virtual void LogNodeData( Kernel::INodeContext * pNC ) override;
        virtual void EndTimestep( float currentTime, float dt ) override;

        virtual void Reduce() override;
        virtual void Finalize() override;

        // -----------------------------
        // --- IIndividualEventObserver
        // -----------------------------
        virtual bool notifyOnEvent( Kernel::IIndividualHumanEventContext *context, 
                                    const EventTrigger& trigger ) override { return false; };

        // --------------
        // --- ISupports
        // --------------
        virtual Kernel::QueryResult QueryInterface(Kernel::iid_t iid, void **ppvObject) override { return Kernel::e_NOINTERFACE; }
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()

        // ----------
        // --- Other
        // ----------
        float GetStartDay() const;
        const std::vector< EventTrigger >& GetEventTriggerList() const;

        bool HaveRegisteredAllEvents()   const ;
        bool HaveUnregisteredAllEvents() const ;
    protected:
        virtual void ConfigureEvents( const Configuration* );
        virtual void CheckConfigurationEvents();
        void RegisterEvents( Kernel::INodeEventContext* pNEC );
        void UnregisterEvents( Kernel::INodeEventContext* pNEC );
        void UnregisterAllNodes();

        Kernel::INodeEventContext* GetFirstINodeEventContext();
        const std::vector<Kernel::INodeEventContext*>& GetNodeEventContextList() const { return nodeEventContextList; }

        std::string GetBaseOutputFilename() const ;

        std::string reportName ;
        ReportFilter report_filter;
        std::vector< EventTrigger > eventTriggerList ; // list of events to listen for
        bool events_registered ;       // true if events have been registered
        bool events_unregistered ;     // true if events have been unregistered
        std::vector<Kernel::INodeEventContext*> nodeEventContextList ; // list of nodes that events are registered with
    };

};
