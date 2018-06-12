/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once
#include "stdafx.h"

#include "NodeAirborne.h"
#include "IndividualCoInfection.h" // for serialization junk
#include "TBContexts.h"
#ifdef ENABLE_TBHIV
#include "IndividualCoInfection.h"
#endif

class ReportTB;

namespace Kernel
{
    class SpatialReportTB;

    class IInfectionIncidenceObserver
    {
    public:
        virtual ~IInfectionIncidenceObserver() { }
    };
    class NodeTB : public NodeAirborne, public IInfectionIncidenceObserver, public INodeTB
    {
        // TODO: Get rid of friending and provide accessors.
        friend class ::ReportTB;
        friend class SpatialReportTB;
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

    public:
        virtual ~NodeTB(void);
        static NodeTB *CreateNode(ISimulationContext *_parent_sim, suids::suid node_suid);
        virtual bool Configure( const Configuration* config ) override;

        virtual ITransmissionGroups* CreateTransmissionGroups() override;
        virtual void BuildTransmissionRoutes( RouteToContagionDecayMap_t& rDecayMap ) override;
        virtual void resetNodeStateCounters(void) override;

        virtual void OnNewInfectionState(InfectionStateChange::_enum inf_state_change, IndividualHuman *ih);

        virtual IIndividualHuman* addNewIndividual(
            float monte_carlo_weight = 1.0,
            float initial_age = 0,
            int gender = 0,
            int initial_infections = 0,
            float immunity_parameter = 1.0,
            float risk_parameter = 1.0,
            float migration_heterogeneity = 1.0) override;

        //for event observers going to reporter
        virtual float GetIncidentCounter() const override;
        virtual float GetMDRIncidentCounter() const override;
        virtual float GetMDREvolvedIncidentCounter() const override;
        virtual float GetMDRFastIncidentCounter() const override;

    protected:
        NodeTB();
        NodeTB(ISimulationContext *_parent_sim, suids::suid node_suid);

        virtual void Initialize() override;

        // Factory methods
        virtual IIndividualHuman* createHuman( suids::suid suid, float monte_carlo_weight, float initial_age, int gender) override;

        float incident_counter;
        float MDR_incident_counter;
        float MDR_evolved_incident_counter;
        float MDR_fast_incident_counter;

        DECLARE_SERIALIZABLE(NodeTB);
    };
}
