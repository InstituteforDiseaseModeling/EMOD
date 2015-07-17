/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include <string>
#include <list>
#include <vector>

#include "BoostLibWrapper.h"
#include "EventCoordinator.h"
#include "NodeEventContext.h"
#include "Configure.h"

namespace Kernel
{
    //////////////////////////////////////////////////////////////////////////
    // Example implementations

    // simple distribution ec that just gives out the intervention once to the fraction of people specified by the coverage parameter 
    class SimpleInterventionDistributionEventCoordinator : public IEventCoordinator, public ITravelLinkedDistributionSource, public IEventCoordinator2, public JsonConfigurable
    {
        DECLARE_FACTORY_REGISTERED(EventCoordinatorFactory, SimpleInterventionDistributionEventCoordinator, IEventCoordinator)    
        DECLARE_CONFIGURED(SimpleInterventionDistributionEventCoordinator)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()
        GET_SCHEMA_STATIC_WRAPPER(SimpleInterventionDistributionEventCoordinator)

    public:
        SimpleInterventionDistributionEventCoordinator();

        // IEventCoordinator
        virtual void SetContextTo(ISimulationEventContext *isec);
        virtual void AddNode( const suids::suid& node_suid)
        {
            node_suids.push_back(node_suid);
            cached_nodes.push_back(parent->GetNodeEventContext(node_suid));
        }
        virtual void Update(float dt);
        virtual void UpdateNodes(float dt);
        virtual bool IsFinished();

        // ITravelLinkedDistributionSource
        virtual void ProcessDeparting(IIndividualHumanEventContext *dc);
        virtual void ProcessArriving(IIndividualHumanEventContext *dc);

        virtual float GetDemographicCoverage() const;
        virtual TargetDemographicType::Enum GetTargetDemographic() const;
        virtual float GetMinimumAge() const;
        virtual float GetMaximumAge() const;

    protected:
        ISimulationEventContext  *parent;

        // attributes      
        float coverage;
        bool distribution_complete;

        InterventionConfig intervention_config;
        std::vector<INodeEventContext*> cached_nodes;
        std::vector<suids::suid> node_suids; // to help with serialization

        // helpers
        void regenerateCachedNodeContextPointers();

#if USE_JSON_SERIALIZATION
    public:
        // IJsonSerializable Interfaces
        virtual void JSerialize( IJsonObjectAdapter* root, JSerializer* helper ) const;
        virtual void JDeserialize( IJsonObjectAdapter* root, JSerializer* helper );
#endif  

#if USE_BOOST_SERIALIZATION
    private:
        friend class ::boost::serialization::access;
        // TODO: Move to cpp
        template<class Archive>
        friend void serialize(Archive &ar, SimpleInterventionDistributionEventCoordinator& ec, const unsigned int v);
#endif
    };
}

#if USE_BOOST_SERIALIZATION
namespace Kernel
{
    template<class Archive>
    void serialize(Archive &ar, SimpleInterventionDistributionEventCoordinator& ec, const unsigned int v)
    {
        boost::serialization::void_cast_register<SimpleInterventionDistributionEventCoordinator, IEventCoordinator>();
        boost::serialization::void_cast_register<SimpleInterventionDistributionEventCoordinator, ITravelLinkedDistributionSource>();
        ar & ec.intervention_config;
        ar & ec.coverage;
        ar & ec.distribution_complete;
        
        // need to save the list of suids and restore from them, rather than saving the context pointers
        //ar & nodes;
        ar & ec.node_suids;
    }
}
#endif
