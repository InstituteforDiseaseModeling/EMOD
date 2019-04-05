/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <set>
#include <list>
#include "BoostLibWrapper.h"
#include "Individual.h"
#include "IIndividualHumanSTI.h"

namespace Kernel
{
    class INodeSTI;
    class STIInterventionsContainer;

    class IndividualHumanSTIConfig : public IndividualHumanConfig
    {
        GET_SCHEMA_STATIC_WRAPPER( IndividualHumanSTIConfig )
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()  
        DECLARE_QUERY_INTERFACE()

    public:

    protected:
        friend class SimulationSTI;
        friend class IndividualHumanSTI;
        friend class Relationship;

        static float debutAgeYrsMale_inv_kappa;
        static float debutAgeYrsFemale_inv_kappa;
        static float debutAgeYrsMin;

        static float debutAgeYrsMale_lambda;
        static float debutAgeYrsFemale_lambda;

        static float sti_coinfection_acq_mult;
        static float sti_coinfection_trans_mult;

        static float min_days_between_adding_relationships;

        static float condom_transmission_blocking_probability;

        static std::vector<float> maleToFemaleRelativeInfectivityAges;
        static std::vector<float> maleToFemaleRelativeInfectivityMultipliers;

        static bool  enable_coital_dilution;

        static float coital_dilution_2_partners;
        static float coital_dilution_3_partners;
        static float coital_dilution_4_plus_partners;

        virtual bool Configure( const Configuration* config ) override;
    };

    class IndividualHumanSTI :  public IndividualHuman, 
                                public IIndividualHumanSTI
    {
    public:
        DECLARE_QUERY_INTERFACE()
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING();

        virtual ~IndividualHumanSTI(void);
        static   IndividualHumanSTI *CreateHuman( INodeContext *context, 
                                                  suids::suid _suid, 
                                                  float monte_carlo_weight = 1.0f, 
                                                  float initial_age = 0.0f, 
                                                  int gender = 0);
        virtual void InitializeHuman() override;
        virtual void Update(float currenttime, float dt) override;
        virtual void UpdateHistory( const IdmDateTime& rCurrentTime, float dt ) override;
        virtual void UpdatePausedRelationships( const IdmDateTime& rCurrentTime, float dt ) override;

        virtual void UpdateSTINetworkParams(const char *prop = nullptr, const char* new_value = nullptr) override;

        virtual suids::suid GetSuid() const override { return IndividualHuman::GetSuid(); }
        virtual bool IsInfected() const override { return IndividualHuman::IsInfected(); }
        virtual suids::suid GetNodeSuid() const override;
        virtual const IPKeyValueContainer& GetPropertiesConst() const override;

        virtual void Die( HumanStateChange ) override;

        // Infections and Susceptibility
        virtual void CreateSusceptibility( float imm_mod=1.0f, float risk_mod=1.0f ) override;
        virtual void ExposeToInfectivity(float dt, TransmissionGroupMembership_t transmissionGroupMembership) override;
        virtual void Expose(const IContagionPopulation* cp, float dt, TransmissionRoute::Enum transmission_route) override;

        virtual void UpdateGroupMembership() override;

        virtual void UpdateInfectiousness(float dt) override;
        virtual void UpdateInfectiousnessSTI(std::vector<act_prob_t> &act_prob_vec, unsigned int rel_id) override;

        virtual void AcquireNewInfection( const IStrainIdentity *infstrain = nullptr, int incubation_period_override = -1 ) override;

        virtual bool AvailableForRelationship(RelationshipType::Enum) const override;

        virtual void UpdateEligibility() override;
        virtual void ConsiderRelationships(float dt) override;
        virtual void AddRelationship( IRelationship * pNewRelationship ) override;
        virtual void RemoveRelationship( IRelationship * pNewRelationship ) override;
        virtual RelationshipSet_t& GetRelationships() override;
        virtual RelationshipSet_t& GetRelationshipsAtDeath() override;

        virtual bool IsBehavioralSuperSpreader() const override;
        virtual unsigned int GetExtrarelationalFlags() const override;
        virtual float GetCoInfectiveTransmissionFactor() const override;
        virtual float GetCoInfectiveAcquisitionFactor() const override;
        virtual void  SetStiCoInfectionState() override;
        virtual void  ClearStiCoInfectionState() override;
        virtual bool  HasSTICoInfection() const override;
        virtual bool IsCircumcised() const override;
        virtual void onEmigrating() override;
        virtual void onImmigrating() override;

        void disengageFromSociety();
        virtual ProbabilityNumber getProbabilityUsingCondomThisAct( const IRelationshipParameters* pRelParams ) const;
        virtual void UpdateNumCoitalActs( uint32_t numActs ) override;
        virtual uint32_t GetTotalCoitalActs() const override;

        virtual void ClearAssortivityIndexes() override;
        virtual int GetAssortivityIndex( RelationshipType::Enum type ) const override;
        virtual void SetAssortivityIndex( RelationshipType::Enum type, int index ) override;

        virtual void SetContextTo(INodeContext* context) override;

        virtual unsigned int GetOpenRelationshipSlot() const override;
        virtual NaturalNumber GetLast6MonthRels() const override;
        virtual NaturalNumber GetLast6MonthRels( RelationshipType::Enum ofType ) const override;
        virtual NaturalNumber GetLast12MonthRels( RelationshipType::Enum ofType ) const override;
        virtual NaturalNumber GetNumUniquePartners( int itp, int irel ) const override;
        virtual NaturalNumber GetLifetimeRelationshipCount() const override;
        virtual NaturalNumber GetLifetimeRelationshipCount( RelationshipType::Enum ofType ) const override;
        virtual NaturalNumber GetNumRelationshipsAtDeath() const override;
        virtual float GetDebutAge() const override;
        virtual void CheckForMigration(float currenttime, float dt) override;

        virtual std::string toString() const override;

        unsigned char GetProbExtraRelationalBitMask( Gender::Enum gender);
        float GetMaxNumRels(Gender::Enum gender, RelationshipType::Enum rel_type);
        virtual void NotifyPotentialExposure() override;

        static void InitializeStaticsSTI( const Configuration* config );

    protected:
        IndividualHumanSTI( suids::suid id = suids::nil_suid(), 
                            float monte_carlo_weight = 1.0f, 
                            float initial_age = 0.0f,
                            int gender = 0);
        virtual void InitializeConcurrency();

        virtual void UpdateAge( float dt ) override;
        virtual IInfection* createInfection(suids::suid _suid) override;
        virtual void setupInterventionsContainer() override;

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
        uint64_t relationshipSlots;
        float delay_between_adding_relationships_timer;
        bool potential_exposure_flag;
        STIInterventionsContainer* m_pSTIInterventionsContainer;
        std::map< int, TransmissionGroupMembership_t > transmissionGroupMembershipByRelationship;

    private:
        virtual void IndividualHumanSTI::SetConcurrencyParameters( const char *prop, const char* prop_value );

        RelationshipSet_t relationships_at_death ;
        unsigned int num_lifetime_relationships[RelationshipType::Enum::COUNT];
        std::list<std::pair<int, float>> last_6_month_relationships;
        std::list<std::pair<int, float>> last_12_month_relationships;
        std::map< unsigned int, suids::suid_data_t > slot2RelationshipDebugMap; // for debug only
        INodeSTI* p_sti_node;
        int m_AssortivityIndex[RelationshipType::COUNT];
        uint32_t m_TotalCoitalActs;

        // Relationship Name to RelationshipID
        std::map<std::string,uint32_t> relationship_properties;

        typedef std::map<suids::suid, float> PartnerIdToRelEndTimeMap_t;
        std::vector<std::vector<PartnerIdToRelEndTimeMap_t>> num_unique_partners; // vector(by time period) of vector(by relationship type) of maps of IndividualID to Relationship End Time

        DECLARE_SERIALIZABLE(IndividualHumanSTI);
    };
}
