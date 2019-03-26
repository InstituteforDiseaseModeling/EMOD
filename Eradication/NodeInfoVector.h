/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <string>
#include <vector>
#include "NodeInfo.h"

namespace Kernel
{
    class NodeInfoVector : public NodeInfo
    {
    public:
        NodeInfoVector();
        NodeInfoVector( int rank, INodeContext* pNC );
        ~NodeInfoVector();

        virtual void Update( INodeContext* pNC ) override;
        float GetAvailableLarvalHabitat( const std::string& rSpeciesID ) const;
        float GetTotalAvailableLarvalHabitat() const;

        virtual void serialize( IArchive& ar, bool firstTime ) override;

    private:
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // !!! We are using a couple of vectors in an effort to reduce the amount
        // !!! of data and processing in scenarios with a large number of nodes.
        // !!! I did have a map and it increase 25_Madagascar run time by 25%
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        std::vector<std::string> m_Species;
        std::vector<float> m_AvailableLarvalHabitat;
    };
}
