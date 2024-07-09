
#pragma once

#include "IArchive.h"
#include "VectorEnums.h"

namespace Kernel
{
    typedef uint32_t VectorGameteBits_t;
    typedef uint64_t VectorGameteBitPair_t; // Female in lower-half, Male in upper-half

    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // !!! The 'final' tag is intended to stop developers from adding virtual methods to
    // !!! this class and then extending it.  There are four VectorGamete objects in every mosquito
    // !!! and adding virtual methods adds a vtable pointer (8-bytes) to every instance, so 32-bytes.
    // !!! This is really impactful when talking about 100's of millions of mosquitos.
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    // A gamete is a cell, egg or sperm, where the cell contains one chromosome 
    // for each chromosome pair in the parent.  This implies that it contains one 
    // allele for each gene.  It is assumed that the gender "allele" are at the
    // zero'th index.
    class VectorGamete final // see final tag comment above
    {
    public:
        // maximum number of alleles per gamete - MAX_LOCI*MAX_ALLELES <= sizeof(VectorGameteBits_t)
        static const uint8_t MAX_LOCI;

        // maximum number of alleles per gene - defines # of bits per allele
        static const uint8_t MAX_ALLELES;

        // number of bits per allele = 2^MAX_ALLELES
        static const uint32_t BITS_PER_ALLELE;

        // maximum number of alleles for the gender gene - defines # of bits per allele
        static const uint8_t GENDER_MAX_ALLELES;

        // number of bits per allele = 2^GENDER_MAX_ALLELES
        static const uint32_t GENDER_BITS_PER_ALLELE;

        // we use one of the bits in the gender gene to indicate whether it is X- or Y-based.
        // X alleles should be 0-3 and Y-alleles should be 4-7
        static const uint32_t GENDER_BIT_MASK;

        static const uint32_t ALLELE_BITS[];

        static VectorGameteBitPair_t Convert( VectorGamete female, VectorGamete male )
        {
            return VectorGameteBitPair_t( female.m_Bits ) + (VectorGameteBitPair_t( male.m_Bits ) << 32);
        }

        static std::pair<VectorGamete, VectorGamete> Convert( VectorGameteBitPair_t bitPair )
        {
            return std::make_pair( VectorGamete( VectorGameteBits_t( bitPair ) ),
                                   VectorGamete( VectorGameteBits_t( bitPair >> 32 ) ) );
        }


        VectorGamete( );
        explicit VectorGamete( VectorGameteBits_t bits );
        ~VectorGamete();

        bool operator==( const VectorGamete& rThat ) const;
        bool operator!=( const VectorGamete& rThat ) const;
        bool operator<( const VectorGamete& rThat ) const;

        void SetLocus( uint8_t index, uint8_t alleleIndex );

        VectorGameteBits_t GetBits() const;
        uint8_t GetLocus( uint8_t index ) const;

        void SetWolbachia( VectorWolbachia::Enum wolb );
        VectorWolbachia::Enum GetWolbachia() const;

        void ClearMicrosporidia();
        void SetMicrosporidiaStrain( int strainIndex );
        bool HasMicrosporidia() const;
        int GetMicrosporidiaStrainIndex() const;

        bool HasYChromosome() const;

        static void serialize( IArchive&, VectorGamete& );

    private:
        VectorGameteBits_t m_Bits;
    };

    struct GameteProbPair
    {
        VectorGamete gamete;
        float prob;

        GameteProbPair()
            : gamete()
            , prob( 0 )
        {
        }

        GameteProbPair( const VectorGamete& rGamete, float _prob )
            : gamete( rGamete )
            , prob( _prob )
        {
        }
    };
    typedef std::vector<GameteProbPair> GameteProbPairVector_t;
}
