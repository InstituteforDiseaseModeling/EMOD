/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "Sugar.h"
#include "Environment.h"
#include "SimpleEventCoordinator.h"
#include "Configuration.h"
#include "ConfigurationImpl.h"
#include "FactorySupport.h"
#include "InterventionFactory.h"
#include "Contexts.h"

SETUP_LOGGING( "SimpleEventCoordinator" )

namespace Kernel
{
    IMPL_QUERY_INTERFACE2(SimpleInterventionDistributionEventCoordinator, IEventCoordinator, IConfigurable)

    IMPLEMENT_FACTORY_REGISTERED(SimpleInterventionDistributionEventCoordinator)

    GET_SCHEMA_STATIC_WRAPPER_IMPL(SimpleInterventionDistributionEventCoordinator, SimpleInterventionDistributionEventCoordinator)

    // ctor
    SimpleInterventionDistributionEventCoordinator::SimpleInterventionDistributionEventCoordinator()
    : parent(nullptr)
    , coverage(0)
    , distribution_complete(false)
    {
        initConfigTypeMap("Coverage", &coverage, Coverage_DESC_TEXT, 0.0f, 1.0f, 1.0f );
    }

    bool
    SimpleInterventionDistributionEventCoordinator::Configure(
        const Configuration * inputJson
    )
    {
        initConfigComplexType("Intervention_Config", &intervention_config, Intervention_Config_DESC_TEXT );
        bool retval = JsonConfigurable::Configure( inputJson );
        if( retval )
        {
            InterventionValidator::ValidateIntervention( intervention_config._json, inputJson->GetDataLocation() );
        }
        return retval ;

    }

    void SimpleInterventionDistributionEventCoordinator::Update( float dt )
    {
        // this simple distributor does not need any global sync
    }

    void SimpleInterventionDistributionEventCoordinator::UpdateNodes( float dt )
    {

        auto qi_as_config = Configuration::CopyFromElement(intervention_config._json,"campaign");

        INodeEventContext::individual_visit_function_t visit_func = 
            [this,qi_as_config](IIndividualHumanEventContext *ihec)
        {
            // intervention logic goes here
            if (randgen->e() < coverage)
            {
                // instantiate and distribute intervention
                IDistributableIntervention *di = InterventionFactory::getInstance()->CreateIntervention(qi_as_config);
                if (di)
                {
                    if (!di->Distribute(ihec->GetInterventionsContext(), nullptr ))
                    {
                        di->Release();
                    }
                } 
                else
                {
                    //LOG_ERR_F("Failed to instantiate intervention of class %s\n", (*intervention_config)["class"]);
                    throw FactoryCreateFromJsonException( __FILE__, __LINE__, __FUNCTION__, "TBD" /* (*intervention_config)["class"]*/ );
                }
            }
        };

        INodeDistributableIntervention *ndi = InterventionFactory::getInstance()->CreateNDIIntervention(qi_as_config);
        INodeDistributableIntervention *ndi2 = nullptr;

        for (auto event_context : cached_nodes)
        {
            if(ndi)
            {
                ndi2 = InterventionFactory::getInstance()->CreateNDIIntervention(qi_as_config);

                if (!ndi2->Distribute( event_context, this ) )
                {
                    ndi2->Release(); 
                }
            }
            else
            {
                event_context->VisitIndividuals(visit_func);
            }
        }

        if(ndi) ndi->Release();

        distribution_complete = true; // we're done, signal disposal ok
        // this signals each process individually that its ok to clean up, in general if the completion times might be different on different nodes 
        // we'd want to coordinate the cleanup signal in Update()

        delete qi_as_config;
        qi_as_config = nullptr;
    }

    void SimpleInterventionDistributionEventCoordinator::regenerateCachedNodeContextPointers()
    {
        // regenerate the cached INodeEventContext* pointers fromthe cached node suids
        // the fact that this needs to happen is probably a good argument for the EC to own the NodeSet, since it needs to query the SEC for the node ids and context pointers anyway
        cached_nodes.clear();
        for (auto& node_id : node_suids)
        {
            cached_nodes.push_back(parent->GetNodeEventContext(node_id));
        }
    }

    void
    SimpleInterventionDistributionEventCoordinator::SetContextTo(ISimulationEventContext *isec) { parent = isec; regenerateCachedNodeContextPointers(); }

    bool
    SimpleInterventionDistributionEventCoordinator::IsFinished() { return distribution_complete; } // returns false when the EC requires no further updates and can be disposed of

    void
    SimpleInterventionDistributionEventCoordinator::ProcessDeparting(IIndividualHumanEventContext *dc) { } // these do nothing for now

    void
    SimpleInterventionDistributionEventCoordinator::ProcessArriving(IIndividualHumanEventContext *dc) { }

    float
    SimpleInterventionDistributionEventCoordinator::GetDemographicCoverage() const { return coverage; }

    TargetDemographicType::Enum
    SimpleInterventionDistributionEventCoordinator::GetTargetDemographic() const { return TargetDemographicType::Everyone; }

    float SimpleInterventionDistributionEventCoordinator::GetMinimumAge() const { return 0.0f; }
    float SimpleInterventionDistributionEventCoordinator::GetMaximumAge() const { return 116.0f; }
}
