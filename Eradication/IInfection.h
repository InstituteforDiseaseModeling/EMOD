
#pragma once
#include "ISusceptibilityContext.h"
#include "suids.hpp"
#include "Common.h"             // InfectionStateChange
#include "Types.h"              // NonNegativeFloat

namespace Kernel
{
    struct IIndividualHumanContext;
    struct IStrainIdentity;

    struct IInfection : ISerializable
    {
        virtual suids::suid GetSuid() const = 0;
        virtual void Update( float currentTime, float dt, ISusceptibilityContext* immunity = nullptr) = 0;
        virtual InfectionStateChange::_enum GetStateChange() const = 0;
        virtual float GetInfectiousness() const = 0;

        virtual float GetInfectiousnessByRoute( const std::string& route) const = 0;
        virtual const IStrainIdentity& GetInfectiousStrainID() const = 0;

        virtual bool IsActive() const = 0;
        virtual NonNegativeFloat GetDuration() const = 0;
        virtual void SetContextTo(IIndividualHumanContext*) = 0;
        virtual void SetParameters( const IStrainIdentity* infstrain, int incubation_period_override = -1 ) = 0;
        virtual void InitInfectionImmunology(ISusceptibilityContext* _immunity) = 0;
        virtual bool StrainMatches( IStrainIdentity * pStrain ) = 0;
        virtual bool IsSymptomatic() const = 0;
        virtual bool IsNewlySymptomatic() const = 0;
        virtual float GetSimTimeCreated() const = 0;

        virtual ~IInfection() {}
    };

    typedef std::vector<IInfection*> infection_list_t;
}
