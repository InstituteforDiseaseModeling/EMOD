/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "../interventions/IDrug.h"
#include "Log.h"
#include "IIndividualHuman.h"
#include "Interventions.h"
#include "ReportUtilities.h"

// Module name for logging
static const char * _module = "ReportUtilities"; 

using namespace Kernel;

int ReportUtilities::GetAgeBin( float age, std::vector<float>& rAges )
{
    float age_years = age / DAYSPERYEAR ;
    if( age_years > rAges[rAges.size()-1] )
    {
        LOG_WARN_F("Age_Bins not large enough for population, found age(years)=%f and Age_Bin.end=%f.  Putting in last bin.\n", age_years, rAges[rAges.size()-1] );
        return rAges.size()-1 ;
    }
    else
    {
        vector<float>::const_iterator it;
        it = std::lower_bound(rAges.begin(), rAges.end(), age_years );
        int agebin_idx = it - rAges.begin();
        return agebin_idx;
    }
}
