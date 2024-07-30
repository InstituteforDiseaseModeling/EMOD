
#include "stdafx.h"

#include "StrainIdentityMalariaGenetics.h"
#include "Exceptions.h"
#include "Debug.h"
#include "Log.h"
#include "IArchive.h"

SETUP_LOGGING( "StrainIdentityMalariaGenetics" )


namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY( StrainIdentityMalariaGenetics )
        HANDLE_INTERFACE(IStrainIdentity)
    END_QUERY_INTERFACE_BODY( StrainIdentityMalariaGenetics )

    StrainIdentityMalariaGenetics::StrainIdentityMalariaGenetics()
        : m_Genome()
        , m_BiteID( 0 )
    {
    }

    StrainIdentityMalariaGenetics::StrainIdentityMalariaGenetics( const ParasiteGenome& rGenome )
        : m_Genome( rGenome )
        , m_BiteID( 0 )
    {
    }

    StrainIdentityMalariaGenetics::StrainIdentityMalariaGenetics( const StrainIdentityMalariaGenetics& rMaster )
        : m_Genome( rMaster.m_Genome )
        , m_BiteID( rMaster.m_BiteID )
    {
    }

    StrainIdentityMalariaGenetics::~StrainIdentityMalariaGenetics( void )
    {
    }

    IStrainIdentity* StrainIdentityMalariaGenetics::Clone() const
    {
        return new StrainIdentityMalariaGenetics( *this );
    }

    int  StrainIdentityMalariaGenetics::GetAntigenID(void) const
    {
        return 0;
    }

    int  StrainIdentityMalariaGenetics::GetGeneticID(void) const
    {
        release_assert( !m_Genome.IsNull() );
        return m_Genome.GetID();
    }

    void StrainIdentityMalariaGenetics::SetAntigenID(int in_antigenID)
    {
        // do nothing because it is fixed at zero
        release_assert( false );
    }

    void StrainIdentityMalariaGenetics::SetGeneticID(int in_geneticID)
    {
        release_assert( false );
        // do nothing because it is fixed in the genome
    }

    const ParasiteGenome& StrainIdentityMalariaGenetics::GetGenome() const
    {
        release_assert( !m_Genome.IsNull() );
        return m_Genome;
    }

    void StrainIdentityMalariaGenetics::SetGenome( const ParasiteGenome& rGenome )
    {
        m_Genome = rGenome;
    }

    uint32_t StrainIdentityMalariaGenetics::GetBiteID() const
    {
        return m_BiteID;
    }

    void StrainIdentityMalariaGenetics::SetBiteID( uint32_t biteID )
    {
        m_BiteID = biteID;
    }

    REGISTER_SERIALIZABLE(StrainIdentityMalariaGenetics);

    void StrainIdentityMalariaGenetics::serialize(IArchive& ar, StrainIdentityMalariaGenetics* obj)
    {
        StrainIdentityMalariaGenetics& strain = *obj;

        ar.labelElement( "m_Genome" ) & strain.m_Genome;
        ar.labelElement( "m_BiteID" ) & strain.m_BiteID;
    }
}