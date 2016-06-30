/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#pragma warning(disable : 4996)

#ifdef ENABLE_PYTHON
#include "Debug.h"
#include "Contexts.h"
#include "RANDOM.h"
#include "Environment.h"
#include "IndividualPy.h"
#include "SusceptibilityPy.h"
#include "InfectionPy.h"
#include "IContagionPopulation.h"
#include "PyInterventionsContainer.h"
#include "IdmString.h"
#include "SimulationConfig.h"
#include "PythonSupport.h"

#ifndef WIN32
#include <sys/time.h>
#endif

#pragma warning(disable: 4244)

static const char * _module = "IndividualPy";

#define UNINIT_TIMER (-100.0f)


namespace Kernel
{
    inline float generateRandFromLogNormal(float m, float s) {
        // inputs: m is mean of underlying distribution, s is std dev
        return (exp((m)+randgen->eGauss()*s));
    }

    BEGIN_QUERY_INTERFACE_DERIVED(IndividualHumanPy, IndividualHuman)
        HANDLE_INTERFACE(IIndividualHumanPy)
    END_QUERY_INTERFACE_DERIVED(IndividualHumanPy, IndividualHuman)

    IndividualHumanPy::IndividualHumanPy(suids::suid _suid, float monte_carlo_weight, float initial_age, int gender, float initial_poverty) :
        IndividualHuman(_suid, monte_carlo_weight, initial_age, gender, initial_poverty)
    {
#ifdef ENABLE_PYTHON_FEVER
        // Call into python script to notify of new individual
        if( PythonSupportPtr != nullptr )
        {
            static auto pFunc = PythonSupportPtr->IdmPyInit( PythonSupport::SCRIPT_PYTHON_FEVER.c_str(), "create" );
            if( pFunc )
            {
                // pass individual id
                static PyObject * vars = PyTuple_New(4); 
                vars = Py_BuildValue( "lffs", _suid.data, monte_carlo_weight, initial_age, PyString_FromFormat( "%s", ( ( gender==0 ) ? "MALE" : "FEMALE" ) ) );
                // now ready to call function
                auto ret = PyObject_CallObject( pFunc, vars );
                if( ret == nullptr )
                {
                    PyErr_Print();
                    std::stringstream msg;
                    msg << "Embedded python code failed: PyObject_CallObject failed in call to 'create'.";
                    throw Kernel::IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
                }
                // vars ref count is always 1 here
            }
        }
#endif
    }

    IndividualHumanPy::~IndividualHumanPy()
    {
#ifdef ENABLE_PYTHON_FEVER
        // Call into python script to notify of new individual
        if( PythonSupportPtr != nullptr )
        {
            static auto pFunc = PythonSupportPtr->IdmPyInit( PythonSupport::SCRIPT_PYTHON_FEVER.c_str(), "destroy" );
            if( pFunc )
            {
                static PyObject * vars = PyTuple_New(1);
                //vars = Py_BuildValue( "l", GetSuid().data ); // this gives errors. :(
                PyObject* py_id = PyLong_FromLong( GetSuid().data );
                PyTuple_SetItem(vars, 0, py_id );
                auto ret = PyObject_CallObject( pFunc, vars );
                if( ret == nullptr )
                {
                    PyErr_Print();
                    std::stringstream msg;
                    msg << "Embedded python code failed: PyObject_CallObject failed in call to 'destroy'.";
                    throw Kernel::IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
                }
            }
        }
#endif
    }

    void IndividualHumanPy::InitializeStaticsPy( const Configuration* config ) // just called once!
    {
        SusceptibilityPyConfig immunity_config;
        immunity_config.Configure( config );
        InfectionPyConfig infection_config;
        infection_config.Configure( config );
    }

    IndividualHumanPy *IndividualHumanPy::CreateHuman(INodeContext *context, suids::suid id, float monte_carlo_weight, float initial_age, int gender, float initial_poverty)
    {
        IndividualHumanPy *newhuman = _new_ IndividualHumanPy(id, monte_carlo_weight, initial_age, gender, initial_poverty);
        
        newhuman->SetContextTo(context);
        LOG_DEBUG_F( "Created human with age=%f\n", newhuman->m_age );
        return newhuman;
    }

    void IndividualHumanPy::PropagateContextToDependents()
    {
        IndividualHuman::PropagateContextToDependents();
        pydemo_susceptibility = static_cast<SusceptibilityPy*>(susceptibility);
    }

    void IndividualHumanPy::setupInterventionsContainer()
    {
        interventions = _new_ PyInterventionsContainer();
    }

    void IndividualHumanPy::CreateSusceptibility(float imm_mod, float risk_mod)
    {
        SusceptibilityPy *newsusceptibility = SusceptibilityPy::CreateSusceptibility(this, m_age, imm_mod, risk_mod);
        pydemo_susceptibility = newsusceptibility;
        susceptibility = newsusceptibility;
    }

    void IndividualHumanPy::Expose( const IContagionPopulation* cp, float dt, TransmissionRoute::Enum transmission_route )
    { 
#ifdef ENABLE_PYTHON_FEVER
        if( cp->GetTotalContagion() == 0 )
        {
            return;
        }

        LOG_DEBUG_F( "Calling py:expose with contagion pop %f\n", cp->GetTotalContagion() );

        static auto pFunc = PythonSupportPtr->IdmPyInit( PythonSupport::SCRIPT_PYTHON_FEVER.c_str(), "expose" );
        if( pFunc )
        {
            // pass individual id AND dt
            static PyObject * vars = PyTuple_New(4);

            vars = Py_BuildValue( "llls", GetSuid().data, int(cp->GetTotalContagion()), int(dt), PyLong_FromLong( transmission_route == TransmissionRoute::TRANSMISSIONROUTE_ENVIRONMENTAL ? 0 : 1 ) ); 
            PyObject * retVal = PyObject_CallObject( pFunc, vars );
            if( retVal == nullptr )
            {
                PyErr_Print();
                std::stringstream msg;
                msg << "Embedded python code failed: PyObject_CallObject failed in call to 'expose'.";
                throw Kernel::IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
            }
            bool val = false;
            PyArg_Parse( retVal, "b", &val );
            if( val )
            {
                StrainIdentity strainId;
                AcquireNewInfection(&strainId);
            }
#if !defined(_WIN32) || !defined(_DEBUG)
            Py_DECREF( retVal );
#endif
        }
        return;
#endif
    }

    void IndividualHumanPy::ExposeToInfectivity(float dt, const TransmissionGroupMembership_t* transmissionGroupMembership)
    {
        IndividualHuman::ExposeToInfectivity(dt, transmissionGroupMembership);
    }

    void IndividualHumanPy::UpdateInfectiousness(float dt)
    {
#ifdef ENABLE_PYTHON_FEVER
        for( auto &route: parent->GetTransmissionRoutes() )
        {
            static auto pFunc = PythonSupportPtr->IdmPyInit( PythonSupport::SCRIPT_PYTHON_FEVER.c_str(), "update_and_return_infectiousness" );
            if( pFunc )
            {
                // pass individual id ONLY
                static PyObject * vars = PyTuple_New(2);

                vars = Py_BuildValue( "ls", GetSuid().data, PyString_FromFormat( "%s", route.c_str() ) );
                auto retVal = PyObject_CallObject( pFunc, vars );
                if( retVal == nullptr )
                {
                    PyErr_Print();
                    std::stringstream msg;
                    msg << "Embedded python code failed: PyObject_CallObject failed in call to 'update_and_return_infectiousness'.";
                    throw Kernel::IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
                }
                auto val = PyFloat_AsDouble(retVal);
                infectiousness += val;
                StrainIdentity tmp_strainID;
                release_assert( transmissionGroupMembershipByRoute.find( route ) != transmissionGroupMembershipByRoute.end() );
                if( val > 0 )
                {
                    LOG_DEBUG_F("Depositing %f to route %s: (antigen=%d, substain=%d)\n", val, route.c_str(), tmp_strainID.GetAntigenID(), tmp_strainID.GetGeneticID());
                    parent->DepositFromIndividual( &tmp_strainID, (float) val, &transmissionGroupMembershipByRoute.at( route ) );
                }
#if !defined(_WIN32) || !defined(_DEBUG)
                Py_DECREF( retVal );
#endif
            }
        }
        return;
#endif
    }

    Infection* IndividualHumanPy::createInfection( suids::suid _suid )
    {
        return InfectionPy::CreateInfection(this, _suid);
    }

    std::string IndividualHumanPy::processPrePatent( float dt )
    {
        return state_to_report;
    }

    void IndividualHumanPy::Update( float currenttime, float dt)
    {
#ifdef ENABLE_PYTHON_FEVER
        static auto pFunc = PythonSupportPtr->IdmPyInit( PythonSupport::SCRIPT_PYTHON_FEVER.c_str(), "update" );
        if( pFunc )
        {
            // pass individual id AND dt
            static PyObject * vars = PyTuple_New(2);
            vars = Py_BuildValue( "ll", GetSuid().data, int(dt) );
            auto pyVal = PyObject_CallObject( pFunc, vars );
            if( pyVal != nullptr )
            {
                char * state = "UNSET";
                PyArg_ParseTuple(pyVal,"si",&state, &state_changed ); //o-> pyobject |i-> int|s-> char*
                state_to_report = state;
            }
            else
            {
                state_to_report = "D";
            }
#if !defined(_WIN32) || !defined(_DEBUG)
            Py_DECREF(pyVal);
#endif
            PyErr_Print();
        }
        LOG_DEBUG_F( "state_to_report for individual %d = %s; Infected = %d, change = %d.\n", GetSuid().data, state_to_report.c_str(), IsInfected(), state_changed );

        if( state_to_report == "S" && state_changed && GetInfections().size() > 0 )
        {
            LOG_DEBUG_F( "[Update] Somebody cleared their infection.\n" );
            // ClearInfection
            auto inf = GetInfections().front();
            IInfectionPy * inf_pydemo  = NULL;
            if (s_OK != inf->QueryInterface(GET_IID(IInfectionPy ), (void**)&inf_pydemo) )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "inf", "IInfectionPy ", "Infection" );
            }
            // get InfectionPy pointer
            inf_pydemo->Clear();
        }
        else if( state_to_report == "D" && state_changed )
        {
            LOG_INFO_F( "[Update] Somebody died from their infection.\n" );
        }
#endif
        return IndividualHuman::Update( currenttime, dt);
    }

    void IndividualHumanPy::AcquireNewInfection(StrainIdentity *infstrain, int incubation_period_override )
    {
        LOG_DEBUG_F("AcquireNewInfection: route %d\n", _routeOfInfection);
        IndividualHuman::AcquireNewInfection( infstrain, incubation_period_override );
#ifdef ENABLE_PYTHON_FEVER
        static auto pFunc = PythonSupportPtr->IdmPyInit( PythonSupport::SCRIPT_PYTHON_FEVER.c_str(), "acquire_infection" );
        if( pFunc )
        {
            // pass individual id ONLY
            static PyObject * vars = PyTuple_New(1);

            PyObject* py_existing_id = PyLong_FromLong( GetSuid().data );
            PyTuple_SetItem(vars, 0, py_existing_id );

            //vars = Py_BuildValue( "l", GetSuid().data ); // BuildValue with 1 param seems to give errors
            auto ret = PyObject_CallObject( pFunc, vars );
            if( ret == nullptr )
            {
                PyErr_Print();
                std::stringstream msg;
                msg << "Embedded python code failed: PyObject_CallObject failed in call to 'acquire_infection'.";
                throw Kernel::IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
            }
        }
#endif
    }

    HumanStateChange IndividualHumanPy::GetStateChange() const
    {
        HumanStateChange retVal = StateChange;
        //auto parsed = IdmString(state_to_report).split();
        if( state_to_report == "D" )
        {
            LOG_INFO_F( "[GetStateChange] Somebody died from their infection.\n" );
            retVal = HumanStateChange::KilledByInfection;
        }
        return retVal;
    }
}

#endif // ENABLE_PYTHON
