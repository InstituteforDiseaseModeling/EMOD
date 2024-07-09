
#pragma once

#include "IStrainIdentity.h"

namespace Kernel
{
    class StrainIdentityMalariaVarGenes: public IStrainIdentity
    {
        IMPLEMENT_NO_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()
    public:
        StrainIdentityMalariaVarGenes();
        StrainIdentityMalariaVarGenes( int32_t msp,
                                       const std::vector<int32_t>& irbc,
                                       const std::vector<int32_t>& minorEpitope );
        StrainIdentityMalariaVarGenes( const StrainIdentityMalariaVarGenes& rMaster );
        virtual ~StrainIdentityMalariaVarGenes(void);

        // IStrainIdentity methods
        virtual IStrainIdentity* Clone() const override;
        virtual int  GetAntigenID(void) const override;
        virtual int  GetGeneticID(void) const override;
        virtual void SetAntigenID(int in_antigenID) override;
        virtual void SetGeneticID(int in_geneticID) override;

        // other
        int32_t GetMSPType() const;
        const std::vector<int32_t>& GetIRBCType() const;
        const std::vector<int32_t>& GetMinorEpitopeType() const;

        DECLARE_SERIALIZABLE(StrainIdentityMalariaVarGenes);

    protected:
        int m_GeneticID;
        int32_t m_MspType;
        std::vector<int32_t> m_IRBCType;
        std::vector<int32_t> m_MinorEpitopeType;
    };
}
