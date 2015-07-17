/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"

#include "StiTransmissionReporter.h"
#include "Exceptions.h"
#include "IIndividualHumanSTI.h"
#include "IRelationship.h"
#include "Infection.h"

static const char* _module = "StiTransmissionReporter";

namespace Kernel
{
    IReport* StiTransmissionReporter::Create(ISimulation* simulation)
    {
        return new StiTransmissionReporter(simulation);
    }

    StiTransmissionReporter::StiTransmissionReporter(ISimulation* sim)
        : BaseTextReport("TransmissionReport.csv")
        , simulation(sim)
        , report_data()
    {
        simulation->RegisterNewNodeObserver(this, [&](INodeContext* node){ this->onNewNode(node); });
    }

    StiTransmissionReporter::~StiTransmissionReporter()
    {
        simulation->UnregisterNewNodeObserver(this);
    }

    void StiTransmissionReporter::onNewNode(Kernel::INodeContext* node)
    {
        node->RegisterNewInfectionObserver(this, [&](IIndividualHuman* individual)
            {
                this->onTransmission(individual);
            }
        );
    }

    void StiTransmissionReporter::onTransmission(IIndividualHuman* individual)
    {
        LOG_DEBUG_F( "transmission event for individual %d\n", individual->GetSuid().data );
        StiTransmissionInfo info;

        IIndividualHumanSTI* p_sti_dest = nullptr;
        if (individual->QueryInterface(GET_IID(IIndividualHumanSTI), (void**)&p_sti_dest) != s_OK)
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "individual", "IIndividualHumanSTI", "IIndividualHuman" );
        }

        // Let's figure out who the Infector was. This info is stored in the HIVInfection's strainidentity object
        //auto infections = dynamic_cast<IInfectable*>(individual)->GetInfections();
        IInfectable* individual_infectable = nullptr;
        if (individual->QueryInterface(GET_IID(IInfectable), (void**)&individual_infectable) != s_OK)
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "individual", "IInfectable", "IIndividualHuman" );
        }
        auto infections = individual_infectable->GetInfections();
        if( infections.size() == 0 )
        {
            // This person cleared their infection on this timestep already! Nothing to report.
            return;
        }
        //release_assert( infections.size() > 0 );
        auto infection = infections.front(); // Assuming no super-infections here obviously.
        StrainIdentity si;
        infection->GetInfectiousStrainID(&si);
        // NOTE: Right now we re-use the Generic StrainIdentity object and store 
        // the InfectorID as the AntigenID (which is really just the arbitrary 
        // major id of the strain to my thinking). In the future, each disease 
        // could sub-class StrainIdentity (like we do with other major classes)
        // and put InfectorID in it's own field for HIV's purposes.
        auto infector = si.GetAntigenID();
        LOG_DEBUG_F( "Infector ID = %d\n", infector );
        if( infector == 0 )
        {
            // presumably from outbreak!?!?
            return;
        }

        bool found = false;

        auto& relationships = p_sti_dest->GetRelationships();
        if( (individual->GetStateChange() == HumanStateChange::DiedFromNaturalCauses) ||
            (individual->GetStateChange() == HumanStateChange::KilledByCoinfection  ) ||
            (individual->GetStateChange() == HumanStateChange::KilledByInfection    ) )
        {
             relationships = p_sti_dest->GetRelationshipsAtDeath();
        }
        release_assert( relationships.size() );

        IIndividualHumanSTI* p_sti_source = nullptr ;

        for (auto relationship : relationships)
        {
            auto sti_partner = relationship->GetPartner(p_sti_dest);
            if (sti_partner)
            {
                LOG_DEBUG_F( "found partner with id %d\n", sti_partner->GetSuid().data );
                if (sti_partner->GetSuid().data == infector )
                {
                    p_sti_source = sti_partner ;

                    IIndividualHuman* partner = nullptr;
                    if (sti_partner->QueryInterface(GET_IID(IIndividualHuman), (void**)&partner) != s_OK)
                    {
                        throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "sti_partner", "IIndividualHuman", "IIndividualHumanSTI");
                    }

                    info.rel_id                                     = relationship->GetSuid().data;
                    info.source_id                                  = partner->GetSuid().data;
                    info.source_is_infected                         = partner->IsInfected();
                    info.source_gender                              = partner->GetGender();
                    info.source_age                                 = partner->GetAge();

                    info.source_current_relationship_count          = p_sti_source->GetRelationships().size();
                    info.source_lifetime_relationship_count         = p_sti_source->GetLifetimeRelationshipCount();
                    info.source_relationships_in_last_6_months      = p_sti_source->GetLast6MonthRels();
                    info.source_extrarelational_flags               = p_sti_source->GetExtrarelationalFlags();
                    info.source_is_circumcised                      = p_sti_source->IsCircumcised();
                    info.source_is_superspreader                    = p_sti_source->IsBehavioralSuperSpreader();
                    info.source_has_sti                             = p_sti_source->HasSTICoInfection();

                    info.source_infection_age                       = -42;
                    break;
                }
            }
            else
            {
                LOG_WARN_F("%s: partner is absent (or deceased)\n", __FUNCTION__);
            }
        }

        release_assert( p_sti_source );

        info.time = individual->GetParent()->GetTime().time;
        info.year = individual->GetParent()->GetTime().Year();

        // DESTINATION
        info.destination_id                             = individual->GetSuid().data;
        info.destination_is_infected                    = individual->IsInfected();
        info.destination_gender                         = individual->GetGender();
        info.destination_age                            = individual->GetAge();

        info.destination_current_relationship_count     = p_sti_dest->GetRelationships().size();
        info.destination_lifetime_relationship_count    = p_sti_dest->GetLifetimeRelationshipCount();
        info.destination_relationships_in_last_6_months = p_sti_dest->GetLast6MonthRels();
        info.destination_extrarelational_flags          = p_sti_dest->GetExtrarelationalFlags();
        info.destination_is_circumcised                 = p_sti_dest->IsCircumcised();
        info.destination_has_sti                        = (p_sti_dest->GetCoInfectiveFactor() > 1.0f);
        info.destination_is_superspreader               = p_sti_dest->IsBehavioralSuperSpreader();
        info.destination_has_sti                        = false ;

        CollectOtherData( info.rel_id, p_sti_source, p_sti_dest );

        report_data.push_back(info);
    }

    std::string StiTransmissionReporter::GetHeader() const
    {
        std::stringstream header ;
        header 
            << "SIM_TIME" << ','
            << "YEAR" << ','
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
