
#include "stdafx.h"

#include "StrainIdentityMalariaVarGenes.h"
#include "Exceptions.h"
#include "Log.h"
#include "IArchive.h"
#include "Debug.h"

SETUP_LOGGING( "StrainIdentityMalariaVarGenes" )


namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY( StrainIdentityMalariaVarGenes )
        HANDLE_INTERFACE(IStrainIdentity)
    END_QUERY_INTERFACE_BODY( StrainIdentityMalariaVarGenes )

    StrainIdentityMalariaVarGenes::StrainIdentityMalariaVarGenes()
        : m_GeneticID( 0 )
        , m_MspType( 0 )
        , m_IRBCType()
        , m_MinorEpitopeType()
    {
    }

    StrainIdentityMalariaVarGenes::StrainIdentityMalariaVarGenes( int32_t msp,
                                                                  const std::vector<int32_t>& irbc,
                                                                  const std::vector<int32_t>& minorEpitope )
        : m_GeneticID( 0 )
        , m_MspType( msp )
        , m_IRBCType( irbc )
        , m_MinorEpitopeType( minorEpitope )
    {
        // CLONAL_PfEMP1_VARIANTS = 50
        release_assert( m_IRBCType.size()         == 50 );
        release_assert( m_MinorEpitopeType.size() == 50 );

        // set genetic ID based on a hash of the underlying values
        m_GeneticID = 37 * m_GeneticID + m_MspType;
        for( auto irbc : m_IRBCType )
        {
            m_GeneticID = 37 * m_GeneticID + irbc;
        }
        for( auto minor : m_MinorEpitopeType )
        {
            m_GeneticID = 37 * m_GeneticID + minor;
        }
    }

    StrainIdentityMalariaVarGenes::StrainIdentityMalariaVarGenes( const StrainIdentityMalariaVarGenes& rMaster )
        : m_GeneticID(        rMaster.m_GeneticID )
        , m_MspType(          rMaster.m_MspType )
        , m_IRBCType(         rMaster.m_IRBCType )
        , m_MinorEpitopeType( rMaster.m_MinorEpitopeType )
    {
    }

    StrainIdentityMalariaVarGenes::~StrainIdentityMalariaVarGenes( void )
    {
    }

    IStrainIdentity* StrainIdentityMalariaVarGenes::Clone() const
    {
        return new StrainIdentityMalariaVarGenes( *this );
    }

    int  StrainIdentityMalariaVarGenes::GetAntigenID(void) const
    {
        return 0;
    }

    int  StrainIdentityMalariaVarGenes::GetGeneticID(void) const
    {
        return m_GeneticID;
    }

    void StrainIdentityMalariaVarGenes::SetAntigenID(int in_antigenID)
    {
        throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "Should not be called" );
    }

    void StrainIdentityMalariaVarGenes::SetGeneticID(int in_geneticID)
    {
        throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "Should not be called" );
    }

    int32_t StrainIdentityMalariaVarGenes::GetMSPType() const
    {
        return m_MspType;
    }

    const std::vector<int32_t>& StrainIdentityMalariaVarGenes::GetIRBCType() const
    {
        return m_IRBCType;
    }

    const std::vector<int32_t>& StrainIdentityMalariaVarGenes::GetMinorEpitopeType() const
    {
        return m_MinorEpitopeType;
    }

    REGISTER_SERIALIZABLE(StrainIdentityMalariaVarGenes);

    void StrainIdentityMalariaVarGenes::serialize(IArchive& ar, StrainIdentityMalariaVarGenes* obj)
    {
        StrainIdentityMalariaVarGenes& strain = *obj;
        ar.labelElement( "m_GeneticID"        ) & strain.m_GeneticID;
        ar.labelElement( "m_MspType"          ) & strain.m_MspType;
        ar.labelElement( "m_IRBCType"         ) & strain.m_IRBCType;
        ar.labelElement( "m_MinorEpitopeType" ) & strain.m_MinorEpitopeType;
    }
}