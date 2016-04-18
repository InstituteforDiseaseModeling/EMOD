/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

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
        bool Configure( const Configuration* inputJson );
        BCGVaccine();
        virtual ~BCGVaccine() { }

        // IVaccine
        virtual void  ApplyVaccineTake(); 

    protected:
        float vaccine_take_age_decay_rate;

        DECLARE_SERIALIZABLE(BCGVaccine);
    };
}
