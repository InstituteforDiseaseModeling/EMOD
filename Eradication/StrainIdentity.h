
#pragma once

#include "IArchive.h"
#include "IStrainIdentity.h"
#include "RANDOM.h"

namespace Kernel
{
    class StrainIdentity : public IStrainIdentity
    {
        IMPLEMENT_NO_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()
    public:
        StrainIdentity(void);
        StrainIdentity(int initial_antigen, int initial_genome, RANDOMBASE * prng = nullptr );
        StrainIdentity( const StrainIdentity& rMaster );
        virtual ~StrainIdentity(void);

        // IStrainIdentity methods
        virtual IStrainIdentity* Clone() const override;
        virtual int  GetAntigenID(void) const override;
        virtual int  GetGeneticID(void) const override;
        virtual void SetAntigenID(int in_antigenID) override;
        virtual void SetGeneticID(int in_geneticID) override;

        static IArchive& serialize(IArchive&, StrainIdentity&);

        DECLARE_SERIALIZABLE(StrainIdentity);

    protected:
        int antigenID;
        int geneticID;
    };
}
