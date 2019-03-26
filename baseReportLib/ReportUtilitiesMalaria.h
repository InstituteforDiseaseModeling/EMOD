/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <string>
#include <list>
#include <vector>

namespace Kernel
{
    class IndividualHuman;
    class RANDOMBASE;
}

namespace ReportUtilitiesMalaria
{
    // Smears infectiousness values using a binomial draw given 40 draws
    float BinomialInfectiousness( Kernel::RANDOMBASE * rng, float infec );

    // Converting +ve microscopic fields of view out of 200 slide views to parasite densities
    float FieldsOfViewToDensity( float positive_fields );

    // Adding uncertainty to PCR derived densities a la Walker et al. (2015)
    float NASBADensityWithUncertainty( Kernel::RANDOMBASE * rng, float true_density );

    // Evaluates polynomial of form : f(x) = p( 0 ) * x ^ (n - 1) + p( 1 ) *x ^ (n - 2)…
    // where n = the length of the vector p
    float PolyVal( const std::vector<double>& p, float x );

    // Contain the information about infections for a particular combination of genome markers
    class GenomeMarkerColumn
    {
    public:
        GenomeMarkerColumn( const std::string& rColumnName = "", uint32_t bitMask = 0 )
            : m_ColumnName( rColumnName )
            , m_BitMask( bitMask )
            , m_Count( 0 )
        {
        }

        ~GenomeMarkerColumn() { }

        const std::string& GetColumnName() const { return m_ColumnName; }
        uint32_t           GetBitMask()    const { return m_BitMask; }
        uint32_t           GetCount()      const { return m_Count; }
        void AddCount( uint32_t count ) { m_Count += count; }
        void ResetCount() { m_Count = 0; }

    private:
        std::string m_ColumnName;
        uint32_t    m_BitMask;
        uint32_t    m_Count;
    };
}