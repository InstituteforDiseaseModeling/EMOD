/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "VectorMatingStructure.h"

#include "Exceptions.h"
#include "Log.h"

SETUP_LOGGING( "VectorMatingStructure" )

namespace Kernel
{
    VectorMatingStructure::VectorMatingStructure( VectorGender::Enum      _gen,
                                                  VectorSterility::Enum   _ster,
                                                  VectorWolbachia::Enum   _Wol,
                                                  AllelePair_t            _pest,
                                                  AllelePair_t            _HEGs )
        {
            // IMPORTANT: start with a clean slate of empty bits
            SetIndex(0);

            // Pack 12 bits: 0000 G S WW RRRR HHHH
            SetGender(_gen);
            SetSterility(_ster);
            SetWolbachia(_Wol);
            SetPesticideResistance(_pest);
            SetHEG(_HEGs);

            // Males cannot be mated.  Forcing that here allows us to still use 
            // the constructor with default arguments and get WILD_WILD for females 
            // and WILD_NotMated for males
            if( _gen == VectorGender::VECTOR_MALE )
            {
                SetUnmated();
            }

            LOG_DEBUG_F( "VectorMatingStructure index = %d\n", GetIndex() );
        }

    VectorMatingStructure::VectorMatingStructure( VectorGeneticIndex_t _index )
    {
        SetIndex(_index);
    }

    VectorMatingStructure::~VectorMatingStructure()
    {
    }

    // ---------------------- Setters of packed bits --------------------
    void VectorMatingStructure::SetIndex(VectorGeneticIndex_t _index)
    {
        vector_mating.index = _index;
    }

    void VectorMatingStructure::SetGender(VectorGender::Enum _gen)
    {
        vector_mating.gender = _gen;
    }

    void VectorMatingStructure::SetSterility(VectorSterility::Enum _ster)
    {
        vector_mating.sterility = _ster;
    }

    void VectorMatingStructure::SetWolbachia(VectorWolbachia::Enum _Wol)
    {
        vector_mating.wolbachia = _Wol;
    }

    void VectorMatingStructure::SetPesticideResistance(AllelePair_t _pest)
    {
        SetPesticideResistance( _pest.first, _pest.second );
    }

    void VectorMatingStructure::SetPesticideResistance(VectorAllele::Enum _pest, VectorAllele::Enum _pestMated)
    {
        vector_mating.pestres_self = _pest;
        vector_mating.pestres_mate = _pestMated;
    }

    void VectorMatingStructure::SetHEG(AllelePair_t _HEGs)
    {
        SetHEG( _HEGs.first, _HEGs.second );
    }

    void VectorMatingStructure::SetHEG(VectorAllele::Enum _HEG, VectorAllele::Enum _HEGmated)
    {
        vector_mating.HEG_self = _HEG;
        vector_mating.HEG_mate = _HEGmated;
    }

    void VectorMatingStructure::SetUnmated()
    {
        SetPesticideResistance( GetPesticideResistance().first, VectorAllele::NotMated );
        SetHEG( GetHEG().first, VectorAllele::NotMated );
    }

    // ---------------------- Getters of packed bits --------------------
    VectorGeneticIndex_t VectorMatingStructure::GetIndex() const
    {
        return vector_mating.index;
    }

    VectorGender::Enum  VectorMatingStructure::GetGender() const
    {
        return (VectorGender::Enum) vector_mating.gender;
    }

    VectorSterility::Enum  VectorMatingStructure::GetSterility() const
    {
        return (VectorSterility::Enum) vector_mating.sterility;
    }

    VectorWolbachia::Enum  VectorMatingStructure::GetWolbachia() const
    {
        return (VectorWolbachia::Enum) vector_mating.wolbachia;
    }

    AllelePair_t  VectorMatingStructure::GetPesticideResistance() const
    {
        return std::make_pair( (VectorAllele::Enum) vector_mating.pestres_self, 
                               (VectorAllele::Enum) vector_mating.pestres_mate );
    }

    AllelePair_t  VectorMatingStructure::GetHEG() const
    {
        return std::make_pair( (VectorAllele::Enum) vector_mating.HEG_self, 
                               (VectorAllele::Enum) vector_mating.HEG_mate );
    }

    bool VectorMatingStructure::IsMated() const
    {
        bool HEGmated  = GetHEG().second != VectorAllele::NotMated;
        bool pestMated = GetPesticideResistance().second != VectorAllele::NotMated;

        if( HEGmated == pestMated )
        {
            // Either both are mated or both are unmated
            return HEGmated && pestMated;
        }
        else
        {
            // Otherwise something has gone wrong
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "Pesticide resistance and HEG must either both be mated or both be unmated." );
        }
    }

    // ---------------------- Helper function to determine cytoplasmic compatibility in mating --------------------
    bool VectorMatingStructure::WolbachiaCompatibleMating(VectorWolbachia::Enum femaleWolbachia, VectorWolbachia::Enum maleWolbachia)
    {
        switch(femaleWolbachia)
        {
            case VectorWolbachia::WOLBACHIA_FREE:
                if ( maleWolbachia != VectorWolbachia::WOLBACHIA_FREE )
                {
                    // Wolbachia-free female is cytoplasmically incompatible with any Wolbachia-infected male
                    return false;
                }
                break;

            case VectorWolbachia::VECTOR_WOLBACHIA_A:
                if ( maleWolbachia == VectorWolbachia::VECTOR_WOLBACHIA_B ||
                     maleWolbachia== VectorWolbachia::VECTOR_WOLBACHIA_AB )
                {
                    // Female with Wolbachia A is incompatible to male with either B or AB
                    return false;
                }
                break;

            case VectorWolbachia::VECTOR_WOLBACHIA_B:
                if ( maleWolbachia == VectorWolbachia::VECTOR_WOLBACHIA_A ||
                     maleWolbachia == VectorWolbachia::VECTOR_WOLBACHIA_AB )
                {
                    // Female with Wolbachia B is incompatible to male with either A or AB
                    return false;
                }
                break;

            case VectorWolbachia::VECTOR_WOLBACHIA_AB:
                // Female with Wolbachia AB is compatible with any males
                break;

            default:
                throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__, "femaleWolbachia", femaleWolbachia, VectorWolbachia::pairs::lookup_key(femaleWolbachia) );

        }

        // Otherwise, the mating is compatible
        return true;
    }

    // ---------------------- Determine egg traits based on mated alleles --------------------
    AlleleFractions_t VectorMatingStructure::GetAlleleFractions(AllelePair_t _alleles)
    {
        AlleleFractions_t fractions;

        std::pair<float,float> allele1 = GetAllele(_alleles.first);
        std::pair<float,float> allele2 = GetAllele(_alleles.second);

        fractions[VectorAllele::WILD] = allele1.first  * allele2.first;
        fractions[VectorAllele::HALF] = allele1.first  * allele2.second + allele1.second * allele2.first;
        fractions[VectorAllele::FULL] = allele1.second * allele2.second;

        return fractions;
    }

    // Function for germline homing in females only
    AlleleFractions_t VectorMatingStructure::GetAlleleFractionsEarlyHoming(AllelePair_t _alleles, float homing)
    {
        AlleleFractions_t fractions;

        std::pair<float,float> allele1 = GetAlleleEarlyHoming(_alleles.first, homing);
        std::pair<float,float> allele2 = GetAllele(_alleles.second);

        fractions[VectorAllele::WILD] = allele1.first  * allele2.first;
        fractions[VectorAllele::HALF] = allele1.first  * allele2.second + allele1.second * allele2.first;
        fractions[VectorAllele::FULL] = allele1.second * allele2.second;

        return fractions;
    }

    // Function for germline homing in females and males
    AlleleFractions_t VectorMatingStructure::GetAlleleFractionsDualEarlyHoming(AllelePair_t _alleles, float homing)
    {
        AlleleFractions_t fractions;

        std::pair<float,float> allele1 = GetAlleleEarlyHoming(_alleles.first, homing);
        std::pair<float,float> allele2 = GetAlleleEarlyHoming(_alleles.second, homing);

        fractions[VectorAllele::WILD] = allele1.first  * allele2.first;
        fractions[VectorAllele::HALF] = allele1.first  * allele2.second + allele1.second * allele2.first;
        fractions[VectorAllele::FULL] = allele1.second * allele2.second;

        return fractions;
    }

    // -------------------------------------------------------------------------------------
    // Helper function to take a single parent's alleles (e.g. HALF)
    // and return the fractional allocation of each gene (e.g. 0.5,0.5) to the eggs
    std::pair<float, float> VectorMatingStructure::GetAllele( VectorAllele::Enum _allele)
    {
        switch(_allele)
        {
        case VectorAllele::WILD:
            return std::make_pair(1.0f, 0.0f);
            
        case VectorAllele::HALF:
            return std::make_pair(0.5f, 0.5f);

        case VectorAllele::FULL:
            return std::make_pair(0.0f, 1.0f);

        default:
            throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__, "_allele", _allele, VectorAllele::pairs::lookup_key(_allele) );
        }
    }

    std::pair<float, float> VectorMatingStructure::GetAlleleEarlyHoming( VectorAllele::Enum _allele, float homing)
    {
        switch(_allele)
        {
        case VectorAllele::WILD:
            return std::make_pair(1.0f, 0.0f);
            
        case VectorAllele::HALF:
            return std::make_pair(0.5f-0.5f*homing, 0.5f+0.5f*homing);

        case VectorAllele::FULL:
            return std::make_pair(0.0f, 1.0f);

        default:
            throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__, "_allele", _allele, VectorAllele::pairs::lookup_key(_allele) );
        }
    }

    void VectorMatingStructure::serialize(IArchive& ar, VectorMatingStructure& structure)
    {
        ar & structure.vector_mating.index;
    }
}
