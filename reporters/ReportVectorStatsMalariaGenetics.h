
#pragma once

#include "ReportVectorStats.h"
#include "ReportFactory.h"
#include "MalariaGeneticsContexts.h"

namespace Kernel
{
    struct INodeContext;

    struct VectorParasiteStats
    {
        uint32_t num_vectors_none;
        uint32_t num_vectors_only_oocysts;
        uint32_t num_vectors_only_sporozoites;
        uint32_t num_vectors_oocysts_sporozoites;
        uint32_t num_cohorts_oocysts;
        uint32_t num_cohorts_sporozoites;
        uint32_t num_oocysts;
        uint32_t num_sporozoites;
        uint32_t num_infectious_to_adult;
        uint32_t num_infectious_to_infected;

        VectorParasiteStats()
            : num_vectors_none( 0 )
            , num_vectors_only_oocysts( 0 )
            , num_vectors_only_sporozoites( 0 )
            , num_vectors_oocysts_sporozoites( 0 )
            , num_cohorts_oocysts( 0 )
            , num_cohorts_sporozoites( 0 )
            , num_oocysts( 0 )
            , num_sporozoites( 0 )
            , num_infectious_to_adult( 0 )
            , num_infectious_to_infected( 0 )
        {
        }

        void Reset()
        {
            num_vectors_none = 0;
            num_vectors_only_oocysts = 0;
            num_vectors_only_sporozoites = 0;
            num_vectors_oocysts_sporozoites = 0;
            num_cohorts_oocysts = 0;
            num_cohorts_sporozoites = 0;
            num_oocysts = 0;
            num_sporozoites = 0;
            num_infectious_to_adult = 0;
            num_infectious_to_infected = 0;
        }

        VectorParasiteStats& operator+=( const VectorParasiteStats& rhs )
        {
            num_vectors_none                += rhs.num_vectors_none;
            num_vectors_only_oocysts        += rhs.num_vectors_only_oocysts;
            num_vectors_only_sporozoites    += rhs.num_vectors_only_sporozoites;
            num_vectors_oocysts_sporozoites += rhs.num_vectors_oocysts_sporozoites;
            num_cohorts_oocysts             += rhs.num_cohorts_oocysts;
            num_cohorts_sporozoites         += rhs.num_cohorts_sporozoites;
            num_oocysts                     += rhs.num_oocysts;
            num_sporozoites                 += rhs.num_sporozoites;
            num_infectious_to_adult         += rhs.num_infectious_to_adult;
            num_infectious_to_infected      += rhs.num_infectious_to_infected;

            return *this;
        }
    };



    class ReportVectorStatsMalariaGenetics : public ReportVectorStats, public IVectorCounter
    {
        DECLARE_FACTORY_REGISTERED( ReportFactory, ReportVectorStatsMalariaGenetics, IReport )
    public:
        ReportVectorStatsMalariaGenetics();
        virtual ~ReportVectorStatsMalariaGenetics();

        // ISupports
        virtual QueryResult QueryInterface( iid_t iid, void** pinstance ) override;
        virtual int32_t AddRef() override { return ReportVectorStats::AddRef(); }
        virtual int32_t Release() override { return ReportVectorStats::Release(); }

        virtual bool Configure( const Configuration* ) override;
        virtual std::string GetHeader() const override;
        virtual void LogNodeData( Kernel::INodeContext * pNC ) override;

        // IVectorCounter
        virtual void CountVector( IVectorCohort* pCohort ) override;

    protected:
        virtual void ResetOtherCounters() override;
        virtual void CollectOtherData( IVectorPopulationReporting* pIVPR ) override;
        virtual void WriteOtherData() override;

        VectorParasiteStats m_ParasiteStats;
        OtherVectorStats m_OtherStats;
        std::vector<ReportUtilitiesMalaria::BarcodeColumn*> m_BarcodeColumns;
        std::map<int64_t,ReportUtilitiesMalaria::BarcodeColumn*> m_BarcodeColumnMap;
        ReportUtilitiesMalaria::BarcodeColumn m_BarcodeColumnOther;
        std::map<int64_t,int32_t> m_SporozoiteBarcodeHascodeCountMap;
        std::map<INodeContext*,bool> m_HaveRegisteredVectorCounter;
    };
}
