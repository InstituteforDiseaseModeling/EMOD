/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include <string>
#include <map>

#include "Common.h"
#include "Node.h"
#include "VectorHabitat.h"
#include "VectorPopulation.h"
#include "NodeVectorEventContext.h"

class ReportVector;
class VectorSpeciesReport;

#include "BoostLibWrapper.h"

namespace Kernel
{
    class SpatialReportVector;
    class NodeVector : public Node, public IVectorNodeContext , public INodeVector
    {
        GET_SCHEMA_STATIC_WRAPPER(NodeVector)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

        friend class ::ReportVector;
        friend class ::VectorSpeciesReport;
        friend class SpatialReportVector;

    public:
        static NodeVector *CreateNode(ISimulationContext *context, suids::suid suid);
        virtual ~NodeVector();
        virtual bool Configure( const Configuration* config );

        // INodeContext methods
        // IVectorNodeContext methods
        virtual VectorProbabilities* GetVectorLifecycleProbabilities();
        virtual VectorHabitat*       GetVectorHabitatByType(VectorHabitatType::Enum type);
        virtual void                 AddVectorHabitat(VectorHabitat* habitat);
        virtual float                GetLarvalHabitatMultiplier(VectorHabitatType::Enum type) const;

        virtual IndividualHuman *processImmigratingIndividual(IndividualHumanVector *);
        virtual IndividualHuman *addNewIndividual(float = 1.0f, float = 0.0f, int = 0, int = 0, float = 1.0f, float = 1.0f, float = 1.0f, float = 0.0f);

        virtual void PopulateFromDemographics();
        virtual void SetupIntranodeTransmission();
        virtual void SetParameters(NodeDemographicsFactory *demographics_factory, ClimateFactory *climate_factory);
        virtual void updateInfectivity(float dt = 0.0f);
        virtual void updatePopulationStatistics(float dt = 1.0f);
        void         updateVectorLifecycleProbabilities(float dt);

        void SetVectorPopulations(void);    //default--1 population as before
        virtual void AddVectors(std::string releasedSpecies, VectorMatingStructure _vector_genetics, unsigned long int releasedNumber);

        virtual void processImmigratingVector( VectorCohort* immigrant );
        void processEmigratingVectors();

        virtual VectorPopulationList_t& GetVectorPopulations();

        static TransmissionGroupMembership_t human_to_vector_all;
        static TransmissionGroupMembership_t human_to_vector_indoor;
        static TransmissionGroupMembership_t human_to_vector_outdoor;
        static TransmissionGroupMembership_t vector_to_human_all;
        static TransmissionGroupMembership_t vector_to_human_indoor;
        static TransmissionGroupMembership_t vector_to_human_outdoor;
        /*static tProperties vectorProperties;
        static tProperties humanProperties;
        static RouteList_t route_all;
        static RouteList_t route_indoor;
        static RouteList_t route_outdoor;*/

    protected:
        VectorHabitatList_t     m_larval_habitats;
        VectorPopulationList_t  m_vectorpopulations;

        VectorProbabilities* m_vector_lifecycle_probabilities;

        std::map<VectorHabitatType::Enum,float> larval_habitat_multiplier;

        bool vector_migration;
        bool vector_migration_wind;
        bool vector_migration_human;
        bool vector_migration_local;
        int32_t mosquito_weight;

        NodeVector();
        NodeVector(ISimulationContext *context, suids::suid node_suid);
        void Initialize();

        virtual void setupEventContextHost();
        virtual void InitializeVectorPopulation(VectorPopulation* vp);
        float HabitatMultiplierByType(VectorHabitatType::Enum type) const;
            
        virtual IndividualHuman *createHuman(suids::suid id, float MCweight, float init_age, int gender, float init_poverty);

        const SimulationConfig *params();
        IVectorSimulationContext *context() const; // N.B. this is returning a non-const context because of the PostMigratingVector function

    private:
        virtual INodeContext *getContextPointer() { return (INodeContext*)this; }
        virtual void propagateContextToDependents();

#if USE_BOOST_SERIALIZATION
        friend class boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive & ar, NodeVector& node, const unsigned int  file_version );
#endif
    };
} // end namespace Kernel
