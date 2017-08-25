/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <string>
#include <list>

#include "IdmApi.h"
#include "BoostLibWrapper.h"

#include "ISupports.h"
#include "Types.h"
#include "Configuration.h"
#include "suids.hpp"
#include "RANDOM.h"
#include "IdmDateTime.h"
#include "Contexts.h"
#include "IndividualEventContext.h"
#include "Interventions.h" // for IIndividualHumanEventObserver

namespace Kernel
{
    struct NodeDemographics;
    class Node;
    class StrainIdentity;
    
    struct INodeContext;
    class IndividualHuman;

    /* possible TODO: could differentiate these in the future to provide extra information related to the travel
    struct IIndividualHumanTravelLinkedDistributionContext : public IIndividualHumanEventContext
    {

    };*/

    struct IDMAPI IOutbreakConsumer : public ISupports
    {
        virtual void AddImportCases(StrainIdentity* outbreak_strainID, float import_age, NaturalNumber num_cases_per_node ) = 0;
        //virtual void IncreasePrevalence(StrainIdentity* outbreak_strainID, IEventCoordinator2* pEC) = 0;
    };

    struct ITravelLinkedDistributionSource : ISupports
    {
        virtual void ProcessDeparting(IIndividualHumanEventContext *dc) = 0;
        virtual void ProcessArriving(IIndividualHumanEventContext *dc) = 0;
    };

    struct IVisitIndividual
    {
        virtual bool visitIndividualCallback( IIndividualHumanEventContext *ihec, float & incrementalCostOut, ICampaignCostObserver * pICCO ) = 0;
        virtual ~IVisitIndividual() {}
    };

    struct IDMAPI INodeEventContext : public ISupports 
    {
        // If you prefer lambda functions/functors, you can use this.
        typedef std::function<void (/*suids::suid, */IIndividualHumanEventContext*)> individual_visit_function_t;
        virtual void VisitIndividuals(individual_visit_function_t func) = 0;
        virtual int VisitIndividuals(IVisitIndividual* pIndividualVisitImpl, int limit = -1) = 0;

        virtual const NodeDemographics& GetDemographics() = 0;
        virtual bool GetUrban() const = 0;
        virtual IdmDateTime GetTime() const = 0;
        //virtual float GetYear() const = 0;

        // to update any node-owned interventions
        virtual void UpdateInterventions(float = 0.0f) = 0;
        
        // methods to install hooks for arrival/departure/birth/etc
        enum TravelEventType { Arrival = 0, Departure = 1 };
        virtual void RegisterTravelDistributionSource(ITravelLinkedDistributionSource *tles, TravelEventType type) = 0;
        virtual void UnregisterTravelDistributionSource(ITravelLinkedDistributionSource *tles, TravelEventType type) = 0;

        virtual const suids::suid & GetId() const = 0;
        virtual void SetContextTo(INodeContext* context) = 0;
        virtual std::list<INodeDistributableIntervention*> GetInterventionsByType(const std::string& type_name) = 0;
        virtual void PurgeExisting( const std::string& iv_name ) = 0;
       
        virtual bool IsInPolygon(float* vertex_coords, int numcoords) = 0;
        virtual bool IsInPolygon( const json::Array &poly ) = 0;
        virtual bool IsInExternalIdSet( const tNodeIdList& nodelist ) = 0;
        virtual ::RANDOMBASE* GetRng() = 0;
        virtual INodeContext* GetNodeContext() = 0;

        virtual int GetIndividualHumanCount() const = 0;
        virtual ExternalNodeId_t GetExternalId() const = 0;
    };

    class Simulation;
    struct IBTNTI;      // BasicTestNodeTargetedIntervention

    struct IBTNTIConsumer : public ISupports
    {
        virtual void GiveBTNTI(IBTNTI* BTNTI) = 0;
        virtual const IBTNTI* GetBTNTI() = 0; // look at the BTNTI they have...why not, some campaigns may want to introspect on this sort of thing. returns NULL if there is no BTNTI currently
    };
}
