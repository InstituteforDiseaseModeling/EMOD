/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "VectorEnums.h"
#include "BoostLibWrapper.h"
#include "IArchive.h"

namespace Kernel
{
    typedef unsigned int VectorGeneticIndex_t;
    typedef std::pair< VectorAllele::Enum, VectorAllele::Enum > AllelePair_t;
    typedef std::map< VectorAllele::Enum, float > AlleleFractions_t;

    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // !!! The 'final' tag is intended to stop developers from adding virtual methods to
    // !!! this class and then extending it.  VectorMatingStructure is in every mosquito and adding 
    // !!! virtual methods adds a vtable pointer (8-bytes) to every instance.  This is really impactful
    // !!! when talking about 100's of millions of mosquitos.
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    class VectorMatingStructure final
    {
    public:
        VectorMatingStructure( VectorGender::Enum      _gen  = VectorGender::VECTOR_FEMALE,
                               VectorSterility::Enum   _ster = VectorSterility::VECTOR_FERTILE,
                               VectorWolbachia::Enum   _Wol  = VectorWolbachia::WOLBACHIA_FREE,
                               AllelePair_t            _pest = std::make_pair(VectorAllele::WILD, VectorAllele::WILD),
                               AllelePair_t            _HEGs = std::make_pair(VectorAllele::WILD, VectorAllele::WILD) );

        VectorMatingStructure( VectorGeneticIndex_t _index );
        ~VectorMatingStructure();

        void SetIndex(VectorGeneticIndex_t _index);
        void SetGender(VectorGender::Enum _gen);
        void SetSterility(VectorSterility::Enum _ster);
        void SetWolbachia(VectorWolbachia::Enum _Wol);
        void SetPesticideResistance(AllelePair_t _pest);
        void SetPesticideResistance(VectorAllele::Enum _pest, VectorAllele::Enum _pestMated = VectorAllele::NotMated);
        void SetHEG(AllelePair_t _HEGs);
        void SetHEG(VectorAllele::Enum _HEG, VectorAllele::Enum _HEGmated = VectorAllele::NotMated);
        void SetUnmated();

        VectorGeneticIndex_t    GetIndex()               const;
        VectorGender::Enum      GetGender()              const;
        VectorSterility::Enum   GetSterility()           const;
        VectorWolbachia::Enum   GetWolbachia()           const;
        AllelePair_t            GetPesticideResistance() const;
        AllelePair_t            GetHEG()                 const;
        bool                    IsMated()                const;

        // Helper function to determine cytoplasmic compatibility in mating
        static bool WolbachiaCompatibleMating(VectorWolbachia::Enum femaleWolbachia, 
                                              VectorWolbachia::Enum maleWolbachia);

        // Determine egg traits based on mated alleles
        static AlleleFractions_t GetAlleleFractions(AllelePair_t _alleles);
        static AlleleFractions_t GetAlleleFractionsEarlyHoming(AllelePair_t _alleles, float homing = 0);
        static AlleleFractions_t GetAlleleFractionsDualEarlyHoming(AllelePair_t _alleles, float homing = 0);

        // Mating structures are the same if they have the same index
        bool operator==(const VectorMatingStructure& lhs) const
        {
            return lhs.GetIndex() == GetIndex();
        }

        static void serialize(IArchive&, VectorMatingStructure&);

    protected:
        // Helper function to take a single parent's alleles (e.g. HALF)
        // and return the fraction of each gene (e.g. 0.5,0.5)
        static std::pair<float, float> GetAllele( VectorAllele::Enum _allele);
        static std::pair<float, float> GetAlleleEarlyHoming( VectorAllele::Enum _allele, float homing);

        union
        {
            // Index equivalent to bit-field representation
            VectorGeneticIndex_t index;

            // Pack 12 bits: 0000 G S WW RRRR HHHH
            struct
            {
                // N.B. architecture-dependent order of packing
                unsigned int HEG_mate     : 2;
                unsigned int HEG_self     : 2;
                unsigned int pestres_mate : 2;
                unsigned int pestres_self : 2;
                unsigned int wolbachia    : 2;
                unsigned int sterility    : 1;
                unsigned int gender       : 1;
                unsigned int              : 4; // pad to 16 bits
            };
        } vector_mating;
    };
}
