
#include "stdafx.h"
#include "VectorGamete.h"
#include "Exceptions.h"
#include "Log.h"

SETUP_LOGGING( "VectorGamete" )

namespace Kernel
{
    const uint8_t  VectorGamete::MAX_LOCI           =  9; // maximum number of alleles per gamete
    const uint32_t VectorGamete::BITS_PER_ALLELE    =  3; // number of bits per allele
    const uint8_t  VectorGamete::MAX_ALLELES        =  8; // maximum number of alleles per gene - 8 => 3-bits per allele

    // -----------------------------------------------------------------------
    // --- We fix the X chromosome alleles to the first four values (0-3) and
    // --- the Y chromosome allels get the last four values (4-7).  This means
    // --- that the 3rd bit (0x00000004) allowed for a gene is 0 for the
    // --- X chromosomes and 1 for the Y chromosomes.  By default, we always
    // --- want to ensure X is at 0 and Y is at 4.  The other X's are then
    // --- 1-3 and the other Y's are 5-7.
    // -----------------------------------------------------------------------
    const uint32_t VectorGamete::GENDER_BITS_PER_ALLELE =  2; // number of bits per allele
    const uint8_t  VectorGamete::GENDER_MAX_ALLELES     =  4; // maximum number of alleles per gender - 4 => 2-bits per allele
    const uint32_t VectorGamete::GENDER_BIT_MASK        = 0x00000004; // 0000 0000 0000 0000 0000 0000 0000 0100

    // (upper)  00 000 HHH ggg FFF EEE DDD CCC BBB AAA *GG (lower) where * is the gender bit indicating if Y Chromosome is present
    const uint32_t MASK_ALLELES       = 0x07FFFFFF; // 0000 0111 1111 1111 1111 1111 1111 1111
    const uint32_t MASK_WOLBACHIA     = 0xC0000000; // 1100 0000 0000 0000 0000 0000 0000 0000
    const uint32_t MASK_MICROSPORIDIA = 0x38000000; // 0011 1000 0000 0000 0000 0000 0000 0000

    const uint32_t NUM_BITS_LOWER_THAN_MICROSPORIDIA = uint32_t( VectorGamete::MAX_LOCI ) * VectorGamete::BITS_PER_ALLELE;
    const uint32_t NUM_BITS_LOWER_THAN_WOLBACHIA     = uint32_t( VectorGamete::MAX_LOCI ) * VectorGamete::BITS_PER_ALLELE + VectorGamete::BITS_PER_ALLELE; //extra 3 bits for microsporidia

    const uint32_t VectorGamete::ALLELE_BITS[] = {
        0x00000007, // 0 - gender
        0x00000038, // 1
        0x000001C0, // 2
        0x00000E00, // 3
        0x00007000, // 4
        0x00038000, // 5
        0x001C0000, // 6
        0x00E00000, // 7
        0x07000000  // 8
    };

    VectorGamete::VectorGamete()
        : m_Bits( 0 )
    {
    }

    VectorGamete::VectorGamete( VectorGameteBits_t bits )
        : m_Bits( bits )
    {
    }

    VectorGamete::~VectorGamete()
    {
    }

    bool VectorGamete::operator==( const VectorGamete& rThat ) const
    {
        return this->m_Bits == rThat.m_Bits;
    }

    bool VectorGamete::operator!=( const VectorGamete& rThat ) const
    {
        return !operator==( rThat );
    }

    bool VectorGamete::operator<( const VectorGamete& rThat ) const
    {
        return (this->m_Bits < rThat.m_Bits);
    }

    void VectorGamete::SetLocus( uint8_t locusIndex, uint8_t alleleIndex )
    {
        if( locusIndex >= MAX_LOCI )
        {
            // casting the values to uint32_t so that they are considered integers instead of characters
            std::stringstream ss;
            ss << "The maximum number of loci is " << uint32_t(MAX_LOCI)
               << " and a locus index of " << uint32_t( locusIndex ) << " was attempted.";
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
        if( alleleIndex > MAX_ALLELES )
        {
            // casting the values to uint32_t so that they are considered integers instead of characters
            std::stringstream ss;
            ss << "The maximum number of alleles is " << uint32_t(MAX_ALLELES)
               << " and an allele index of " << uint32_t(alleleIndex) << " was attempted.";
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
        // clear the bits for this locus
        m_Bits &= ~ALLELE_BITS[ locusIndex ];

        // set the bits at this locus
        m_Bits |= ALLELE_BITS[ locusIndex ] & (uint32_t(alleleIndex) << (uint32_t(locusIndex) * BITS_PER_ALLELE));
    }

    uint8_t VectorGamete::GetLocus( uint8_t locusIndex ) const
    {
        if( locusIndex >= MAX_LOCI )
        {
            // casting the values to uint32_t so that they are considered integers instead of characters
            std::stringstream ss;
            ss << "The maximum number of loci is " << uint32_t(MAX_LOCI)
               << " and a locus index of " << uint32_t( locusIndex ) << " was attempted.";
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
        return (m_Bits & ALLELE_BITS[ locusIndex ]) >> (uint32_t( locusIndex ) * BITS_PER_ALLELE);
    }

    void VectorGamete::SetWolbachia( VectorWolbachia::Enum wolb )
    {
        //clear wolbachia bits
        m_Bits &= ~MASK_WOLBACHIA;

        // set wolbachia bits
        uint32_t shifted_bits = MASK_WOLBACHIA & (uint32_t( wolb ) << NUM_BITS_LOWER_THAN_WOLBACHIA);
        m_Bits |= MASK_WOLBACHIA & (uint32_t( wolb ) << NUM_BITS_LOWER_THAN_WOLBACHIA);
    }

    VectorWolbachia::Enum VectorGamete::GetWolbachia() const
    {
        uint32_t wolb = (m_Bits & MASK_WOLBACHIA) >> NUM_BITS_LOWER_THAN_WOLBACHIA;
        return VectorWolbachia::Enum(wolb);
    }

    void VectorGamete::ClearMicrosporidia()
    {
        // -----------------------------------------------------------------------------------
        // --- NOTE: strainIndex = 0 implies no microsporidia, the microsporidia_strains array in
        // --- VectorSpeciesParameters will automatically add one strain for "no microspordia"
        // -----------------------------------------------------------------------------------

        //clear microsporida bits
        m_Bits &= ~MASK_MICROSPORIDIA;

        // set microsporida bits
        m_Bits |= MASK_MICROSPORIDIA & (uint32_t( 0 ) << NUM_BITS_LOWER_THAN_MICROSPORIDIA);
    }

    void VectorGamete::SetMicrosporidiaStrain( int strainIndex )
    {
        // -----------------------------------------------------------------------------------
        // --- NOTE: strainIndex = 0 implies no microsporidia, the microsporidia_strains array in
        // --- VectorSpeciesParameters will automatically add one strain for "no microspordia"
        // -----------------------------------------------------------------------------------
        //clear microsporida bits
        m_Bits &= ~MASK_MICROSPORIDIA;

        // set microsporida bits
        m_Bits |= MASK_MICROSPORIDIA & (uint32_t( strainIndex ) << NUM_BITS_LOWER_THAN_MICROSPORIDIA);
    }

    bool VectorGamete::HasMicrosporidia() const
    {
        // -----------------------------------------------------------------------------------
        // --- NOTE: strainIndex = 0 implies no microsporidia, the microsporidia_strains array in
        // --- VectorSpeciesParameters will automatically add one strain for "no microspordia"
        // -----------------------------------------------------------------------------------

        int strain_index = GetMicrosporidiaStrainIndex();
        // the value stored starts at one compared to what is returned
        return (strain_index > 0);
    }

    int VectorGamete::GetMicrosporidiaStrainIndex() const
    {
        // -----------------------------------------------------------------------------------
        // --- NOTE: strainIndex = 0 implies no microsporidia, the microsporidia_strains array in
        // --- VectorSpeciesParameters will automatically add one strain for "no microspordia"
        // -----------------------------------------------------------------------------------
        int strain_index = ((m_Bits & MASK_MICROSPORIDIA) >> NUM_BITS_LOWER_THAN_MICROSPORIDIA);
        return strain_index;
    }

    VectorGameteBits_t VectorGamete::GetBits() const
    {
        return m_Bits;
    }

    bool VectorGamete::HasYChromosome() const
    {
        // See note above about the gender alleles
        return ((m_Bits & GENDER_BIT_MASK) == GENDER_BIT_MASK);
    }

    void VectorGamete::serialize( IArchive& ar, VectorGamete& vg )
    {
        ar & (uint32_t&)vg.m_Bits;
    }
}
