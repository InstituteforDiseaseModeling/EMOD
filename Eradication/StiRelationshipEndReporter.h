
#pragma once

#include <unordered_map>
#include "BaseTextReport.h"
#include "ISimulation.h"
#include "RelationshipReporting.h"

namespace Kernel
{
    class StiRelationshipEndReporter : public BaseTextReport
    {
    public:
        static IReport* Create(ISimulation* simulation);

        // IReport
        virtual void BeginTimestep();
        virtual void EndTimestep( float currentTime, float dt );

    protected:
        StiRelationshipEndReporter(ISimulation* simulation);
        virtual ~StiRelationshipEndReporter();

        // BaseTextReport
        virtual std::string GetHeader() const ;

        void onNewNode(Kernel::INodeContext* node);
        void onRelationshipTermination(IRelationship* relationship);

        ISimulation* simulation;
        std::vector<RelationshipEndInfo> report_data;
        std::unordered_map<unsigned int,RelationshipEndInfo> paused_data;
    };
}