/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "Vaccine.h"

namespace Kernel
{
    class BCGVaccine : public SimpleVaccine
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, BCGVaccine, IDistributableIntervention)

    public:
        BCGVaccine();
        virtual ~BCGVaccine() { }
        virtual bool Configure( const Configuration* inputJson ) override;

        // IVaccine
        virtual bool  ApplyVaccineTake( IIndividualHumanContext* pihc ); 

    protected:
        float vaccine_take_age_decay_rate;

        DECLARE_SERIALIZABLE(BCGVaccine);
    };
}
