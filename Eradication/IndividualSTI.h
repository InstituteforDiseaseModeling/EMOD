/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <set>
#include <list>
#include "BoostLibWrapper.h"
#include "Individual.h"
#include "IIndividualHumanSTI.h"
#include "STINetworkParameters.h"

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

        static float min_days_between_adding_relationships;
    public:
        static float condom_transmission_blocking_probability;

        static std::vector<float> maleToFemaleRelativeInfectivityAges;
        static std::vector<float> maleToFemaleRelativeInfectivityMultipliers;

        friend class IndividualHumanSTI;
        friend class Relationship;
        virtual bool Configure( const Configuration* config ) override;
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
        virtual void InitializeHuman() override;
        virtual bool Configure( const Configuration* config ) override;
        virtual void Update(float currenttime, float dt) override;

        virtual void UpdateSTINetworkParams(const char *prop = nullptr, const char* new_value = nullptr) override;

        virtual suids::suid GetSuid() const override { return IndividualHuman::GetSuid(); }
        virtual bool IsInfected() const override { return IndividualHuman::IsInfected(); }
        virtual suids::suid GetNodeSuid() const override;

        virtual void Die( HumanStateChange ) override;

        // Infections and Susceptibility
        virtual void CreateSusceptibility( float imm_mod=1.0f, float risk_mod=1.0f ) override;
        virtual void ExposeToInfectivity(float dt, const TransmissionGroupMembership_t* transmissionGroupMembership) override;
        virtual void Expose(const IContagionPopulation* cp, float dt, TransmissionRoute::Enum transmission_route) override;

        virtual void UpdateInfectiousness(float dt) override;
        virtual void UpdateInfectiousnessSTI(std::vector<act_prob_t> &act_prob_vec, unsigned int rel_id) override;
        
        virtual void AcquireNewInfection(StrainIdentity *infstrain = nullptr, int incubation_period_override = -1) override;

        virtual bool AvailableForRelationship(RelationshipType::Enum) const override;

        virtual void UpdateEligibility() override;
        virtual void ConsiderRelationships(float dt) override;
        virtual void AddRelationship( IRelationship * pNewRelationship ) override;
        virtual void RemoveRelationship( IRelationship * pNewRelationship ) override;
        virtual RelationshipSet_t& GetRelationships() override;
        virtual RelationshipSet_t& GetRelationshipsAtDeath() override;

        virtual bool IsBehavioralSuperSpreader() const override;
        virtual unsigned int GetExtrarelationalFlags() const override;
        virtual float GetCoInfectiveFactor() const override;
        virtual void  SetStiCoInfectionState() override;
        virtual void  ClearStiCoInfectionState() override;
        virtual bool  HasSTICoInfection() const override;
        virtual bool IsCircumcised() const override;
        virtual void onEmigrating();
        virtual void onImmigrating();

        void disengageFromSociety();
        virtual ProbabilityNumber getProbabilityUsingCondomThisAct( const IRelationshipParameters* pRelParams ) const;

        virtual void SetContextTo(INodeContext* context) override;

        virtual unsigned int GetOpenRelationshipSlot() const override;
        virtual NaturalNumber GetLast6MonthRels() const override;
        virtual NaturalNumber GetLifetimeRelationshipCount() const override;
        virtual NaturalNumber GetNumRelationshipsAtDeath() const override;
        virtual float GetDebutAge() const override;
        virtual void CheckForMigration(float currenttime, float dt) override;

        virtual std::string toString() const override;

        unsigned char GetProbExtraRelationalBitMask( Gender::Enum gender);
        float GetMaxNumRels(Gender::Enum gender, RelationshipType::Enum rel_type);
        virtual void NotifyPotentialExposure() override;


    protected:
        IndividualHumanSTI( suids::suid id = suids::nil_suid(), 
                            float monte_carlo_weight = 1.0f, 
                            float initial_age = 0.0f,
                            int gender = 0,
                            float initial_poverty = 0.5f);

        virtual IInfection* createInfection(suids::suid _suid) override;
        virtual void setupInterventionsContainer() override;
        virtual void ReportInfectionState() override;

        // Local version of individual-property-dependent parameters from STINetworkParameters
        STINetworkParameters net_params;

        RelationshipSet_t relationships;
        unsigned int max_relationships[RelationshipType::Enum::COUNT];
        unsigned int queued_relationships[RelationshipType::Enum::COUNT];
        unsigned int active_relationships[RelationshipType::Enum::COUNT];

        bool migrating_because_of_partner;
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

        DECLARE_SERIALIZABLE(IndividualHumanSTI);
    };
}
