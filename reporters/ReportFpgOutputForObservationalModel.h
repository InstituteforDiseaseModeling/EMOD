

#pragma once

#include "BaseTextReport.h"
#include "ReportFactory.h"
#include "ReportFilter.h"
#include "ParasiteGenome.h"

namespace Kernel
{
    class ReportFpgOutputForObservationalModel: public BaseTextReport
    {
        DECLARE_FACTORY_REGISTERED( ReportFactory, ReportFpgOutputForObservationalModel, IReport )
    public:
        ReportFpgOutputForObservationalModel();
        virtual ~ReportFpgOutputForObservationalModel();

        // ISupports
        virtual QueryResult QueryInterface( iid_t iid, void** pinstance ) override;
        virtual int32_t AddRef() override { return BaseTextReport::AddRef(); }
        virtual int32_t Release() override { return BaseTextReport::Release(); }

        // BaseEventReport
        virtual bool Configure( const Configuration* ) override;

        virtual void Initialize( unsigned int nrmSize ) override;
        virtual std::string GetHeader() const override;
        virtual void CheckForValidNodeIDs( const std::vector<ExternalNodeId_t>& demographicNodeIds ) override;
        virtual void UpdateEventRegistration( float currentTime,
                                              float dt,
                                              std::vector<INodeEventContext*>& rNodeEventContextList,
                                              ISimulationEventContext* pSimEventContext ) override;
        virtual bool IsCollectingIndividualData( float currentTime, float dt ) const override;
        virtual void LogIndividualData( IIndividualHuman* individual ) override;
        virtual void LogNodeData( INodeContext* pNC ) override;
        virtual void Finalize() override;

    protected:
        ReportFpgOutputForObservationalModel( const std::string& rReportName );

        uint32_t AddGenome( const ParasiteGenome& rGenome );
        void WriteNumpyFile( const std::string& filename );

        ReportFilter m_Filter;
        float m_MinimumParasiteDensity;
        float m_SamplingPeriod;
        bool m_IncludeGenomeID;
        bool m_IsValidTime;
        bool m_IsTimeToSample;
        float m_NextTimeToSample;
        uint32_t m_InfIndex;
        uint32_t m_Year;
        uint32_t m_Month;
        uint32_t m_Day;
        std::string m_NumpyFilenameAlleles;
        std::string m_NumpyFilenameRoots;
        std::vector<size_t> m_GenomeDimensions;

        uint32_t m_NextGenomeIndex;
        std::vector<ParasiteGenome> m_GenomeList;
        std::map<int32_t,uint32_t> m_GenomeMap;
    };
}
