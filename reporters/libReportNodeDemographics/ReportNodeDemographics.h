/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/


#pragma once

#include "BaseTextReport.h"
#include "PropertiesString.h"

namespace Kernel
{
    struct NodeData
    {
        NodeData()
        : num_people(0)
        , num_infected(0)
        {
        };

        int num_people;
        int num_infected;
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
        virtual void LogIndividualData( IIndividualHuman* individual ) override;
        virtual void LogNodeData( INodeContext* pNC ) override;

    private:
        int GetIPIndex( tProperties* pProps ) const;

        std::vector<float> m_AgeYears;
        std::string m_IPKeyToCollect;
        std::vector<std::string> m_IPValuesList;
        std::vector<std::vector<std::vector<NodeData>>> m_Data;
    };
}
