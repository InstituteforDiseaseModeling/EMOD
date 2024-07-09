
#pragma once

#include "Types.h"
#include "ExternalNodeId.h"
#include "IRelationship.h"
#include "IRelationshipParameters.h"
#include "IRelationshipManager.h"
#include "IIndividualHumanSTI.h"
#include "Sigmoid.h"

namespace Kernel 
{
    class Relationship : public IRelationship
    {
        public:
            friend class IndividualHumanSTI;
            friend class RelationshipFactory;
            DECLARE_QUERY_INTERFACE()
            IMPLEMENT_DEFAULT_REFERENCE_COUNTING()

        public:
            virtual ~Relationship();

            // public non-const
            virtual IRelationship* Pause( IIndividualHumanSTI* departee, const suids::suid& rMigrationDestination ) override;
            virtual void Terminate( RelationshipTerminationReason::Enum terminationReason ) override;
            virtual void Migrate( const suids::suid& rMigrationDestination ) override;
            virtual void Resume( ISociety* pSociety, IIndividualHumanSTI* returnee ) override;
            virtual void UpdatePaused() override;
            virtual bool Update( float dt ) override;
            virtual void Consummate( RANDOMBASE* pRNG, float dt, IRelationshipManager * relMan ) override;

            // public const
            virtual bool IsOutsidePFA() const override;
            virtual RelationshipState::Enum GetState() const override;
            virtual RelationshipState::Enum GetPreviousState() const override;
            virtual RelationshipMigrationAction::Enum GetMigrationAction( RANDOMBASE* prng ) const override;
            virtual RelationshipTerminationReason::Enum GetTerminationReason() const override;
            virtual IIndividualHumanSTI* MalePartner() const override;
            virtual IIndividualHumanSTI* FemalePartner() const override;
            virtual IIndividualHumanSTI* GetPartner( IIndividualHumanSTI* pIndiv ) const override;
            virtual const suids::suid& GetSuid() const override;
            virtual unsigned int GetSlotNumberForPartner( bool forPartnerB ) const override;
            virtual const tRelationshipMembers GetMembers() const override;
            virtual bool IsDiscordant() const override;
            virtual float GetTimer() const override;
            virtual float GetDuration() const override;
            virtual float GetStartTime() const override;
            virtual float GetScheduledEndTime() const override;
            virtual RelationshipType::Enum GetType() const override;
            virtual suids::suid GetPartnerId( const suids::suid& myID ) const override;
            virtual suids::suid GetMalePartnerId() const override;
            virtual suids::suid GetFemalePartnerId() const override;
            virtual ExternalNodeId_t GetOriginalNodeId() const override;
            virtual bool IsMalePartnerAbsent() const override;
            virtual bool IsFemalePartnerAbsent() const override;

            virtual float GetCoitalRate() const override;
            virtual ProbabilityNumber getProbabilityUsingCondomThisAct() const override;
            virtual unsigned int GetTotalCoitalActs() const override;
            virtual unsigned int GetNumCoitalActs() const override;
            virtual unsigned int GetNumCoitalActsUsingCondoms() const override;
            virtual bool HasMigrated() const override;
            virtual suids::suid GetMigrationDestination() const override;

        protected:
            friend class RelationshipFactory;

            Relationship();
            Relationship( const Relationship& rMaster );
            Relationship( RANDOMBASE* pRNG,
                          const suids::suid& rRelId,
                          IRelationshipParameters* pParams, 
                          IIndividualHumanSTI* male_partner, 
                          IIndividualHumanSTI* female_partner,
                          bool isOutsidePFA,
                          Sigmoid* pCondomUsage );

            void SetParameters( ISociety* pSociety );

            virtual Relationship* Clone();

            DECLARE_SERIALIZABLE(Relationship);

            suids::suid _suid;
            bool is_outside_pfa;
            RelationshipState::Enum state;
            RelationshipState::Enum previous_state;
            RelationshipType::Enum relationship_type;
            RelationshipTerminationReason::Enum termination_reason;
            IRelationshipParameters* p_rel_params ;
            IIndividualHumanSTI* male_partner; // change to male_partner and female_partner at some point, but sounds so sterile...
            IIndividualHumanSTI* female_partner;
            suids::suid absent_male_partner_id;
            suids::suid absent_female_partner_id;
            float rel_timer;
            float rel_duration;
            float start_time;
            float scheduled_end_time;
            ExternalNodeId_t original_node_id;
            act_prob_vec_t act_prob_vec;
            unsigned int total_coital_acts;
            unsigned int coital_acts_this_dt;
            unsigned int coital_acts_this_dt_using_condoms;
            bool has_migrated;
            suids::suid migration_destination;
            unsigned int male_slot_number;
            unsigned int female_slot_number;
            Sigmoid* p_condom_usage;
    };

    class RelationshipFactory
    {
    public:
        static IRelationship* CreateRelationship( RANDOMBASE* pRNG,
                                                  const suids::suid& rRelId,
                                                  IRelationshipParameters* pParams, 
                                                  IIndividualHumanSTI* male_partner, 
                                                  IIndividualHumanSTI* female_partner,
                                                  bool isOutsidePFA,
                                                  Sigmoid* pCondomUsage );
    };
}
