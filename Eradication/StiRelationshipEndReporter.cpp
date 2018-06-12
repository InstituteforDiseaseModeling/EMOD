/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include <string>
#include "StiRelationshipEndReporter.h"
#include "Log.h"
#include "Exceptions.h"
#include "INodeSTI.h"
#include "NodeSTI.h"
#include "NodeEventContext.h"

SETUP_LOGGING( "RelationshipEndReporter" )

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
        , paused_data()
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
        IIndividualHuman* p_partner = nullptr;

        float male_age_years = -1.0;
        if( !relationship->IsMalePartnerAbsent() &&  (relationship->MalePartner() != nullptr) )
        {
            p_partner = dynamic_cast <IndividualHuman*>(relationship->MalePartner());
            male_age_years = p_partner->GetAge() / DAYSPERYEAR;
        }
        suids::suid male_id = relationship->GetMalePartnerId();

        float female_age_years = -1.0;
        if( !relationship->IsFemalePartnerAbsent() &&  (relationship->FemalePartner() != nullptr) )
        {
            p_partner = dynamic_cast <IndividualHuman*>(relationship->FemalePartner());
            female_age_years = p_partner->GetAge() / DAYSPERYEAR;
        }
        suids::suid female_id = relationship->GetFemalePartnerId();

        RelationshipEndInfo info;
        info.end_time           = float(simulation->GetSimulationTime().time); // current timestep
        info.start_time         = relationship->GetStartTime();
        info.scheduled_end_time = relationship->GetScheduledEndTime();
        info.id                 = relationship->GetSuid().data;
        info.node_id            = p_partner->GetParent()->GetExternalID();
        info.relationship_type  = (unsigned int) relationship->GetType(); 
        info.male_id            = male_id.data;
        info.female_id          = female_id.data;
        info.male_age           = male_age_years;
        info.female_age         = female_age_years;
        info.termination_reason = (unsigned int) relationship->GetTerminationReason(); 

        if( (male_age_years < 0) || (female_age_years < 0) )
        {
            if( paused_data.count( info.id ) == 0 )
            {
                paused_data.insert( std::make_pair( info.id, info ) );
            }
            else
            {
                RelationshipEndInfo partner_info = paused_data.at( info.id );
                if( info.male_age < 0 )
                {
                    info.male_age = partner_info.male_age;
                }
                if( info.female_age < 0 )
                {
                    info.female_age= partner_info.female_age;
                }
                paused_data.erase( info.id );
                report_data.push_back(info);
            }
        }
        else
        {
            report_data.push_back(info);
        }
    }

    std::string StiRelationshipEndReporter::GetHeader() const
    {
        std::stringstream header ;
        header << "Rel_ID,"
               << "Node_ID,"
               << "Rel_start_time,"
               << "Rel_scheduled_end_time,"
               << "Rel_actual_end_time,";

        header << "Rel_type (";
        for( int i = 0 ; i < RelationshipType::COUNT ; ++i )
        {
            header << i << " = " << RelationshipType::pairs::get_keys()[i];
            if( (i+1) < RelationshipType::COUNT )
            {
                header << "; ";
            }
        }
        header << "),";

        header 
               << "male_ID,"
               << "female_ID,"
               << "male_age,"
               << "female_age,"
               << "Termination_Reason";
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
            GetOutputStream() << entry.id                 << ','
                              << entry.node_id            << ','
                              << entry.start_time         << ','
                              << entry.scheduled_end_time << ','
                              << entry.end_time           << ','
                              << entry.relationship_type  << ','
                              << entry.male_id            << ','
                              << entry.female_id          << ','
                              << entry.male_age           << ','
                              << entry.female_age         << ','
                              << RelationshipTerminationReason::pairs::lookup_key( entry.termination_reason )
                              << endl;
        }

        BaseTextReport::EndTimestep( currentTime, dt );
    }
}
