
#pragma once
#include "Infection.h"

namespace Kernel
{
    class InfectionSTIConfig : public InfectionConfig
    {
    protected:
        friend class InfectionSTI;
    };

    class InfectionSTI : public Infection
    {
    public:
        virtual ~InfectionSTI(void);
        static InfectionSTI *CreateInfection(IIndividualHumanContext *context, suids::suid _suid);
        virtual float GetInfectiousness() const override;
        virtual void Update( float currentTime, float dt, ISusceptibilityContext* immunity = nullptr ) override;

    protected:
        InfectionSTI();
        InfectionSTI(IIndividualHumanContext *context);

        DECLARE_SERIALIZABLE(InfectionSTI);
    };
}
