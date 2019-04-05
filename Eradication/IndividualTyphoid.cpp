/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#ifdef ENABLE_TYPHOID
#include "Debug.h"
#include "RANDOM.h"
#include "Environment.h"
#include "IndividualTyphoid.h"
#include "SusceptibilityTyphoid.h"
#include "InfectionTyphoid.h"
#include "IContagionPopulation.h"
#include "TyphoidInterventionsContainer.h"
#include "IdmString.h"
#include "SimulationConfig.h"
#include "NodeTyphoid.h"
#include "NodeTyphoidEventContext.h"
#include "IdmDateTime.h"

#ifdef ENABLE_PYTHOID
#include "PythonSupport.h"
#endif

#ifndef WIN32
#include <sys/time.h>
#endif

#pragma warning(disable: 4244)

SETUP_LOGGING( "IndividualTyphoid" )

namespace Kernel
{
//#ifdef LOG_INFO_F
//#undef LOG_INFO_F
//#endif
//#define LOG_INFO_F printf
//#ifdef LOG_DEBUG_F
//#undef LOG_DEBUG_F
//#endif
//#define LOG_DEBUG_F printf

    float IndividualHumanTyphoidConfig::environmental_incubation_period = 0.0f; // NaturalNumber please
    float IndividualHumanTyphoidConfig::typhoid_acute_infectiousness = 0.0f;
    float IndividualHumanTyphoidConfig::typhoid_chronic_relative_infectiousness = 0.0f;

    //        float IndividualHumanTyphoidConfig::typhoid_environmental_exposure = 0.0f;
    float IndividualHumanTyphoidConfig::typhoid_prepatent_relative_infectiousness = 0.0f;
    float IndividualHumanTyphoidConfig::typhoid_protection_per_infection = 0.0f;
    float IndividualHumanTyphoidConfig::typhoid_subclinical_relative_infectiousness = 0.0f;
    float IndividualHumanTyphoidConfig::typhoid_carrier_probability = 0.0f;
    float IndividualHumanTyphoidConfig::typhoid_3year_susceptible_fraction = 0.0f;
    float IndividualHumanTyphoidConfig::typhoid_6month_susceptible_fraction = 0.0f;
    float IndividualHumanTyphoidConfig::typhoid_6year_susceptible_fraction = 0.0f;
    float IndividualHumanTyphoidConfig::typhoid_symptomatic_fraction = 0.0f;
    float IndividualHumanTyphoidConfig::typhoid_exposure_lambda = 0.0f;

    float IndividualHumanTyphoidConfig::typhoid_environmental_exposure_rate = 0.0f;
    float IndividualHumanTyphoidConfig::typhoid_contact_exposure_rate = 0.0f;


    GET_SCHEMA_STATIC_WRAPPER_IMPL(Typhoid.Individual,IndividualHumanTyphoidConfig)

    BEGIN_QUERY_INTERFACE_BODY(IndividualHumanTyphoidConfig)
    END_QUERY_INTERFACE_BODY(IndividualHumanTyphoidConfig)

    bool
    IndividualHumanTyphoidConfig::Configure( const Configuration* config ) // just called once!
    {
        LOG_DEBUG( "Configure\n" );
        // 
        // typhoid
        //
        //initConfigTypeMap( "Environmental_Incubation_Period", &environmental_incubation_period, "So-called Waiting Period for environmental contagion, time between deposit and exposure.", 0, 100, 30 );
        initConfigTypeMap( "Typhoid_Acute_Infectiousness", &typhoid_acute_infectiousness, Typhoid_Acute_Infectiousness_DESC_TEXT, 0, 1e7, 4000 );
        initConfigTypeMap( "Typhoid_Chronic_Relative_Infectiousness", &typhoid_chronic_relative_infectiousness, Typhoid_Chronic_Relative_Infectiousness_DESC_TEXT, 0, 1, 1 ); 
        initConfigTypeMap( "Typhoid_Prepatent_Relative_Infectiousness", &typhoid_prepatent_relative_infectiousness, Typhoid_Prepatent_Relative_Infectiousness_DESC_TEXT, 0, 1, 1 ); 
        initConfigTypeMap( "Typhoid_Protection_Per_Infection", &typhoid_protection_per_infection, Typhoid_Protection_Per_Infection_DESC_TEXT, 0, 1, 0.1f ); 
        initConfigTypeMap( "Typhoid_Subclinical_Relative_Infectiousness", &typhoid_subclinical_relative_infectiousness, Typhoid_Subclinical_Relative_Infectiousness_DESC_TEXT, 0, 1, 1 );
        initConfigTypeMap( "Typhoid_Carrier_Probability", &typhoid_carrier_probability, Typhoid_Carrier_Probability_DESC_TEXT, 0, 1, 0.25 );
        initConfigTypeMap( "Typhoid_6month_Susceptible_Fraction", &typhoid_6month_susceptible_fraction, Typhoid_6month_Susceptible_Fraction_DESC_TEXT, 0, 1, 0.5 );
        initConfigTypeMap( "Typhoid_3year_Susceptible_Fraction", &typhoid_3year_susceptible_fraction, Typhoid_3year_Susceptible_Fraction_DESC_TEXT, 0, 1, 0.5 );
        initConfigTypeMap( "Typhoid_Environmental_Exposure_Rate", &typhoid_environmental_exposure_rate, Typhoid_Environmental_Exposure_Rate_DESC_TEXT, 0, 10, 0.5 );
        initConfigTypeMap( "Typhoid_Contact_Exposure_Rate", &typhoid_contact_exposure_rate, Typhoid_Contact_Exposure_Rate_DESC_TEXT, 0, 100, 0.5 );
        initConfigTypeMap( "Typhoid_6year_Susceptible_Fraction", &typhoid_6year_susceptible_fraction, Typhoid_6year_Susceptible_Fraction_DESC_TEXT, 0, 1, 0.5 );
        initConfigTypeMap( "Typhoid_Symptomatic_Fraction", &typhoid_symptomatic_fraction, Typhoid_Symptomatic_Fraction_DESC_TEXT, 0, 1, 0.5 );   
        initConfigTypeMap( "Typhoid_Exposure_Lambda", &typhoid_exposure_lambda, Typhoid_Exposure_Lambda_DESC_TEXT, -1, 100, 2 );
        
        SusceptibilityTyphoidConfig fakeImmunity;
        fakeImmunity.Configure( config );
        InfectionTyphoidConfig fakeInfection;
        fakeInfection.Configure( config );

        //do we need to call initConfigTypeMap? DLC 
        bool ret = JsonConfigurable::Configure( config );
        return ret;
    }

    void IndividualHumanTyphoid::InitializeStatics( const Configuration * config )
    {
        IndividualHumanTyphoidConfig human_config;
        human_config.Configure( config );
    }

    class Stopwatch
    {
        public:
            Stopwatch( const char* label )
            {
#ifndef WIN32
                gettimeofday( &start_tv, nullptr );
#endif
                _label = label;
            }
            ~Stopwatch()
            {
#ifndef WIN32
                struct timeval stop_tv;
                gettimeofday( &stop_tv, nullptr );
                float timespentinms = ( stop_tv.tv_sec - start_tv.tv_sec ) + 1e-06* ( stop_tv.tv_usec - start_tv.tv_usec );
#endif
                //std::cout << "[" << _label << "] duration = " << timespentinms << std::endl;
            }
#ifndef WIN32
            struct timeval start_tv;
#endif
            std::string _label;
    };
    const float IndividualHumanTyphoid::P5 = 0.05f; // probability of typhoid death
    const float IndividualHumanTyphoid::P7 = 0.0f; // probability of clinical immunity after acute infection

    // environmental exposure constants
    const int IndividualHumanTyphoid::N50 = 1110000;
    const float IndividualHumanTyphoid::alpha = 0.175f;


    BEGIN_QUERY_INTERFACE_DERIVED(IndividualHumanTyphoid, IndividualHumanEnvironmental)
        HANDLE_INTERFACE(IIndividualHumanTyphoid)
    END_QUERY_INTERFACE_DERIVED(IndividualHumanTyphoid, IndividualHumanEnvironmental)

    IndividualHumanTyphoid::IndividualHumanTyphoid(suids::suid _suid, float monte_carlo_weight, float initial_age, int gender) :
        IndividualHumanEnvironmental(_suid, monte_carlo_weight, initial_age, gender)
    {
#ifdef ENABLE_PYTHOID
        volatile Stopwatch * check = new Stopwatch( __FUNCTION__ );
        // Call into python script to notify of new individual
        static auto pFunc = PythonSupport::GetPyFunction( PythonSupport::SCRIPT_TYPHOID.c_str(), "create" );
        if( pFunc )
        {
            // pass individual id
            static PyObject * vars = PyTuple_New(4);
            PyObject* py_newid = PyLong_FromLong( _suid.data );
            PyObject* py_newmcweight = PyFloat_FromDouble( monte_carlo_weight );
            PyObject* py_newage = PyFloat_FromDouble( initial_age );
            PyObject* py_newsex_str = PyString_FromFormat( "%s", ( ( gender==0 ) ? "MALE" : "FEMALE" ) );

            PyTuple_SetItem(vars, 0, py_newid );
            PyTuple_SetItem(vars, 1, py_newmcweight );
            PyTuple_SetItem(vars, 2, py_newage );
            PyTuple_SetItem(vars, 3, py_newsex_str );
            PyObject_CallObject( pFunc, vars );

            //Py_DECREF( vars );
            //Py_DECREF( py_newid_str );
            //Py_DECREF( py_newmcweight_str );
            //Py_DECREF( py_newage_str );
            PyErr_Print();
        }
        delete check;
#else 
        _infection_count=0; // should not be necessary
        doseTracking = "Low";
        exposureRoute = TransmissionRoute::TRANSMISSIONROUTE_OUTBREAK;
#endif
    }

    IndividualHumanTyphoid::~IndividualHumanTyphoid()
    {
#ifdef ENABLE_PYTHOID
        volatile Stopwatch * check = new Stopwatch( __FUNCTION__ );
        // Call into python script to notify of new individual
        static auto pFunc = PythonSupport::GetPyFunction( PythonSupport::SCRIPT_TYPHOID.c_str(), "destroy" );
        if( pFunc )
        {
            static PyObject * vars = PyTuple_New(1);
            PyObject* py_id = PyLong_FromLong( GetSuid().data );
            PyTuple_SetItem(vars, 0, py_id );
            PyObject_CallObject( pFunc, vars );
            //Py_DECREF( vars );
            //Py_DECREF( py_id_str  );
            PyErr_Print();
        }
        delete check;
#endif
    }

    IndividualHumanTyphoid *IndividualHumanTyphoid::CreateHuman(INodeContext *context, suids::suid id, float monte_carlo_weight, float initial_age, int gender)
    {
        IndividualHumanTyphoid *newhuman = _new_ IndividualHumanTyphoid(id, monte_carlo_weight, initial_age, gender);

        newhuman->SetContextTo(context);
        LOG_DEBUG_F( "Created human %d with age=%f and sex=%s\n", id.data, newhuman->m_age, ( gender == 0 ? "Male" : "Female" ) );
        return newhuman;
    }

    void IndividualHumanTyphoid::PropagateContextToDependents()
    {
        IndividualHuman::PropagateContextToDependents();
        typhoid_susceptibility = static_cast<SusceptibilityTyphoid*>(susceptibility);
    }

    void IndividualHumanTyphoid::setupInterventionsContainer()
    {
        interventions = _new_ TyphoidInterventionsContainer();
    }

    void IndividualHumanTyphoid::CreateSusceptibility(float imm_mod, float risk_mod)
    {
        SusceptibilityTyphoid *newsusceptibility = SusceptibilityTyphoid::CreateSusceptibility(this, m_age, imm_mod, risk_mod);
        typhoid_susceptibility = newsusceptibility;
        susceptibility = newsusceptibility;
    }


#define HIGH_ENVIRO_DOSE_THRESHOLD (275050000)
    void IndividualHumanTyphoid::quantizeEnvironmentalDoseTracking( float environment )
    {
        if (environment <= HIGH_ENVIRO_DOSE_THRESHOLD)
        { 
            doseTracking = "Low";
        }

        else if( environment > HIGH_ENVIRO_DOSE_THRESHOLD ) // just else should do
        {
            doseTracking = "High";
        }
        LOG_VALID_F( "doseTracking set to %s based on value of %f.\n", doseTracking.c_str(), environment );
        release_assert( doseTracking != "None" );
    }

    void IndividualHumanTyphoid::quantizeContactDoseTracking( float fContact )
    {
        doseTracking = "High";
    }

    void IndividualHumanTyphoid::Expose( const IContagionPopulation* cp, float dt, TransmissionRoute::Enum transmission_route )
    { 
#ifdef ENABLE_PYTHOID
        if( GetRng()->e() > IndividualHumanTyphoidConfig::typhoid_exposure_fraction )
        {
            return;
        }

        volatile Stopwatch * check = new Stopwatch( __FUNCTION__ );
        static auto pFunc = PythonSupport::GetPyFunction( PythonSupport::SCRIPT_TYPHOID.c_str(), "expose" );
        if( pFunc )
        {
            // pass individual id AND dt
            static PyObject * vars = PyTuple_New(4);
            PyObject* py_existing_id = PyLong_FromLong( GetSuid().data );

            // silly. can't seem to figure out how to do floats so doing this way for now!
            PyObject* py_contagion_pop = PyLong_FromLong( cp->GetTotalContagion() );

            //PyObject* py_contagion_pop = Py_BuildValue( "%f", 
            PyObject* py_dt = PyLong_FromLong( dt );

            //PyObject* py_tx_route = PyString_FromFormat( "%s", TransmissionRoute::pairs::lookup_key( transmission_route ) );
            PyObject* py_tx_route = PyLong_FromLong( transmission_route == TransmissionRoute::TRANSMISSIONROUTE_ENVIRONMENTAL ? 0 : 1 );
            PyTuple_SetItem(vars, 0, py_existing_id );
            PyTuple_SetItem(vars, 1, py_contagion_pop  );
            PyTuple_SetItem(vars, 2, py_dt  );
            PyTuple_SetItem(vars, 3, py_tx_route );
            PyObject * retVal = PyObject_CallObject( pFunc, vars );
            PyErr_Print();
            auto val = (bool) PyInt_AsLong(retVal);
            if( val )
            {
                StrainIdentity strainId;
                AcquireNewInfection(&strainId);
            }
            //Py_DECREF( vars );
            //Py_DECREF( py_existing_id_str );
            //Py_DECREF( py_contagion_pop );
            //Py_DECREF( py_dt );
            //Py_DECREF( py_tx_route );
            Py_DECREF( retVal );
        }
        delete check;
        return;
#else
        //LOG_DEBUG_F("Expose route: %d, %f\n", transmission_route, cp->GetTotalContagion());
        if ( IsInfected() )
        {
            return;
        }

        if ( susceptibility->getModAcquire() == 0 )
        {
            return;
        }

        if( IndividualHumanConfig::enable_skipping )
        {
            bool bEnvironmentalAllowed = false;
            bool bContactAllowed = false;
            // Decide with route to pick (ignore) based on probabilities.
            float e_prob = parent->GetMaxInfectionProb( TransmissionRoute::TRANSMISSIONROUTE_ENVIRONMENTAL ); 
            float c_prob = parent->GetMaxInfectionProb( TransmissionRoute::TRANSMISSIONROUTE_CONTACT ); 
            if ( ( e_prob + c_prob ) == 0 )
            {
                return;
            }

            float route_prob_draw = GetRng()->e();

            if ( route_prob_draw < (c_prob/(c_prob+e_prob)) )
            {
                if ( c_prob == 0 )
                {
                    return;
                }

                // Contact route
                LOG_DEBUG_F( "Expose only through contact route.\n" );
                bContactAllowed = true;
            }
            else if ( route_prob_draw >= ((1.0-e_prob)/(1.0-((1.0-c_prob)*(1.0-e_prob)))) ) // both?
            {
                // Double-exposure
                LOG_DEBUG_F( "Expose through both routes.\n" );
                bContactAllowed       = true;
                bEnvironmentalAllowed = true;
            }
            else
            {
                if ( e_prob == 0 )
                {
                    return;
                }

                // Environmental route
                LOG_DEBUG_F( "Expose only through environmental route.\n" );
                bEnvironmentalAllowed = true;
            }

            if ( !bEnvironmentalAllowed && (transmission_route==TransmissionRoute::TRANSMISSIONROUTE_ENVIRONMENTAL) )
            {
                LOG_DEBUG_F( "Return without exposure: route is enviro and we're only doing contact.\n" );
                return;
            }

            if ( !bContactAllowed && (transmission_route==TransmissionRoute::TRANSMISSIONROUTE_CONTACT) )
            {
                LOG_DEBUG_F( "Return without exposure: route is contact and we're only doing enviro.\n" );
                return;
            }
        }

        doseTracking = "Low";
        if ( transmission_route == TransmissionRoute::TRANSMISSIONROUTE_ENVIRONMENTAL )
        {
            // Reduce exposure amount by node-level (environmental) intervention effect.
            auto typhoid_node_iv_effects = (NodeTyphoidEventContextHost*)(parent->GetEventContext());
            auto eda = typhoid_node_iv_effects->GetEnviroDoseAttenuation( GetProperties() );
            auto exa = typhoid_node_iv_effects->GetEnviroExposuresAttenuation( GetProperties() );
            NonNegativeFloat exposure = cp->GetTotalContagion() * eda;
            LOG_VALID_F( "environment = %f based on contagion of %f and interventions %f.\n", float(exposure), cp->GetTotalContagion(), float(eda) );
            if ( exposure == 0 )
            {
                return;
            }

            if ( exposure > 0 )
            {
                NonNegativeFloat infects = 1.0f-pow( 1.0f + exposure * ( pow( 2.0f, (1.0/alpha) ) - 1.0f )/N50, -alpha ); // Dose-response for prob of infection 
                ProbabilityNumber immunity= pow(1.0f-IndividualHumanTyphoidConfig::typhoid_protection_per_infection, _infection_count);
                LOG_VALID_F( "Individual=%d: immunity calculated as %f from typhoid_protection_per_infection=%f and _infection_count=%d.\n",
                             GetSuid().data, float(immunity), IndividualHumanTyphoidConfig::typhoid_protection_per_infection, _infection_count );
                NaturalNumber number_of_exposures = GetRng()->Poisson(IndividualHumanTyphoidConfig::typhoid_environmental_exposure_rate * dt * exa );
                if( IndividualHumanConfig::enable_skipping )
                {
                    NO_MORE_THAN( number_of_exposures, 3 )
                }

                ProbabilityNumber prob = 0;
                if ( number_of_exposures > 0 )
                {
                    auto ira = interventions->GetInterventionReducedAcquire();
                    if ( ira < 1.0f )
                    {
                        LOG_VALID_F( "SimpleVaccine(?) modifier for individual %d (enviro route) = %f.\n", GetSuid().data, ira );
                    }
                    try
                    {
                        prob = 1.0f - pow(1.0f - immunity * infects * ira, number_of_exposures);
                    }
                    catch( DetailedException& /*ex*/ )
                    {
                        throw CalculatedValueOutOfRangeException( __FILE__, __LINE__, __FUNCTION__, "prob (of exposure)", prob, 1.0 );
                    }
                }
                LOG_VALID_F( "Exposing individual %d age %f on route 'environment': prob=%f, infects=%f, immunity=%f, num_exposures=%d, exposure=%f, environment=%f, iv_mult=%f.\n",
                             GetSuid().data, GetAge(), float(prob), float(infects), float(immunity), int(number_of_exposures), float(exposure), float(exposure), exa
                           );

                bool acquire = false; 
                if ( IndividualHumanConfig::enable_skipping )
                {
                    // Skipping code coming soon! Yes, this commented-out code is here for a reason.
                    float maxProb = parent->GetMaxInfectionProb( transmission_route ); 
                    assert(maxProb>=0.0 && maxProb<=1.0);
                    assert(maxProb>=prob);
                    if ( maxProb==prob ) // individual is maximally susceptible
                    {
                        acquire = true; 
                    }
                    else if( prob/maxProb > GetRng()->e() ) 
                    {
                        // DLC - individual is not maximally susceptible
                        acquire = true;
                    }
                } 
                else if( GetRng()->SmartDraw( prob ) )
                {
                    acquire = true;
                }
                
                if ( acquire )
                {
                    exposureRoute = transmission_route;
                    StrainIdentity strainId;
                    LOG_DEBUG("INDIVIDUAL INFECTED BY ENVIRONMENT.\n"); // This is for reporting DON'T DELETE :)
                    quantizeEnvironmentalDoseTracking( exposure );
                    //LOG_INFO_F("dose %f, tracking %s", environment, doseTracking);
                    AcquireNewInfection(&strainId);
                }
                return;
            }
        }
        else if ( transmission_route==TransmissionRoute::TRANSMISSIONROUTE_CONTACT )
        {
            float fContact = cp->GetTotalContagion() * ((TyphoidInterventionsContainer*)interventions)->GetContactDoseAttenuation();
            if ( fContact == 0 )
            {
                return;
            }

            ProbabilityNumber infects = fContact / IndividualHumanTyphoidConfig::typhoid_acute_infectiousness;

            NonNegativeFloat immunity= pow(1-IndividualHumanTyphoidConfig::typhoid_protection_per_infection, _infection_count);
            LOG_VALID_F( "Individual=%d: immunity calculated as %f from typhoid_protection_per_infection=%f and _infection_count=%d.\n",
                         GetSuid().data, float(immunity), IndividualHumanTyphoidConfig::typhoid_protection_per_infection, _infection_count );
            ProbabilityNumber intervention_multiplier = ((TyphoidInterventionsContainer*)interventions)->GetContactExposuresAttenuation();
            NaturalNumber number_of_exposures = GetRng()->Poisson(IndividualHumanTyphoidConfig::typhoid_contact_exposure_rate * dt * intervention_multiplier);
            if( IndividualHumanConfig::enable_skipping )
            {
                NO_MORE_THAN( number_of_exposures, 3 )
            }
            ProbabilityNumber prob = 0;
            if (number_of_exposures > 0)
            {
                auto ira = interventions->GetInterventionReducedAcquire();
                if( ira < 1.0f )
                {
                    LOG_VALID_F( "SimpleVaccine(?) modifier for individual %d (contact route) = %f.\n", GetSuid().data, ira );
                }
                try
                {
                    prob = 1.0f - pow(1.0f - immunity * infects * ira, number_of_exposures);
                }
                catch( DetailedException& /*ex*/ )
                {
                    throw CalculatedValueOutOfRangeException( __FILE__, __LINE__, __FUNCTION__, "prob (of exposure)", 1.0f - pow(1.0f - immunity * infects * ira, number_of_exposures), 1.0 );
                }
            }
            LOG_VALID_F( "Exposing individual %d age %f on route 'contact': prob=%f, infects=%f, immunity=%f, num_exposures=%d, fContact=%f, iv_mult=%f.\n",
                         GetSuid().data, GetAge(), float(prob), float(infects), float(immunity), int(number_of_exposures), float(fContact), float(intervention_multiplier)
                       );


            bool acquire = false; 
            if( IndividualHumanConfig::enable_skipping )
            {
                float maxProb = parent->GetMaxInfectionProb( transmission_route );  // contact
                release_assert(maxProb>=0.0 && maxProb<=1.0);
                release_assert(maxProb>=prob || fabs(maxProb-prob) < 0.001);
                if( maxProb==prob ) // individual is maximally susceptible
                {
                    acquire = true; 
                }
                else if( prob/maxProb > GetRng()->e() )
                {
                    // DLC - individual is not maximally susceptible
                    acquire = true;
                }
            }
            else if( GetRng()->SmartDraw( prob ) )
            {
                acquire = true;
            }

            if( acquire )
            {
                LOG_DEBUG("INDIVIDUAL INFECTED BY CONTACT.\n"); // FOR REPORTING
                exposureRoute = transmission_route;
                StrainIdentity strainId;
                quantizeContactDoseTracking( fContact ); 
                AcquireNewInfection(&strainId);
            }

            return;
        }
        
#endif
    }

    /*void IndividualHumanTyphoid::ExposeToInfectivity(float dt, const TransmissionGroupMembership_t* transmissionGroupMembership)
    {
        if( IndividualHumanConfig::enable_skipping )
        {
            route_prob_draw = GetRng()->e();
        }
        IndividualHumanEnvironmental::ExposeToInfectivity(dt, &transmissionGroupMembershipByRoute.at("contact"));
        IndividualHumanEnvironmental::ExposeToInfectivity(dt, &transmissionGroupMembershipByRoute["environmental"]);
    }*/

    void IndividualHumanTyphoid::UpdateInfectiousness(float dt)
    {
#ifdef ENABLE_PYTHOID
        //volatile Stopwatch * check = new Stopwatch( __FUNCTION__ );
        for( auto &route: parent->GetTransmissionRoutes() )
        {
            static auto pFunc = PythonSupport::GetPyFunction( PythonSupport::SCRIPT_TYPHOID.c_str(), "update_and_return_infectiousness" );
            if( pFunc )
            {
                // pass individual id ONLY
                static PyObject * vars = PyTuple_New(2);
                PyObject* py_existing_id = PyLong_FromLong( GetSuid().data );
                PyTuple_SetItem( vars, 0, py_existing_id );
                PyObject* py_route_str = PyString_FromFormat( "%s", route.c_str() );
                PyTuple_SetItem( vars, 1, py_route_str );
                PyObject * retVal = PyObject_CallObject( pFunc, vars );
                PyErr_Print();
                auto val = PyFloat_AsDouble(retVal);
                infectiousness += val;
                StrainIdentity tmp_strainID;
                release_assert( transmissionGroupMembershipByRoute.find( route ) != transmissionGroupMembershipByRoute.end() );
                LOG_DEBUG_F("Depositing %f to route %s: (antigen=%d, substain=%d)\n", val, route.c_str(), tmp_strainID.GetAntigenID(), tmp_strainID.GetGeneticID());
                parent->DepositFromIndividual( tmp_strainID, (float) val, &transmissionGroupMembershipByRoute.at( route ) );
                //Py_DECREF( vars );
                //Py_DECREF( py_existing_id_str );
                //Py_DECREF( py_route_str );
                Py_DECREF( retVal );
            }
        }
        //delete check;
        return;
#else
        if( !IsInfected() && !IsChronicCarrier() )
        {
            return;
        }

        state_to_report = ((InfectionTyphoid*)infections.front())->GetStateToReport();
        for (auto infection : infections)
        {
            //LOG_DEBUG("Getting infectiousness by route.\n");

            StrainIdentity tmp_strainID;
            infection->GetInfectiousStrainID(&tmp_strainID);

            //deposit oral to 'contact', fecal to 'environmental' pool 
            for(auto& entry : transmissionGroupMembershipByRoute)
            {
                //LOG_DEBUG_F("Found route:%s.\n",entry.first.c_str());
                auto tic = (TyphoidInterventionsContainer*)interventions;
                auto irt = interventions->GetInterventionReducedTransmit(); // from regular vaccine
                if (entry.first==string( CONTACT ))
                {
                    auto cda = tic->GetContactDepositAttenuation(); // from environmental intervention
                    float tmp_infectiousnessOral = m_mc_weight * infection->GetInfectiousness() * cda * irt;
                    if (tmp_infectiousnessOral > 0.0f)
                    {
                        LOG_VALID_F("Individual %d depositing %f to route %s: (antigen=%d, substrain=%d) at time %f in state %s. Intervention factor %f.\n",
                                    GetSuid().data, tmp_infectiousnessOral, entry.first.c_str(), tmp_strainID.GetAntigenID(), tmp_strainID.GetGeneticID(), float(parent->GetTime().timestep), state_to_report.c_str(), cda
                                   );
                        parent->DepositFromIndividual( tmp_strainID, tmp_infectiousnessOral, entry.second, TransmissionRoute::TRANSMISSIONROUTE_CONTACT );
                    } 
                }
                else if (entry.first==string( ENVIRONMENTAL ))
                {
                    float tmp_infectiousnessFecal =  m_mc_weight * infection->GetInfectiousness() * irt;
                    if (tmp_infectiousnessFecal > 0.0f)
                    {
                        auto typhoid_node_event = (NodeTyphoidEventContextHost*)(parent->GetEventContext());
                        auto eda = typhoid_node_event->GetEnviroDepositAttenuation( GetProperties() ); 
                        tmp_infectiousnessFecal *= eda;
                        LOG_VALID_F( "Individual %d depositing %f to route %s: (antigen=%d, substrain=%d) at time %f in state %s..\n",
                                     GetSuid().data, tmp_infectiousnessFecal, entry.first.c_str(), tmp_strainID.GetAntigenID(), tmp_strainID.GetGeneticID(), float(parent->GetTime().timestep)
                                   );
                        parent->DepositFromIndividual( tmp_strainID, tmp_infectiousnessFecal, entry.second, TransmissionRoute::TRANSMISSIONROUTE_ENVIRONMENTAL );
                        ///LOG_DEBUG_F("contagion= %f\n", cp->GetTotalContagion());
                    }
                }
                else
                {
                    LOG_WARN_F("unknown route %s, do not deposit anything.\n", entry.first.c_str());
                }
            }
        }
#endif
    }

    Infection* IndividualHumanTyphoid::createInfection( suids::suid _suid )
    {
        return InfectionTyphoid::CreateInfection(this, _suid);
    }

    std::string IndividualHumanTyphoid::processPrePatent( float dt )
    {
        return state_to_report;
    }

    void IndividualHumanTyphoid::Update( float currenttime, float dt)
    {
#ifdef ENABLE_PYTHOID
        volatile Stopwatch * check = new Stopwatch( __FUNCTION__ );
        static auto pFunc = PythonSupport::GetPyFunction( PythonSupport::SCRIPT_TYPHOID.c_str(), "update" );
        if( pFunc )
        {
            // pass individual id AND dt
            static PyObject * vars = PyTuple_New(2);
            PyObject* py_existing_id = PyLong_FromLong( GetSuid().data );
            PyObject* py_dt = PyLong_FromLong( (int) dt );
            PyTuple_SetItem(vars, 0, py_existing_id );
            PyTuple_SetItem(vars, 1, py_dt );
            auto pyVal = PyObject_CallObject( pFunc, vars );
            if( pyVal != nullptr )
            {
                //state_to_report = PyString_AsString(pyVal); 
                // parse tuple: char, bool
                //PyObject *ob1,*ob2;
                char * state = "UNSET";
                PyArg_ParseTuple(pyVal,"si",&state, &state_changed ); //o-> pyobject |i-> int|s-> char*
                state_to_report = state;
                //= PyString_AsString(ob1); 
                //state_changed = (bool) PyInt_AsLong(ob2); 
            }
            else
            {
                state_to_report = "DED";
            }
            //Py_DECREF( vars );
            //Py_DECREF( py_existing_id_str );
            //Py_DECREF( py_dt_str );
            PyErr_Print();
        }
        delete check;
        LOG_DEBUG_F( "state_to_report for individual %d = %s; Infected = %d.\n", GetSuid().data, state_to_report.c_str(), IsInfected() );

        if( state_to_report == "SUS" && state_changed && GetInfections().size() > 0 )
        {
            // ClearInfection
            auto inf = GetInfections().front();
            IInfectionTyphoid * inf_typhoid  = NULL;
            if (s_OK != inf->QueryInterface(GET_IID(IInfectionTyphoid ), (void**)&inf_typhoid) )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "inf", "IInfectionTyphoid ", "Infection" );
            }
            // get InfectionTyphoid pointer
            inf_typhoid->Clear();
        }
        else if( state_to_report == "DED" && state_changed )
        {
            LOG_INFO_F( "[Update] Somebody died from their infection.\n" );
        }
#endif
        IndividualHumanEnvironmental::Update( currenttime, dt);
        if( infections.size() == 0 )
        {
            state_to_report = "SUS";
        }
        else
        {
            state_to_report = ((InfectionTyphoid*)infections.front())->GetStateToReport();
            state_changed = ((InfectionTyphoid*)infections.front())->IsStateChanged();
        }
    }

    void IndividualHumanTyphoid::AcquireNewInfection( const IStrainIdentity *infstrain, int incubation_period_override )
    {
        if(IsInfected() )
        {
            return;
        }

        LOG_DEBUG_F("AcquireNewInfection: individual=%d, route=%d, IsInfected=%d\n", GetSuid().data, exposureRoute, IsInfected() );
        // neither environmental nor contact source. probably from initial seeding
        IndividualHumanEnvironmental::AcquireNewInfection( infstrain, incubation_period_override );
#ifdef ENABLE_PYTHOID
        volatile Stopwatch * check = new Stopwatch( __FUNCTION__ );
        static auto pFunc = PythonSupport::GetPyFunction( PythonSupport::SCRIPT_TYPHOID.c_str(), "acquire_infection" );
        if( pFunc )
        {
            // pass individual id ONLY
            static PyObject * vars = PyTuple_New(1);
            PyObject* py_existing_id = PyLong_FromLong( GetSuid().data );
            PyTuple_SetItem(vars, 0, py_existing_id );
            PyObject_CallObject( pFunc, vars );
            //Py_DECREF( vars );
        }
        delete check;
#else
        //LOG_INFO_F("Prepatent %d. dose %s \n", _prepatent_duration, doseTracking);
        _infection_count ++;
#endif
        exposureRoute = TransmissionRoute::TRANSMISSIONROUTE_OUTBREAK;
        doseTracking = "Low";
    }

    const std::string IndividualHumanTyphoid::getDoseTracking() const
    {
        return doseTracking;
    }

    HumanStateChange IndividualHumanTyphoid::GetStateChange() const
    {
        HumanStateChange retVal = StateChange;
        if( infections.size() == 0 )
        {
            return retVal;
        }
        //auto parsed = IdmString(state_to_report).split();
        if( state_to_report == "DED" )
        {
            LOG_INFO_F( "[GetStateChange] Somebody died from their infection.\n" );
            retVal = HumanStateChange::KilledByInfection;
        }
        return retVal;
    }

    float IndividualHumanTyphoid::GetImmunityReducedAcquire() const
    {
        // immunity = 1 - susceptibility
        auto base_susceptibility = susceptibility->getModAcquire();
        auto acquired_susceptibility = pow(1-IndividualHumanTyphoidConfig::typhoid_protection_per_infection, _infection_count);
        float immunity = (1-base_susceptibility) + (1-acquired_susceptibility);
        NO_MORE_THAN( immunity, 1.0f );
        return immunity;
    }

    bool IndividualHumanTyphoid::IsChronicCarrier( bool incidence_only ) const
    {
        if( state_to_report == "CHR" &&
                ( ( incidence_only && state_changed ) ||
                  ( incidence_only == false )
                )
          )
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    bool IndividualHumanTyphoid::IsSubClinical( bool incidence_only ) const
    {
        if( state_to_report == "SUB" &&
                ( ( incidence_only && state_changed ) ||
                  ( incidence_only == false )
                )
          )
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    bool IndividualHumanTyphoid::IsAcute( bool incidence_only ) const
    {
        if( state_to_report == "ACU" &&
                ( ( incidence_only && state_changed ) ||
                  ( incidence_only == false )
                )
          )
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    bool IndividualHumanTyphoid::IsPrePatent( bool incidence_only ) const
    {
        if( state_to_report == "PRE" &&
                ( ( incidence_only && state_changed ) ||
                  ( incidence_only == false )
                )
          )
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    void IndividualHumanTyphoid::ForceClearInfection()
    {
        // Maybe this should only work in Chronic?
        if( GetInfections().size() > 0 )
        {
            // ClearInfection
            auto inf = GetInfections().front();
            IInfectionTyphoid * inf_typhoid  = NULL;
            if (s_OK != inf->QueryInterface(GET_IID(IInfectionTyphoid ), (void**)&inf_typhoid) )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "inf", "IInfectionTyphoid ", "Infection" );
            }
            // get InfectionTyphoid pointer
            LOG_DEBUG_F( "Clearing infection in individual %d in state %s based on intervention.\n", GetSuid().data, state_to_report.c_str() );
            inf_typhoid->Clear();
            //release_assert( state_to_report == "CHR" ); // Keeping this during early smoke-testing of the feature
        }
    }

    REGISTER_SERIALIZABLE(IndividualHumanTyphoid);
    //template PoolManager<IndividualHumanTyphoid> IndividualHumanTyphoid::_pool;
    //template<> std::stack<IndividualHumanTyphoid*> PoolManager<IndividualHumanTyphoid>::_pool;                                   
    void IndividualHumanTyphoid::serialize(IArchive& ar, IndividualHumanTyphoid* obj)
    {
        IndividualHumanTyphoid& individual = *obj;
        //ar.labelElement("P1") & individual.P1;
        ar.labelElement("state_to_report") & individual.state_to_report;
        ar.labelElement("isChronic") & individual.isChronic;
        ar.labelElement("_infection_count") & individual._infection_count;
        //ar.labelElement("_routeOfInfection") & individual._routeOfInfection;
        ar.labelElement("state_changed") & individual.state_changed;
        ar.labelElement("doseTracking") & individual.doseTracking;
        ar.labelElement("_infection_count") & individual._infection_count;
        IndividualHumanEnvironmental::serialize(ar, obj);
    }
}

#endif // ENABLE_TYPHOID
