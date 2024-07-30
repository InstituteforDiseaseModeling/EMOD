
#include "stdafx.h"

#include <string>
#include "ReportEventRecorderMalaria.h"
#include "Log.h"
#include "Exceptions.h"
#include "IndividualEventContext.h"
#include "IIndividualHumanContext.h"
#include "Individual.h"
#include "VectorContexts.h"
#include "MalariaContexts.h"

SETUP_LOGGING( "ReportEventRecorderMalaria" )

namespace Kernel
{
    IReport* ReportEventRecorderMalaria::CreateReport()
    {
        return new ReportEventRecorderMalaria();
    }

    ReportEventRecorderMalaria::ReportEventRecorderMalaria()
        : ReportEventRecorder()
    {
    }

    ReportEventRecorderMalaria::~ReportEventRecorderMalaria()
    {
    }

    std::string ReportEventRecorderMalaria::GetHeader() const
    {
        std::stringstream ss;
        ss << ",RelativeBitingRate";
        ss << ",TrueParasiteDensity";
        ss << ",TrueGametocyteDensity";
        ss << ",HasClinicalSymptoms";

        std::string header = ReportEventRecorder::GetHeader() ;
        header += ss.str();
        return header;
    }

    std::string ReportEventRecorderMalaria::GetOtherData( IIndividualHumanEventContext *context, 
                                                          const EventTrigger& trigger )
    {
        IIndividualHumanVectorContext * p_ind_vector = nullptr;
        if (s_OK != context->QueryInterface(GET_IID(IIndividualHumanVectorContext), (void**)&p_ind_vector) )
        {
            throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "context", "IIndividualHumanVectorContext", "IIndividualHumanEventContext");
        }

        IMalariaHumanContext * p_ind_malaria = nullptr;
        if (s_OK != context->QueryInterface(GET_IID(IMalariaHumanContext), (void**)&p_ind_malaria) )
        {
            throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "context", "IMalariaHumanContext", "IIndividualHumanEventContext");
        }

        char has_symptoms = p_ind_malaria->HasClinicalSymptomContinuing( ClinicalSymptomsEnum::CLINICAL_DISEASE ) ? 'T' : 'F';
        std::stringstream ss;
        ss << "," << p_ind_vector->GetRelativeBitingRate();
        ss << "," << p_ind_malaria->GetParasiteDensity();
        ss << "," << p_ind_malaria->GetGametocyteDensity();
        ss << "," << has_symptoms;

        std::stringstream ret_ss ;
        ret_ss << ReportEventRecorder::GetOtherData( context, trigger );
        ret_ss << ss.str();

        return ret_ss.str() ;
    }
}
