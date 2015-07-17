/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include "Debug.h"
#include <string>
#include "IRelationship.h"

namespace Kernel
{
    struct ParticipantInfo
    {
        unsigned int id;
        bool is_infected;
        unsigned int gender;
        float age;
        unsigned int active_relationship_count;
        unsigned int cumulative_lifetime_relationships;
        unsigned int relationships_in_last_six_months;
        unsigned int extrarelational_flags;
        bool is_circumcised;
        bool has_sti;
        bool is_superspreader;
        unsigned int transitory_relationship_count;
        unsigned int informal_relationship_count;
        unsigned int marital_relationship_count;
        std::string props ;

        ParticipantInfo();
    };

    struct RelationshipStartInfo
    {
        unsigned int id;
        float start_time;
        float scheduled_end_time;
        unsigned int relationship_type;
        ParticipantInfo participant_a;
        ParticipantInfo participant_b;
        unsigned int original_node_id;

    };

    struct RelationshipEndInfo
    {
        float end_time;
        float start_time;
        float scheduled_end_time;        
        unsigned int id;
        unsigned int relationship_type;
        unsigned int male_id;
        unsigned int female_id;
        float male_age;
        float female_age;
    };

    struct CoitalActInfo
    {
        float time;
        std::string _line;
        virtual std::string GetHeader() const;
        virtual void GatherLineFromRelationship( const IRelationship* pRel );

        virtual std::string GetLine()
        {
            return _line;
        }
    };
}
