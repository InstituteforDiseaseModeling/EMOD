
#pragma once

#include "IStrainIdentity.h"
#include "ParasiteGenome.h"

namespace Kernel
{
    class StrainIdentityMalariaGenetics : public IStrainIdentity
    {
        IMPLEMENT_NO_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()
    public:
        StrainIdentityMalariaGenetics();
        StrainIdentityMalariaGenetics( const ParasiteGenome& rGenome );
        StrainIdentityMalariaGenetics( const StrainIdentityMalariaGenetics& rMaster );
        virtual ~StrainIdentityMalariaGenetics(void);

        // IStrainIdentity methods
        virtual IStrainIdentity* Clone() const override;
        virtual int  GetAntigenID(void) const override;
        virtual int  GetGeneticID(void) const override;
        virtual void SetAntigenID( int in_antigenID ) override;
        virtual void SetGeneticID( int in_geneticID ) override;

        // other
        const ParasiteGenome& GetGenome() const;
        void SetGenome( const ParasiteGenome& rGenome );
        uint32_t GetBiteID() const;
        void SetBiteID( uint32_t biteID );

        DECLARE_SERIALIZABLE(StrainIdentityMalariaGenetics);

    protected:
        ParasiteGenome m_Genome;
        uint32_t m_BiteID;
    };
}
