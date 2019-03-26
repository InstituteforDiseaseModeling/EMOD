/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "IArchive.h"
#include "IStrainIdentity.h"
#include "RANDOM.h"

namespace Kernel
{
    class StrainIdentity : public IStrainIdentity
    {
    public:
        StrainIdentity(void);
        StrainIdentity(int initial_antigen, int initial_genome, RANDOMBASE * prng = nullptr );
        StrainIdentity( const IStrainIdentity *copy );
        virtual ~StrainIdentity(void);

        // IStrainIdentity methods
        virtual int  GetAntigenID(void) const override;
        virtual int  GetGeneticID(void) const override;
        virtual void SetAntigenID(int in_antigenID) override;
        virtual void SetGeneticID(int in_geneticID) override;
        virtual void ResolveInfectingStrain( IStrainIdentity* strainId ) const;

        static IArchive& serialize(IArchive&, StrainIdentity*&);
        static IArchive& serialize(IArchive&, StrainIdentity&);

    protected:
        int antigenID;
        int geneticID;
    };
}
