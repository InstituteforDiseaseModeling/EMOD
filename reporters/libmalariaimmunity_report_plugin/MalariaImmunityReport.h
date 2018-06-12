/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <vector>
#include <map>

#include "BaseEventReportIntervalOutput.h"

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
    public:
        MalariaImmunityReport();
        virtual ~MalariaImmunityReport();

        //BaseEventReportIntervalOutput
        virtual bool Configure( const Configuration * inputJson ) override;
        virtual bool notifyOnEvent( IIndividualHumanEventContext *context, const EventTrigger& trigger ) override;

    protected:
        // BaseEventReportIntervalOutput
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