
#include "stdafx.h"
#include "ReportFilterRelationship.h"

#include "report_params.rc"
#include "Common.h"
#include "Configure.h"
#include "Log.h"
#include "INodeContext.h"
#include "NodeEventContext.h"
#include "IIndividualHuman.h"
#include "IIndividualHumanSTI.h"
#include "IRelationship.h"
#include "IdmDateTime.h"


SETUP_LOGGING( "ReportFilterRelationship" )

namespace Kernel
{
    ReportFilterRelationship::ReportFilterRelationship( const char* pReportEnableParameterName,
                                                        const char* pParamNamePrefix )
        : ReportFilter( pReportEnableParameterName,
                        pParamNamePrefix,
                        true,
                        true,
                        true )
    {
        m_DescriptionMinAgeYears          = ReportFilterRelationship_Min_Age_Years_DESC_TEXT;
        m_DescriptionMaxAgeYears          = ReportFilterRelationship_Max_Age_Years_DESC_TEXT;
        m_DescriptionMustHaveIP           = ReportFilterRelationship_Must_Have_IP_Key_Value_DESC_TEXT;
        m_DescriptionMustHaveIntervention = ReportFilterRelationship_Must_Have_Intervention_DESC_TEXT;
    }

    ReportFilterRelationship::~ReportFilterRelationship()
    {
    }

    bool ReportFilterRelationship::IsValidRelationship( IRelationship* pRel ) const
    {
        if( (pRel->MalePartner() == nullptr) || (pRel->FemalePartner() == nullptr) ) return false;

        IIndividualHuman* p_male   = pRel->MalePartner()->GetIndividualHuman();
        IIndividualHuman* p_female = pRel->FemalePartner()->GetIndividualHuman();

        float male_age_days   = p_male->GetAge();
        float female_age_days = p_female->GetAge();

        if( !IsValidTime( p_male->GetParent()->GetTime() ) ) return false;
        if( !IsValidNode( p_male->GetParent()->GetEventContext() ) ) return false;

        if( ((male_age_days   < m_MinAgeDays) || (m_MaxAgeDays < male_age_days  )) &&
            ((female_age_days < m_MinAgeDays) || (m_MaxAgeDays < female_age_days)) ) return false;

        if( m_IPFilter.IsValid() )
        {
            if( !(p_male->GetProperties()->Contains( m_IPFilter )) && !(p_female->GetProperties()->Contains( m_IPFilter )) ) return false;
        }
        if( !m_MustHaveInterventionName.empty() )
        {
            if( !(p_male->GetInterventionsContext()->ContainsExistingByName(   m_MustHaveInterventionName )) &&
                !(p_female->GetInterventionsContext()->ContainsExistingByName( m_MustHaveInterventionName )) ) return false;
        }

        return true;
    }
}