/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"

#include "../interventions/IDrug.h"
#include "Log.h"
#include "Individual.h"
#include "ReportUtilities.h"

// Module name for logging
static const char * _module = "ReportUtilities"; 

using namespace Kernel;

std::list<IDrug*> ReportUtilities::GetDrugList( const IndividualHuman * individual, const std::string& rDrugClassName )
{
    IIndividualHumanInterventionsContext * intervs = individual->GetInterventionsContext();
    std::list<IDistributableIntervention*> idi_list = intervs->GetInterventionsByType( rDrugClassName );

    std::list<IDrug*> drugs_of_type;
    for (auto idi : idi_list)
    {
        IDrug * pDrug = NULL;
        if( s_OK == idi->QueryInterface(GET_IID(IDrug), (void**) &pDrug) )
        {
            drugs_of_type.push_back( pDrug );
        }
    }

    return drugs_of_type;
}

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
