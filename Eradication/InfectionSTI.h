/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once
#include "Infection.h"

namespace Kernel
{
    class InfectionSTI : public Infection
    {
    public:
        virtual ~InfectionSTI(void);
        static InfectionSTI *CreateInfection(IIndividualHumanContext *context, suids::suid _suid);
        virtual float GetInfectiousness() const;
        virtual void Update(float dt, Susceptibility* immunity = NULL);

    protected:
        InfectionSTI();
        InfectionSTI(IIndividualHumanContext *context);

    private:
#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
        friend class boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive & ar, InfectionSTI& inf, const unsigned int file_version );
#endif
    };
}
