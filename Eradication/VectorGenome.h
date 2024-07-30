
#pragma once

#include "VectorGamete.h"

namespace Kernel
{
    ENUM_DEFINE( VectorGenomeGameteIndex,
        ENUM_VALUE_SPEC( GAMETE_INDEX_MOM, 0 )
        ENUM_VALUE_SPEC( GAMETE_INDEX_DAD, 1 ))

    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // !!! The 'final' tag is intended to stop developers from adding virtual methods to
    // !!! this class and then extending it.  There are two VectorGenomes in every mosquito and adding 
    // !!! virtual methods adds a vtable pointer (8-bytes) to every instance - (16-bytes per mosquito).
    // !!! This is really impactful when talking about 100's of millions of mosquitos.
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    // The set of genes or genetic material present in an organism.
    // It contains two VectorGamete objects.  The allele pair for a particular gene
    // are obtained by getting the allele for that Gene from each VectorGamete object.
    class VectorGenome final
    {
    public:
        // Gender is being treated like a gene with a particular locus in the gametes
        // and the X and Y chromosomes have allele indexes like other genes.
        static uint8_t GENDER_LOCUS_INDEX;

        VectorGenome( );
        VectorGenome( const VectorGamete& rMom, const VectorGamete& rDad );
        explicit VectorGenome( VectorGameteBitPair_t bits );
        ~VectorGenome();

        bool operator==( const VectorGenome& rThat ) const;
        bool operator!=( const VectorGenome& rThat ) const;
        bool operator<( const VectorGenome& rThat ) const;

        VectorGamete& GetGamete( VectorGenomeGameteIndex::Enum index );

        void SetLocus( uint16_t locusIndex, uint8_t momAlleleIndex, uint8_t dadAlleleIndex );
        std::pair<uint8_t, uint8_t> GetLocus( uint8_t locusIndex ) const;

        VectorGameteBitPair_t GetBits() const;

        VectorGender::Enum GetGender() const;

        void SetWolbachia( VectorWolbachia::Enum wolb );
        VectorWolbachia::Enum GetWolbachia() const;

        void ClearMicrosporidia();
        void SetMicrosporidiaStrain( int strainIndex );
        bool HasMicrosporidia() const;
        int GetMicrosporidiaStrainIndex() const;

        static void serialize( IArchive&, VectorGenome& );

    private:
        VectorGamete m_GameteMom;
        VectorGamete m_GameteDad;
    };

    struct GenomeCountPair
    {
        VectorGenome genome;
        uint32_t count;

        GenomeCountPair()
            : genome()
            , count( 0 )
        {
        }

        GenomeCountPair( const VectorGenome& rGenome, uint32_t _count )
            : genome( rGenome )
            , count( _count )
        {
        }
    };
    typedef std::vector<GenomeCountPair> GenomeCountPairVector_t;

    struct GenomeNamePair
    {
        VectorGenome genome;
        std::string name;

        GenomeNamePair()
            : genome()
            , name()
        {
        }

        GenomeNamePair( const VectorGenome& rGenome, const std::string& rName )
            : genome( rGenome )
            , name( rName )
        {
        }
    };
    typedef std::vector<GenomeNamePair> GenomeNamePairVector_t;

    struct PossibleGenome : GenomeNamePair
    {
        std::vector<VectorGenome> similar_genomes;

        PossibleGenome()
            : GenomeNamePair()
            , similar_genomes()
        {
        }

        PossibleGenome( const GenomeNamePair& rOrig )
            : GenomeNamePair( rOrig.genome, rOrig.name )
            , similar_genomes()
        {
        }
    };

    struct GenomeProbPair
    {
        VectorGenome genome;
        float prob;

        GenomeProbPair()
            : genome()
            , prob( 0 )
        {
        }

        GenomeProbPair( const VectorGenome& rGenome, float _prob )
            : genome( rGenome )
            , prob( _prob )
        {
        }
    };
    typedef std::vector<GenomeProbPair> GenomeProbPairVector_t;

}
