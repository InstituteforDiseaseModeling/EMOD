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

#include "Interventions.h"
#include "SimpleTypemapRegistration.h"
#include "Configuration.h"
#include "InterventionFactory.h"
#include "InterventionEnums.h"
#include "NodeEventContext.h"
#include "Configure.h"

namespace Kernel
{
    class NodeLevelHealthTriggeredIV : public IIndividualEventObserver, public BaseNodeIntervention
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, NodeLevelHealthTriggeredIV, INodeDistributableIntervention)

        class tPropertyRestrictions : public JsonConfigurable
        {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) { return e_NOINTERFACE; }
        public:
            tPropertyRestrictions() {}
            virtual void ConfigureFromJsonAndKey( const Configuration *, const std::string &key );
            virtual json::QuickBuilder GetSchema();

            std::list< std::map< std::string, std::string > > _restrictions;
        };

    public:        
        NodeLevelHealthTriggeredIV();
        virtual ~NodeLevelHealthTriggeredIV();
        virtual int AddRef();
        virtual int Release();
        virtual bool Configure( const Configuration* config );
        virtual bool ConfigureTriggers( const Configuration* config );

        // INodeDistributableIntervention
        virtual bool Distribute( INodeEventContext *pNodeEventContext, IEventCoordinator2 *pEC );
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject);
        virtual void SetContextTo(INodeEventContext *context);
        virtual void Update(float dt);

        // IIndividualEventObserver
        virtual bool notifyOnEvent( IIndividualHumanEventContext *context, const std::string& StateChange );

    protected:
        INodeEventContext* parent;

        //std::string   m_trigger_condition; // TODO: is this obsolete now?
        std::vector<std::string>   m_trigger_conditions;
        float max_duration;
        float duration;
        float demographic_coverage;

        float target_age_min;
        float target_age_max;

        IndividualInterventionConfig actual_intervention_config;
        IDistributableIntervention *_di;

        tPropertyRestrictions property_restrictions;
        bool property_restrictions_verified;

        virtual bool qualifiesToGetIntervention( const IIndividualHumanEventContext * pIndividual );
        virtual float getDemographicCoverage() const;
        virtual void onDisqualifiedByCoverage( IIndividualHumanEventContext *pIndiv );

        bool m_disqualified_by_coverage_only;

#if USE_JSON_SERIALIZATION || USE_JSON_MPI
    public:
        // IJsonSerializable Interfaces
        virtual void JSerialize( IJsonObjectAdapter* root, JSerializer* helper ) const;
        virtual void JDeserialize( IJsonObjectAdapter* root, JSerializer* helper );
#endif

#if USE_BOOST_SERIALIZATION
    private:
        template<class Archive>
        friend void serialize(Archive &ar, NodeLevelHealthTriggeredIV& iv, const unsigned int v);
#endif
    };
}
