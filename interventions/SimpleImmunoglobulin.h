/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include "Vaccine.h"

namespace Kernel
{
    // Not used anywhere as of this time, but creating for consistency.
    class ISimpleImmunoglobulin : public ISupports
    {
    };

    class SimpleImmunoglobulin : public SimpleVaccine
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, SimpleImmunoglobulin, IDistributableIntervention)

    public:
        SimpleImmunoglobulin();
        bool Configure( const Configuration* pConfig );

    private:
#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
        template<class Archive>
        friend void serialize(Archive &ar, SimpleImmunoglobulin &vacc, const unsigned int v);
#endif    
    };
}
