/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

// put all contexts in one place to reduce clutter in includes
#pragma once
#include <vector>
#include "suids.hpp"
#include "ISupports.h"
#include "IdmApi.h"
#include "ExternalNodeId.h"

namespace Kernel
{
    class IReport;
    class SimulationConfig;
    class IInterventionFactory;
    struct DemographicsContext;
    struct INodeQualifier;
    struct IIndividualHuman;
    struct IdmDateTime;
    class EventTrigger;
    struct ISimulationEventContext;

    ////////////////////////////////////////////////////////////////////////
    /* Design pattern for allowing access of child objects to methods of Simulation
    ISimulationContext:
    interface to common simulation-context services
    provides ability for child objects to access common resources
    such as distributed uuids and shared class configurations (flags)
    */

    ////////////////////////////////////////////////////////////////////////
    // The design of IGlobalContext is following Encapsulated Context which has two purposes: 
    // 1. Delink the complex dependence of various components in DTK coming from report, intervention, and disease
    // 2. Provide the common instances and variables that are needed for those components
    struct IDMAPI IGlobalContext : public ISupports
    {
        virtual const SimulationConfig* GetSimulationConfigObj() const = 0;
        virtual const IInterventionFactory* GetInterventionFactory() const = 0;
    };

    struct IDMAPI ISimulationContext : public IGlobalContext
    {
        // demographics
        virtual const DemographicsContext* GetDemographicsContext() const = 0;

        // time services
        virtual const IdmDateTime& GetSimulationTime() const = 0;

        // id services
        virtual suids::suid GetNextInfectionSuid() = 0;
        virtual suids::suid GetNodeSuid( ExternalNodeId_t external_node_id ) = 0;
        virtual ExternalNodeId_t GetNodeExternalID( const suids::suid& rNodeSuid ) = 0;
        virtual uint32_t    GetNodeRank( const suids::suid& rNodeSuid ) = 0;

        // migration
        virtual void PostMigratingIndividualHuman( IIndividualHuman *i ) = 0;
        virtual bool CanSupportFamilyTrips() const = 0;

        // events
        virtual void DistributeEventToOtherNodes( const EventTrigger& rEventTrigger, INodeQualifier* pQualifier ) = 0;
        virtual void UpdateNodeEvents() = 0;
        virtual ISimulationEventContext* GetSimulationEventContext() = 0;

        // reporting
        virtual std::vector<IReport*>& GetReports() = 0;
        virtual std::vector<IReport*>& GetReportsNeedingIndividualData() = 0;
    };
}
