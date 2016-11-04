/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <string>
#include <list>
#include <map>
#include <functional>

#include "IdmApi.h"
#include "Configuration.h"
#include "FactorySupport.h"
#include "Configure.h"
#include "SimulationEnums.h"

// This macro, GET_CONFIGURABLE(Category), is the convenient and fast way to access the SimulationConfig parameters
// within monolithic executable Eradication.exe.  This can be used in DLL's as well since we include the pointer
// to the SimulationConfig object in Environment.
#define GET_CONFIGURABLE(Category)  ((Kernel::Category*)(Environment::get##Category()))

using namespace std;

namespace Kernel
{
    struct VectorParameters;
    struct MalariaParameters;
    struct TBParameters;
    struct PolioParameters;
    struct TBHIVParameters;

    class SimulationConfig;

    class ISimulationConfigFactory
    {
    public:
        virtual void Register(string classname, instantiator_function_t _if) = 0;
    };

    class IDMAPI SimulationConfigFactory : public ISimulationConfigFactory
    {
    public:
        static ISimulationConfigFactory * getInstance();

        static SimulationConfig* CreateInstance(const Configuration * config);
        void Register(string classname, instantiator_function_t _if);

    protected:
        static support_spec_map_t& getRegisteredClasses();

    private:
        static ISimulationConfigFactory * _instance;
    };


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SimulationConfig Class Layout Special Notes
// Note 1:
// All non-primitive objects on the stack (like STL or user-defined type) have to be declared at the end
// for the primitive objects in the SimulationConfig object crossing the DLL/EModule boundary
// without affecting the memory image and therefore its member values
// Note 2:
// All the non-primitive _static_ objects will not be able to cross DLL/EModule boundary
// so their values will be different on two sides of memory space even for the same SimulationConfig object
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!! DMB 9/7/2016 - This would be a bigger issue if we were supporting the ability of users to create their DLL to
// !!! work with our EXE.  Since we only support the situation where the user builds both the EXE and DLL,
// !!! the memory layout and boundary issues are not really a concern.
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details

    class IDMAPI SimulationConfig : public JsonConfigurable
    {

        DECLARE_FACTORY_REGISTERED(SimulationConfigFactory, SimulationConfig, IConfigurable)

    public:
        DECLARE_CONFIGURED(SimulationConfig)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

        // Enum type name                                    Enum variable name                                name in config.json
        //
        DistributionType::Enum                               immunity_initialization_distribution_type;        // Immunity_Initialization_Distribution_Type

        MigrationStructure::Enum                             migration_structure;                              // MIGRATION_STRUCTURE
        SusceptibilityScaling::Enum                          susceptibility_scaling;                           // Susceptibility_Scale_Type
        SimType::Enum                                        sim_type;                                         // Sim_Type
        VitalDeathDependence::Enum                           vital_death_dependence;                           // Vital_Death_Dependence

        float susceptibility_scaling_rate;     // Susceptibility_Scaling_Rate, only for Susceptibility_Scale_Type = *_FUNCTION_OF_TIME
        float susceptibility_scaling_intercept;

        bool demographics_initial;
        int default_torus_size;
        int default_node_population;

        float lloffset; // half the size of a grid edge in degrees, set by SetFlags()

        // parameters for individual
        bool vital_dynamics;
        bool vital_disease_mortality;

        int infection_updates_per_tstep;
        bool interventions;

        // constant climate params
        float          airtemperature;
        float          landtemperature;
        float          rainfall;
        float          humidity;

        // scale params
        float          airtemperature_offset;
        float          landtemperature_offset;
        float          rainfall_scale_factor;
        float          humidity_scale_factor;

        // stochasticity params
        float          airtemperature_variance;
        float          landtemperature_variance;
        bool           rainfall_variance;
        float          humidity_variance;

        int number_basestrains;
        int number_substrains; // genetic variants

        // flag for heterogeneity in mixing (true) or uniform mixing (false)
        bool heterogeneous_intranode_transmission_enabled;

        float Sim_Duration;
        float Sim_Tstep;
        float starttime;
        float node_grid_size;
        int Run_Number;

        int branch_duration;
        int branch_end_state;
        int branch_start_state;

        int burnin_period;
        int serialization_test_cycles;

        jsonConfigurable::tDynamicStringSet listed_events;

        std::string ConfigName;
        std::string airmig_filename;
        std::string campaign_filename;
        std::string climate_airtemperature_filename;
        std::string climate_koppen_filename;
        std::string climate_landtemperature_filename;
        std::string climate_rainfall_filename;
        std::string climate_relativehumidity_filename;
        std::string loadbalance_filename;
        std::string localmig_filename;
        std::string regionmig_filename;
        std::string seamig_filename;
        std::string TerminationPredicate;

        VectorParameters*  vector_params;
        MalariaParameters* malaria_params;
        TBParameters*      tb_params;
        PolioParameters*   polio_params;
        TBHIVParameters*   tbhiv_params;

        ///////////////////////////////////////////
        SimulationConfig();
        virtual ~SimulationConfig();

        virtual QuickBuilder SimulationConfig::GetSchema() override;

        const Configuration* GetJsonConfigObj() const { return m_jsonConfig; }

    protected:

    private: // for serialization to work

        const Configuration* m_jsonConfig;

        void VectorInitConfig( const Configuration* inputJson );
        void VectorCheckConfig( const Configuration* inputJson );
        void VectorAddSchema( json::QuickBuilder& retJson );

        void MalariaInitConfig( const Configuration* inputJson );
        void MalariaCheckConfig( const Configuration* inputJson );
        void MalariaAddSchema( json::QuickBuilder& retJson );

        void TBInitConfig( const Configuration* inputJson );
        void TBCheckConfig( const Configuration* inputJson );
        void TBAddSchema( json::QuickBuilder& retJson );
        jsonConfigurable::tDynamicStringSet tb_drug_names_for_this_sim;

        void PolioInitConfig( const Configuration* inputJson );
        void PolioCheckConfig( const Configuration* inputJson );
        void PolioAddSchema( json::QuickBuilder& retJson );
        std::string tmpSubstrainInfectivityString[3];
        std::string tmpSiteRatesStrings[3];

        void TBHIVInitConfig( const Configuration* inputJson );
        void TBHIVCheckConfig( const Configuration* inputJson );
        void TBHIVAddSchema( json::QuickBuilder& retJson );

    };
#pragma warning( pop )
}
