
#pragma once

#include "Properties.h"
#include "ExternalNodeId.h"
#include "Configure.h"
#include "InterventionName.h"


namespace Kernel
{
    struct INodeEventContext;
    struct IIndividualHuman;
    struct IdmDateTime;
    struct IIndividualHumanEventContext;

    class ReportFilter 
    {
    public:
        ReportFilter( const char* pReportEnableParameterName,
                      const char* pParamNamePrefix,
                      bool useYears,
                      bool useHumanMinMaxAge,
                      bool useHumanOther,
                      bool disableFilenameSuffix = false );
        ~ReportFilter();

        virtual void ConfigureParameters( JsonConfigurable& rOwner, const Configuration* pInputJson );
        virtual void CheckParameters(const Configuration* inputJson );
        virtual void CheckForValidNodeIDs( const std::string& rReportName, const std::vector<ExternalNodeId_t>& nodeIds_demographics );
        virtual void Initialize();

        virtual std::string GetNewReportName( const std::string& rBaseReportName ) const;
        virtual bool IsValidTime( const IdmDateTime& rDateTime ) const;
        virtual bool IsValidNode( INodeEventContext* pNEC ) const;
        virtual bool IsValidHuman( const IIndividualHuman* pHuman ) const;

        virtual float GetStartDay() const;
        virtual uint32_t GetNumNodesIncluded() const;

    protected:
        const char* m_pReportEnableParameterName;
        std::string m_ParameterNameStartDay;
        std::string m_ParameterNameEndDay;
        std::string m_ParameterNameStartYear;
        std::string m_ParameterNameEndYear;
        std::string m_ParameterNameFilenameSuffix;
        std::string m_ParameterNameNodeIds;
        std::string m_ParameterNameMinAge;
        std::string m_ParameterNameMaxAge;
        std::string m_ParameterNameMustHaveIP;
        std::string m_ParameterNameMustHaveIntervention;
        std::string m_DescriptionMinAgeYears;
        std::string m_DescriptionMaxAgeYears;
        std::string m_DescriptionMustHaveIP;
        std::string m_DescriptionMustHaveIntervention;
        bool m_IsUsingYears;
        bool m_IsUsingHumanMinMaxAge;
        bool m_IsUsingHumanOther;
        bool m_DisableFilenameSuffix;
        float m_StartDay;
        float m_EndDay;
        float m_StartYear;
        float m_EndYear;
        std::string m_FilenameSuffix;
        std::vector<ExternalNodeId_t> m_NodeIdList;
        std::map<ExternalNodeId_t, bool> m_NodesToInclude;
        float m_MinAgeDays;
        float m_MaxAgeDays;
        float m_MinAgeYears;
        float m_MaxAgeYears;
        std::string m_IPFilterString;
        IPKeyValue m_IPFilter;
        std::string m_MustHaveInterventionNameString;
        InterventionName m_MustHaveInterventionName;
    };
}
