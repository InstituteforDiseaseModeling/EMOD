
#pragma once

#include "ISupports.h"

namespace Kernel
{
    struct ReportInterventionData
    {
        std::string intervention_name;
        bool is_individual;
        float efficacy_attracting;
        float efficacy_repelling;
        float efficacy_blocking;
        float efficacy_killing;
        float efficacy_usage;
        float efficacy_acq;
        float efficacy_tran;
        float efficacy_mort;
        float concentration_drug;

        ReportInterventionData( bool isIndividual = true, const std::string& rName="" )
            : intervention_name( rName )
            , is_individual( isIndividual )
            , efficacy_attracting( 0.0f )
            , efficacy_repelling( 0.0f )
            , efficacy_blocking( 0.0f )
            , efficacy_killing( 0.0f )
            , efficacy_usage( 0.0f )
            , efficacy_acq( 0.0f )
            , efficacy_tran( 0.0f )
            , efficacy_mort( 0.0f )
            , concentration_drug( 0.0f )
        {
        }

        virtual void AddData( const ReportInterventionData& rData )
        {
            efficacy_attracting += rData.efficacy_attracting;
            efficacy_repelling  += rData.efficacy_repelling;
            efficacy_blocking   += rData.efficacy_blocking;
            efficacy_killing    += rData.efficacy_killing;
            efficacy_usage      += rData.efficacy_usage;
            efficacy_acq        += rData.efficacy_acq;
            efficacy_tran       += rData.efficacy_tran;
            efficacy_mort       += rData.efficacy_mort;
            concentration_drug  += rData.concentration_drug;
        }
    };

    struct IReportInterventionDataAccess : ISupports
    {
        virtual ReportInterventionData GetReportInterventionData() const = 0;
    };
}
