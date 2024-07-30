
#pragma once

#include "IndividualMalaria.h"
#include "StrainIdentityMalariaCoTran.h"
#include "MalariaCoTransmissionContexts.h"

namespace Kernel
{
    struct INodeMalariaCoTransmission;

    class IndividualHumanMalariaCoTransmission : public IndividualHumanMalaria, public IMalariaHumanReport
    {
        friend class SimulationMalaria;
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING();
        DECLARE_QUERY_INTERFACE();

    public:
        static IndividualHumanMalariaCoTransmission *CreateHuman( INodeContext *context,
                                                                  suids::suid _suid,
                                                                  double monte_carlo_weight = 1.0f,
                                                                  double initial_age = 0.0f,
                                                                  int gender = 0 );
        virtual ~IndividualHumanMalariaCoTransmission();

        virtual void SetContextTo(INodeContext* context) override;
        virtual void ExposeToInfectivity(float dt, TransmissionGroupMembership_t transmissionGroupMembership) override;

        // IMalariaHumanReport
        virtual const StrainIdentityMalariaCoTran& GetRecentTransmissionInfo() const override;

    protected:
        virtual void AcquireNewInfection( const IStrainIdentity *infstrain = nullptr,
                                          int incubation_period_override = -1) override;
        virtual void AddExposure( const StrainIdentity& rStrainId,
                                  float totalExposure,
                                  TransmissionRoute::Enum transmission_route ) override;
        virtual void DepositInfectiousnessFromGametocytes() override;

        INodeMalariaCoTransmission *m_pNodeCoTran;
        StrainIdentityMalariaCoTran m_VectorToHumanStrainIdentity;
        std::vector<std::pair<StrainIdentity,bool>> m_strain_indoors;

        DECLARE_SERIALIZABLE(IndividualHumanMalariaCoTransmission);

    private:
        static void InitializeStatics( const Configuration* config );

        IndividualHumanMalariaCoTransmission( suids::suid id = suids::nil_suid(),
                                              double monte_carlo_weight = 1.0,
                                              double initial_age = 0.0,
                                              int gender = 0 );
        IndividualHumanMalariaCoTransmission( INodeContext *context );
    };
}
