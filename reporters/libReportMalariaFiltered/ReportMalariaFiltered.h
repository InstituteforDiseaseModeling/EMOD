/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <map>

#include "BaseTextReport.h"
#include "ReportMalaria.h"

namespace Kernel
{
    class ReportMalariaFiltered : public ReportMalaria
    {
    public:
        ReportMalariaFiltered();
        virtual ~ReportMalariaFiltered();

        // ReportMalaria
        virtual bool Configure( const Configuration* ) override;
        virtual void Initialize( unsigned int nrmSize ) override;

        virtual void UpdateEventRegistration( float currentTime, 
                                              float dt, 
                                              std::vector<INodeEventContext*>& rNodeEventContextList,
                                              ISimulationEventContext* pSimEventContext ) override;
        virtual void BeginTimestep() override;
        virtual void LogIndividualData( IIndividualHuman* individual ) override;
        virtual bool IsCollectingIndividualData( float currentTime, float dt ) const override;
        virtual void LogNodeData( INodeContext* pNC ) override;
    private:
        bool IsValidNode( uint32_t externalNodeID ) const;

        std::map<uint32_t,bool> m_NodesToInclude;
        float m_StartDay;
        float m_EndDay;
        bool m_IsValidDay;
    };
}
