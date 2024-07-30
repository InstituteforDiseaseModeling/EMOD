
#include "stdafx.h"

#include "RelationshipReporting.h"
#include "Log.h"

SETUP_LOGGING( "RelationshipReporting" )

namespace Kernel
{
    ParticipantInfo::ParticipantInfo()
        : id(0)
        , is_infected(false)
        , gender(-1)
        , age(0)
        , active_relationship_count(0)
        , cumulative_lifetime_relationships(0)
        , relationships_in_last_six_months(0)
        , extrarelational_flags(0)
        , is_circumcised( false )
        , has_sti( false )
        , is_superspreader( false )
        , props()
    {}

}
