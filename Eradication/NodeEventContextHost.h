/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include <string>
#include <list>
#include <vector>

#include "IdmApi.h"
#include "NodeEventContext.h"

namespace Kernel
{
    struct NodeDemographics;
    class Node;
    
    struct INodeContext;
    class IndividualHuman;

    // The NodeEventContextHost implements functionality properly belonging to the Node class but it split out manually to make development easier.
    // Like, you know, what partial class declarations are for.
    class IDMAPI NodeEventContextHost : public INodeEventContext,
                                        public INodeInterventionConsumer,
                                        public IOutbreakConsumer,
                                        public ICampaignCostObserver,
                                        public INodeTriggeredInterventionConsumer
    {
        IMPLEMENT_NO_REFERENCE_COUNTING()

    public:
        NodeEventContextHost();
        NodeEventContextHost(Node* _node);
        virtual ~NodeEventContextHost();

        // INodeEventContext
        virtual QueryResult QueryInterface(iid_t iid, void** pinstance);
        virtual void VisitIndividuals(individual_visit_function_t func);
        virtual int VisitIndividuals(IVisitIndividual*, int limit = -1 );
        virtual const NodeDemographics& GetDemographics();
        virtual bool GetUrban() const;
        virtual IdmDateTime GetTime() const;
        virtual bool IsInPolygon(float* vertex_coords, int numcoords); // might want to create a real polygon object at some point
        virtual bool IsInPolygon( const json::Array &poly );
        virtual bool IsInExternalIdSet( const tNodeIdList& nodelist );
        virtual ::RANDOMBASE* GetRng();
        virtual INodeContext* GetNodeContext();
        virtual int GetIndividualHumanCount() const;
        virtual int GetExternalId() const;

        virtual void UpdateInterventions(float = 0.0f);

        // TODO: methods to install hooks for birth and other things...can follow similar pattern presumably

        virtual void RegisterTravelDistributionSource(ITravelLinkedDistributionSource *tles, TravelEventType type);
        virtual void UnregisterTravelDistributionSource(ITravelLinkedDistributionSource *tles, TravelEventType type);

        virtual const suids::suid & GetId() const;
        virtual void SetContextTo(INodeContext* context);
        virtual std::list<INodeDistributableIntervention*> GetInterventionsByType(const std::string& type_name);
        virtual void PurgeExisting( const std::string& iv_name );

        // INodeInterventionConsumer
        virtual bool GiveIntervention( INodeDistributableIntervention * pIV );

        // IOutbreakConsumer
        virtual void AddImportCases(StrainIdentity* outbreak_strainID, float import_age, NaturalNumber num_cases_per_node );
        //virtual void IncreasePrevalence(StrainIdentity* outbreak_strainID, IEventCoordinator2* pEC);

        // IIndividualTriggeredInterventionConsumer
        virtual void RegisterNodeEventObserver( IIndividualEventObserver *pIEO, const IndividualEventTriggerType::Enum &trigger );
        virtual void UnregisterNodeEventObserver( IIndividualEventObserver *pIEO, const IndividualEventTriggerType::Enum &trigger );
        virtual void TriggerNodeEventObservers( IIndividualHumanEventContext *ihec, const IndividualEventTriggerType::Enum &trigger );
        virtual void RegisterNodeEventObserverByString( IIndividualEventObserver *pIEO, const std::string &trigger );
        virtual void UnregisterNodeEventObserverByString( IIndividualEventObserver *pIEO, const std::string &trigger );
        virtual void TriggerNodeEventObserversByString( IIndividualHumanEventContext *ihec, const std::string &trigger );

        //////////////////////////////////////////////////////////////////////////
         
        void ProcessArrivingIndividual(IndividualHuman *ih);
        void ProcessDepartingIndividual(IndividualHuman *ih);

        // ICampaignCostObserver
        virtual void notifyCampaignExpenseIncurred( float expenseIncurred, const IIndividualHumanEventContext * pIndiv );
        virtual void notifyCampaignEventOccurred( /*const*/ ISupports * pDistributedIntervention, /*const*/ ISupports * pDistributor, /*const*/ IIndividualHumanContext * pDistributeeIndividual );

    protected:
        void DisposeOfUnregisteredObservers();

#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        Node* node;

        typedef std::map<ITravelLinkedDistributionSource*,int> travel_distribution_source_map_t;
        travel_distribution_source_map_t arrival_distribution_sources;
        travel_distribution_source_map_t departure_distribution_sources;

        void cleanupDistributionSourceMap( travel_distribution_source_map_t &map );
        travel_distribution_source_map_t* sourcesMapForType( TravelEventType type );
        typedef std::list< INodeDistributableIntervention * > interventions_list_t;
        interventions_list_t interventions;

        typedef std::map< std::string, std::list<IIndividualEventObserver*> > ieo_list_t;
        ieo_list_t individual_event_observers;
        ieo_list_t disposed_observers;

        typedef std::list<INodeDistributableIntervention*> ndi_list_t;
        ndi_list_t node_interventions;
#pragma warning( pop )

        virtual void PropagateContextToDependents(); // pass context to interventions if they need it
        void IncrementCampaignCost(float cost);

#if USE_JSON_SERIALIZATION
    public:
        // IJsonSerializable Interfaces
        virtual void JSerialize( IJsonObjectAdapter* root, JSerializer* helper ) const;
        virtual void JDeserialize( IJsonObjectAdapter* root, JSerializer* helper );
#endif

#if USE_BOOST_SERIALIZATION
    private:
        friend class boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive &ar, NodeEventContextHost& nec, const unsigned int v);
#endif
    };
}
