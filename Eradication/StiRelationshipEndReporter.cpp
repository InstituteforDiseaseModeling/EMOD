/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"

#include <string>
#include "StiRelationshipEndReporter.h"
#include "Log.h"
#include "Exceptions.h"
#include "INodeSTI.h"
#include "NodeSTI.h"

static const char* _module = "RelationshipEndReporter";

using namespace std;

namespace Kernel
{
    IReport* StiRelationshipEndReporter::Create(ISimulation* simulation)
    {
        return new StiRelationshipEndReporter(simulation);
    }

    StiRelationshipEndReporter::StiRelationshipEndReporter(ISimulation* sim)
        : BaseTextReport("RelationshipEnd.csv")
        , simulation(sim)
        , report_data()
    {
        sim->RegisterNewNodeObserver(this, [&](INodeContext* node){ this->onNewNode(node); });
    }

    StiRelationshipEndReporter::~StiRelationshipEndReporter()
    {
        simulation->UnregisterNewNodeObserver(this);
    }

    void StiRelationshipEndReporter::onNewNode(Kernel::INodeContext* node)
    {
        INodeSTI* sti = nullptr;

        if (node->QueryInterface(GET_IID(INodeSTI), (void**)&sti) != s_OK)
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "node", "INodeSTI*", "INodeContext*" );
        }

        auto manager = sti->GetRelationshipManager();
        manager->RegisterRelationshipTerminationObserver([&](IRelationship* relationship){ this->onRelationshipTermination(relationship); });
    }

    void StiRelationshipEndReporter::onRelationshipTermination(IRelationship* relationship)
    {
        RelationshipEndInfo info;
        info.end_time = (float) simulation->GetSimulationTime().time; // current timestep
        info.start_time = relationship->GetStartTime();
        info.scheduled_end_time = relationship->GetScheduledEndTime();
        info.id = relationship->GetId();
        info.relationship_type = (unsigned int) relationship->GetType(); 
        info.male_id = relationship->GetMalePartnerId().data;
        info.female_id = relationship->GetFemalePartnerId().data;
        auto male_cast_as_base_class = dynamic_cast <IndividualHuman*> (relationship->MalePartner());
        info.male_age = male_cast_as_base_class->GetAge()/365;
        auto female_cast_as_base_class = dynamic_cast <IndividualHuman*> (relationship->FemalePartner());
        info.female_age = female_cast_as_base_class->GetAge()/365;
        report_data.push_back(info);
    }

    std::string StiRelationshipEndReporter::GetHeader() const
    {
        std::stringstream header ;
        header << "Rel_ID,"
               << "Rel_start_time,"
               << "Rel_scheduled_end_time,"
               << "Rel_actual_end_time,"
               << "Rel_type (0 = transitory 1 = informal 2 = marital),"
               << "male_ID,"
               << "female_ID,"
               << "male_age,"
               << "female_age" ;
        return header.str() ;
    }

    void StiRelationshipEndReporter::BeginTimestep()
    {
        LOG_DEBUG_F("%s\n", __FUNCTION__);
        report_data.clear();
    }

    void StiRelationshipEndReporter::EndTimestep( float currentTime, float dt )
    {
        LOG_DEBUG_F("%s\n", __FUNCTION__);
        // TODO - per time step data reduction (if multi-core)
        for (auto& entry : report_data)
        {
            GetOutputStream() << entry.id << ','
                              << entry.start_time << ','
                              << entry.scheduled_end_time << ','
                              << entry.end_time << ','
                              << entry.relationship_type << ','
                              << entry.male_id << ','
                              << entry.female_id << ','
                              << entry.male_age << ','
                              << entry.female_age
                              << endl;
        }

        BaseTextReport::EndTimestep( currentTime, dt );
    }
}
