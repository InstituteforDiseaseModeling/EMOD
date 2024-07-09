
#pragma once

#include "ReportVectorGenetics.h"
#include "MalariaGeneticsContexts.h"

namespace Kernel
{
    struct OccystAndSporozoiteData
    {
        float sum_oocyst_duration;
        float num_maturing_oocyst;
        float sum_sporozoites;
        float num_vectors;
        std::vector<ReportUtilitiesMalaria::BarcodeColumn*> barcode_columns;
        ReportUtilitiesMalaria::BarcodeColumn barcode_column_other;
        std::map<int32_t,ReportUtilitiesMalaria::BarcodeColumn*> barcode_column_map;

        OccystAndSporozoiteData()
            : sum_oocyst_duration( 0.0 )
            , num_maturing_oocyst( 0.0 )
            , sum_sporozoites( 0.0 )
            , num_vectors( 0.0 )
            , barcode_columns()
            , barcode_column_other()
            , barcode_column_map()
        {
        }

        ~OccystAndSporozoiteData()
        {
            for( auto p_column : barcode_columns )
            {
                delete p_column;
            }
            barcode_columns.clear();
            barcode_column_map.clear();
        }

        void Reset()
        {
            sum_oocyst_duration = 0.0;
            num_maturing_oocyst = 0.0;
            sum_sporozoites     = 0.0;
            num_vectors         = 0.0;
            for( auto p_column : barcode_columns )
            {
                p_column->ResetCount();
            }
            barcode_column_other.ResetCount();
        }
    };


    class ReportVectorGeneticsMalariaGenetics : public ReportVectorGenetics, public IVectorCounter
    {
        DECLARE_FACTORY_REGISTERED( ReportFactory, ReportVectorGeneticsMalariaGenetics, IReport )
    public:
        ReportVectorGeneticsMalariaGenetics();
        virtual ~ReportVectorGeneticsMalariaGenetics();

        // ISupports
        virtual QueryResult QueryInterface( iid_t iid, void** pinstance );
        virtual int32_t AddRef() { return ReportVectorGenetics::AddRef(); }
        virtual int32_t Release() { return ReportVectorGenetics::Release(); }

        // BaseEventReport
        virtual bool Configure( const Configuration* ) override;

        virtual std::string GetHeader() const override;
        virtual void LogNodeData( INodeContext* pNC ) override;

        // IVectorCounter
        virtual void CountVector( IVectorCohort* pCohort ) override;

    protected:
        ReportVectorGeneticsMalariaGenetics( const std::string& rReportName );

        virtual void InitializeStratificationData( IVectorPopulationReporting* pIVPR ) override;
        virtual void ResetOtherCounters() override;
        virtual void CollectOtherDataByGenome( NameToColumnData& rColumnData ) override;

        std::map<INodeContext*,bool> m_HaveRegisteredVectorCounter;
        std::vector<std::string> m_ParasiteBarcodes;
        std::map<std::string,          OccystAndSporozoiteData*> m_OocystAndSporozoitDataNameMap;
        std::map<VectorGameteBitPair_t,OccystAndSporozoiteData*> m_OocystAndSporozoitDataBitsMap;
        std::map<int64_t,int32_t> m_SporozoiteBarcodeHascodeCountMap;
    };
}
