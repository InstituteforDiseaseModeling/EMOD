/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <map>
#include <vector>

#include "IdmApi.h"
#include "BoostLibWrapper.h"
#include "Configure.h"
#include "Climate.h"
#include "Common.h"
#include "Contexts.h"
#include "Environment.h"

#include "NodeDemographics.h"
#include "ITransmissionGroups.h"
#include "suids.hpp"
#include "IInfectable.h"
#include "MathFunctions.h"
#include "Serialization.h"

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
    struct IMigrationInfoFactory;

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
        static Node *CreateNode(ISimulationContext *_parent_sim, suids::suid node_suid); 

        Node(ISimulationContext *_parent_sim, suids::suid _suid);
        Node(); // constructor for serialization use
        virtual ~Node();


        // INodeContext
        virtual void Update(float dt) override;
        virtual ISimulationContext* GetParent() override;
        virtual suids::suid   GetSuid() const override;
        virtual suids::suid   GetNextInfectionSuid() override;
        virtual ::RANDOMBASE* GetRng() override;
        virtual void AddEventsFromOtherNodes( const std::vector<std::string>& rEventNameList ) override;


        virtual IMigrationInfo*   GetMigrationInfo() override;
        virtual const NodeDemographics* GetDemographics()  const override;
        virtual const NodeDemographicsDistribution* GetDemographicsDistribution(std::string key) const override;
        virtual std::vector<bool> GetMigrationTypeEnabledFromDemographics() const override;

        virtual INodeEventContext* GetEventContext() override;

        // Migration
        virtual void SetupMigration( IMigrationInfoFactory * migration_factory, 
                                     MigrationStructure::Enum ms,
                                     const boost::bimap<ExternalNodeId_t, suids::suid>& rNodeIdSuidMap ) override;
        virtual IIndividualHuman* processImmigratingIndividual( IIndividualHuman* ) override;

        // Initialization
        virtual void SetContextTo(ISimulationContext* context) override;
        virtual void SetMonteCarloParameters(float indsamplerate =.05, int nummininf = 0) override;
        virtual void SetParameters(NodeDemographicsFactory *demographics_factory, ClimateFactory *climate_factory) override;
        virtual void PopulateFromDemographics() override;

        // Campaign event-related
        bool IsInPolygon(float* vertex_coords, int numcoords); // might want to create a real polygon object at some point
        bool IsInPolygon( const json::Array &poly );
        bool IsInExternalIdSet( const tNodeIdList& nodelist );
        bool GetUrban() const;

        // Reporting to higher levels (intermediate form)
        // Possible TODO: refactor into common interfaces if there is demand
        virtual IdmDateTime GetTime()            const override;
        virtual float GetInfected()              const override;
        virtual float GetStatPop()               const override;
        virtual float GetBirths()                const override;
        virtual float GetCampaignCost()          const override;
        virtual float GetInfectivity()           const override;
        virtual float GetInfectionRate()         const override;
        virtual float GetSusceptDynamicScaling() const override;
        virtual const Climate* GetLocalWeather() const override;
        virtual long int GetPossibleMothers()    const override;

        virtual float GetMeanAgeInfection()      const override;
        virtual float GetBasePopulationScaleFactor() const override;

        // This method will ONLY be used for spatial reporting by input node ID, don't use it elsewhere!
        virtual ExternalNodeId_t GetExternalID() const;

        // Heterogeneous intra-node transmission
        virtual void ExposeIndividual(IInfectable* candidate, const TransmissionGroupMembership_t* individual, float dt) override;
        virtual void DepositFromIndividual(StrainIdentity* strain_IDs, float contagion_quantity, const TransmissionGroupMembership_t* individual) override;
        virtual void GetGroupMembershipForIndividual(const RouteList_t& route, tProperties* properties, TransmissionGroupMembership_t* membershipOut) override;
        virtual void UpdateTransmissionGroupPopulation(const TransmissionGroupMembership_t* membership, float size_changes,float mc_weight) override;
        virtual void SetupIntranodeTransmission();
        virtual ITransmissionGroups* CreateTransmissionGroups();
        virtual void AddDefaultRoute( RouteToContagionDecayMap_t& rDecayMap );
        virtual void AddRoute( RouteToContagionDecayMap_t& rDecayMap, const std::string& rRouteName );
        virtual void BuildTransmissionRoutes( RouteToContagionDecayMap_t& rDecayMap );
        virtual bool IsValidTransmissionRoute( string& transmissionRoute );

        virtual act_prob_vec_t DiscreteGetTotalContagion(const TransmissionGroupMembership_t* membership) override;

        virtual void ValidateIntranodeTransmissionConfiguration();

        virtual float GetTotalContagion(const TransmissionGroupMembership_t* membership) override;
        virtual const RouteList_t& GetTransmissionRoutes() const override;
        //Methods for implementing time dependence in various quantities; infectivity, birth rate, migration rate
        virtual float getSinusoidalCorrection(float sinusoidal_amplitude, float sinusoidal_phase) const override;
        virtual float getBoxcarCorrection(float boxcar_amplitude, float boxcar_start_time, float boxcar_end_time) const override;

        // These methods are not const because they will extract the value from the demographics
        // if it has not been done yet.
        virtual float GetLatitudeDegrees()override;
        virtual float GetLongitudeDegrees() override;

        virtual bool IsEveryoneHome() const override;
        virtual void SetWaitingForFamilyTrip( suids::suid migrationDestination,
                                              MigrationType::Enum migrationType,
                                              float timeUntilTrip,
                                              float timeAtDestination,
                                              bool isDestinationNewHome ) override;

        virtual ProbabilityNumber GetProbMaternalTransmission() const;

        virtual void ManageFamilyTrip( float currentTime, float dt );

    private:
#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        SerializationFlags serializationMask;

        // Do not access these directly but use the access methods above.
        float _latitude;
        float _longitude;

    protected:
        // moved from SimulationConfig
        IndSamplingType::Enum                        ind_sampling_type;                         // Individual_Sampling_Type
        PopulationDensityInfectivityCorrection::Enum population_density_infectivity_correction; // Population_Density_Infectivity_Correction
        DistributionType::Enum                       age_initialization_distribution_type;      // Age_Initialization_Distribution_Type
        PopulationScaling::Enum                      population_scaling;                        // POPULATION_SCALING

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
        std::vector<IIndividualHuman*> individualHumans;
        std::map<int,suids::suid> home_individual_ids; // people who call this node home

        bool                family_waiting_to_migrate;
        suids::suid         family_migration_destination;
        MigrationType::Enum family_migration_type;
        float               family_time_until_trip;
        float               family_time_at_destination;
        bool                family_is_destination_new_home;

        float Ind_Sample_Rate;   // adapted sampling parameter

        // Heterogeneous intra-node transmission
        ITransmissionGroups *transmissionGroups;

        float susceptibility_dynamic_scaling;
        
        // Climate and demographics
        Climate *localWeather;
        IMigrationInfo *migration_info;
        NodeDemographics demographics;
        std::map<std::string, NodeDemographicsDistribution*> demographic_distributions;
        ExternalNodeId_t externalId; // DON'T USE THIS EXCEPT FOR INPUT/OUTPUT PURPOSES!

        // Event handling
        friend class NodeEventContextHost;
        friend class Simulation; // so migration can call configureAndAdd?????
        NodeEventContextHost *event_context_host;
        std::vector<std::string> events_from_other_nodes ;

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

        float mean_age_infection;      // (years)
        float newInfectedPeopleAgeProduct;
        static const int infection_averaging_window = 1;   // = 30 time steps
        std::list<float> infected_people_prior; // [infection_averaging_window];
        std::list<float> infected_age_people_prior; // [infection_averaging_window];

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
        float sample_rate_immune;
        float immune_threshold_for_downsampling;
        float prob_maternal_transmission;
        float population_density_c50;
        float population_scaling_factor;
        bool maternal_transmission;
        bool vital_birth;
        VitalBirthDependence::Enum                           vital_birth_dependence;                           // Vital_Birth_Dependence
        VitalBirthTimeDependence::Enum                       vital_birth_time_dependence;                      //Time dependence in Birth Rate
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

        AnimalReservoir::Enum      animal_reservoir_type;
        InfectivityScaling::Enum                             infectivity_scaling;                              // Infectivity_Scale_Type
        float                      zoonosis_rate;

        RouteList_t routes;

        /* clorton virtual */ void Initialize() /* clorton override */;
        virtual void setupEventContextHost();
        virtual bool Configure( const Configuration* config ) override;
        void ExtractDataFromDemographics();
        virtual void LoadImmunityDemographicsDistribution();

        // Updates
        virtual void updateInfectivity(float dt = 0.0f);
        virtual void updatePopulationStatistics(float=1.0);     // called by updateinfectivity to gather population statistics
        virtual void accumulateIndividualPopulationStatistics(float dt, IIndividualHuman* individual);
        virtual float getDensityContactScaling(); // calculate correction to infectivity due to lower population density
        virtual float getClimateInfectivityCorrection()  const;
        virtual float getSeasonalInfectivityCorrection();
 
        void  updateVitalDynamics(float dt = 1.0f);             // handles births and non-disease mortality

        // Population Initialization
        virtual void populateNewIndividualsFromDemographics(int count_new_individuals = 100);
        virtual void populateNewIndividualsByBirth(int count_new_individuals = 100);
        virtual void populateNewIndividualFromPregnancy(IIndividualHuman* temp_mother);
        void  conditionallyInitializePregnancy(IIndividualHuman*);
        float getPrevalenceInPossibleMothers();
        virtual float drawInitialImmunity(float ind_init_age);

        virtual IIndividualHuman *createHuman( suids::suid id, float MCweight, float init_age, int gender, float init_poverty);
        IIndividualHuman* configureAndAddNewIndividual(float=1.0, float=(20*DAYSPERYEAR), float= 0, float = 0.5);
        virtual IIndividualHuman* addNewIndividual(
            float monte_carlo_weight = 1.0,
            float initial_age = 0,
            int gender = 0,
            int initial_infections = 0,
            float immunity_parameter = 1.0,
            float risk_parameter = 1.0,
            float migration_heterogeneity = 1.0,
            float poverty_parameter = 0);

        virtual void RemoveHuman( int index );

        virtual IIndividualHuman* addNewIndividualFromSerialization();

        double calculateInitialAge( double temp_age );
        Fraction adjustSamplingRateByImmuneState( Fraction sampling_rate, bool is_immune ) const;
        Fraction adjustSamplingRateByAge( Fraction sampling_rate, double age ) const;
        virtual void EraseAndDeleteDemographicsDistribution(std::string key); // for removing distributions used only in initialization

        // Reporting
        virtual void resetNodeStateCounters(void);
        virtual void updateNodeStateCounters(IIndividualHuman *ih);
        virtual void finalizeNodeStateCounters(void);
        virtual void reportNewInfection(IIndividualHuman *ih);
        virtual void reportDetectedInfection(IIndividualHuman *ih);

        // Migration
        virtual void processEmigratingIndividual(IIndividualHuman* i); // not a public method since decision to emigrate is an internal one;
        virtual void postIndividualMigration(IIndividualHuman* ind);
        void resolveEmigration(IIndividualHuman *tempind);

        // Fix up child object pointers after deserializing
        virtual INodeContext *getContextPointer();
        virtual void propagateContextToDependents();


        const SimulationConfig* params() const;
        float infectivity_sinusoidal_forcing_amplitude; // Only for Infectivity_Scale_Type = SINUSOIDAL_FUNCTION_OF_TIME
        float infectivity_sinusoidal_forcing_phase;     // Only for Infectivity_Scale_Type = SINUSOIDAL_FUNCTION_OF_TIME
        float infectivity_boxcar_forcing_amplitude;     // Only for Infectivity_Scale_Type = ANNUAL_BOXCAR_FUNCTION
        float infectivity_boxcar_start_time;            // Only for Infectivity_Scale_Type = ANNUAL_BOXCAR_FUNCTION
        float infectivity_boxcar_end_time;              // Only for Infectivity_Scale_Type = ANNUAL_BOXCAR_FUNCTION

        float birth_rate_sinusoidal_forcing_amplitude;  // Only for Birth_Rate_Time_Dependence = SINUSOIDAL_FUNCTION_OF_TIME
        float birth_rate_sinusoidal_forcing_phase;      // Only for Birth_Rate_Time_Dependence = SINUSOIDAL_FUNCTION_OF_TIME
        float birth_rate_boxcar_forcing_amplitude;      // Only for Birth_Rate_Time_Dependence = ANNUAL_BOXCAR_FUNCTION
        float birth_rate_boxcar_start_time;             // Only for Birth_Rate_Time_Dependence = ANNUAL_BOXCAR_FUNCTION
        float birth_rate_boxcar_end_time;               // Only for Birth_Rate_Time_Dependence = ANNUAL_BOXCAR_FUNCTION

        DECLARE_SERIALIZABLE(Node);

#pragma warning( pop )
    };
}
