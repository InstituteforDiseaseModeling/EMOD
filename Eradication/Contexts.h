/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

// put all contexts in one place to reduce clutter in includes
#pragma once
#include <list>
#include <vector>
#include "suids.hpp"
#include "ISupports.h"
#include "IdmDateTime.h"
#include "IInfectable.h"
#include "INodeContext.h"
#include "IdmApi.h"
#include "ISusceptibilityContext.h"

class RANDOMBASE;

namespace Kernel
{
    class IReport;
    class SimulationConfig;
    class IInterventionFactory;
    struct DemographicsContext;
    struct NodeDemographics;
    struct NodeDemographicsDistribution;
    struct INodeQualifier;
    class EventTrigger;

    ////////////////////////////////////////////////////////////////////////
    /* Design pattern for allowing access of child objects to methods of Simulation
    ISimulationContext: 
    interface to common simulation-context services
    provides ability for child objects to access common resources
    such as distributed uuids and shared class configurations (flags)
    */

    class IndividualHuman;

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
        virtual IdmDateTime GetSimulationTime() const = 0;

        // id services
        virtual suids::suid GetNextNodeSuid() = 0;
        virtual suids::suid GetNextIndividualHumanSuid() = 0;
        virtual suids::suid GetNextInfectionSuid() = 0;
        virtual suids::suid GetNodeSuid( ExternalNodeId_t external_node_id ) = 0 ;
        virtual ExternalNodeId_t GetNodeExternalID( const suids::suid& rNodeSuid ) = 0;
        virtual uint32_t    GetNodeRank( const suids::suid& rNodeSuid ) = 0;

        // random number services
        virtual RANDOMBASE* GetRng() = 0;

        // migration
        virtual void PostMigratingIndividualHuman(IIndividualHuman *i) = 0;
        virtual bool CanSupportFamilyTrips() const = 0;

        // events
        virtual void DistributeEventToOtherNodes( const EventTrigger& rEventTrigger, INodeQualifier* pQualifier ) = 0;
        virtual void UpdateNodeEvents() = 0;

        // reporting
        virtual std::vector<IReport*>& GetReports() = 0;
        virtual std::vector<IReport*>& GetReportsNeedingIndividualData() = 0;

    };

    class  Climate;
    struct INodeEventContext;

    struct IIndividualHumanInterventionsContext;
    struct IIndividualHumanEventContext;

    struct IIndividualHumanContext : ISupports
    {
        virtual suids::suid GetSuid() const = 0;

        virtual suids::suid GetNextInfectionSuid() = 0;
        virtual ::RANDOMBASE* GetRng() = 0; 

        virtual IIndividualHumanInterventionsContext *GetInterventionsContext() const = 0; // internal components of individuals interact with interventions via this interface
        
        virtual IIndividualHumanInterventionsContext *GetInterventionsContextbyInfection(Infection* infection) = 0; // internal components of individuals interact with interventions via this interface
        virtual IIndividualHumanEventContext *GetEventContext() = 0;                       // access to specific attributes of the individual useful for events
        virtual ISusceptibilityContext *GetSusceptibilityContext() const = 0;              // access to immune attributes useful for infection, interventions, reporting, etc.

        virtual const NodeDemographics* GetDemographics() const = 0;

        virtual void UpdateGroupMembership() = 0;
        virtual void UpdateGroupPopulation(float size_changes) = 0;

        virtual const std::string& GetPropertyReportString() const = 0;
        virtual void SetPropertyReportString( const std::string& str ) = 0;
    };

// helper macro for readability
#define randgen (Environment::getInstance()->RNG)
    typedef uint32_t tNodeId;
    typedef std::list< tNodeId > tNodeIdList;
}
