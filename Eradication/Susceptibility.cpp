/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"

#include "Environment.h"
#include "Susceptibility.h"
#include "RANDOM.h"
#include "SimulationConfig.h"
#include "Individual.h"
#include "RapidJsonImpl.h"

static const char* _module = "Susceptibility";

namespace Kernel
{
    bool SusceptibilityConfig::immune_decay = true;

    float SusceptibilityConfig::acqdecayrate = 1.0f;
    float SusceptibilityConfig::trandecayrate = 1.0f;
    float SusceptibilityConfig::mortdecayrate = 1.0f;
    float SusceptibilityConfig::baseacqupdate = 1.0f;
    float SusceptibilityConfig::basetranupdate = 1.0f;
    float SusceptibilityConfig::basemortupdate = 1.0f;
    float SusceptibilityConfig::baseacqoffset = 1.0f;
    float SusceptibilityConfig::basetranoffset = 1.0f;
    float SusceptibilityConfig::basemortoffset = 1.0f;

    GET_SCHEMA_STATIC_WRAPPER_IMPL(Susceptibility,SusceptibilityConfig)
    BEGIN_QUERY_INTERFACE_BODY(SusceptibilityConfig)
    END_QUERY_INTERFACE_BODY(SusceptibilityConfig)

    Susceptibility::Susceptibility() :
        parent(NULL)
    {
    }

    // QI stuff in case we want to use it more extensively
    BEGIN_QUERY_INTERFACE_BODY(Susceptibility)
        HANDLE_INTERFACE(ISusceptibilityContext)
        HANDLE_ISUPPORTS_VIA(ISusceptibilityContext)
    END_QUERY_INTERFACE_BODY(Susceptibility)

    Susceptibility::Susceptibility(IIndividualHumanContext *context) :
        parent(context)
    {
        //SetFlags(parent != NULL ? parent->GetSusceptibilityFlags() : NULL);
    }

    bool 
    SusceptibilityConfig::Configure(
        const Configuration* config
    )
    {
        initConfigTypeMap( "Enable_Immune_Decay", &immune_decay, Enable_Immune_Decay_DESC_TEXT, true );
        initConfigTypeMap( "Acquisition_Blocking_Immunity_Decay_Rate", &acqdecayrate, Acquisition_Blocking_Immunity_Decay_Rate_DESC_TEXT, 0.0f, 1000.0f, 0.001f );
        initConfigTypeMap( "Transmission_Blocking_Immunity_Decay_Rate", &trandecayrate, Transmission_Blocking_Immunity_Decay_Rate_DESC_TEXT, 0.0f, 1000.0f, 0.001f );
        initConfigTypeMap( "Mortality_Blocking_Immunity_Decay_Rate", &mortdecayrate, Mortality_Blocking_Immunity_Decay_Rate_DESC_TEXT, 0.0f, 1000.0f, 0.001f );
        initConfigTypeMap( "Immunity_Acquisition_Factor", &baseacqupdate, Immunity_Acquisition_Factor_DESC_TEXT, 0.0f, 1000.0f, 0.0f );
        initConfigTypeMap( "Immunity_Transmission_Factor", &basetranupdate, Immunity_Transmission_Factor_DESC_TEXT, 0.0f, 1000.0f, 0.0f );
        initConfigTypeMap( "Immunity_Mortality_Factor", &basemortupdate, Immunity_Mortality_Factor_DESC_TEXT, 0.0f, 1000.0f, 0.0f );
        initConfigTypeMap( "Acquisition_Blocking_Immunity_Duration_Before_Decay", &baseacqoffset, Acquisition_Blocking_Immunity_Duration_Before_Decay_DESC_TEXT, 0.0f, MAX_HUMAN_LIFETIME, 0.0f );
        initConfigTypeMap( "Transmission_Blocking_Immunity_Duration_Before_Decay", &basetranoffset, Transmission_Blocking_Immunity_Duration_Before_Decay_DESC_TEXT, 0.0f, MAX_HUMAN_LIFETIME, 0.0f );
        initConfigTypeMap( "Mortality_Blocking_Immunity_Duration_Before_Decay", &basemortoffset, Mortality_Blocking_Immunity_Duration_Before_Decay_DESC_TEXT, 0.0f, MAX_HUMAN_LIFETIME, 0.0f );
        bool bRet = JsonConfigurable::Configure( config );
        return bRet;
    }

    void Susceptibility::Initialize(float _age, float immmod, float riskmod)
    {
        age = _age;

        // immune modifiers
        mod_acquire   = 1;
        mod_transmit  = 1;
        mod_mortality = 1;

        // decay rates
        acqdecayoffset  = 0;
        trandecayoffset = 0;
        mortdecayoffset = 0;
    }

    Susceptibility *Susceptibility::CreateSusceptibility(IIndividualHumanContext *context, float _age, float immmod, float riskmod)
    {
        Susceptibility *newsusceptibility = _new_ Susceptibility(context);
        newsusceptibility->Initialize(_age, immmod, riskmod);

        return newsusceptibility;
    }
    
    Susceptibility::~Susceptibility()
    {
    }

    void Susceptibility::SetContextTo(IIndividualHumanContext* context)
    {
        parent = context;
    }
    IIndividualHumanContext* Susceptibility::GetParent() { return parent; }
    const SimulationConfig* Susceptibility::params()
    {
        return GET_CONFIGURABLE(SimulationConfig);
        //return base_flags_ptr;
    }

    float Susceptibility::getAge()
    const
    { return age; }

    float Susceptibility::getModAcquire()
    const
    {
        /*std::ostringstream msg;
        msg << "Individual "
            << parent->GetSuid().data
            << " age "
            << ((IndividualHuman*)parent)->GetAge()
            << " has mod_acquire "
            << mod_acquire 
            << std::endl;
        LOG_DEBUG_F( msg.str().c_str() );*/
        return mod_acquire;
    }

    float Susceptibility::GetModTransmit()
    const
    { return mod_transmit; }

    float
    Susceptibility::getModMortality()
    const
    { return mod_mortality; }


    void Susceptibility::Update( float dt )
    {
        age += dt; // tracks age for immune purposes

        if (mod_acquire < 1) { acqdecayoffset -= dt; }
        if (immune_decay && acqdecayoffset < 0)
        {
            mod_acquire += (1.0f - mod_acquire) * acqdecayrate * dt;
        }

        if (mod_transmit < 1) {trandecayoffset -= dt;}
        if (immune_decay && trandecayoffset < 0)
        {
            mod_transmit += (1.0f - mod_transmit) * trandecayrate * dt;
        }

        if (mod_mortality < 1) {mortdecayoffset -= dt;}
        if (immune_decay && mortdecayoffset < 0)
        {
            mod_mortality += (1.0f - mod_mortality) * mortdecayrate * dt;
        }
    }

    void Susceptibility::UpdateInfectionCleared()
    {
        mod_acquire   *= baseacqupdate;
        mod_transmit  *= basetranupdate;
        mod_mortality *= basemortupdate;

        acqdecayoffset  = baseacqoffset;
        trandecayoffset = basetranoffset;
        mortdecayoffset = basemortoffset;
    }

    bool Susceptibility::IsImmune() const
    {
        LOG_WARN( "Placeholder functionality hard-coded to return false.\n" );
        return false;
    }

    void Susceptibility::InitNewInfection()
    {
        LOG_WARN( "Not implemented (but not throwing exception.\n" );
        // no-op
    }

} // end Kernel namespace


#if USE_JSON_SERIALIZATION || USE_JSON_MPI
namespace Kernel
{
    void Susceptibility::JSerialize( IJsonObjectAdapter* root, JSerializer* helper ) const
    {
        root->BeginObject();

        root->Insert("age", age);

        root->Insert("mod_acquire", mod_acquire);
        root->Insert("mod_transmit", mod_transmit);
        root->Insert("mod_mortality", mod_mortality);

        root->Insert("acqdecayoffset", acqdecayoffset);
        root->Insert("trandecayoffset", trandecayoffset);
        root->Insert("mortdecayoffset", mortdecayoffset);

        root->EndObject();
    }

    void Susceptibility::JDeserialize( IJsonObjectAdapter* root, JSerializer* helper )
    {
        rapidjson::Document * doc = (rapidjson::Document*) root; // total hack to get around build path issues with rapid json and abstraction

        age             = (*doc)[ "age" ].GetDouble();
        mod_acquire     = (*doc)[ "mod_acquire" ].GetDouble();
        mod_transmit    = (*doc)[ "mod_transmit" ].GetDouble();
        mod_mortality   = (*doc)[ "mod_mortality" ].GetDouble();
        acqdecayoffset  = (*doc)[ "acqdecayoffset" ].GetDouble();
        trandecayoffset = (*doc)[ "trandecayoffset" ].GetDouble();
        mortdecayoffset = (*doc)[ "mortdecayoffset" ].GetDouble();

        LOG_DEBUG_F("Susceptibility::JDeserialize age=%f,%f,%f,%f,%f,%f,%f\n",age,mod_acquire,mod_transmit,mod_mortality,acqdecayoffset,trandecayoffset,mortdecayoffset);
    }
} // namespace Kernel

#endif


#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
BOOST_CLASS_EXPORT(Kernel::Susceptibility)
namespace Kernel {
    template<class Archive>
    void serialize(Archive & ar, Susceptibility& sus, const unsigned int file_version )
    {
        // current status
        ar & sus.age;

        // immune modifiers
        ar & sus.mod_acquire;
        ar & sus.mod_transmit;
        ar & sus.mod_mortality;

        ar & sus.acqdecayoffset;
        ar & sus.trandecayoffset;
        ar & sus.mortdecayoffset;
    }
    template void serialize( boost::archive::binary_oarchive&, Kernel::Susceptibility&, unsigned int);
    template void serialize( boost::mpi::detail::content_oarchive&, Kernel::Susceptibility&, unsigned int);
    template void serialize( boost::mpi::packed_skeleton_oarchive&, Kernel::Susceptibility&, unsigned int);
    template void serialize( boost::mpi::detail::mpi_datatype_oarchive&, Kernel::Susceptibility&, unsigned int);
    template void serialize( boost::mpi::packed_oarchive&, Kernel::Susceptibility&, unsigned int);
    template void serialize( boost::mpi::packed_iarchive&, Kernel::Susceptibility&, unsigned int);
    template void serialize( boost::archive::binary_iarchive&, Kernel::Susceptibility&, unsigned int);
    template void serialize( boost::mpi::packed_skeleton_iarchive&, Kernel::Susceptibility&, unsigned int);
}

BOOST_CLASS_IMPLEMENTATION(Kernel::Susceptibility, boost::serialization::object_serializable);
BOOST_CLASS_TRACKING(Kernel::Susceptibility, boost::serialization::track_never);
#endif
