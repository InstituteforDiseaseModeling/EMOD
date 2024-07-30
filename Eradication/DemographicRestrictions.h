
#pragma once

#include <string>
#include <map>
#include <vector>

#include "Configure.h"
#include "InterventionEnums.h"
#include "PropertyRestrictions.h"
#include "IAdditionalRestrictions.h"

namespace Kernel
{
    struct IIndividualHumanEventContext;

    class DemographicRestrictions
    {
    public:
        DemographicRestrictions( bool age_restrictions = true,
                                 TargetDemographicType::Enum defaultTargetDemographic = TargetDemographicType::Everyone,
                                 bool use_coverage = true,
                                 const char* pDemographicCoverageDescText = Demographic_Coverage_DESC_TEXT );
        virtual ~DemographicRestrictions() { } 

        void ConfigureRestrictions( JsonConfigurable* pParent, const Configuration * inputJson );
        void CheckConfiguration();
        bool HasDefaultRestrictions() const;
        bool IsQualified( const IIndividualHumanEventContext* pIndividual );

        void SetDemographicCoverage( float coverage );
        float GetDemographicCoverage() const;
        TargetDemographicType::Enum GetTargetDemographic() const;
        TargetGender::Enum GetTargetGender() const;
        float GetMinimumAge() const;
        float GetMaximumAge() const;

        std::string GetPropertyRestrictionsAsString() const;

    protected:
        bool allow_age_restrictions;
        bool use_demographic_coverage;
        float demographic_coverage;
        const char* p_demographic_coverage_desc_text;
        TargetDemographicType::Enum default_target_demographic;
        TargetDemographicType::Enum target_demographic;
        float target_age_min_years;
        float target_age_max_years;
        float target_age_min_days;
        float target_age_max_days;
        TargetGender::Enum target_gender;
        jsonConfigurable::tDynamicStringSet property_restrictions_set;
        PropertyRestrictions<IPKey,IPKeyValue,IPKeyValueContainer> property_restrictions;
        bool target_residents_only;
        AdditionalTargetingConfig targeting_config;
        IAdditionalRestrictions* additional_restrictions;
    };
}
