
#include "stdafx.h"
#include "Types.h"

act_prob_t::act_prob_t()
: num_acts(0)
, prob_per_act(0.0f)
{
}

void act_prob_t::serialize( Kernel::IArchive& ar, act_prob_t& ap )
{
    ar.startObject();
    ar.labelElement("num_acts"    ) & ap.num_acts;
    ar.labelElement("prob_per_act") & ap.prob_per_act;
    ar.endObject();
}
