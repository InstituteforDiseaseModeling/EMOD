/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "StiTransmissionReporter.h"
#include "Exceptions.h"
#include "IIndividualHumanSTI.h"
#include "IRelationship.h"
#include "IInfection.h"
#include "ReportUtilitiesSTI.h"
#include "IndividualEventContext.h"
#include "StrainIdentity.h"

SETUP_LOGGING( "StiTransmissionReporter" )

namespace Kernel
{
    IReport* StiTransmissionReporter::Create(ISimulation* simulation)
    {
        return new StiTransmissionReporter();
    }

    StiTransmissionReporter::StiTransmissionReporter()
        : BaseTextReportEvents("TransmissionReport.csv")
        , report_data()
    {
        eventTriggerList.push_back( "STINewInfection" );
    }

    StiTransmissionReporter::~StiTransmissionReporter()
    {
    }

    bool StiTransmissionReporter::notifyOnEvent( IIndividualHumanEventContext *context, const EventTrigger& trigger )
    {
        LOG_DEBUG_F( "transmission event for individual %d\n", context->GetSuid().data );
        StiTransmissionInfo info;

        IIndividualHuman* individual = nullptr;
        if (context->QueryInterface(GET_IID(IIndividualHuman), (void**)&individual) != s_OK)
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "IIndividualHuman", "IIndividualHumanEventContext" );
        }

        IIndividualHumanSTI* p_sti_dest = nullptr;
        if (individual->QueryInterface(GET_IID(IIndividualHumanSTI), (void**)&p_sti_dest) != s_OK)
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "individual", "IIndividualHumanSTI", "IIndividualHuman" );
        }

        IIndividualHumanSTI* p_sti_source = ReportUtilitiesSTI::GetTransmittingPartner( individual->GetEventContext() );
        if( p_sti_source == nullptr )
        {
            // no partner implies infection was result of outbreak or maternal transmission
            return true;
        }

        IIndividualHuman* partner = nullptr;
        if (p_sti_source->QueryInterface(GET_IID(IIndividualHuman), (void**)&partner) != s_OK)
        {
            throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "p_sti_source", "IIndividualHuman", "IIndividualHumanSTI");
        }

        auto& relationships = p_sti_dest->GetRelationships();
        if( (individual->GetStateChange() == HumanStateChange::DiedFromNaturalCauses) ||
            (individual->GetStateChange() == HumanStateChange::KilledByCoinfection  ) ||
            (individual->GetStateChange() == HumanStateChange::KilledByInfection    ) )
        {
            relationships = p_sti_dest->GetRelationshipsAtDeath();
        }
        release_assert( relationships.size() );

        IRelationship* p_rel = nullptr;
        for (auto relationship : relationships)
        {
            if( relationship->GetPartner( p_sti_dest ) == p_sti_source )
            {
                p_rel = relationship;
                break;
            }
        }
        release_assert( p_rel );

        release_assert( p_sti_source );

        info.time = individual->GetParent()->GetTime().time;
        info.year = individual->GetParent()->GetTime().Year();

        info.node_id = individual->GetParent()->GetExternalID();
        info.rel_id  = p_rel->GetSuid().data;

        // SOURCE
        info.source_id                                  = partner->GetSuid().data;
        info.source_is_infected                         = (partner->GetInfections().size() > 0); // see GitHub-589 - Remove m_is_infected
        info.source_gender                              = partner->GetGender();
        info.source_age                                 = partner->GetAge();

        info.source_current_relationship_count          = p_sti_source->GetRelationships().size();
        info.source_lifetime_relationship_count         = p_sti_source->GetLifetimeRelationshipCount();
        info.source_relationships_in_last_6_months      = p_sti_source->GetLast6MonthRels();
        info.source_extrarelational_flags               = p_sti_source->GetExtrarelationalFlags();
        info.source_is_circumcised                      = p_sti_source->IsCircumcised();
        info.source_has_sti                             = p_sti_source->HasSTICoInfection();
        info.source_is_superspreader                    = p_sti_source->IsBehavioralSuperSpreader();

        info.source_infection_age                       = -42;

        // DESTINATION
        info.destination_id                             = individual->GetSuid().data;
        info.destination_is_infected                    = (individual->GetInfections().size() > 0); // see GitHub-589 - Remove m_is_infected
        info.destination_gender                         = individual->GetGender();
        info.destination_age                            = individual->GetAge();

        info.destination_current_relationship_count     = p_sti_dest->GetRelationships().size();
        info.destination_lifetime_relationship_count    = p_sti_dest->GetLifetimeRelationshipCount();
        info.destination_relationships_in_last_6_months = p_sti_dest->GetLast6MonthRels();
        info.destination_extrarelational_flags          = p_sti_dest->GetExtrarelationalFlags();
        info.destination_is_circumcised                 = p_sti_dest->IsCircumcised();
        info.destination_has_sti                        = p_sti_dest->HasSTICoInfection();
        info.destination_is_superspreader               = p_sti_dest->IsBehavioralSuperSpreader();

        CollectOtherData( info.rel_id, p_sti_source, p_sti_dest );

        report_data.push_back(info);

        return true;
    }

    std::string StiTransmissionReporter::GetHeader() const
    {
        std::stringstream header ;
        header 
            << "SIM_TIME" << ','
            << "YEAR" << ','
            << "NODE_ID" << ','
            << "SRC_ID" << ','
            << "SRC_INFECTED" << ','
            << "SRC_GENDER" << ','
            << "SRC_AGE" << ','
            << "SRC_current_relationship_count" << ','
            << "SRC_lifetime_relationship_count" << ','
            << "SRC_relationships_in_last_6_months" << ','
            << "SRC_FLAGS" << ','
            << "SRC_CIRCUMSIZED" << ','
            << "SRC_STI" << ','
            << "SRC_SUPERSPREADER" << ','
            << "SRC_INF_AGE" << ','
            << "DEST_ID" << ','
            << "DEST_INFECTED" << ','
            << "DEST_GENDER" << ','
            << "DEST_AGE" << ','
            << "DEST_current_relationship_count" << ','
            << "DEST_lifetime_relationship_count" << ','
            << "DEST_relationships_in_last_6_months" << ','
            << "DEST_FLAGS" << ','
            << "DEST_CIRCUMSIZED" << ','
            << "DEST_STI" << ','
            << "DEST_SUPERSPREADER" ;
        return header.str();
    }

    void StiTransmissionReporter::BeginTimestep()
    {
        LOG_DEBUG_F("%s\n", __FUNCTION__);
        ClearData();
    }

    void StiTransmissionReporter::EndTimestep( float currentTime, float dt )
    {
        LOG_DEBUG_F("%s\n", __FUNCTION__);

        for (auto& entry : report_data)
        {
            GetOutputStream() 
                << entry.time << ','
                << entry.year << ','
                << entry.node_id << ','
                << entry.source_id << ','
                << entry.source_is_infected << ','
                << entry.source_gender << ','
                << entry.source_age << ','
                << entry.source_current_relationship_count << ','
                << entry.source_lifetime_relationship_count << ','
                << entry.source_relationships_in_last_6_months << ','
                << entry.source_extrarelational_flags << ','
                << entry.source_is_circumcised << ','
                << entry.source_has_sti << ','
                << entry.source_is_superspreader << ','
                << entry.source_infection_age << ','
                << entry.destination_id << ','
                << entry.destination_is_infected << ','
                << entry.destination_gender << ','
                << entry.destination_age << ','
                << entry.destination_current_relationship_count << ','
                << entry.destination_lifetime_relationship_count << ','
                << entry.destination_relationships_in_last_6_months << ','
                << entry.destination_extrarelational_flags << ','
                << entry.destination_is_circumcised << ','
                << entry.destination_has_sti << ','
                << entry.destination_is_superspreader
                << GetOtherData( entry.rel_id )
                << endl;
        }

        BaseTextReport::EndTimestep( currentTime, dt );
    }

    void StiTransmissionReporter::ClearData()
    {
        report_data.clear();
    }
}
