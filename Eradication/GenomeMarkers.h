/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "IGenomeMarkers.h"

namespace Kernel
{
    // GenomeMarkers maintains the relationship between the name of marker and the "bit" used
    // to represent that marker in a strain of an infection.  An infection strain has antigen
    // and genetic components where the bits of these components provide information about
    // the makeup of the infection.  These genome markers are bits in the genetic component
    // of the strain.  This class is important in developing a mapping between the bit and
    // a user friendly name.
    class GenomeMarkers : public IGenomeMarkers
    {
    public:
        GenomeMarkers();
        virtual ~GenomeMarkers();

        // IGenomeMarkers methods
        virtual int Initialize( const std::vector<std::string>& rAllGenomeMarkerNames ) override;

        // Return an integer that has the bits set for each marker name given
        virtual uint32_t CreateBits( const std::vector<std::string>& rGenomeMarkerNames ) const override;

        // Return all the marker names but in a set - used with initConfigTypeMap() when an object is reading
        // a subset of marker names.
        virtual const std::set<std::string>& GetNameSet() const override;

        // Return all of the possible (order-independent) combinations of markers and the bits for that
        // combination of markers.  For example, if the markers are A, B and C, then this method should return
        // four combinations: NoMarkers, A, B, C, A-B. A-C, B-C, A-B-C.  Notice that the string representing
        // the markers has dashes between the marker names.  Also notice that "NoMarkers" is used for the empty set.
        // This method was initially created for reports that needed statistics on all of the combinations.
        virtual std::vector<std::pair<std::string, uint32_t>> CreatePossibleCombinations() const override;

        // Return the number of markers
        virtual uint32_t Size() const override;

        // Return the bits for the given name.  This is an integer with the bit set for this marker.
        virtual uint32_t GetBits( const std::string& rName ) const override;

        // Return all of the combinatinos (order-independent) of given list of names.
        // This does not include the empty set.
        static std::vector<std::vector<std::string>> GetCombinations( const std::vector<std::string>& rNames );

        // Combine the bits of the two genetic ID's into one new one.
        static uint32_t FromOutcrossing( uint32_t id1, uint32_t id2 );

    protected:
        const std::pair<std::string, uint32_t>* Get( const std::string& rName ) const;

        std::map<std::string,std::pair<std::string,uint32_t>> m_MarkerMap;  // this maps lowercase name to input name/bits
        std::set<std::string> m_NameSet;
    };
}
