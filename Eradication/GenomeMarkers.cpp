/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "GenomeMarkers.h"
#include "IStrainIdentity.h"
#include "Exceptions.h"
#include "RANDOM.h"

#include "Log.h"

SETUP_LOGGING( "GenomeMarkers" )

namespace Kernel
{
    // ------------------------------------------------------------------------
    // --- GenomeMarkers
    // ------------------------------------------------------------------------

    GenomeMarkers::GenomeMarkers()
        : m_MarkerMap()
        , m_NameSet()
    {
    }

    GenomeMarkers::~GenomeMarkers()
    {
    }

    int GenomeMarkers::Initialize( const std::vector<std::string>& rAllGenomeMarkerNames )
    {
        if( rAllGenomeMarkerNames.size() > 32 ) // 32 because 32 bits in uint32_t
        {
            std::stringstream ss;
            ss << rAllGenomeMarkerNames.size() << " genome markers have been defined.  The maximum is 32.\n.";
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }

        // -------------------------------------------------------------------------------
        // --- We start the bits at one, because by default there can be an infection
        // --- with no markers.  If the user inputs an empty list of markers, everything
        // --- will still work.
        // -------------------------------------------------------------------------------
        uint32_t bit_mask = 1;
        for( auto& r_name : rAllGenomeMarkerNames )
        {
            if( Get( r_name ) != nullptr )
            {
                std::stringstream ss;
                ss << "Duplicate name in Genome_Markers = '" << r_name << "'.  Each name must be case-insensitive unique.\n";
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }
            std::string lower_name = r_name;
            std::transform( lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower );

            m_MarkerMap[ lower_name ] = std::make_pair( r_name, bit_mask );
            m_NameSet.insert( r_name );
            bit_mask *= 2;
        }
        return pow( 2, rAllGenomeMarkerNames.size() ); // two options (on/off) for each marker
    }

    uint32_t GenomeMarkers::CreateBits( const std::vector<std::string>& rGenomeMarkerNames ) const
    {
        uint32_t bit_mask = 0;
        for( auto& r_name : rGenomeMarkerNames )
        {
            uint32_t bits = GetBits( r_name );
            bit_mask |= bits;
        }
        return bit_mask;
    }

    const std::set<std::string>& GenomeMarkers::GetNameSet() const
    {
        return m_NameSet;
    }

    uint32_t GenomeMarkers::Size() const
    {
        return m_MarkerMap.size();
    }

    uint32_t GenomeMarkers::GetBits( const std::string& rName ) const
    {
        const std::pair<std::string, uint32_t>* p_marker = Get( rName );
        if( p_marker == nullptr )
        {
            std::stringstream ss;
            ss << "Unknown genome marker name = '" << rName << "'.  Known markers are:\n";
            for( auto it : m_MarkerMap )
            {
                ss << "'" << it.second.first << "'...";
            }
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
        return p_marker->second;
    }

    const std::pair<std::string, uint32_t>* GenomeMarkers::Get( const std::string& rName ) const
    {
        std::string lower_name = rName;
        std::transform( lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower );

        if( m_MarkerMap.count( lower_name ) == 0 )
        {
            return nullptr;
        }
        else
        {
            return &m_MarkerMap.at( lower_name );
        }
    }


    std::vector<std::vector<std::string>> CreateCombinations( const std::vector<std::string>& rNames, int len, int startPosition, std::vector<std::string>& tmp )
    {
        std::vector<std::vector<std::string>> combos;
        if( len > 0 )
        {
            for( int i = startPosition; i <= rNames.size() - len; ++i )
            {
                tmp[ tmp.size() - len ] = rNames[ i ];
                std::vector<std::vector<std::string>> sub_combos = CreateCombinations( rNames, len - 1, i + 1, tmp );

                combos.insert( combos.end(), sub_combos.begin(), sub_combos.end() );
            }
        }
        else
        {
            combos.push_back( tmp );
        }
        return combos;
    }

    std::vector<std::vector<std::string>> GenomeMarkers::GetCombinations( const std::vector<std::string>& rNames )
    {
        std::vector<std::vector<std::string>> possible_list;

        int n = rNames.size();
        int r = 1;
        while( r <= n )
        {
            std::vector<std::string> tmp( r );
            std::vector<std::vector<std::string>> nCr = CreateCombinations( rNames, r, 0, tmp );
            possible_list.insert( possible_list.end(), nCr.begin(), nCr.end() );
            ++r;
        }

        return possible_list;
    }

    std::vector<std::pair<std::string, uint32_t>> GenomeMarkers::CreatePossibleCombinations() const
    {
        std::vector<std::pair<std::string, uint32_t>> all_combos;
        if( m_MarkerMap.size() > 0 )
        {
            std::vector<std::string> names_vector;
            for( auto& r_marker : m_MarkerMap )
            {
                names_vector.push_back( r_marker.second.first );
            }

            std::vector<std::vector<std::string>> all_combo_names = GetCombinations( names_vector );

            all_combos.push_back( std::make_pair( std::string("NoMarkers"), 0 ) );
            for( auto& combo_names : all_combo_names )
            {
                uint32_t bit_mask = CreateBits( combo_names );
                std::string name = combo_names[ 0 ];
                for( int i = 1 ; i < combo_names.size(); ++i )
                {
                    name += "-" + combo_names[ i ];
                }
                all_combos.push_back( std::make_pair( name, bit_mask) );
            }
        }
        return all_combos;
    }

    uint32_t GenomeMarkers::FromOutcrossing( RANDOMBASE* pRNG, uint32_t id1, uint32_t id2 )
    {
        uint32_t mask = pRNG->ul();
        
        //TODO: we could consider using the number of markers defined in GenomeMarkers
        // to mask out bits we don't care about.
        //mask = mask & ((1 << GenomeMarkers.Size()) - 1);

        uint32_t child_strain = (id1 & mask) | (id2 & ~mask);
        LOG_DEBUG_F( "%d + %d with mask=%d --> %d\n", id1, id2, mask, child_strain );
        return child_strain;
    }
}
