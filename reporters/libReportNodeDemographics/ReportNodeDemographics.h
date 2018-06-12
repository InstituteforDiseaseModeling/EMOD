/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/


#pragma once

#include "BaseTextReport.h"

namespace Kernel
{
    class IPKeyValueContainer;

    class NodeData
    {
    public:
        NodeData()
        : num_people(0)
        , num_infected(0)
        {
        };

        virtual void Reset()
        {
            num_people = 0;
            num_infected = 0;
        }

        uint32_t num_people;
        uint32_t num_infected;
    };

    class ReportNodeDemographics: public BaseTextReport
    {
    public:
        ReportNodeDemographics();
        virtual ~ReportNodeDemographics();

        // BaseEventReport
        virtual bool Configure( const Configuration* ) override;

        virtual void Initialize( unsigned int nrmSize ) override;
        virtual std::string GetHeader() const override;
        virtual bool IsCollectingIndividualData( float currentTime, float dt ) const override;
        virtual void LogNodeData( INodeContext* pNC ) override;
        virtual void LogIndividualData( IIndividualHuman* individual ) override;

    protected:
        ReportNodeDemographics( const std::string& rReportName );
        virtual NodeData* CreateNodeData();
        virtual void WriteNodeData( const NodeData* pData );
        virtual void LogIndividualData( IIndividualHuman* individual, NodeData* pNodeData ) {};

        int GetIPIndex( IPKeyValueContainer* pProps ) const;

        bool m_StratifyByGender;
        bool m_StratifyByAge;
        std::vector<float> m_AgeYears;
        std::string m_IPKeyToCollect;
        std::vector<std::string> m_IPValuesList;
        std::vector<std::vector<std::vector<NodeData*>>> m_Data;
    };
}
