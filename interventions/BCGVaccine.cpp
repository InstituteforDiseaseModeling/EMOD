/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#ifdef ENABLE_TB

#include "BCGVaccine.h"

#include "Common.h"                  // for DAYSPERYEAR
#include "IndividualEventContext.h"  // for GetAge
#include "RANDOM.h"                  // for ApplyVaccineTake random draw

SETUP_LOGGING( "BCGVaccine" )

namespace Kernel
{

#ifdef ENABLE_TB
    IMPLEMENT_FACTORY_REGISTERED(BCGVaccine)
    bool
    BCGVaccine::Configure(
        const Configuration * inputJson
    )
    {
        bool ret = SimpleVaccine::Configure( inputJson );
        vaccine_type = SimpleVaccineType::AcquisitionBlocking;
        return ret;
    }

    BCGVaccine::BCGVaccine()
    : SimpleVaccine()
    , vaccine_take_age_decay_rate(0)
    {
        initSimTypes( 2, "TB_SIM", "TBHIV_SIM" );
        initConfigTypeMap("Vaccine_Take_Age_Decay_Rate", &vaccine_take_age_decay_rate, BCG_Vaccine_Take_Age_Decay_Rate_DESC_TEXT );
    }

    bool BCGVaccine::ApplyVaccineTake( IIndividualHumanContext* pihc )
    {
        bool vaccine_took = true;
        if (!pihc)
        {
            // Trying to do BCGVaccine::ApplyVaccine with a NULL IndividualHumanContext (needed for age information).
            throw NullPointerException( __FILE__, __LINE__, __FUNCTION__, "pihc", "IndividualHumanContext" );
        }

        // Decay vaccine take with the age of the individual according to specified rate
        double age_in_years = pihc->GetEventContext()->GetAge() / DAYSPERYEAR;
        double fraction_of_take = exp( -1.0 * vaccine_take_age_decay_rate * age_in_years );
        double rand = pihc->GetRng()->e();
        if ( rand  > vaccine_take * fraction_of_take )
        {
            vaccine_took = false;
        }
        return vaccine_took;
    }

    REGISTER_SERIALIZABLE(BCGVaccine);

    void BCGVaccine::serialize(IArchive& ar, BCGVaccine* obj)
    {
        SimpleVaccine::serialize(ar, obj);
        BCGVaccine& vaccine = *obj;
        ar.labelElement("vaccine_take_age_decay_rate") & vaccine.vaccine_take_age_decay_rate;
    }
#endif
}

#endif // ENABLE_TB
