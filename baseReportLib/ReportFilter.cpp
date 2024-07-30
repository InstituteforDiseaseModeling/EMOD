
#include "stdafx.h"
#include "ReportFilter.h"

#include "report_params.rc"
#include "Common.h"
#include "Configure.h"
#include "Log.h"
#include "NodeEventContext.h"
#include "IIndividualHuman.h"
#include "IdmDateTime.h"
#include "IdmString.h"

SETUP_LOGGING( "ReportFilter" )

namespace Kernel
{
    ReportFilter::ReportFilter( const char* pReportEnableParameterName, //can be empty string or nullptr
                                const char* pParamNamePrefix, // cannont be nullptr
                                bool useYears,
                                bool useHumanMinMaxAge,
                                bool useHumanOther,
                                bool disableFilenameSuffix )
        : m_pReportEnableParameterName( nullptr )
        , m_ParameterNameStartDay( pParamNamePrefix )
        , m_ParameterNameEndDay( pParamNamePrefix )
        , m_ParameterNameStartYear( pParamNamePrefix )
        , m_ParameterNameEndYear( pParamNamePrefix )
        , m_ParameterNameFilenameSuffix( pParamNamePrefix )
        , m_ParameterNameNodeIds( pParamNamePrefix )
        , m_ParameterNameMinAge( pParamNamePrefix )
        , m_ParameterNameMaxAge( pParamNamePrefix )
        , m_ParameterNameMustHaveIP( pParamNamePrefix )
        , m_ParameterNameMustHaveIntervention( pParamNamePrefix )
        , m_DescriptionMinAgeYears()
        , m_DescriptionMaxAgeYears()
        , m_DescriptionMustHaveIP()
        , m_DescriptionMustHaveIntervention()
        , m_IsUsingYears( useYears )
        , m_IsUsingHumanMinMaxAge( useHumanMinMaxAge )
        , m_IsUsingHumanOther( useHumanOther )
        , m_DisableFilenameSuffix( disableFilenameSuffix )
        , m_StartDay( 0.0f )
        , m_EndDay( FLT_MAX )
        , m_StartYear( MIN_YEAR )
        , m_EndYear( MAX_YEAR )
        , m_FilenameSuffix()
        , m_NodeIdList()
        , m_NodesToInclude()
        , m_MinAgeDays( 0.0 )
        , m_MaxAgeDays( FLT_MAX )
        , m_MinAgeYears( 0.0 )
        , m_MaxAgeYears( FLT_MAX / DAYSPERYEAR )
        , m_IPFilterString()
        , m_IPFilter()
        , m_MustHaveInterventionNameString()
        , m_MustHaveInterventionName()
    {
        if( (pReportEnableParameterName == nullptr) || (strlen(pReportEnableParameterName) == 0)  )
        {
            m_pReportEnableParameterName = nullptr;
        }
        else
        {
            m_pReportEnableParameterName = pReportEnableParameterName;
        }

        std::string underscore;
        if( m_ParameterNameStartDay.length() > 0 )
        {
            // there is a prefix so include a separator
            underscore = "_";
        }
        m_ParameterNameStartDay             += underscore + "Start_Day";
        m_ParameterNameEndDay               += underscore + "End_Day";
        m_ParameterNameStartYear            += underscore + "Start_Year";
        m_ParameterNameEndYear              += underscore + "End_Year";
        m_ParameterNameFilenameSuffix       += underscore + "Filename_Suffix";
        m_ParameterNameNodeIds              += underscore + "Node_IDs_Of_Interest";
        m_ParameterNameMinAge               += underscore + "Min_Age_Years";
        m_ParameterNameMaxAge               += underscore + "Max_Age_Years";
        m_ParameterNameMustHaveIP           += underscore + "Must_Have_IP_Key_Value";
        m_ParameterNameMustHaveIntervention += underscore + "Must_Have_Intervention";

        m_DescriptionMinAgeYears          = ReportFilter_Min_Age_Years_DESC_TEXT;
        m_DescriptionMaxAgeYears          = ReportFilter_Max_Age_Years_DESC_TEXT;
        m_DescriptionMustHaveIP           = ReportFilter_Must_Have_IP_Key_Value_DESC_TEXT;
        m_DescriptionMustHaveIntervention = ReportFilter_Must_Have_Intervention_DESC_TEXT;
    }

    ReportFilter::~ReportFilter()
    {
    }

    // ---------------------
    // --- JsonConfigurable
    // ---------------------

    void ReportFilter::ConfigureParameters( JsonConfigurable& rOwner, const Configuration* inputJson )
    {
        // if there is an enable parameter, then it is in the config and there can be only one.
        if( (m_pReportEnableParameterName == nullptr) && !m_DisableFilenameSuffix )
        {
            rOwner.initConfigTypeMap( m_ParameterNameFilenameSuffix.c_str(), &m_FilenameSuffix, ReportFilter_Filename_Suffix_DESC_TEXT, "" );
        }

        if( m_IsUsingYears )
        {
            rOwner.initConfigTypeMap( m_ParameterNameStartYear.c_str(), &m_StartYear, ReportFilter_Start_Year_DESC_TEXT, MIN_YEAR, MAX_YEAR, MIN_YEAR, m_pReportEnableParameterName );
            rOwner.initConfigTypeMap( m_ParameterNameEndYear.c_str(),   &m_EndYear,   ReportFilter_End_Year_DESC_TEXT,   MIN_YEAR, MAX_YEAR, MAX_YEAR, m_pReportEnableParameterName );

        }
        else
        {
            rOwner.initConfigTypeMap( m_ParameterNameStartDay.c_str(), &m_StartDay, ReportFilter_Start_Day_DESC_TEXT, 0.0f, FLT_MAX, 0.0f,    m_pReportEnableParameterName );
            rOwner.initConfigTypeMap( m_ParameterNameEndDay.c_str(),   &m_EndDay,   ReportFilter_End_Day_DESC_TEXT,   0.0f, FLT_MAX, FLT_MAX, m_pReportEnableParameterName );
        }
        rOwner.initConfigTypeMap( m_ParameterNameNodeIds.c_str(), &m_NodeIdList, ReportFilter_Node_IDs_Of_Interest_DESC_TEXT, 0, INT_MAX, false, m_pReportEnableParameterName );

        if( m_IsUsingHumanMinMaxAge )
        {
            rOwner.initConfigTypeMap( m_ParameterNameMinAge.c_str(), &m_MinAgeYears, m_DescriptionMinAgeYears.c_str(), 0.0f, FLT_MAX / DAYSPERYEAR, 0.0f, m_pReportEnableParameterName );
            rOwner.initConfigTypeMap( m_ParameterNameMaxAge.c_str(), &m_MaxAgeYears, m_DescriptionMaxAgeYears.c_str(), 0.0f, FLT_MAX / DAYSPERYEAR, FLT_MAX / DAYSPERYEAR, m_pReportEnableParameterName );
        }

        if( m_IsUsingHumanOther )
        {
            rOwner.initConfigTypeMap( m_ParameterNameMustHaveIP.c_str(),           &m_IPFilterString,                 m_DescriptionMustHaveIP.c_str(),           "", m_pReportEnableParameterName );
            rOwner.initConfigTypeMap( m_ParameterNameMustHaveIntervention.c_str(), &m_MustHaveInterventionNameString, m_DescriptionMustHaveIntervention.c_str(), "", m_pReportEnableParameterName );
        }
    }

    void ReportFilter::CheckParameters( const Configuration* inputJson )
    {
        if( !JsonConfigurable::_dryrun )
        {
            if( !m_MustHaveInterventionNameString.empty() )
            {
                m_MustHaveInterventionName = m_MustHaveInterventionNameString;
            }

            if( m_IsUsingYears )
            {
                if( m_StartYear >= m_EndYear )
                {
                    throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                            m_ParameterNameStartYear.c_str(), m_StartYear, m_ParameterNameEndYear.c_str(), m_EndYear );
                }
            }
            else
            {
                if( m_StartDay >= m_EndDay )
                {
                    throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                            m_ParameterNameStartDay.c_str(), m_StartDay, m_ParameterNameEndDay.c_str(), m_EndDay );
                }
            }
            if( m_MinAgeYears >= m_MaxAgeYears )
            {
                throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, "Min_Age_Years", m_MinAgeYears, "Max_Age_Years", m_MaxAgeYears );
            }
            m_MinAgeDays = m_MinAgeYears * DAYSPERYEAR;
            m_MaxAgeDays = m_MaxAgeYears * DAYSPERYEAR;
        }
    }

    void ReportFilter::CheckForValidNodeIDs( const std::string& rReportName,
                                             const std::vector<ExternalNodeId_t>& nodeIds_demographics )
    {
        if( m_NodeIdList.size() == 0 )
        {
            for( auto id : nodeIds_demographics )
            {
                m_NodeIdList.push_back( id );
            }
        }
        else
        {
            // ---------------------------------------------------------------------
            // --- Find the collection of node IDs provided by the user that do not
            // --- exist in the demographics.
            // ---------------------------------------------------------------------
            std::sort( m_NodeIdList.begin(), m_NodeIdList.end() );

            std::vector<ExternalNodeId_t> demographic_node_ids( nodeIds_demographics );
            std::sort( demographic_node_ids.begin(), demographic_node_ids.end() );

            std::vector<ExternalNodeId_t> nodes_missing_in_demographics;
            std::set_difference( m_NodeIdList.begin(), m_NodeIdList.end(),
                                 demographic_node_ids.begin(), demographic_node_ids.end(),
                                 std::inserter( nodes_missing_in_demographics, nodes_missing_in_demographics.begin() ) );

            // ----------------------------------------------
            // --- Report on node IDs not in the demographics
            // ----------------------------------------------
            if( !nodes_missing_in_demographics.empty() )
            {
                std::stringstream nodes_missing_in_demographics_str;
                std::copy( nodes_missing_in_demographics.begin(),
                           nodes_missing_in_demographics.end(),
                           ostream_iterator<int>( nodes_missing_in_demographics_str, " " ) );  // list of missing nodes

                std::stringstream error_msg;
                error_msg << "Found NodeIDs in " << rReportName << " that are missing in demographics: ";
                error_msg << nodes_missing_in_demographics_str.str() << ".\n";
                error_msg << "Only nodes configured in demographics can be used in a report.";
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, error_msg.str().c_str() );
            }
        }

        // ------------------------------------------------
        // --- Add the node IDs to a map for fast look-up
        // ------------------------------------------------
        for( auto node_id : m_NodeIdList )
        {
            m_NodesToInclude.insert( std::make_pair( node_id, true ) );
        }
    }

    void ReportFilter::Initialize()
    {
        if( !m_IPFilterString.empty() )
        {
            m_IPFilter = IPKeyValue( m_IPFilterString );
        }
    }

    std::string ReportFilter::GetNewReportName( const std::string& rReportName ) const
    {
        IdmString tmp_report_name = rReportName;
        std::vector<IdmString> parts = tmp_report_name.split( '.' );
        release_assert( parts.size() > 0 );

        std::string output_fn = parts[ 0 ];
        if( !m_FilenameSuffix.empty() )
        {
            output_fn += "_" + m_FilenameSuffix;
        }

        for( int i = 1; i < parts.size(); ++i )
        {
            output_fn += "." + parts[ i ];
        }

        return output_fn;
    }

    bool ReportFilter::IsValidTime( const IdmDateTime& rDateTime ) const
    {
        if( m_IsUsingYears )
        {
            float current_year = rDateTime.Year();
            return ((m_StartYear <= current_year) && (current_year < m_EndYear));
        }
        else
        {
            return ((m_StartDay <= rDateTime.time) && (rDateTime.time < m_EndDay));
        }
    }

    bool ReportFilter::IsValidNode( INodeEventContext* pNEC ) const
    {
        return ((m_NodesToInclude.size() == 0) ||
                (m_NodesToInclude.find( pNEC->GetExternalId() ) != m_NodesToInclude.end()));
    }

    bool ReportFilter::IsValidHuman( const IIndividualHuman* pHuman ) const
    {
        if( m_IsUsingHumanMinMaxAge )
        {
            if( (pHuman->GetAge() < m_MinAgeDays) || (m_MaxAgeDays < pHuman->GetAge()) ) return false;
        }
        if( m_IsUsingHumanOther )
        {
            IIndividualHuman* p_human_non_const = const_cast<IIndividualHuman*>(pHuman);
            if( m_IPFilter.IsValid() && !(p_human_non_const->GetProperties()->Contains( m_IPFilter )) ) return false;
            if( !m_MustHaveInterventionName.empty() &&
                !(pHuman->GetInterventionsContext()->ContainsExistingByName( this->m_MustHaveInterventionName )) ) return false;
        }

        // if both are false, then we aren't checking anything and should return false
        return m_IsUsingHumanMinMaxAge || m_IsUsingHumanOther;
    }

    float ReportFilter::GetStartDay() const
    {
        return m_StartDay;
    }

    uint32_t ReportFilter::GetNumNodesIncluded() const
    {
        return m_NodesToInclude.size();
    }
}
