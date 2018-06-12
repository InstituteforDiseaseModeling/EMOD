/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <stdafx.h>

#include "suids.hpp"
#include "INodeInfo.h"
#include "INodeContext.h"

namespace Kernel
{
    class NodeInfo : public INodeInfo
    {
    public:
        NodeInfo();
        NodeInfo( int rank, INodeContext* pNC );
        virtual ~NodeInfo();

        virtual void Update( INodeContext* pNC ) override;

        virtual const suids::suid& GetSuid()       const override { return m_Suid; }
        virtual ExternalNodeId_t   GetExternalID() const override { return m_ExternalId; }

        virtual int   GetRank()             const override { return m_Rank;       }
        virtual float GetPopulation()       const override { return m_Population; }
        virtual float GetLongitudeDegrees() const override { return m_LongitudeDegrees;  }
        virtual float GetLatitudeDegrees()  const override { return m_LatitudeDegrees;   }

        virtual void serialize( IArchive& ar, bool firstTime ) override;
    private:
        suids::suid m_Suid ;
        ExternalNodeId_t m_ExternalId ;
        int   m_Rank ;
        float m_Population ;
        float m_LongitudeDegrees ;
        float m_LatitudeDegrees ;
    };
}
