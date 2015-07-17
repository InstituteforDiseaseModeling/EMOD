/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include <set>
#include <list>
#include "BoostLibWrapper.h"
#include "Individual.h"
#include "IIndividualHumanSTI.h"
#include "STINetworkParameters.h"
#include "HIVEnums.h"

namespace Kernel
{
    class IndividualHumanSTIConfig : public JsonConfigurable
    {
        GET_SCHEMA_STATIC_WRAPPER( IndividualHumanSTIConfig )
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()  
        DECLARE_QUERY_INTERFACE()

    public:
        friend class SimulationSTI;

    protected:

        static STINetworkParametersMap net_param_map;

        static float extra_relational_rate_ratio_male;
        static float extra_relational_rate_ratio_female;

        static float debutAgeYrsMale_inv_kappa;
        static float debutAgeYrsFemale_inv_kappa;
        static float debutAgeYrsMin;

        static float debutAgeYrsMale_lambda;
        static float debutAgeYrsFemale_lambda;

        static float sti_coinfection_mult;

        static float circumcision_reduced_acquire;
        
        static float min_days_between_adding_relationships;
    public:
        static float condom_transmission_blocking_probability;
        static float condom_usage_probability_in_marital_relationships_midyear;
        static float condom_usage_probability_in_marital_relationships_rate;
        static float condom_usage_probability_in_marital_relationships_early;
        static float condom_usage_probability_in_marital_relationships_late;
        static float condom_usage_probability_in_informal_relationships_midyear;
        static float condom_usage_probability_in_informal_relationships_rate;
        static float condom_usage_probability_in_informal_relationships_early;
        static float condom_usage_probability_in_informal_relationships_late;
        static float condom_usage_probability_in_transitory_relationships_midyear;
        static float condom_usage_probability_in_transitory_relationships_rate;
        static float condom_usage_probability_in_transitory_relationships_early;
        static float condom_usage_probability_in_transitory_relationships_late;

        static std::vector<float> maleToFemaleRelativeInfectivityAges;
        static std::vector<float> maleToFemaleRelativeInfectivityMultipliers;

        friend class IndividualHumanSTI;
        friend class Relationship;
        virtual bool Configure( const Configuration* config );
    };

    class IndividualHumanSTI :  public IndividualHuman, 
                                public IIndividualHumanSTI,
                                public IndividualHumanSTIConfig
    {
    public:
        DECLARE_QUERY_INTERFACE()
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING();

        virtual ~IndividualHumanSTI(void);
        static   IndividualHumanSTI *CreateHuman( INodeContext *context, 
                                                  suids::suid _suid, 
                                                  float monte_carlo_weight = 1.0f, 
                                                  float initial_age = 0.0f, 
                                                  int gender = 0, 
                                                  float initial_poverty = 0.5f );
        virtual void InitializeHuman();
        virtual bool Configure( const Configuration* config );
        virtual void Update(float currenttime, float dt);

        virtual void UpdateSTINetworkParams(const char *prop = NULL, const char* new_value = NULL);

        virtual suids::suid GetSuid() const { return IndividualHuman::GetSuid(); }
        virtual bool IsInfected() const { return IndividualHuman::IsInfected(); }

        virtual void Die( HumanStateChange );

        // Infections and Susceptibility
        virtual void CreateSusceptibility( float imm_mod=1.0f, float risk_mod=1.0f );
        virtual void ExposeToInfectivity(float dt, const TransmissionGroupMembership_t* transmissionGroupMembership);
        virtual void Expose(const IContagionPopulation* cp, float dt, TransmissionRoute::Enum transmission_route);

        virtual void UpdateInfectiousness(float dt);
        virtual void UpdateInfectiousnessSTI(std::vector<act_prob_t> &act_prob_vec, unsigned int rel_id);
        
        virtual void AcquireNewInfection(StrainIdentity *infstrain = NULL, int incubation_period_override = -1);

        virtual bool AvailableForRelationship(RelationshipType::Enum) const;

        virtual void UpdateEligibility();
        virtual void ConsiderRelationships(float dt);
        virtual void AddRelationship( IRelationship * pNewRelationship );
        virtual void RemoveRelationship( IRelationship * pNewRelationship );
        virtual void VacateRelationship( IRelationship* relationship );
        virtual void RejoinRelationship( IRelationship* relationship );
        virtual RelationshipSet_t& GetRelationships();
        virtual RelationshipSet_t& GetRelationshipsAtDeath();

        virtual bool IsBehavioralSuperSpreader() const;
        virtual unsigned int GetExtrarelationalFlags() const;
        virtual float GetCoInfectiveFactor() const;
        virtual void  SetStiCoInfectionState();
        virtual void  ClearStiCoInfectionState();
        virtual bool  HasSTICoInfection() const;
        virtual bool IsCircumcised() const;
        virtual void onEmigrating();

        void disengageFromSociety();
        virtual ProbabilityNumber getProbabilityUsingCondomThisAct( RelationshipType::Enum ) const;

        virtual void onImmigratingToNode();
        virtual void SetContextTo(INodeContext* context);

        virtual unsigned int GetOpenRelationshipSlot() const;
        virtual NaturalNumber GetLast6MonthRels() const;
        virtual NaturalNumber GetLifetimeRelationshipCount() const;
        virtual NaturalNumber GetNumRelationshipsAtDeath() const;
        virtual float GetDebutAge() const;
        virtual void CheckForMigration(float currenttime, float dt);

        virtual std::string toString() const;

        unsigned char GetProbExtraRelationalBitMask( Gender::Enum gender);
        float GetMaxNumRels(Gender::Enum gender, RelationshipType::Enum rel_type);
        virtual void NotifyPotentialExposure();


    protected:
        IndividualHumanSTI( suids::suid id = suids::nil_suid(), 
                            float monte_carlo_weight = 1.0f, 
                            float initial_age = 0.0f,
                            int gender = 0,
                            float initial_poverty = 0.5f);

        virtual Infection* createInfection(suids::suid _suid);
        virtual void setupInterventionsContainer();
        virtual void ReportInfectionState();

        // Local version of individual-property-dependent parameters from STINetworkParameters
        STINetworkParameters net_params;

        RelationshipSet_t relationships;
        unsigned int max_relationships[RelationshipType::Enum::COUNT];
        unsigned int queued_relationships[RelationshipType::Enum::COUNT];
        unsigned int active_relationships[RelationshipType::Enum::COUNT];
        unsigned int remote_relationships[RelationshipType::Enum::COUNT];

        unsigned char promiscuity_flags;
        float sexual_debut_age;
        float co_infective_factor;
        bool  has_other_sti_co_infection;
        bool  transmissionInterventionsDisabled;
        unsigned int relationshipSlots;
        float delay_between_adding_relationships_timer;
        bool potential_exposure_flag;

    private:
        virtual void SetSTINetworkParams( const STINetworkParameters& rNewNetParams );

        RelationshipSet_t relationships_at_death ;
        unsigned int num_lifetime_relationships;
        std::list<int> last_6_month_relationships;
        std::map< int, int > slot2RelationshipDebugMap; // for debug only
        float age_for_transitory_stats;
        float age_for_informal_stats;
        float age_for_marital_stats;
        int transitory_eligibility;
        int informal_eligibility;
        int marital_elibigility;

#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
        friend class boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive & ar, IndividualHumanSTI& human, const unsigned int  file_version );
#endif
    };
}
