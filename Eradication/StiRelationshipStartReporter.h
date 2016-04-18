/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "BaseTextReport.h"
#include "ISimulation.h"
#include "RelationshipReporting.h"

namespace Kernel
{
    struct  IIndividualHumanSTI ;
    struct  IIndividualHumanEventContext ;

    class StiRelationshipStartReporter : public BaseTextReport
    {
    public:
        static IReport* Create(ISimulation* simulation);

        // IReport
        virtual void BeginTimestep();
        virtual void EndTimestep( float currentTime, float dt );

    protected:
        StiRelationshipStartReporter(ISimulation* simulation);
        virtual ~StiRelationshipStartReporter();

        // BaseTextReport
        virtual std::string GetHeader() const ;

        void onNewNode(INodeContext* node);
        void onNewRelationship(IRelationship* relationship);

        virtual void ClearData();
        virtual void CollectOtherData( unsigned int relationshipID,
                                       IIndividualHumanSTI* pPartnerA,
                                       IIndividualHumanSTI* pPartnerB ) {} ;
        virtual std::string GetOtherData( unsigned int relationshipID ) { return ""; };

        std::string GetPropertyString( IIndividualHumanEventContext* individual );

        ISimulation* simulation;
        std::vector<RelationshipStartInfo> report_data;
    };
}
