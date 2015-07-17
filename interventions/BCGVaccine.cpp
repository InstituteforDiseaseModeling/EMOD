/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"

#ifdef ENABLE_TB

#include "BCGVaccine.h"

#include "Common.h"                  // for DAYSPERYEAR
#include "IndividualEventContext.h"  // for GetAge
#include "RANDOM.h"                  // for ApplyVaccineTake random draw

static const char * _module = "BCGVaccine";

namespace Kernel
{

#ifdef ENABLE_TB
    IMPLEMENT_FACTORY_REGISTERED(BCGVaccine)
    /*QuickBuilder BCGVaccine::GetSchema()
    {
        return QuickBuilder( jsonSchemaBase );
    }*/

    bool
    BCGVaccine::Configure(
        const Configuration * inputJson
    )
    {
        return SimpleVaccine::Configure( inputJson );
    }

    BCGVaccine::BCGVaccine()
    : SimpleVaccine()
    , vaccine_take_age_decay_rate(0)
    {
        initSimTypes( 1, "TB_SIM" );
        initConfigTypeMap("Vaccine_Take_Age_Decay_Rate", &vaccine_take_age_decay_rate, BCG_Vaccine_Take_Age_Decay_Rate_DESC_TEXT );
    }

    void BCGVaccine::ApplyVaccineTake()
    {
        if (!parent)
        {
            // Trying to do BCGVaccine::ApplyVaccine with a NULL IndividualHumanContext (needed for age information).
            throw NullPointerException( __FILE__, __LINE__, __FUNCTION__, "parent", "IndividualHumanContext" );
        }

        // Decay vaccine take with the age of the individual according to specified rate
        double age_in_years = parent->GetEventContext()->GetAge() / DAYSPERYEAR;
        double fraction_of_take = exp( -1.0 * vaccine_take_age_decay_rate * age_in_years );
        double rand = parent->GetRng()->e();
        if ( rand  > vaccine_take * fraction_of_take )
        {
            current_reducedacquire = 0.0;
            current_reducedtransmit = 0.0;
        }
    }
#endif
}

// Boo-hoo, would rather have in main serialization block.
#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
BOOST_CLASS_EXPORT(Kernel::BCGVaccine)
namespace Kernel {
    template<class Archive>
    void serialize(Archive &ar, BCGVaccine& vacc, const unsigned int v)
    {
        boost::serialization::void_cast_register<BCGVaccine, IDistributableIntervention>();
        ar & vacc.vaccine_take_age_decay_rate;
        ar & boost::serialization::base_object<SimpleVaccine>(vacc);
    }
}

#endif

#endif // ENABLE_TB
