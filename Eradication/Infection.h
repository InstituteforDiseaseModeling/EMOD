/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once
#include <string>
#include <list>
#include <map>

#include "BoostLibWrapper.h"

#include "suids.hpp"
#include "Contexts.h"

class Configuration;

#include "Sugar.h"
#include "SimulationEnums.h" // to get DistributionFunction enum. Don't want utils reaching into Eradication though. TBD!!!
#include <boost/serialization/binary_object.hpp>

#include "StrainIdentity.h"
#include "Common.h"

#include <typeinfo>
#include "Configure.h"
#include "Types.h"

namespace Kernel
{
    class Susceptibility;

    class InfectionConfig : public JsonConfigurable 
    {
    public:
        bool Configure( const Configuration* config );

    protected:
        static float incubation_period;
        static float incubation_period_mean;
        static float incubation_period_std_dev;
        static float incubation_period_min;
        static float incubation_period_max;
        static float infectious_period;
        static float infectious_period_mean;
        static float infectious_period_std_dev;
        static float infectious_period_min;
        static float infectious_period_max;
        static float base_infectivity;
        static float base_mortality;
        static MortalityTimeCourse::Enum                          mortality_time_course;                            // MORTALITY_TIME_COURSE
        static DistributionFunction::Enum                         incubation_distribution;                          // INCUBATION_DISTRIBUTION
        static DistributionFunction::Enum                         infectious_distribution;                          // INFECTIOUS_DISTRIBUTION

        GET_SCHEMA_STATIC_WRAPPER(InfectionConfig)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()  
        DECLARE_QUERY_INTERFACE()
    };

    // generic infection base class
    // may not necessary want to derive from this for real infections
    class Infection : public InfectionConfig
    {
    public:
        static Infection *CreateInfection(IIndividualHumanContext *context, suids::suid _suid);
        virtual ~Infection();

        virtual void SetContextTo(IIndividualHumanContext* context);
        IIndividualHumanContext* GetParent();

        virtual suids::suid GetSuid() const;

        virtual void SetParameters(StrainIdentity* infstrain=NULL, int incubation_period_override = -1 );
        virtual void Update(float, Susceptibility* =NULL);

        InfectionStateChange::_enum GetStateChange() const;
        virtual float GetInfectiousness() const;
        float GetInfectiousnessByRoute(string route) const; //used in multi-route simulations
        float GetContactSheddingFraction() const;
        float GetInfectiousPeriod() const;

        virtual void InitInfectionImmunology(Susceptibility* _immunity);
        virtual void GetInfectiousStrainID(StrainIdentity* infstrain); // the ID of the strain being shed
        virtual bool IsActive() const;
        virtual NonNegativeFloat GetDuration() const;

    protected:
        IIndividualHumanContext *parent;

        suids::suid suid; // unique id of this infection within the system

        float duration;         // local timer
        float total_duration;
        float incubation_timer;
        float infectious_timer;
        float infectiousness;

        float contact_shedding_fraction;            // fraction of viral shedding to contact type groups, currently only used for polio

        map<string, float> infectiousnessByRoute; //used in multi-route simulations (e.g. environmental, polio)
        
        InfectionStateChange::_enum StateChange;    //  Lets individual know something has happened

        StrainIdentity* infection_strain;           // this a pointer because disease modules may wish to implement derived types 

        Infection();
        Infection(IIndividualHumanContext *context);
        void Initialize(suids::suid _suid);

        const SimulationConfig* params();

        virtual void CreateInfectionStrain(StrainIdentity* infstrain);
        virtual void EvolveStrain(Susceptibility* immunity, float dt);

    private:
#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
        friend class boost::serialization::access;

        template<class Archive>
        friend void serialize(Archive & ar, Infection &inf, const unsigned int  file_version );

#endif

#if USE_JSON_SERIALIZATION || USE_JSON_MPI
    public:
     // IJsonSerializable Interfaces
     virtual void JSerialize( IJsonObjectAdapter* root, JSerializer* helper ) const;
     virtual void JDeserialize( IJsonObjectAdapter* root, JSerializer* helper );
#endif
    };
}
