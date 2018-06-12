/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include <string>
#include "StiRelationshipConsummatedReporter.h"
#include "Log.h"
#include "Exceptions.h"
#include "INodeSTI.h"
#include "IIndividualHuman.h"
#include "INodeContext.h"
#include "IRelationship.h"
#include "RelationshipReporting.h"

SETUP_LOGGING( "RelationshipConsummatedReporter" )

using namespace std;

namespace Kernel
{
    IReport* StiRelationshipConsummatedReporter::Create(ISimulation* simulation)
    {
        return new StiRelationshipConsummatedReporter(simulation);
    }

    StiRelationshipConsummatedReporter::StiRelationshipConsummatedReporter(ISimulation* sim)
        : BaseTextReport("RelationshipConsummated.csv")
        , simulation(sim)
        , report_data()
    {
        sim->RegisterNewNodeObserver(this, [&](INodeContext* node){ this->onNewNode(node); });
    }

    StiRelationshipConsummatedReporter::~StiRelationshipConsummatedReporter()
    {
        simulation->UnregisterNewNodeObserver(this);
    }

    void StiRelationshipConsummatedReporter::onNewNode(Kernel::INodeContext* node)
    {
        INodeSTI* sti = nullptr;

        if (node->QueryInterface(GET_IID(INodeSTI), (void**)&sti) != s_OK)
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "node", "INodeSTI*", "INodeContext*" );
        }

        auto manager = sti->GetRelationshipManager();
        manager->RegisterRelationshipConsummationObserver([&](IRelationship* relationship){ this->onCoitalAct(relationship); });
    }

    void StiRelationshipConsummatedReporter::onCoitalAct(IRelationship* relationship)
    {
        auto info = CreateInfoObject();
        info->time = (float) simulation->GetSimulationTime().time; // current time
        info->GatherLineFromRelationship( relationship );
        report_data.push_back(info);
    }



    std::string StiRelationshipConsummatedReporter::GetHeader() const
    {
        auto tmpObjectForHeader = CreateInfoObject();
        std::string header = tmpObjectForHeader->GetHeader();
        delete tmpObjectForHeader;

        return header ;
    }

    void StiRelationshipConsummatedReporter::BeginTimestep()
    {
        LOG_DEBUG_F("%s\n", __FUNCTION__);
        report_data.clear();
    }

    void StiRelationshipConsummatedReporter::EndTimestep( float currentTime, float dt )
    {
        LOG_DEBUG_F("%s\n", __FUNCTION__);
        // TODO - per time step data reduction (if multi-core)
        for (auto& entry : report_data)
        {
            GetOutputStream() << entry->GetLine();
            delete entry;
        }

        BaseTextReport::EndTimestep( currentTime, dt );
    }

    CoitalActInfo * StiRelationshipConsummatedReporter::CreateInfoObject() const
    {
        return new CoitalActInfo();
    }
}
