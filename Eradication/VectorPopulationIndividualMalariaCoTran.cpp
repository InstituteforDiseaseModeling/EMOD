
#include "stdafx.h"

#include "VectorPopulationIndividualMalariaCoTran.h"
#include "NodeMalariaCoTransmission.h"
#include "VectorCohortIndividual.h"
#include "NodeVector.h"
#include "Exceptions.h"
#include "Log.h"
#include "Debug.h"
#include "RANDOM.h"
#include "VectorToHumanAdapter.h"

#include "NodeEventContext.h"
#include "MalariaContexts.h"
#include "StrainIdentityMalariaCoTran.h"
#include "IdmDateTime.h"

SETUP_LOGGING("VectorPopulationIndividualMalariaCoTran")

namespace Kernel
{
    class MalariaHumanAdapter : public VectorToHumanAdapter,
                                public IMalariaHumanReport
    {
        IMPLEMENT_NO_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

    public:
        MalariaHumanAdapter( INodeContext* pNodeContext,
                             uint32_t vectorID,
                             const StrainIdentityMalariaCoTran* pStrainIdentityMalaria )
            : VectorToHumanAdapter( pNodeContext, vectorID )
            , m_pStrainIdentityMalaria( pStrainIdentityMalaria )
        {
        }

        // IMalariaHumanReport
        virtual const StrainIdentityMalariaCoTran& GetRecentTransmissionInfo() const override
        {
            return *m_pStrainIdentityMalaria;
        }

    private:
        const StrainIdentityMalariaCoTran* m_pStrainIdentityMalaria;
    };
    BEGIN_QUERY_INTERFACE_DERIVED( MalariaHumanAdapter, VectorToHumanAdapter )
        HANDLE_INTERFACE(IMalariaHumanReport)
    END_QUERY_INTERFACE_DERIVED( MalariaHumanAdapter, VectorToHumanAdapter )


    // QI stuff
    BEGIN_QUERY_INTERFACE_DERIVED(VectorPopulationIndividualMalariaCoTran, VectorPopulationIndividual)
    END_QUERY_INTERFACE_DERIVED(VectorPopulationIndividualMalariaCoTran, VectorPopulationIndividual)

    VectorPopulationIndividualMalariaCoTran::VectorPopulationIndividualMalariaCoTran(uint32_t mosquito_weight)
        : VectorPopulationIndividual()
        , m_pNodeCoTran( nullptr )
    {
    }

    VectorPopulationIndividualMalariaCoTran*
        VectorPopulationIndividualMalariaCoTran::CreatePopulation( INodeContext *context,
                                                                   int speciesIndex,
                                                                   uint32_t adult,
                                                                   uint32_t mosquito_weight)
    {
        VectorPopulationIndividualMalariaCoTran *newpopulation = _new_ VectorPopulationIndividualMalariaCoTran(mosquito_weight);
        newpopulation->Initialize(context, speciesIndex, adult);

        return newpopulation;
    }

    VectorPopulationIndividualMalariaCoTran::~VectorPopulationIndividualMalariaCoTran()
    {
    }

    void VectorPopulationIndividualMalariaCoTran::SetContextTo(INodeContext* context)
    {
        VectorPopulationIndividual::SetContextTo(context);

        if( s_OK != context->QueryInterface( GET_IID( INodeMalariaCoTransmission ), (void**)&m_pNodeCoTran ) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "m_context", "INodeMalariaCoTransmission", "INodeContext" );
        }
    }

    void VectorPopulationIndividualMalariaCoTran::queueIncrementNumInfs( IVectorCohort* cohort )
    {
        if( ( cohort->GetState() == VectorStateEnum::STATE_INFECTED   ) ||
            ( cohort->GetState() == VectorStateEnum::STATE_INFECTIOUS ) )
        {
            const IStrainIdentity* p_new_si = current_vci->GetStrainIdentity();
            StrainIdentityMalariaCoTran* p_new_si_malaria = static_cast<StrainIdentityMalariaCoTran*>(const_cast<IStrainIdentity*>(p_new_si));
            num_infs_per_state_counts[ cohort->GetState() ] += p_new_si_malaria->GetTransmittedInfections().size();
        }
    }

    void VectorPopulationIndividualMalariaCoTran::AcquireNewInfection( uint32_t vectorID,
                                                                       IVectorCohortIndividual* pVCI,
                                                                       const StrainIdentity& rStrain,
                                                                       bool isIndoors )
    {
        uint32_t human_id = rStrain.GetGeneticID(); // geneticID should the human id
        const StrainIdentityMalariaCoTran& r_si_malaria = m_pNodeCoTran->GetCoTranStrainIdentityForPerson( isIndoors, human_id );

        MalariaHumanAdapter adapter( m_context, vectorID, &r_si_malaria );
        IIndividualEventBroadcaster* broadcaster = m_context->GetEventContext()->GetIndividualEventBroadcaster();
        broadcaster->TriggerObservers( &adapter, EventTrigger::HumanToVectorTransmission );

        pVCI->AcquireNewInfection( &r_si_malaria );

        const IStrainIdentity* p_new_si = pVCI->GetStrainIdentity();
        StrainIdentityMalariaCoTran* p_new_si_malaria = static_cast<StrainIdentityMalariaCoTran*>(const_cast<IStrainIdentity*>(p_new_si));
        p_new_si_malaria->SetGeneticID( vectorID );
        p_new_si_malaria->SetVectorID( vectorID );
        p_new_si_malaria->SetTimeOfVectorInfection( m_context->GetTime().time );
    }

    REGISTER_SERIALIZABLE(VectorPopulationIndividualMalariaCoTran);

    void VectorPopulationIndividualMalariaCoTran::serialize(IArchive& ar, VectorPopulationIndividualMalariaCoTran* obj)
    {
        VectorPopulationIndividual::serialize(ar, obj);

        //num_infs_per_state_counts should be transiant and should not need to be saved
    }
}