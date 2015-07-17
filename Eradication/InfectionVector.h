/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include "VectorContexts.h"
#include "Infection.h"

#include "Common.h"

namespace Kernel
{
    class InfectionVector : public Kernel::Infection
    {
    public:
        static InfectionVector *CreateInfection(IIndividualHumanContext *context, suids::suid _suid);
        virtual ~InfectionVector(void);

    protected:
        InfectionVector();
        InfectionVector(IIndividualHumanContext *context);
        void Initialize(suids::suid _suid);

    private:
#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
        friend class boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive & ar, InfectionVector& inf, unsigned int  file_version );
#endif

#if USE_JSON_SERIALIZATION || USE_JSON_MPI
    public:
         // IJsonSerializable Interfaces
         virtual void JSerialize( IJsonObjectAdapter* root, JSerializer* helper ) const;
         virtual void JDeserialize( IJsonObjectAdapter* root, JSerializer* helper );
#endif
    };
}
