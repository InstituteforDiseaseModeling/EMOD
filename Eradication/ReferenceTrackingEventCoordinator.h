/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "StandardEventCoordinator.h"
#include "InterpolatedValueMap.h"
#include "Types.h"

namespace Kernel
{
    class ReferenceTrackingEventCoordinator : public StandardInterventionDistributionEventCoordinator 
    {
        DECLARE_FACTORY_REGISTERED_EXPORT(EventCoordinatorFactory, ReferenceTrackingEventCoordinator, IEventCoordinator)    

    public:
        DECLARE_CONFIGURED(ReferenceTrackingEventCoordinator)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

        ReferenceTrackingEventCoordinator();
        virtual ~ReferenceTrackingEventCoordinator() { } 

        virtual void Update(float dt) override;
        virtual void preDistribute() override;
        virtual void CheckStartDay( float campaignStartDay ) const override;

    protected:
        virtual void InitializeRepetitions( const Configuration* inputJson ) override;
        virtual bool TargetedIndividualIsCovered(IIndividualHumanEventContext *ihec) override;

        InterpolatedValueMap year2ValueMap;
        float end_year;
        std::vector< suids::suid_data_t > haves;
    };
}
