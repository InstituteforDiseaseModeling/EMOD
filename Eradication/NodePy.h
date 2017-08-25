/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once
#include "Node.h"
#include "IndividualPy.h"

class ReportPy;

namespace Kernel
{
    class SimulationConfig;
    class SpatialReportPy;

    class INodePy : public ISupports
    {
    public:
    };

    class NodePy : public Node, public INodePy
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

        // TODO Get rid of friending and provide accessors for all these floats
        friend class ::ReportPy;
        friend class Kernel::SpatialReportPy;

    public:
        static NodePy *CreateNode(ISimulationContext *_parent_sim, suids::suid node_suid);
        virtual ~NodePy(void);
        bool Configure( const Configuration* config );

        //virtual void SetupIntranodeTransmission();
        virtual void resetNodeStateCounters(void);
        virtual void updateNodeStateCounters(IndividualHuman *ih);
        virtual void finalizeNodeStateCounters(void);
        virtual std::map< std::string, float > GetTotalContagion() const;

    protected:
        NodePy();
        NodePy(ISimulationContext *_parent_sim, suids::suid node_suid);
        void Initialize();

        const SimulationConfig* params();

        // Factory methods
        virtual Kernel::IndividualHuman *createHuman(suids::suid suid, float monte_carlo_weight, float initial_age, int gender, float above_poverty);

        // wrap base-class function between creation and deletion of polio vaccine immunity initialization distributions.
        virtual void populateNewIndividualsFromDemographics(int count_new_individuals);
    };

    class NodePyTest : public NodePy
    {
        public:
            static NodePyTest *CreateNode(ISimulationContext *_parent_sim, suids::suid node_suid);
        protected:
            NodePyTest(ISimulationContext *_parent_sim, suids::suid node_suid);
        private:
    };
}
