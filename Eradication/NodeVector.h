/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <map>

#include "Node.h"
#include "VectorHabitat.h"
#include "VectorPopulation.h"
#include "LarvalHabitatMultiplier.h"

class ReportVector;
class VectorSpeciesReport;

namespace Kernel
{
    struct IMigrationInfoVector;
    class SpatialReportVector;
    class NodeVector : public Node, public IVectorNodeContext, public INodeVector
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
        virtual bool Configure( const Configuration* config ) override;

        // INodeContext methods
        // IVectorNodeContext methods
        virtual VectorProbabilities* GetVectorLifecycleProbabilities() override;
        virtual IVectorHabitat*      GetVectorHabitatBySpeciesAndType( std::string& species, VectorHabitatType::Enum type, const Configuration* inputJson) override;
        virtual VectorHabitatList_t* GetVectorHabitatsBySpecies( std::string& species ) override;
        virtual float                GetLarvalHabitatMultiplier(VectorHabitatType::Enum type, const std::string& species ) const override;

        virtual IIndividualHuman* processImmigratingIndividual(IIndividualHuman*) override;
        virtual IIndividualHuman* addNewIndividual(float = 1.0f, float = 0.0f, int = 0, int = 0, float = 1.0f, float = 1.0f, float = 1.0f, float = 0.0f) override;

        virtual void PopulateFromDemographics() override;
        virtual void SetupIntranodeTransmission() override;
        virtual void SetParameters(NodeDemographicsFactory *demographics_factory, ClimateFactory *climate_factory) override;
        virtual void updateInfectivity(float dt = 0.0f) override;
        virtual void updatePopulationStatistics(float dt = 1.0f) override;
        void         updateVectorLifecycleProbabilities(float dt);

        void SetVectorPopulations(void);    //default--1 population as before
        virtual void AddVectors(std::string releasedSpecies, VectorMatingStructure _vector_genetics, uint64_t releasedNumber) override;

        virtual void SetupMigration( IMigrationInfoFactory * migration_factory, 
                                     MigrationStructure::Enum ms,
                                     const boost::bimap<ExternalNodeId_t, suids::suid>& rNodeIdSuidMap ) override;
        virtual void processImmigratingVector( VectorCohort* immigrant ) override;
        void processEmigratingVectors();

        virtual VectorPopulationList_t& GetVectorPopulations() override;

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

        std::map<std::string, VectorHabitatList_t> m_larval_habitats;
        VectorPopulationList_t  m_vectorpopulations;

        VectorProbabilities* m_vector_lifecycle_probabilities;


        LarvalHabitatMultiplier larval_habitat_multiplier;

        bool vector_mortality;
        int32_t mosquito_weight;
        IMigrationInfoVector* vector_migration_info;

        NodeVector();
        NodeVector(ISimulationContext *context, suids::suid node_suid);
        /* clorton virtual */ void Initialize() /* clorton override */;

        virtual void setupEventContextHost() override;
        virtual void InitializeVectorPopulation(VectorPopulation* vp);
        void VectorMigrationBasedOnFiles();
        void VectorMigrationToAdjacentNodes();
            
        virtual IIndividualHuman *createHuman( suids::suid id, float MCweight, float init_age, int gender, float init_poverty) override;

        /* clorton virtual */ const SimulationConfig *params() /* clorton override */;
        IVectorSimulationContext *context() const; // N.B. this is returning a non-const context because of the PostMigratingVector function

        DECLARE_SERIALIZABLE(NodeVector);

    private:
        virtual INodeContext *getContextPointer() override { return static_cast<INodeContext*>(this); }
        virtual void propagateContextToDependents() override;
    };
} // end namespace Kernel
