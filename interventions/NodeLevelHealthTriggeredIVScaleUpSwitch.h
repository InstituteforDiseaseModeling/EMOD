/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include <string>
#include <list>
#include <vector>

#include "NodeLevelHealthTriggeredIV.h"

namespace Kernel
{
    class NodeLevelHealthTriggeredIVScaleUpSwitch : public NodeLevelHealthTriggeredIV
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, NodeLevelHealthTriggeredIVScaleUpSwitch, INodeDistributableIntervention)

    public:        
        NodeLevelHealthTriggeredIVScaleUpSwitch();
        virtual ~NodeLevelHealthTriggeredIVScaleUpSwitch();
        virtual bool Configure( const Configuration* config );

    protected:

        ScaleUpProfile::Enum demographic_coverage_time_profile;
        float initial_demographic_coverage;
        float primary_time_constant;

        virtual float getDemographicCoverage() const;

        virtual void onDisqualifiedByCoverage( IIndividualHumanEventContext *pIndiv );
		IndividualInterventionConfig not_covered_intervention_configs; // TBD
        bool campaign_contains_not_covered_config;

    private:
#if USE_BOOST_SERIALIZATION
        template<class Archive>
        friend void serialize(Archive &ar, NodeLevelHealthTriggeredIVScaleUpSwitch& iv, const unsigned int v);
#endif
    };
}
