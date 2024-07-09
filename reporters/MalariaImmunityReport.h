
#pragma once

#include <vector>
#include <map>

#include "BaseEventReportIntervalOutput.h"

#ifndef _REPORT_DLL
#include "ReportFactory.h"
#endif

namespace Kernel
{
    typedef std::vector<double> agebinned_t;
    typedef std::vector<agebinned_t> PfPRbinned_t;

    class ImmunityData : public IIntervalData
    {
    public:
        ImmunityData();
        virtual ~ImmunityData();

        // IIntervalData methods
        virtual void Clear() override;
        virtual void Update( const IIntervalData& rOther ) override;
        virtual void Serialize( IJsonObjectAdapter& rjoa, JSerializer& js ) override;
        virtual void Deserialize( IJsonObjectAdapter& rjoa ) override;

        // other
        void SetVectorSize( int size );

        agebinned_t sum_population_by_agebin;
        agebinned_t sum_MSP_by_agebin;
        agebinned_t sum_nonspec_by_agebin;
        agebinned_t sum_pfemp1_by_agebin;
        agebinned_t sumsqr_MSP_by_agebin;
        agebinned_t sumsqr_nonspec_by_agebin;
        agebinned_t sumsqr_pfemp1_by_agebin;
    };

    class MalariaImmunityReport : public BaseEventReportIntervalOutput
    {
#ifndef _REPORT_DLL
        DECLARE_FACTORY_REGISTERED( ReportFactory, MalariaImmunityReport, IReport )
#endif
    public:
        MalariaImmunityReport();
        virtual ~MalariaImmunityReport();

        // ISupports
        virtual QueryResult QueryInterface( iid_t iid, void** pinstance ) override;
        virtual int32_t AddRef() override { return BaseEventReportIntervalOutput::AddRef(); }
        virtual int32_t Release() override { return BaseEventReportIntervalOutput::Release(); }

        //BaseEventReportIntervalOutput
        virtual bool Configure( const Configuration * inputJson ) override;
        virtual bool notifyOnEvent( IIndividualHumanEventContext *context, const EventTrigger& trigger ) override;

    protected:
        // BaseEventReportIntervalOutput
        virtual void ConfigureEvents( const Configuration* ) override;
        virtual void AccumulateOutput() override;
        virtual void SerializeOutput( float currentTime, IJsonObjectAdapter& output, JSerializer& js ) override;

    private:
        
        std::vector<float> ages;

        // accumulated on each timestep, reset on reporting interval
        ImmunityData* m_pImmunityData;

        // accumulated on each reporting interval, written to output
        std::vector<agebinned_t> MSP_mean_by_agebin;
        std::vector<agebinned_t> MSP_std_by_agebin;
        std::vector<agebinned_t> nonspec_mean_by_agebin;
        std::vector<agebinned_t> nonspec_std_by_agebin;
        std::vector<agebinned_t> PfEMP1_mean_by_agebin;
        std::vector<agebinned_t> PfEMP1_std_by_agebin;

    };
}