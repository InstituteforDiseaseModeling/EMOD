/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <string>
#include <map>
#include <vector>

#include "BoostLibWrapper.h"
#include "Configure.h"
#include "InterventionEnums.h"
#include "PropertyRestrictions.h"

namespace Kernel
{
    struct IIndividualHumanEventContext;

    class DemographicRestrictions
    {
    public:
        DemographicRestrictions( bool age_restrictions = true,
                                 TargetDemographicType::Enum defaultTargetDemographic = TargetDemographicType::Everyone,
                                 bool use_coverage = true );
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
    };
}
