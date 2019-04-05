/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once
#include "NodeEnvironmental.h"
#include "IndividualTyphoid.h"
#include "TyphoidDefs.h"
#include "TyphoidInterventionsContainer.h"
#include <iostream>
#include <list>

class ReportTyphoid;

namespace Kernel
{
    class SimulationConfig;
    class SpatialReportTyphoid;

    class NodeTyphoid :
        public NodeEnvironmental
        //,public INodeTyphoid
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        //DECLARE_QUERY_INTERFACE()

        // TODO Get rid of friending and provide accessors for all these floats
        friend class ::ReportTyphoid;
        friend class Kernel::SpatialReportTyphoid;

    public:
        static NodeTyphoid *CreateNode(ISimulationContext *_parent_sim, ExternalNodeId_t externalNodeId, suids::suid node_suid);
        virtual ~NodeTyphoid(void);
        bool Configure( const Configuration* config );

        virtual void SetupIntranodeTransmission() override;
        virtual void resetNodeStateCounters(void);
        virtual void updateNodeStateCounters(IndividualHuman *ih);
        virtual void finalizeNodeStateCounters(void);

        virtual int calcGap() override;

    protected:
        NodeTyphoid();
        NodeTyphoid(ISimulationContext *_parent_sim, ExternalNodeId_t externalNodeId, suids::suid node_suid);
        virtual void Initialize() override;
        virtual void setupEventContextHost() override;

        const SimulationConfig* params();

        // Factory methods
        virtual Kernel::IndividualHuman *createHuman(suids::suid suid, float monte_carlo_weight, float initial_age, int gender);

        // wrap base-class function between creation and deletion of polio vaccine immunity initialization distributions.
        virtual void populateNewIndividualsFromDemographics(int count_new_individuals);

        virtual void computeMaxInfectionProb( float dt ) override;

    private:
    };
}
