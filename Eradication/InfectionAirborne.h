/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once
#include "Infection.h"

namespace Kernel
{
    class InfectionAirborne : public Infection
    {
    public:
        virtual ~InfectionAirborne(void);
        static InfectionAirborne *CreateInfection(IIndividualHumanContext *context, suids::suid _suid);

    protected:
        InfectionAirborne();
        InfectionAirborne(IIndividualHumanContext *context);

    private:
#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
        friend class boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive & ar, InfectionAirborne& inf, const unsigned int file_version );
#endif
    };
}
