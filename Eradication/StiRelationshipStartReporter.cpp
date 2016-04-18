/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include <string>
#include "StiRelationshipStartReporter.h"
#include "Log.h"
#include "Exceptions.h"
#include "INodeSTI.h"
#include "Properties.h"
#include "NodeEventContext.h"
#include "IIndividualHumanSTI.h"

static const char* _module = "RelationshipStartReporter";

using namespace std;

namespace Kernel
{
    IReport* StiRelationshipStartReporter::Create(ISimulation* simulation)
    {
        return new StiRelationshipStartReporter(simulation);
    }

    StiRelationshipStartReporter::StiRelationshipStartReporter(ISimulation* sim)
        : BaseTextReport("RelationshipStart.csv")
        , simulation(sim)
        , report_data()
    {
        simulation->RegisterNewNodeObserver(this, [&](INodeContext* node){ this->onNewNode(node); });
    }

    StiRelationshipStartReporter::~StiRelationshipStartReporter()
    {
        simulation->UnregisterNewNodeObserver(this);
    }

    void StiRelationshipStartReporter::onNewNode(INodeContext* node)
    {
        INodeSTI* sti = nullptr;

        if (node->QueryInterface(GET_IID(INodeSTI), (void**)&sti) != s_OK)
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "node", "INodeSTI*", "INodeContext*" );
        }

        auto manager = sti->GetRelationshipManager();
        manager->RegisterNewRelationshipObserver([&](IRelationship* relationship){ this->onNewRelationship(relationship); });
    }

    void StiRelationshipStartReporter::onNewRelationship(IRelationship* relationship)
    {
        LOG_DEBUG_F("%s: rel id = %d, male id = %d, female id = %d\n", __FUNCTION__,
                    relationship->GetSuid().data,
                    relationship->MalePartner()->GetSuid().data,
                    relationship->FemalePartner()->GetSuid().data );
        // TODO - set the relationship suid in the relationship code (or relationship manager)
        auto male_partner = relationship->MalePartner();
        auto female_partner    = relationship->FemalePartner();

        if (male_partner && female_partner)
        {
            // get set of relationships in order to count the number for each type
            RelationshipSet_t &his_relationships =  male_partner->GetRelationships();
            RelationshipSet_t &her_relationships =  female_partner->GetRelationships();

            RelationshipStartInfo info;
            info.id                 = relationship->GetSuid().data;
            info.start_time         = relationship->GetStartTime();
            info.scheduled_end_time = relationship->GetScheduledEndTime();
            info.relationship_type  = (unsigned int)relationship->GetType();

            IIndividualHumanEventContext* individual = nullptr;

            if (male_partner->QueryInterface(GET_IID(IIndividualHumanEventContext), (void**)&individual) != s_OK)
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "male_partner", "IIndividualHumanContext", "IIndividualHumanSTI*" );
            }

            // --------------------------------------------------------
            // --- Assuming that the individuals in a relationship
            // --- must be in the same node.
            //release_assert( false );
            // --------------------------------------------------------
            info.original_node_id = relationship->GetOriginalNodeId();
            info.current_node_id  = individual->GetNodeEventContext()->GetNodeContext()->GetExternalID();

            info.participant_a.id                                = male_partner->GetSuid().data;
            info.participant_a.is_infected                       = male_partner->IsInfected();
            info.participant_a.gender                            = individual->GetGender();
            info.participant_a.age                               = individual->GetAge()/365;
            info.participant_a.active_relationship_count         = male_partner->GetRelationships().size();
            info.participant_a.props                             = GetPropertyString( individual ) ;

            info.participant_a.transitory_relationship_count     = 0;
            info.participant_a.informal_relationship_count       = 0;
            info.participant_a.marital_relationship_count        = 0;

            for (auto relationship : his_relationships)
            {
                switch (relationship->GetType())
                {
                case RelationshipType::TRANSITORY:
                    info.participant_a.transitory_relationship_count++;
                    break;

                case RelationshipType::INFORMAL:
                    info.participant_a.informal_relationship_count++;
                    break;

                case RelationshipType::MARITAL:
                    info.participant_a.marital_relationship_count++;
                    break;

                default:
                    //warning if it's none of these types?
                    break;
                }
            }

            /*for (std::set<IRelationship*, RelationshipSetSorter>::iterator rel_iter = his_relationships.begin();
                rel_iter != his_relationships.end();
                rel_iter = rel_iter++) {
                    switch (rel_iter->GetType())						
                    {
                    case RelationshipType::TRANSITORY:
            info.participant_a.transitory_relationship_count++
                        break;

                    case RelationshipType::INFORMAL:
            info.participant_a.informal_relationship_count++;
                        break;

                    case RelationshipType::MARITAL:
            info.participant_a.marital_relationship_count++;
                        break;

                    default:
                        // warning?
                        break;
                    }

            }; */


            info.participant_a.cumulative_lifetime_relationships = male_partner->GetLifetimeRelationshipCount();
            info.participant_a.relationships_in_last_six_months  = male_partner->GetLast6MonthRels();
            info.participant_a.extrarelational_flags             = male_partner->GetExtrarelationalFlags();
            info.participant_a.is_circumcised                    = male_partner->IsCircumcised();
            /*TODO*/ info.participant_a.has_sti                  = (male_partner->GetCoInfectiveFactor() > 1.0f);
            info.participant_a.is_superspreader                  = male_partner->IsBehavioralSuperSpreader();


            if (female_partner->QueryInterface(GET_IID(IIndividualHumanEventContext), (void**)&individual) != s_OK)
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "female_partner", "IIndividualHumanContext", "IIndividualHumanSTI*" );
            }

            info.participant_b.id                                = female_partner->GetSuid().data;
            info.participant_b.is_infected                       = female_partner->IsInfected();
            info.participant_b.gender                            = individual->GetGender();
            info.participant_b.age                               = individual->GetAge()/365;
            info.participant_b.active_relationship_count         = female_partner->GetRelationships().size();
            info.participant_b.props                             = GetPropertyString( individual ) ;

            info.participant_b.transitory_relationship_count     = 0;
            info.participant_b.informal_relationship_count       = 0;
            info.participant_b.marital_relationship_count        = 0;

            for (auto relationship : her_relationships)
            {
                switch (relationship->GetType())
                {
                case RelationshipType::TRANSITORY:
                    info.participant_b.transitory_relationship_count++;
                    break;

                case RelationshipType::INFORMAL:
                    info.participant_b.informal_relationship_count++;
                    break;

                case RelationshipType::MARITAL:
                    info.participant_b.marital_relationship_count++;
                    break;

                default:
                    //warning if it's none of these types?
                    break;
                }
            }

            info.participant_b.cumulative_lifetime_relationships = female_partner->GetLifetimeRelationshipCount();
            info.participant_b.relationships_in_last_six_months  = female_partner->GetLast6MonthRels();
            info.participant_b.extrarelational_flags             = female_partner->GetExtrarelationalFlags();
            info.participant_b.is_circumcised                    = female_partner->IsCircumcised();
            /*TODO*/ info.participant_b.has_sti                  = (female_partner->GetCoInfectiveFactor() > 1.0f);
            info.participant_b.is_superspreader                  = female_partner->IsBehavioralSuperSpreader();

            CollectOtherData( info.id, male_partner, female_partner );

            report_data.push_back(info);
        }
        else
        {
            LOG_WARN_F( "%s: one or more partners of new relationship %d has already migrated\n", __FUNCTION__, relationship->GetSuid().data );
        }
    }

    std::string StiRelationshipStartReporter::GetHeader() const
    {
        std::stringstream header ;
        header 
            << "Rel_ID,"
            << "Rel_start_time,"
            << "Rel_scheduled_end_time,"
            << "Rel_type (0 = transitory 1 = informal 2 = marital),"
            << "Original_node_ID,"
            << "Current_node_ID,"
            << "A_ID,"
            << "A_is_infected,"
            << "A_gender,"
            << "A_age,"
            << "A_total_num_active_rels,"
            << "A_num_active_transitory_rels,"
            << "A_num_active_informal_rels,"
            << "A_num_active_marital_rels,"
            << "A_num_lifetime_rels,"
            << "A_num_rels_last_6_mo,"
            << "A_extra_relational_bitmask,"
            << "A_is_circumcised,"
            << "A_has_STI_coinfection,"
            << "A_is_superspreader,"
            << "B_ID,"
            << "B_is_infected,"
            << "B_gender,"
            << "B_age,"
            << "B_total_num_active_rels,"
            << "B_num_active_transitory_rels,"
            << "B_num_active_informal_rels,"
            << "B_num_active_marital_rels,"
            << "B_num_lifetime_rels,"
            << "B_num_rels_last_6_mo,"
            << "B_extra_relational_bitmask,"
            << "B_is_circumcised,"
            << "B_has_STI_coinfection,"
            << "B_is_superspreader,"
            << "A_IndividualProperties,"
            << "B_IndividualProperties" ;
        return header.str();
    }

    void StiRelationshipStartReporter::BeginTimestep()
    {
        LOG_DEBUG_F("%s\n", __FUNCTION__);
        ClearData();
    }

    void StiRelationshipStartReporter::EndTimestep( float currentTime, float dt )
    {
        LOG_DEBUG_F("%s\n", __FUNCTION__);
        // TODO - per time step data reduction (if multi-core)
        for (auto& entry : report_data)
        {
            GetOutputStream() << entry.id << ','
                              << entry.start_time << ','
                              << entry.scheduled_end_time << ','
                              << entry.relationship_type << ','
                              << entry.original_node_id << ','
                              << entry.current_node_id << ','
                              << entry.participant_a.id << ','
                              << entry.participant_a.is_infected << ','
                              << entry.participant_a.gender << ','
                              << entry.participant_a.age << ','
                              << entry.participant_a.active_relationship_count << ','
                              << entry.participant_a.transitory_relationship_count << ','
                              << entry.participant_a.informal_relationship_count << ','
                              << entry.participant_a.marital_relationship_count << ','
                              << entry.participant_a.cumulative_lifetime_relationships << ','
                              << entry.participant_a.relationships_in_last_six_months << ','
                              << entry.participant_a.extrarelational_flags << ','
                              << entry.participant_a.is_circumcised << ','
                              << entry.participant_a.has_sti << ','
                              << entry.participant_a.is_superspreader << ','
                              << entry.participant_b.id << ','
                              << entry.participant_b.is_infected << ','
                              << entry.participant_b.gender << ','
                              << entry.participant_b.age << ','
                              << entry.participant_b.active_relationship_count << ','
                              << entry.participant_b.transitory_relationship_count << ','
                              << entry.participant_b.informal_relationship_count << ','
                              << entry.participant_b.marital_relationship_count << ','
                              << entry.participant_b.cumulative_lifetime_relationships << ','
                              << entry.participant_b.relationships_in_last_six_months << ','
                              << entry.participant_b.extrarelational_flags << ','
                              << entry.participant_b.is_circumcised << ','
                              << entry.participant_b.has_sti << ','
                              << entry.participant_b.is_superspreader << ','
                              << entry.participant_a.props << ','
                              << entry.participant_b.props 
                              << GetOtherData( entry.id )
                              << endl;
        }

        BaseTextReport::EndTimestep( currentTime, dt );
    }

    void StiRelationshipStartReporter::ClearData()
    {
        report_data.clear();
    }

    std::string StiRelationshipStartReporter::GetPropertyString( IIndividualHumanEventContext* individual )
    {
        std::string propertyString ;
        for (const auto& entry : *(individual->GetProperties()) )
        {
            const std::string& key   = entry.first;
            const std::string& value = entry.second;

            // ------------------------------------------------------------------
            // --- Do not include the auto-generated Relationship property.
            // --- (If find() returns zero, then the key stats with "Relationship")
            // ------------------------------------------------------------------
            if( key.find("Relationship") != 0 )
            {
                propertyString += key + "-" + value + ";" ;
            }
        }

        if( propertyString.empty() )
        {
            propertyString = "None" ;
        }
        else
        {
#ifdef WIN32
            propertyString.pop_back();
#else
            propertyString.resize(propertyString.size() - 1);
#endif
        }
        return propertyString ;
    }

}
