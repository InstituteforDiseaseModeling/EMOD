
#pragma once

#include "NodeMalaria.h"
#include "MalariaCoTransmissionContexts.h"

namespace Kernel
{
    class NodeMalariaCoTransmission : public NodeMalaria, public INodeMalariaCoTransmission
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

    public:
        static NodeMalariaCoTransmission *CreateNode(ISimulationContext *simulation, ExternalNodeId_t externalNodeId, suids::suid suid);
        virtual ~NodeMalariaCoTransmission();

        //virtual bool Configure( const Configuration* config ) override;
        virtual void updateInfectivity( float dt ) override;
        virtual void DepositFromIndividual( const IStrainIdentity& strain_IDs,
                                            const GeneticProbability& contagion_quantity,
                                            TransmissionRoute::Enum route ) override;

        virtual ITransmissionGroups* CreateTransmissionGroups() override;

        // INodeMalariaCoTransmission
        virtual const StrainIdentityMalariaCoTran& GetCoTranStrainIdentityForPerson( bool isIndoor, uint32_t personID ) const override;
        virtual const StrainIdentityMalariaCoTran& GetCoTranStrainIdentityForVector( bool isIndoor, uint32_t vectorID ) const override;
        virtual void VectorBitPerson( bool isIndoor, uint32_t vectorID ) override;

    protected:
        NodeMalariaCoTransmission();
        NodeMalariaCoTransmission(ISimulationContext *simulation, ExternalNodeId_t externalNodeId, suids::suid suid);
        virtual IIndividualHuman* createHuman( suids::suid id, float MCweight, float init_age, int gender) override;
        void ClearMap( std::map<uint32_t, StrainIdentityMalariaCoTran*>& rIdToStrainMap );

        virtual IVectorPopulation* CreateVectorPopulation( VectorSamplingType::Enum vectorSamplingType,
                                                           int speciesIndex,
                                                           int32_t populationPerSpecies ) override;

        // Simple Co-Transmission
        // EntityId = Human or VectorID
        std::map<uint32_t,StrainIdentityMalariaCoTran*> m_HumanIdToStrainIdentityMapIndoor;
        std::map<uint32_t,StrainIdentityMalariaCoTran*> m_HumanIdToStrainIdentityMapOutdoor;
        std::map<uint32_t,StrainIdentityMalariaCoTran*> m_VectorIdToStrainIdentityMapIndoor;
        std::map<uint32_t,StrainIdentityMalariaCoTran*> m_VectorIdToStrainIdentityMapOutdoor;

        DECLARE_SERIALIZABLE(NodeMalariaCoTransmission);
    };
}
