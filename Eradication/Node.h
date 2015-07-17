/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include <stdafx.h>


#ifdef __GNUC__
#include <ext/hash_map>
namespace std
{
     using namespace __gnu_cxx;
}
#else
#include <hash_map>
#endif

#include <map>
#include <vector>

#include "IdmApi.h"
#include "BoostLibWrapper.h"
#include "CajunIncludes.h"
#include "Configure.h"
#include "Climate.h"
#include "ClimateKoppen.h"
#include "ClimateByData.h"
#include "ClimateConstant.h"
#include "Common.h"
#include "Contexts.h"
#include "Environment.h"
//#include "NodeEventContext.h"
#include "Migration.h"
#include "NodeDemographics.h"
#include "ITransmissionGroups.h"
#include "suids.hpp"
#include "InterventionFactory.h"
#include "IInfectable.h"

class RANDOMBASE;

class Report;
class ReportVector;
class DemographicsReport;
class BaseChannelReport;

namespace Kernel
{
    struct INodeEventContext;
    //typedef 
    class  NodeEventContextHost;
    class  Simulation;

    class IDMAPI Node : public INodeContext, public JsonConfigurable
    {
        GET_SCHEMA_STATIC_WRAPPER(Node)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

        // This needs to go once we COM-ify and/or make accessors
        // BTW, the scope operators are needed by MSVC, not GCC (or is it the other way around?)
        friend class ::Report;
        friend class ::ReportVector;
        friend class ::DemographicsReport;

    public:

        Node(ISimulationContext *_parent_sim, suids::suid _suid);
        Node(); // constructor for serialization use
        static Node *CreateNode(ISimulationContext *_parent_sim, suids::suid node_suid); 
        static void VerifyPropertyDefinedInDemographics( const std::string& rKey, const std::string& rVal );
        static std::vector<std::string> GetIndividualPropertyValuesList( const std::string& rKey );
        virtual ~Node();

        virtual void Update(float dt);

        // INodeContext
        virtual suids::suid   GetSuid() const;
        virtual suids::suid   GetNextInfectionSuid();
        virtual ::RANDOMBASE* GetRng();
        virtual const INodeContext::tDistrib& GetIndividualPropertyDistributions() const;

        virtual const MigrationInfo*    GetMigrationInfo() const;
        virtual const NodeDemographics* GetDemographics()  const;
        virtual const NodeDemographicsDistribution* GetDemographicsDistribution(std::string key) const;

        virtual INodeEventContext* GetEventContext();

        // Migration
        void SetupMigration(MigrationInfoFactory * migration_factory);
        virtual IndividualHuman* processImmigratingIndividual( IndividualHuman *immigrant );

        // Initialization
        virtual void PopulateFromDemographics();
        virtual void SetContextTo(ISimulationContext* context);
        virtual void SetMonteCarloParameters(float indsamplerate =.05, int nummininf = 0);
        virtual void SetParameters(NodeDemographicsFactory *demographics_factory, ClimateFactory *climate_factory);

        // Campaign event-related
        bool IsInPolygon(float* vertex_coords, int numcoords); // might want to create a real polygon object at some point
        bool IsInPolygon( const json::Array &poly );
        bool IsInExternalIdSet( const tNodeIdList& nodelist );
        bool GetUrban() const;

        // Reporting to higher levels (intermediate form)
        // Possible TODO: refactor into common interfaces if there is demand
        virtual IdmDateTime GetTime()                  const;
        virtual float GetInfected()              const;
        virtual float GetStatPop()               const;
        virtual float GetBirths()                const;
        virtual float GetCampaignCost()          const;
        virtual float GetInfectivity()           const;
        virtual float GetInfectionRate()         const;
        virtual float GetSusceptDynamicScaling() const;
        virtual const Climate* GetLocalWeather() const;
        virtual long int GetPossibleMothers()    const;

        virtual void RegisterNewInfectionObserver(void* id, INodeContext::callback_t observer);
        virtual void UnregisterNewInfectionObserver(void* id);

        // This method will ONLY be used for spatial reporting by input node ID, don't use it elsewhere!
        virtual int GetExternalID() const;

        // Heterogeneous intra-node transmission
        virtual void ExposeIndividual(IInfectable* candidate, const TransmissionGroupMembership_t* individual, float dt);
        virtual void DepositFromIndividual(StrainIdentity* strain_IDs, float contagion_quantity, const TransmissionGroupMembership_t* individual);
        virtual void GetGroupMembershipForIndividual(RouteList_t& route, tProperties* properties, TransmissionGroupMembership_t* membershipOut);
        virtual void UpdateTransmissionGroupPopulation(const TransmissionGroupMembership_t* membership, float size_changes,float mc_weight);
        virtual void SetupIntranodeTransmission();

        virtual act_prob_vec_t DiscreteGetTotalContagion(const TransmissionGroupMembership_t* membership);

        virtual void ValidateIntranodeTransmissionConfiguration();
        virtual bool IsValidTransmissionRoute( string& transmissionRoute );

        virtual float GetTotalContagion(const TransmissionGroupMembership_t* membership);
        virtual RouteList_t& GetTransmissionRoutes();

        // These methods are not const because they will extract the value from the demographics
        // if it has not been done yet.
        float GetLatitudeDegrees();
        float GetLongitudeDegrees();

        static void TestOnly_ClearProperties();
        static void TestOnly_AddPropertyKeyValue( const char* key, const char* value );
    private:
#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        static INodeContext::tDistrib base_distribs;

        // Do not access these directly but use the access methods above.
        float _latitude;
        float _longitude;

    protected:
        INodeContext::tDistrib distribs;

        // moved from SimulationConfig
        IndSamplingType::Enum                        ind_sampling_type;                         // Individual_Sampling_Type
        PopulationDensityInfectivityCorrection::Enum population_density_infectivity_correction; // Population_Density_Infectivity_Correction

        // Node properties
        suids::suid suid;
        bool  urban;
        float birthrate;
        float Above_Poverty; // fraction of the population above poverty

        // ----------------------------------------------------------------------------------------
        // --- DMB 9-16-2014 Through comparison, it was determined that using a vector (and moving
        // --- the last human in the list to the location being removed) was faster and used less
        // --- memory than using a std::list (even with a variety of allocators).
        // --- Please the IDM Wiki for more details.
        // --- http://ivlabsdvapp50:8090/pages/viewpage.action?pageId=30015603
        // ----------------------------------------------------------------------------------------
        std::vector<IndividualHuman*> individualHumans ;

        float Ind_Sample_Rate;   // adapted sampling parameter

        // Heterogeneous intra-node transmission
        ITransmissionGroups *transmissionGroups;

        float susceptibility_dynamic_scaling;
        
        // Climate and demographics
        Climate *localWeather;
        MigrationInfo *migration_info;
        NodeDemographics demographics;
        std::map<std::string, NodeDemographicsDistribution*> demographic_distributions;
        uint32_t externalId; // DON'T USE THIS EXCEPT FOR INPUT/OUTPUT PURPOSES!

        // Event handling
        friend class NodeEventContextHost;
        friend class Simulation; // so migration can call configureAndAdd?????
        NodeEventContextHost *event_context_host;

        //  Counters (some for reporting, others also for internal calculations)
        float statPop;
        float Infected;
        float Births;
        float Disease_Deaths;
        float new_infections;
        float new_reportedinfections;
        float Cumulative_Infections;
        float Cumulative_Reported_Infections;
        float Campaign_Cost;
        long int Possible_Mothers;

        float infectionrate; // TODO: this looks like its only a reporting counter now and possibly not accurately updated in all cases
        float mInfectivity;

        ISimulationContext *parent;     // Access back to simulation methods

        bool demographics_birth;
        bool demographics_gender;
        bool demographics_other;

        float max_sampling_cell_pop;
        float sample_rate_birth;
        float sample_rate_0_18mo;
        float sample_rate_18mo_4yr;
        float sample_rate_5_9;
        float sample_rate_10_14;
        float sample_rate_15_19;
        float sample_rate_20_plus;
        float population_density_c50;
        float population_scaling_factor;
        bool maternal_transmission;
        bool vital_birth;
        float x_birth;

        // Cached values to be used when initializing new individuals
        DistributionFunction::Enum immunity_dist_type ;
        float immunity_dist1 ;
        float immunity_dist2 ;
        DistributionFunction::Enum risk_dist_type ;
        float risk_dist1 ;
        float risk_dist2 ;
        DistributionFunction::Enum migration_dist_type ;
        float migration_dist1 ;
        float migration_dist2 ;

        map<void*, INodeContext::callback_t> new_infection_observers;
        AnimalReservoir::Enum      animal_reservoir_type;
        InfectivityScaling::Enum   infectivity_scaling;                              // Infectivity_Scale_Type
        float                      zoonosis_rate;

        RouteList_t routes;

        void Initialize();
        virtual void setupEventContextHost();
        virtual bool Configure( const Configuration* config );
        void ExtractDataFromDemographics();

        // Updates
        virtual void updateInfectivity(float dt = 0.0f);
        virtual void updatePopulationStatistics(float=1.0);     // called by updateinfectivity to gather population statistics
        virtual void accumulateIndividualPopulationStatistics(float dt, IndividualHuman* individual);
        virtual float getDensityContactScaling(); // calculate correction to infectivity due to lower population density
        virtual float getClimateInfectivityCorrection()  const;
        virtual float getSeasonalInfectivityCorrection();
        void  updateVitalDynamics(float dt = 1.0f);             // handles births and non-disease mortality

        // Population Initialization
        virtual void populateNewIndividualsFromDemographics(int count_new_individuals = 100);
        virtual void populateNewIndividualsByBirth(int count_new_individuals = 100);
        virtual void populateNewIndividualFromPregnancy(IndividualHuman* temp_mother);
        void  conditionallyInitializePregnancy(IndividualHuman*);
        float getPrevalenceInPossibleMothers();

        virtual IndividualHuman *createHuman( suids::suid id, float MCweight, float init_age, int gender, float init_poverty);
        IndividualHuman* configureAndAddNewIndividual(float=1.0, float=(20*DAYSPERYEAR), float= 0, float = 0.5);
        virtual IndividualHuman* addNewIndividual(
            float monte_carlo_weight = 1.0,
            float initial_age = 0,
            int gender = 0,
            int initial_infections = 0,
            float immunity_parameter = 1.0,
            float risk_parameter = 1.0,
            float migration_heterogeneity = 1.0,
            float poverty_parameter = 0);

        virtual void RemoveHuman( int index );

        virtual IndividualHuman* addNewIndividualFromSerialization();

        double calculateInitialAge( double temp_age );
        float  adjustSamplingRateByAge( float sampling_rate, double age );
        virtual void EraseAndDeleteDemographicsDistribution(std::string key); // for removing distributions used only in initialization

        // Reporting
        virtual void resetNodeStateCounters(void);
        virtual void updateNodeStateCounters(IndividualHuman *ih);
        virtual void finalizeNodeStateCounters(void);
        virtual void reportNewInfection(IndividualHuman *ih);
        virtual void reportDetectedInfection(IndividualHuman *ih);

        // Migration
        virtual void processEmigratingIndividual(IndividualHuman *i); // not a public method since decision to emigrate is an internal one;
        virtual void postIndividualMigration(IndividualHuman* ind);
        void resolveEmigration(IndividualHuman *tempind);

        // Fix up child object pointers after deserializing
        virtual INodeContext *getContextPointer();
        virtual void propagateContextToDependents();

        const SimulationConfig* params() const;
        void checkIpKeyInWhitelist(const std::string& key, size_t numValues );
        void convertTransitions( const NodeDemographics &trans, json::Object& tx_camp, const std::string& key );
        std::string getAgeBinPropertyNameFromIndex( const NodeDemographics& demo, unsigned int idx );
        virtual void checkValidIPValue( const std::string& key, const std::string& to_value );

        //Verify that the user entered in set of property key/value pairs which are included in the demographics file
        virtual void VerifyPropertyDefined( const std::string& rKey, const std::string& rVal ) const;

        bool whitelist_enabled;
        std::set< std::string > ipkeys_whitelist;
        static const char* _age_bins_key;
        static const char* transitions_dot_json_filename;
#pragma warning( pop )

#if USE_JSON_SERIALIZATION
     public:

         // IJsonSerializable Interfaces
         virtual void JSerialize( IJsonObjectAdapter* root, JSerializer* helper ) const;
         virtual void JDeserialize( IJsonObjectAdapter* root, JSerializer* helper );
#endif

    private:

#if USE_BOOST_SERIALIZATION
        friend class boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive &ar, Node&, const unsigned int v);
#endif
    };
}
