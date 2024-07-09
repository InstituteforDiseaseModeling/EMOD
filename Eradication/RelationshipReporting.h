
#pragma once

#include "Debug.h"
#include <string>
#include "IRelationship.h"
#include "ExternalNodeId.h"

namespace Kernel
{
    struct ParticipantInfo
    {
        unsigned int id;
        bool is_infected;
        unsigned int gender;
        float age;
        unsigned int previous_coita_acts_count;
        unsigned int active_relationship_count;
        unsigned int cumulative_lifetime_relationships;
        unsigned int relationships_in_last_six_months;
        unsigned int extrarelational_flags;
        bool is_circumcised;
        bool has_sti;
        bool is_superspreader;
        unsigned int relationship_count[RelationshipType::COUNT];
        std::vector<std::string> props ;

        ParticipantInfo();
    };

    struct RelationshipStartInfo
    {
        unsigned int id;
        float start_time;
        float scheduled_end_time;
        unsigned int relationship_type;
        bool is_outside_pfa;
        ParticipantInfo participant_a;
        ParticipantInfo participant_b;
        ExternalNodeId_t original_node_id;
        ExternalNodeId_t current_node_id;
    };

    struct RelationshipEndInfo
    {
        float end_time;
        float start_time;
        float scheduled_end_time;
        unsigned int id;
        ExternalNodeId_t node_id;
        unsigned int relationship_type;
        bool is_outside_pfa;
        unsigned int male_id;
        unsigned int female_id;
        float male_age;
        float female_age;
        unsigned int num_total_coital_acts;
        unsigned int termination_reason;
    };
}
