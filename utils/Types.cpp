/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

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
