
#pragma once
#include <string>
#include <map>

#include "suids.hpp"

class Configuration;

#include "Sugar.h"
#include "SimulationEnums.h"
#include "IDistribution.h"
#include "IInfection.h"
#include "Configure.h"

namespace Kernel
{
    class StrainIdentity;
    class Susceptibility;
    class SimulationConfig;
    struct IIndividualHumanContext;

    class InfectionConfig : public JsonConfigurable
    {
    public:
        InfectionConfig() {};
        virtual bool Configure( const Configuration* config ) override;

        static bool enable_disease_mortality;
        static float symptomatic_infectious_offset;
        static unsigned int number_basestrains;
        static unsigned int number_substrains; // genetic variants

    protected:
        friend class Infection;
        
        static IDistribution* infectious_distribution;
        static IDistribution* incubation_distribution;
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

        virtual void SetParameters( const IStrainIdentity* infstrain, int incubation_period_override = -1 ) override;
        virtual void Update( float currentTime, float dt, ISusceptibilityContext* immunity = nullptr ) override;

        virtual InfectionStateChange::_enum GetStateChange() const override;
        virtual float GetInfectiousness() const override;
        virtual float GetInfectiousnessByRoute( const string& route ) const override; //used in multi-route simulations

        virtual void InitInfectionImmunology(ISusceptibilityContext* _immunity) override;
        virtual const IStrainIdentity& GetInfectiousStrainID() const override; // the ID of the strain being shed
        virtual bool IsActive() const override;
        virtual NonNegativeFloat GetDuration() const override;
        virtual bool StrainMatches( IStrainIdentity * pStrain );
        
        virtual bool IsSymptomatic() const override;
        virtual float GetSimTimeCreated() const override;

    protected:
        Infection();
        Infection(IIndividualHumanContext *context);
        virtual void Initialize(suids::suid _suid);

        /* clorton virtual */ const SimulationConfig* params() /* clorton override */;

        virtual void CreateInfectionStrain( const IStrainIdentity* infstrain );
        virtual void EvolveStrain(ISusceptibilityContext* immunity, float dt);

        virtual bool  IsNewlySymptomatic() const override;
        virtual void  UpdateSymptomatic( float const duration, float const incubation_timer );
        virtual bool  DetermineSymptomatology( float const duration, float const incubation_timer );

        IIndividualHumanContext *parent;

        suids::suid suid; // unique id of this infection within the system

        float sim_time_created;
        float duration;         // local timer
        float total_duration;
        float incubation_timer;
        float infectious_timer;
        float infectiousness;

        map<string, float> infectiousnessByRoute; //used in multi-route simulations (e.g. environmental, polio)
        
        InfectionStateChange::_enum StateChange;    //  Lets individual know something has happened

        IStrainIdentity* infection_strain;           // this a pointer because disease modules may wish to implement derived types 

        bool m_is_newly_symptomatic;
        bool m_is_symptomatic;

        DECLARE_SERIALIZABLE(Infection);
    };
}
