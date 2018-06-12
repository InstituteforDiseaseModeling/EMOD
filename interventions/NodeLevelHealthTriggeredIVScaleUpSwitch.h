/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <string>
#include <list>
#include <vector>

#include "NodeLevelHealthTriggeredIV.h"

namespace Kernel
{
    ENUM_DEFINE(ScaleUpProfile,
        ENUM_VALUE_SPEC(Immediate       , 1)
        ENUM_VALUE_SPEC(Linear          , 2)
        ENUM_VALUE_SPEC(Sigmoid         , 3))

    class NodeLevelHealthTriggeredIVScaleUpSwitch : public NodeLevelHealthTriggeredIV
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, NodeLevelHealthTriggeredIVScaleUpSwitch, INodeDistributableIntervention)

    public:        
        NodeLevelHealthTriggeredIVScaleUpSwitch();
        virtual ~NodeLevelHealthTriggeredIVScaleUpSwitch();
        virtual bool Configure( const Configuration* config ) override;

    protected:

        ScaleUpProfile::Enum demographic_coverage_time_profile;
        float initial_demographic_coverage;
        float primary_time_constant;

        virtual float getDemographicCoverage() const override;

        virtual void onDisqualifiedByCoverage( IIndividualHumanEventContext *pIndiv );
        IndividualInterventionConfig not_covered_intervention_configs; // TBD
        bool campaign_contains_not_covered_config;
    };
}
