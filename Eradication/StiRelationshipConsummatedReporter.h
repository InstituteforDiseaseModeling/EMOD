/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "BaseTextReport.h"
#include "ISimulation.h"
#include "RelationshipReporting.h"

namespace Kernel
{
    class StiRelationshipConsummatedReporter : public BaseTextReport
    {
    public:
        static IReport* Create(ISimulation* simulation);

        // IReport
        virtual void BeginTimestep();
        virtual void EndTimestep( float currentTime, float dt );

    protected:
        StiRelationshipConsummatedReporter(ISimulation* simulation);
        virtual ~StiRelationshipConsummatedReporter();

        // BaseTextReport
        virtual std::string GetHeader() const ;

        virtual CoitalActInfo* CreateInfoObject() const;

        void onNewNode(Kernel::INodeContext* node);
        void onCoitalAct(IRelationship* relationship);

        ISimulation* simulation;
        std::vector<CoitalActInfo*> report_data;
    };
}
