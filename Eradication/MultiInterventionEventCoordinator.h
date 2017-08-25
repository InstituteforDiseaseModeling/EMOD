/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "StandardEventCoordinator.h"

namespace Kernel
{
    class MultiInterventionEventCoordinator : public StandardInterventionDistributionEventCoordinator
    {
        DECLARE_FACTORY_REGISTERED_EXPORT(EventCoordinatorFactory, MultiInterventionEventCoordinator, IEventCoordinator)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

    public:
        MultiInterventionEventCoordinator();

    protected:
        virtual void initializeInterventionConfig( const Configuration * inputJson ) override;
        virtual void validateInterventionConfig( const json::Element& rElement, const std::string& rDataLocation ) override;
        virtual bool HasNodeLevelIntervention() const override;
        virtual void ExtractInterventionNameForLogging() override;
        virtual void InitializeInterventions() override;
        virtual void DistributeInterventionsToNodes( INodeEventContext* event_context ) override;
        virtual void DistributeInterventionsToIndividuals( INodeEventContext* event_context ) override;
        virtual bool DistributeInterventionsToIndividual( IIndividualHumanEventContext *ihec,
                                                          float & incrementalCostOut,
                                                          ICampaignCostObserver * pICCO ) override;

        std::vector<IDistributableIntervention *> m_IndividualInterventions;
    };
}
