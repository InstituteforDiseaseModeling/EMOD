/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include "IdmApi.h"
#include "IReport.h"
#include "suids.hpp"
#include "NodeSet.h"
#include "Interventions.h" //IIndividualEventObserver


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
        virtual bool Configure( const Configuration* );

        virtual std::string GetReportName() const;
        virtual void Initialize( unsigned int nrmSize );

        virtual void UpdateEventRegistration( float currentTime, 
                                              float dt, 
                                              std::vector<INodeEventContext*>& rNodeEventContextList );
        virtual void BeginTimestep() ;
        virtual bool IsCollectingIndividualData( float currentTime, float dt ) const ;
        virtual void LogIndividualData( Kernel::IndividualHuman * individual );
        virtual void LogNodeData( Kernel::INodeContext * pNC );
        virtual void EndTimestep( float currentTime, float dt );

        virtual void Reduce();
        virtual void Finalize();

        // -----------------------------
        // --- IIndividualEventObserver
        // -----------------------------
        virtual bool notifyOnEvent( Kernel::IIndividualHumanEventContext *context, 
                                    const std::string& StateChange) { return false; };

#if USE_JSON_SERIALIZATION
        // For JSON serialization
        virtual void JSerialize( Kernel::IJsonObjectAdapter* root, Kernel::JSerializer* helper ) const;
        virtual void JDeserialize( Kernel::IJsonObjectAdapter* root, Kernel::JSerializer* helper );
#endif

        // --------------
        // --- ISupports
        // --------------
        virtual Kernel::QueryResult QueryInterface(Kernel::iid_t iid, void **ppvObject) { return Kernel::e_NOINTERFACE; }
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()

        // ----------
        // --- Other
        // ----------
        float GetStartDay() const;
        float GetDurationDays() const;
        const std::vector< std::string >& GetEventTriggerList() const;

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
        std::vector< std::string > eventTriggerList ; // list of events to listen for
        bool events_registered ;       // true if events have been registered
        bool events_unregistered ;     // true if events have been unregistered
        std::vector<Kernel::INodeEventContext*> nodeEventContextList ; // list of nodes that events are registered with
#pragma warning( pop )
    };

};
