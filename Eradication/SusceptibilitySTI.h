/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once
#include "Susceptibility.h"

namespace Kernel
{
    class SusceptibilitySTI : public Susceptibility
    {
    public:
        virtual ~SusceptibilitySTI(void);
        static SusceptibilitySTI *CreateSusceptibility(IIndividualHumanContext *context, float age, float immmod, float riskmod);

    protected:

        SusceptibilitySTI();
        SusceptibilitySTI(IIndividualHumanContext *context);
        void Initialize(float age, float immmod, float riskmod);

        // additional members of airborne susceptibility
        float demographic_risk;

    private:
#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
        friend class boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive & ar, SusceptibilitySTI& sus, const unsigned int file_version );
#endif
    };
}
