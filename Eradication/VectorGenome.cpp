
#include "stdafx.h"
#include "VectorGenome.h"
#include "Exceptions.h"
#include "Log.h"
#include "Debug.h"

SETUP_LOGGING( "VectorGenome" )

namespace Kernel
{
    uint8_t VectorGenome::GENDER_LOCUS_INDEX    = 0;

    VectorGenome::VectorGenome()
        : m_GameteMom()
        , m_GameteDad()
    {
    }

    VectorGenome::VectorGenome( const VectorGamete& rMom, const VectorGamete& rDad )
        : m_GameteMom( rMom )
        , m_GameteDad( rDad )
    {
    }

    VectorGenome::VectorGenome( VectorGameteBitPair_t bits )
        : m_GameteMom()
        , m_GameteDad()
    {
        std::pair<VectorGamete, VectorGamete> mom_dad_bits = VectorGamete::Convert( bits );
        m_GameteMom = mom_dad_bits.first;
        m_GameteDad = mom_dad_bits.second;
    }

    VectorGenome::~VectorGenome()
    {
    }

    bool VectorGenome::operator==( const VectorGenome& rThat ) const
    {
        if( this->m_GameteMom != rThat.m_GameteMom ) return false;
        if( this->m_GameteDad != rThat.m_GameteDad ) return false;
        return true;
    }

    bool VectorGenome::operator!=( const VectorGenome& rThat ) const
    {
        return !operator==( rThat );
    }

    bool VectorGenome::operator<( const VectorGenome& rThat ) const
    {
        if( this->m_GameteMom < rThat.m_GameteMom )
            return true;
        else if( this->m_GameteMom == rThat.m_GameteMom )
            return (this->m_GameteDad < rThat.m_GameteDad);
        else
            return false;
    }

    VectorGamete& VectorGenome::GetGamete( VectorGenomeGameteIndex::Enum index )
    {
        switch( index )
        {
            case VectorGenomeGameteIndex::GAMETE_INDEX_MOM:
                return m_GameteMom;

            case VectorGenomeGameteIndex::GAMETE_INDEX_DAD:
                return m_GameteDad;

            default:
                throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__,
                                                         "index",
                                                         index,
                                                         VectorGenomeGameteIndex::pairs::lookup_key( index ) );
        }
    }

    void VectorGenome::SetLocus( uint16_t locusIndex, uint8_t momAlleleIndex, uint8_t dadAlleleIndex )
    {
        m_GameteMom.SetLocus( locusIndex, momAlleleIndex );
        m_GameteDad.SetLocus( locusIndex, dadAlleleIndex );
    }

    std::pair<uint8_t, uint8_t> VectorGenome::GetLocus( uint8_t locusIndex ) const
    {
        std::pair<uint8_t, uint8_t> allele_index_pair;
        allele_index_pair.first = m_GameteMom.GetLocus( locusIndex );
        allele_index_pair.second = m_GameteDad.GetLocus( locusIndex );
        return allele_index_pair;
    }

    VectorGameteBitPair_t VectorGenome::GetBits() const
    {
        VectorGameteBitPair_t bits = VectorGamete::Convert( m_GameteMom, m_GameteDad );
        return bits;
    }

    VectorGender::Enum VectorGenome::GetGender() const
    {
        return m_GameteDad.HasYChromosome() ? VectorGender::VECTOR_MALE : VectorGender::VECTOR_FEMALE;
    }

    void VectorGenome::SetWolbachia( VectorWolbachia::Enum wolb )
    {
        m_GameteMom.SetWolbachia( wolb );
        m_GameteDad.SetWolbachia( wolb );
    }

    VectorWolbachia::Enum VectorGenome::GetWolbachia() const
    {
        return m_GameteMom.GetWolbachia();
    }

    void VectorGenome::ClearMicrosporidia()
    {
        m_GameteMom.ClearMicrosporidia();
        m_GameteDad.ClearMicrosporidia();
    }

    void VectorGenome::SetMicrosporidiaStrain( int strainindex )
    {
        m_GameteMom.SetMicrosporidiaStrain( strainindex );
        m_GameteDad.SetMicrosporidiaStrain( strainindex );
    }

    bool VectorGenome::HasMicrosporidia() const
    {
        return m_GameteMom.HasMicrosporidia() || m_GameteDad.HasMicrosporidia();
    }

    int VectorGenome::GetMicrosporidiaStrainIndex() const
    {
        release_assert( m_GameteMom.GetMicrosporidiaStrainIndex() == m_GameteDad.GetMicrosporidiaStrainIndex() );
        return m_GameteMom.GetMicrosporidiaStrainIndex();
    }

    void VectorGenome::serialize( IArchive& ar, VectorGenome& vg )
    {
        ar.startObject();
        ar.labelElement( "m_GameteMom" ) & vg.m_GameteMom;
        ar.labelElement( "m_GameteDad" ) & vg.m_GameteDad;
        ar.endObject();
    }
}
