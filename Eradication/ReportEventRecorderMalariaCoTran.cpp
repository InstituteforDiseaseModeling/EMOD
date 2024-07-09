
#include "stdafx.h"

#include <string>
#include "ReportEventRecorderMalariaCoTran.h"
#include "Log.h"
#include "Exceptions.h"
#include "IndividualEventContext.h"
#include "IIndividualHumanContext.h"
#include "Individual.h"
#include "MalariaContexts.h"

SETUP_LOGGING( "ReportEventRecorderMalariaCoTran" )

namespace Kernel
{
    IReport* ReportEventRecorderMalariaCoTran::CreateReport()
    {
        return new ReportEventRecorderMalariaCoTran();
    }

    ReportEventRecorderMalariaCoTran::ReportEventRecorderMalariaCoTran()
        : ReportEventRecorder()
    {
    }

    ReportEventRecorderMalariaCoTran::~ReportEventRecorderMalariaCoTran()
    {
    }

    std::string ReportEventRecorderMalariaCoTran::GetHeader() const
    {
        std::string header = ReportEventRecorder::GetHeader() ;
        if( IndividualHumanConfig::superinfection )
        {
            std::stringstream ss;
            for( int i = 1 ; i <= IndividualHumanConfig::max_ind_inf; ++i )
            {
                ss << ",Infection" << i;
            }
            for( int i = 1 ; i <= IndividualHumanConfig::max_ind_inf; ++i )
            {
                ss << ",AsexualDensity" << i;
            }
            ss << ",HasClinicalSymptoms";
            header += ss.str();
        }

        return header;
    }

    std::string ReportEventRecorderMalariaCoTran::GetOtherData( IIndividualHumanEventContext *context, 
                                                                const EventTrigger& trigger )
    {
        const IIndividualHuman * individual = context->GetIndividualHumanConst();

        IMalariaHumanContext * p_ind_malaria = nullptr;
        if( s_OK != context->QueryInterface( GET_IID( IMalariaHumanContext ), (void**)&p_ind_malaria ) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "IMalariaHumanContext", "IIndividualHumanEventContext" );
        }

        // -----------------------
        // --- Write Infection IDs
        // -----------------------
        std::stringstream ss;
        for( auto p_inf : individual->GetInfections() )
        {
            ss << "," << p_inf->GetSuid().data;
        }
        // append 0's for the infections less than the max
        for( int i = individual->GetInfections().size(); i < IndividualHumanConfig::max_ind_inf; ++i )
        {
            ss << ",0";
        }

        // --------------------------------------------
        // --- Write Asexual Density for eac infection
        // --------------------------------------------
        for( auto p_inf : individual->GetInfections() )
        {
            IInfectionMalaria * p_inf_malaria = nullptr;
            if (s_OK != p_inf->QueryInterface(GET_IID(IInfectionMalaria), (void**)&p_inf_malaria) )
            {
                throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "p_inf", "IInfectionMalaria", "IInfection");
            }
            ss << "," << p_inf_malaria->get_asexual_density();
        }
        // append 0's for the infections less than the max
        for( int i = individual->GetInfections().size(); i < IndividualHumanConfig::max_ind_inf; ++i )
        {
            ss << ",0";
        }

        char has_symptoms = p_ind_malaria->HasClinicalSymptomContinuing( ClinicalSymptomsEnum::CLINICAL_DISEASE ) ? 'T' : 'F';
        ss << "," << has_symptoms;

        std::stringstream ret_ss ;
        ret_ss << ReportEventRecorder::GetOtherData( context, trigger );
        ret_ss << ss.str();

        return ret_ss.str() ;
    }
}
