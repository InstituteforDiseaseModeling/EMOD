
#pragma once

#include "MalariaContexts.h"
#include "SimulationVector.h"

namespace Kernel
{
    class SimulationMalaria : public SimulationVector, public IMalariaSimulationContext
    {
        GET_SCHEMA_STATIC_WRAPPER( SimulationMalaria )
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

    public:
        SimulationMalaria();
        static SimulationMalaria *CreateSimulation();
        static SimulationMalaria *CreateSimulation(const ::Configuration *config);
        virtual ~SimulationMalaria();

        virtual bool Configure( const Configuration * inputJson ) override;

        virtual void Update(float dt) override;

        // Allows correct type of community to be added by derived class Simulations
        virtual void addNewNodeFromDemographics( ExternalNodeId_t externalNodeId,
                                                 suids::suid node_suid,
                                                 NodeDemographicsFactory *nodedemographics_factory,
                                                 ClimateFactory *climate_factory ) override;

        // IMalariaSimulationContext
        virtual suids::suid GetNextParasiteSuid() override;
        virtual suids::suid GetNextBiteSuid() override;

    protected:
        static bool ValidateConfiguration(const ::Configuration *config);

        virtual void InitializeFlags(const ::Configuration *config);  // override in derived classes to instantiate correct flag classes

        DECLARE_SERIALIZABLE(SimulationMalaria);

    private:
        virtual void Initialize(const ::Configuration *config) override;

        virtual ISimulationContext *GetContextPointer() override;

        std::vector<float> m_DetectionThresholds; // if measure is above this threshold, they are counted - initConfig
        MalariaModel::Enum m_MalariaModel;
        suids::distributed_generator m_ParasiteCohortSuidGenerator;
        suids::distributed_generator m_VectorBiteSuidGenerator;
    };
}
