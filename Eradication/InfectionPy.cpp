/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#ifdef ENABLE_PYTHON

#include "InfectionPy.h"
#include "SusceptibilityPy.h"
#include "InterventionsContainer.h"
#include "Environment.h"
#include "Debug.h"

#include "Common.h"
#include "MathFunctions.h"
#include "RANDOM.h"
#include "Exceptions.h"
#include "SimulationConfig.h"
using namespace std;

static const char* _module = "InfectionPy";

namespace Kernel
{
    GET_SCHEMA_STATIC_WRAPPER_IMPL(Py.Infection,InfectionPyConfig)
    BEGIN_QUERY_INTERFACE_BODY(InfectionPyConfig)
    END_QUERY_INTERFACE_BODY(InfectionPyConfig)

    bool
    InfectionPyConfig::Configure(
        const Configuration * config
    )
    {
        LOG_DEBUG( "Configure\n" );
        //initConfigTypeMap( "Enable_Contact_Tracing", &tracecontact_mode, Enable_Contact_Tracing_DESC_TEXT, false ); // polio
        bool bRet = JsonConfigurable::Configure( config );
        return bRet;
    }

    BEGIN_QUERY_INTERFACE_BODY(InfectionPy)
        HANDLE_INTERFACE(IInfectionPy)
    END_QUERY_INTERFACE_BODY(InfectionPy)

    InfectionPy::InfectionPy()
    {
    }

    const SimulationConfig*
    InfectionPy::params()
    {
        return GET_CONFIGURABLE(SimulationConfig);
    }

    InfectionPy::InfectionPy(IIndividualHumanContext *context) : Infection(context)
    {
    }

    void InfectionPy::Initialize(suids::suid _suid)
    {
        Infection::Initialize(_suid);
    }

    InfectionPy *InfectionPy::CreateInfection(IIndividualHumanContext *context, suids::suid _suid)
    {
        //VALIDATE(boost::format(">InfPy::CreateInfection(%1%, %2%)") % context->GetSuid().data % _suid.data );

        InfectionPy *newinfection = _new_ InfectionPy(context);
        newinfection->Initialize(_suid);

        return newinfection;
    }

    InfectionPy::~InfectionPy()
    {
    }

    void InfectionPy::SetParameters(StrainIdentity* infstrain, int incubation_period_override)
    {
        Infection::SetParameters(infstrain, incubation_period_override); // setup infection timers and infection state
        if(infstrain == NULL)
        {
            // using default strainIDs
            //infection_strain->SetAntigenID(default_antigen);
        }
        else
        {
            *infection_strain = *infstrain;
        }
    }

    void InfectionPy::InitInfectionImmunology(Susceptibility* _immunity)
    {
        ISusceptibilityPy* immunity = NULL;
        if( _immunity->QueryInterface( GET_IID( ISusceptibilityPy ), (void**)&immunity ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "_immunity", "ISusceptibilityPy", "Susceptibility" );
        }

        StateChange = InfectionStateChange::New;
        return Infection::InitInfectionImmunology( _immunity );
    }

    void InfectionPy::Update(float dt, ISusceptibilityContext* _immunity)
    {
        return;
        /*
        StateChange = InfectionStateChange::None;
        ISusceptibilityPy* immunity = NULL;
        if( _immunity->QueryInterface( GET_IID( ISusceptibilityPy ), (void**)&immunity ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "_immunity", "Susceptibility", "SusceptibilityPy" );
        } */
        //return InfectionEnvironmental::Update( dt, _immunity );
    }

    void InfectionPy::Clear()
    {
        StateChange = InfectionStateChange::Cleared;
    }
}

#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
BOOST_CLASS_EXPORT(Kernel::InfectionPy)
namespace Kernel
{
    template<class Archive>
    void serialize(Archive & ar, InfectionPy& inf, const unsigned int file_version )
    {
        ar & boost::serialization::base_object<Kernel::Infection>(inf);
    }
    template void serialize( boost::mpi::packed_skeleton_iarchive&, Kernel::InfectionPy&, unsigned int);
}
#endif

#endif // ENABLE_PYTHON
