/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <string>
#include <list>
#include <vector>

#include "BoostLibWrapper.h"

#include "EventCoordinator.h"
#include "Configure.h"
#include "NodeEventContext.h"
#include "DemographicRestrictions.h"

namespace Kernel
{
    struct ICampaignCostObserver; // TODO - Nasty fwd declaration because I'm scared to include NodeEventContext.h! :)

    // Standard distribution ec that just gives out the intervention once to the fraction of people specified by the coverage parameter
    class StandardInterventionDistributionEventCoordinator : public IEventCoordinator, public ITravelLinkedDistributionSource, public IVisitIndividual, public IEventCoordinator2, public JsonConfigurable
    {
        DECLARE_FACTORY_REGISTERED_EXPORT(EventCoordinatorFactory, StandardInterventionDistributionEventCoordinator, IEventCoordinator)

    public:
        DECLARE_CONFIGURED(StandardInterventionDistributionEventCoordinator)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

        StandardInterventionDistributionEventCoordinator( bool useDemographicCoverage = true );
        virtual ~StandardInterventionDistributionEventCoordinator() { }

        // IEventCoordinator
        virtual void SetContextTo(ISimulationEventContext *isec);
        virtual void CheckStartDay( float campaignStartDay ) const override {};

        virtual void AddNode( const suids::suid& node_suid);

        virtual void Update(float dt);
        virtual void UpdateNodes(float dt);
        virtual bool visitIndividualCallback(IIndividualHumanEventContext *ihec, float &incrementalCostOut, ICampaignCostObserver * pICCO );

        virtual bool IsFinished(); // returns true when the EC requires no further updates and can be disposed of

        virtual IEventCoordinatorEventContext* GetEventContext() override { return nullptr; }

        // ITravelLinkedDistributionSource
        virtual void ProcessDeparting(IIndividualHumanEventContext *dc);
        virtual void ProcessArriving(IIndividualHumanEventContext *dc);

        virtual float GetDemographicCoverage() const;
        virtual TargetDemographicType::Enum GetTargetDemographic() const;
        virtual float GetMinimumAge() const;
        virtual float GetMaximumAge() const;

        virtual bool qualifiesDemographically( const IIndividualHumanEventContext * pIndividual );

    protected:
        virtual float getDemographicCoverageForIndividual( const IIndividualHumanEventContext *pInd ) const;
        virtual void preDistribute(); 
        virtual void ExtractInterventionNameForLogging();
        virtual void InitializeInterventions();
        virtual void InitializeRepetitions( const Configuration* inputJson );
        virtual void CheckRepetitionConfiguration();
        virtual void UpdateRepetitions();
        virtual bool IsTimeToUpdate( float dt );
        virtual void DistributeInterventionsToNodes( INodeEventContext* event_context );
        virtual void DistributeInterventionsToIndividuals( INodeEventContext* event_context );
        virtual bool DistributeInterventionsToIndividual( IIndividualHumanEventContext *ihec,
                                                          float & incrementalCostOut,
                                                          ICampaignCostObserver * pICCO );

        ISimulationEventContext  *parent;
        bool distribution_complete;
        int num_repetitions;
        int tsteps_between_reps;
        int tsteps_since_last;
        //bool include_emigrants;
        //bool include_immigrants;
        int intervention_activated;
        InterventionConfig intervention_config;
        std::vector<INodeEventContext*> cached_nodes;
        std::vector<suids::suid> node_suids; // to help with serialization
        IDistributableIntervention *_di;
        DemographicRestrictions demographic_restrictions;
        float demographic_coverage;
        PropertyRestrictions<NPKey, NPKeyValue, NPKeyValueContainer> node_property_restrictions;

        std::ostringstream log_intervention_name;

        // helpers
        void regenerateCachedNodeContextPointers();
        void formatInterventionClassNames( std::ostringstream&, json::QuickInterpreter*);
        virtual bool TargetedIndividualIsCovered(IIndividualHumanEventContext *ihec);
        bool avoid_duplicates;
        bool has_node_level_intervention;

    };
}
