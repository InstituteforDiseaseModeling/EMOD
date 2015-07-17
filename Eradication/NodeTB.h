/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once
#include "stdafx.h"

#include "NodeAirborne.h"
#include "IndividualTB.h" // for serialization junk
#include "TBContexts.h"

class ReportTB;

namespace Kernel
{
    class SpatialReportTB;

    class IInfectionIncidenceObserver
    {
    public:
        virtual void notifyOnInfectionIncidence (IndividualHumanTB * pIncident ) = 0;
        virtual void notifyOnInfectionMDRIncidence (IndividualHumanTB * pIncident ) = 0;
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
        bool Configure( const Configuration* config );

        virtual void SetupIntranodeTransmission();
        virtual void notifyOnInfectionIncidence ( IndividualHumanTB * pIncident );
        virtual void notifyOnInfectionMDRIncidence ( IndividualHumanTB * pIncident );
        virtual void resetNodeStateCounters(void);

        virtual void OnNewInfectionState(InfectionStateChange::_enum inf_state_change, IndividualHuman *ih);

        virtual IndividualHuman* addNewIndividual(
            float monte_carlo_weight = 1.0,
            float initial_age = 0,
            int gender = 0,
            int initial_infections = 0,
            float immunity_parameter = 1.0,
            float risk_parameter = 1.0,
            float migration_heterogeneity = 1.0,
            float poverty_parameter = 0);
        virtual void processEmigratingIndividual(IndividualHuman *i);
        virtual IndividualHuman* processImmigratingIndividual( IndividualHuman *immigrant );

        //for event observers going to reporter
        virtual float GetIncidentCounter() const;
        virtual float GetMDRIncidentCounter() const;
        virtual float GetMDREvolvedIncidentCounter() const;
        virtual float GetMDRFastIncidentCounter() const;

    protected:
        NodeTB();
        NodeTB(ISimulationContext *_parent_sim, suids::suid node_suid);

        void Initialize();

        // Factory methods
        virtual IndividualHuman *createHuman( suids::suid suid, float monte_carlo_weight, float initial_age, int gender, float above_poverty);
        const SimulationConfig* params();
        
        float incident_counter;
        float MDR_incident_counter;
        float MDR_evolved_incident_counter;
        float MDR_fast_incident_counter;

    private:
#if USE_BOOST_SERIALIZATION
        friend class boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive & ar, NodeTB& node, const unsigned int  file_version );
#endif
    };
}
