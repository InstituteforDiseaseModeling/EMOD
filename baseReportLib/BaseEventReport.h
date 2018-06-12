/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "IdmApi.h"
#include "IReport.h"
#include "NodeSet.h"
#include "Interventions.h" //IIndividualEventObserver
#include "EventTrigger.h"


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

        BaseEventReport( const std::string& rReportName );
        virtual ~BaseEventReport();

        // -----------------------
        // --- IReport Methods
        // -----------------------
        virtual bool Configure( const Configuration* ) override;

        virtual std::string GetReportName() const override;
        virtual void Initialize( unsigned int nrmSize ) override;

        virtual void CheckForValidNodeIDs(const std::vector<ExternalNodeId_t>& nodeIds_demographics);
        virtual void UpdateEventRegistration( float currentTime, 
                                              float dt, 
                                              std::vector<INodeEventContext*>& rNodeEventContextList ) override;
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
        float GetDurationDays() const;
        const std::vector< EventTrigger >& GetEventTriggerList() const;

        bool HaveRegisteredAllEvents()   const ;
        bool HaveUnregisteredAllEvents() const ;
    protected:
        void RegisterEvents( Kernel::INodeEventContext* pNEC );
        void UnregisterEvents( Kernel::INodeEventContext* pNEC );
        void UnregisterAllNodes();
        Kernel::INodeTriggeredInterventionConsumer* GetNodeTriggeredConsumer( Kernel::INodeEventContext* pNEC );

        Kernel::INodeEventContext* GetFirstINodeEventContext();

        std::string GetBaseOutputFilename() const ;
    private:
#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        std::string reportName ;
        float startDay ;               // Day to register for events
        float durationDays ;           // Number of days to listen for events - unregister on day = startDay + durationDays
        std::string reportDescription; // Text to add to report name when creating file name - helps to distinguish from other instances
        Kernel::INodeSet *pNodeSet;    // Nodes to listen for events on
        Kernel::NodeSetConfig nodesetConfig;
        std::vector< EventTrigger > eventTriggerList ; // list of events to listen for
        bool events_registered ;       // true if events have been registered
        bool events_unregistered ;     // true if events have been unregistered
        std::vector<Kernel::INodeEventContext*> nodeEventContextList ; // list of nodes that events are registered with
#pragma warning( pop )
    };

};
