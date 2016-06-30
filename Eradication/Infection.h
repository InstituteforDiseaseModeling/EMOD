/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once
#include <string>
#include <map>

#include "suids.hpp"
#include "Contexts.h"

class Configuration;

#include "Sugar.h"
#include "SimulationEnums.h"
#include "DurationDistribution.h"


#include "IInfection.h"

#include "Configure.h"

namespace Kernel
{
    class Susceptibility;

    class InfectionConfig : public JsonConfigurable
    {
    public:
        InfectionConfig();
        virtual bool Configure( const Configuration* config ) override;

    protected:
        friend class Infection;

        static DurationDistribution incubation_distribution;
        static DurationDistribution infectious_distribution;
        static float base_infectivity;
        static float base_mortality;
        static MortalityTimeCourse::Enum                          mortality_time_course;                            // MORTALITY_TIME_COURSE

        GET_SCHEMA_STATIC_WRAPPER(InfectionConfig)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()
    };

    // generic infection base class
    // may not necessary want to derive from this for real infections
    class Infection : public IInfection
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

    public:
        static Infection *CreateInfection(IIndividualHumanContext *context, suids::suid _suid);
        virtual ~Infection();

        virtual void SetContextTo(IIndividualHumanContext* context) override;
        IIndividualHumanContext* GetParent();

        virtual suids::suid GetSuid() const;

        virtual void SetParameters(StrainIdentity* infstrain=nullptr, int incubation_period_override = -1 ) override;
        virtual void Update(float, ISusceptibilityContext* =nullptr) override;

        virtual InfectionStateChange::_enum GetStateChange() const override;
        virtual float GetInfectiousness() const override;
        virtual float GetInfectiousnessByRoute(string route) const override; //used in multi-route simulations

        virtual void InitInfectionImmunology(ISusceptibilityContext* _immunity) override;
        virtual void GetInfectiousStrainID(StrainIdentity* infstrain) override; // the ID of the strain being shed
        virtual bool IsActive() const override;
        virtual NonNegativeFloat GetDuration() const override;

    protected:
        IIndividualHumanContext *parent;

        suids::suid suid; // unique id of this infection within the system

        float duration;         // local timer
        float total_duration;
        float incubation_timer;
        float infectious_timer;
        float infectiousness;

        map<string, float> infectiousnessByRoute; //used in multi-route simulations (e.g. environmental, polio)
        
        InfectionStateChange::_enum StateChange;    //  Lets individual know something has happened

        StrainIdentity* infection_strain;           // this a pointer because disease modules may wish to implement derived types 

        Infection();
        Infection(IIndividualHumanContext *context);
        /* clorton virtual */ void Initialize(suids::suid _suid) /* clorton override */;

        /* clorton virtual */ const SimulationConfig* params() /* clorton override */;

        virtual void CreateInfectionStrain(StrainIdentity* infstrain);
        virtual void EvolveStrain(ISusceptibilityContext* immunity, float dt);

        DECLARE_SERIALIZABLE(Infection);
    };
}
