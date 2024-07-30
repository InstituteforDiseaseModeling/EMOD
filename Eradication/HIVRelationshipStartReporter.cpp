
#include "stdafx.h"

#include <string>
#include "HIVRelationshipStartReporter.h"
#include "Log.h"
#include "Exceptions.h"
#include "IIndividualHumanSTI.h"
#include "IIndividualHumanHIV.h"
#include "HIVEnums.h"
#include "IHIVInterventionsContainer.h"
#include "SusceptibilityHIV.h"
#include "InfectionHIV.h"

SETUP_LOGGING( "HIVRelationshipStartReporter" )

using namespace std;

namespace Kernel
{
    GET_SCHEMA_STATIC_WRAPPER_IMPL( HIVRelationshipStartReporter, HIVRelationshipStartReporter )

    IReport* HIVRelationshipStartReporter::Create(ISimulation* simulation)
    {
        return new HIVRelationshipStartReporter(simulation);
    }

    HIVRelationshipStartReporter::HIVRelationshipStartReporter(ISimulation* sim)
        : StiRelationshipStartReporter( sim )
        , hiv_report_data()
        , m_IncludeHivData( true )
    {
    }

    HIVRelationshipStartReporter::~HIVRelationshipStartReporter()
    {
    }

    bool HIVRelationshipStartReporter::Configure( const Configuration* inputJson )
    {
        initConfigTypeMap( "Report_Relationship_Start_Include_HIV_Disease_Statistics", &m_IncludeHivData, RelStart_Include_HIV_Disease_Statistics_DESC_TEXT, true, "Report_Relationship_Start" );

        bool ret = StiRelationshipStartReporter::Configure( inputJson );

        return ret;
    }

    std::string HIVRelationshipStartReporter::GetHeader() const
    {
        std::string header = StiRelationshipStartReporter::GetHeader();
        if( m_IncludeHivData )
        {
            header += ",";
            header += "A_CD4_count,";
            header += "B_CD4_count,";
            header += "A_viral_load,";
            header += "B_viral_load,";
            header += "A_HIV_disease_stage,";
            header += "B_HIV_disease_stage,";
            header += "A_HIV_Tested_Positive,";
            header += "B_HIV_Tested_Positive,";
            header += "A_HIV_Received_Results,";
            header += "B_HIV_Received_Results";
        }
        return header;
    }

    void HIVRelationshipStartReporter::ClearData()
    {
        StiRelationshipStartReporter::ClearData();
        hiv_report_data.clear();
    }

    void HIVRelationshipStartReporter::CollectOtherData( unsigned int relationshipID,
                                                         IIndividualHumanSTI* pPartnerA,
                                                         IIndividualHumanSTI* pPartnerB )
    {
        if( !m_IncludeHivData )
        {
            return;
        }
        IIndividualHumanHIV* p_partner_hiv_A = GetIndividualHumanHIV( pPartnerA );
        IIndividualHumanHIV* p_partner_hiv_B = GetIndividualHumanHIV( pPartnerB );

        float a_cd4_count = p_partner_hiv_A->GetHIVSusceptibility()->GetCD4count();
        float b_cd4_count = p_partner_hiv_B->GetHIVSusceptibility()->GetCD4count();

        float a_viral_load = -1.0 ;
        float b_viral_load = -1.0 ;
        unsigned int a_disease_stage = 0 ;
        unsigned int b_disease_stage = 0 ;

        if( pPartnerA->IsInfected() )
        {
            a_viral_load    = p_partner_hiv_A->GetHIVInfection()->GetViralLoad();
            a_disease_stage = p_partner_hiv_A->GetHIVInfection()->GetStage();
        }
        if( pPartnerB->IsInfected() )
        {
            b_viral_load    = p_partner_hiv_B->GetHIVInfection()->GetViralLoad();
            b_disease_stage = p_partner_hiv_B->GetHIVInfection()->GetStage();
        }

        int a_tested_positive = GetHivTestedPositive( p_partner_hiv_A );
        int b_tested_positive = GetHivTestedPositive( p_partner_hiv_B );

        std::string a_received_results = GetReceivedTestResultForHIV( p_partner_hiv_A );
        std::string b_received_results = GetReceivedTestResultForHIV( p_partner_hiv_B );

        std::stringstream ss ;
        ss                       << ","
           << a_cd4_count        << ","
           << b_cd4_count        << ","
           << a_viral_load       << ","
           << b_viral_load       << ","
           << a_disease_stage    << ","
           << b_disease_stage    << ","
           << a_tested_positive  << ","
           << b_tested_positive  << ","
           << a_received_results << ","
           << b_received_results ;

        hiv_report_data[ relationshipID ] = ss.str();
    }

    std::string HIVRelationshipStartReporter::GetOtherData( unsigned int relationshipID )
    {
        if( m_IncludeHivData )
        {
            return hiv_report_data[ relationshipID ];
        }
        else
        {
            return "";
        }
    }

    IIndividualHumanHIV* HIVRelationshipStartReporter::GetIndividualHumanHIV( IIndividualHumanSTI* pPartner )
    {
        IIndividualHumanHIV* p_hiv_individual = nullptr;
        if( pPartner->QueryInterface(GET_IID(IIndividualHumanHIV), (void**)&p_hiv_individual) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "pPartner", "IIndividualHumanHIV", "IIndividualHumanSTI*" );
        }
        return p_hiv_individual ;
    }

    int HIVRelationshipStartReporter::GetHivTestedPositive( IIndividualHumanHIV* pPartner )
    {
        return pPartner->GetMedicalHistory()->EverTestedHIVPositive() ? 1 : 0;
    }

    std::string HIVRelationshipStartReporter::GetReceivedTestResultForHIV( IIndividualHumanHIV* pPartner )
    {
        IHIVMedicalHistory * p_med_history = pPartner->GetMedicalHistory();

        ReceivedTestResultsType::Enum results_enum = p_med_history->ReceivedTestResultForHIV();
        std::string results_str = ReceivedTestResultsType::pairs::lookup_key( results_enum );
        return results_str ;
    }
}
