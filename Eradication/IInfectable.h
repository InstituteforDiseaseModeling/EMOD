
#pragma once

#include "ISupports.h"
#include "IntranodeTransmissionTypes.h"
#include "IContagionPopulation.h"
#include <list>
#include "IInfection.h"

namespace Kernel
{
    class Infection;
    struct IInfectable : ISupports
    {
        virtual void Expose( const IContagionPopulation* cp, float dt, TransmissionRoute::Enum transmission_route = TransmissionRoute::TRANSMISSIONROUTE_CONTACT ) = 0;
        virtual const infection_list_t& GetInfections() const = 0;
        virtual float GetInterventionReducedAcquire() const = 0;
        virtual suids::suid GetSuid() const = 0;

        virtual ~IInfectable() {}
    };

    class StrainIdentity;
    struct IInfectionAcquirable : ISupports
    {
        virtual void AcquireNewInfection( const IStrainIdentity *infstrain, int incubation_period_override = -1) = 0;

        virtual ~IInfectionAcquirable() {}
    };
}
