
#pragma once

#include "InfectionMalaria.h"

namespace Kernel
{
    class InfectionMalariaGeneticsConfig : public InfectionMalariaConfig
    {
        GET_SCHEMA_STATIC_WRAPPER(InfectionMalariaGeneticsConfig)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

    public:
        InfectionMalariaGeneticsConfig() {};
        virtual void ConfigureMalariaStrainModel( const Configuration* config ) override;
    };

    class InfectionMalariaGenetics : public InfectionMalaria
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

    public:
        static InfectionMalariaGenetics *CreateInfection( IIndividualHumanContext *context, suids::suid suid, int initial_hepatocytes=1 );
        virtual ~InfectionMalariaGenetics();

        virtual void SetParameters( const IStrainIdentity* _infstrain, int incubation_period_override = -1 ) override;

    protected:
        InfectionMalariaGenetics();
        InfectionMalariaGenetics(IIndividualHumanContext *context);

        virtual int64_t CalculateTotalIRBCWithHRP( int64_t totalIRBC ) const override;

        DECLARE_SERIALIZABLE(InfectionMalariaGenetics);
    };
}
