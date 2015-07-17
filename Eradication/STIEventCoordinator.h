/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include <string>
#include <list>
#include <vector>

#include "StandardEventCoordinator.h"
#include "InterventionEnums.h"

namespace Kernel
{
    
    // STI distribution ec that just gives out the intervention once to the fraction of people specified by the coverage parameter
    class STIInterventionDistributionEventCoordinator : public StandardInterventionDistributionEventCoordinator
    {
        DECLARE_FACTORY_REGISTERED_EXPORT(EventCoordinatorFactory, STIInterventionDistributionEventCoordinator, IEventCoordinator)    

    public:
        DECLARE_CONFIGURED(STIInterventionDistributionEventCoordinator)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

        STIInterventionDistributionEventCoordinator();

        // IEventCoordinator
        //virtual void SetContextTo(ISimulationEventContext *isec);
        
    protected:
        // helpers
        virtual float getDemographicCoverageForIndividual( const IIndividualHumanEventContext *pInd ) const;
        BssTargetingType::Enum bss_targeting;

    private:

#if USE_BOOST_SERIALIZATION
        friend class ::boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive &ar, STIInterventionDistributionEventCoordinator &ec, const unsigned int v);
#endif
    };
}
