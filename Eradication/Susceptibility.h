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

#include "Contexts.h"
#include "Sugar.h"
#include "Configure.h"

class Configuration;

namespace Kernel
{
    class SusceptibilityConfig : public JsonConfigurable 
    {
        friend class Individual;

    public:
        virtual bool Configure( const Configuration* config );

    protected:
        static bool  immune_decay;

        static float acqdecayrate;
        static float trandecayrate;
        static float mortdecayrate;
        static float baseacqupdate;
        static float basetranupdate;
        static float basemortupdate;
        static float baseacqoffset;
        static float basetranoffset;
        static float basemortoffset;

        GET_SCHEMA_STATIC_WRAPPER(SusceptibilityConfig)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()  
        DECLARE_QUERY_INTERFACE()
    };

    class Susceptibility : public ISusceptibilityContext, protected SusceptibilityConfig
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

    public:
        static Susceptibility *Susceptibility::CreateSusceptibility(IIndividualHumanContext *context, float _age, float immmod, float riskmod);
        virtual ~Susceptibility();
        virtual void SetContextTo(IIndividualHumanContext* context);
        IIndividualHumanContext* GetParent();

        virtual void Update(float dt=0.0);
        virtual void UpdateInfectionCleared();
        
        // functions to mediate interaction with Infection and Individual objects
        float getAge() const;

        // ISusceptibilityContext interfaces
        virtual float getModAcquire() const;
        virtual float GetModTransmit() const;
        virtual float getModMortality() const;
        virtual bool  IsImmune() const;
        virtual void  InitNewInfection();

    protected:
        // current status
        float age;

        // immune modifiers
        float mod_acquire;
        float mod_transmit;
        float mod_mortality;

        float acqdecayoffset;
        float trandecayoffset;
        float mortdecayoffset;

        Susceptibility();
        Susceptibility(IIndividualHumanContext *context);
        void Initialize(float _age, float immmod, float riskmod);

        IIndividualHumanContext *parent;

        const SimulationConfig* params();

    private:
#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
        friend class boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive & ar, Susceptibility& sus, const unsigned int file_version );
#endif

#if USE_JSON_SERIALIZATION || USE_JSON_MPI
    public:
     // IJsonSerializable Interfaces
     virtual void JSerialize( IJsonObjectAdapter* root, JSerializer* helper ) const;
     virtual void JDeserialize( IJsonObjectAdapter* root, JSerializer* helper );
#endif
    };
}
