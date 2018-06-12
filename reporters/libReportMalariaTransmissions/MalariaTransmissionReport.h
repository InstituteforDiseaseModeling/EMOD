/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <vector>
#include <map>

#include "ReportHelpers.h"
#include "BaseEventReport.h"
#include "IVectorMigrationReporting.h"
#include "INodeContext.h"

namespace Kernel
{
    struct IVectorCohortIndividual;
    struct MigratingVector
    {
        // Helper struct to hold a few variables for migrating vectors
        uint64_t id;
        ExternalNodeId_t from_node_id;
        ExternalNodeId_t to_node_id;

        MigratingVector(uint64_t id, ExternalNodeId_t from_node_id, ExternalNodeId_t to_node_id);
        MigratingVector(IVectorCohortIndividual* pivci, ISimulationContext* pSim, const suids::suid& nodeSuid);
    };

    class MalariaTransmissionReport : public BaseEventReport, public IVectorMigrationReporting
    {
    public:
        MalariaTransmissionReport();
        virtual ~MalariaTransmissionReport();

        // BaseEventReport
        virtual bool Configure( const Configuration* ) override;
        virtual bool notifyOnEvent( IIndividualHumanEventContext *context, const EventTrigger& trigger) override;
        virtual bool IsCollectingIndividualData( float currentTime, float dt ) const override;
        virtual void LogIndividualData( IIndividualHuman* individual ) override;
        virtual void LogNodeData( INodeContext * pNC ) override;
        virtual void EndTimestep( float currentTime, float dt ) override;

        // IVectorMigrationReporting
        virtual void LogVectorMigration(ISimulationContext* pSim, 
        float currentTime, const suids::suid& nodeSuid, IVectorCohort* pivc) override;

    protected:
        InfectiousMosquitos_t BufferInfectiousVectors( INodeEventContext *context );
        InfectiousMosquitos_t BufferInfectiousVectors( INodeContext *pNC );

        bool IsActive() const;
        bool IsFinished() const;

        void SerializeTransmissions(IJsonObjectAdapter& pIJsonObj, JSerializer& js);
        void WriteOutput( float currentTime );

        bool m_PrettyFormat;
        bool outputWritten;

        int timeStep;

        std::vector<Transmission*> transmissions;
        std::vector<ClinicalSample*> samples;

        std::map<ExternalNodeId_t, InfectiousMosquitos_t> infectious_mosquitos_current;
        std::map<ExternalNodeId_t, InfectiousMosquitos_t> infectious_mosquitos_previous;

        std::map<InfectedMosquito_t, Location> infected_mosquito_buffer;
        std::map<Location, InfectiousReservoir_t> infected_human_buffer;
    };
}