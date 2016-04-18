/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "Types.h"
#include "INodeContext.h"
#include "IRelationship.h"
#include "IRelationshipParameters.h"
#include "IRelationshipManager.h"
#include "IIndividualHumanSTI.h"
#include "Sigmoid.h"

namespace Kernel 
{
    class IDMAPI Relationship : public IRelationship
    {
        public:
            friend class IndividualHumanSTI;
            friend class RelationshipFactory;
            DECLARE_QUERY_INTERFACE()
            IMPLEMENT_DEFAULT_REFERENCE_COUNTING()

        public:
            virtual ~Relationship();

            // public non-const
            virtual void Pause( IIndividualHumanSTI* departee ) override;
            virtual void Terminate( RelationshipTerminationReason::Enum terminationReason ) override;
            virtual void Migrate() override;
            virtual void Resume( IRelationshipManager* pRelMan, ISociety* pSociety, IIndividualHumanSTI* returnee ) override;
            virtual bool Update( float dt ) override;
            virtual void Consummate( float dt ) override;

            // public const
            virtual RelationshipState::Enum GetState() const override;
            virtual RelationshipState::Enum GetPreviousState() const override;
            virtual RelationshipMigrationAction::Enum GetMigrationAction( RANDOMBASE* prng ) const override;
            virtual RelationshipTerminationReason::Enum GetTerminationReason() const override;
            virtual IIndividualHumanSTI* MalePartner() const override;
            virtual IIndividualHumanSTI* FemalePartner() const override;
            virtual IIndividualHumanSTI* GetPartner( IIndividualHumanSTI* pIndiv ) const override;
            virtual const suids::suid& GetSuid() const override;
            virtual const std::string& GetPropertyKey() override;
            virtual const std::string& GetPropertyName() const override;
            virtual const tRelationshipMembers GetMembers() const override;
            virtual bool IsDiscordant() const override;
            virtual float GetTimer() const override;
            virtual float GetDuration() const override;
            virtual float GetStartTime() const override;
            virtual float GetScheduledEndTime() const override;
            virtual bool GetUsingCondom() const override;
            virtual RelationshipType::Enum GetType() const override;
            virtual suids::suid GetMalePartnerId() const override;
            virtual suids::suid GetFemalePartnerId() const override;
            virtual ExternalNodeId_t GetOriginalNodeId() const override;

            virtual float GetCoitalRate() const override;
            virtual ProbabilityNumber getProbabilityUsingCondomThisAct() const override;
            virtual unsigned int GetNumCoitalActs() const override;
            virtual bool HasMigrated() const override;

        protected:
            Relationship();
            Relationship( const suids::suid& rRelId,
                          IRelationshipManager* pRelMan,
                          IRelationshipParameters* pParams, 
                          IIndividualHumanSTI* male_partner, 
                          IIndividualHumanSTI* female_partner );

            void SetManager( IRelationshipManager* pRelMan, ISociety* pSociety );

            virtual Relationship* Clone() = 0;

            static void serialize( IArchive& ar, Relationship* obj );

            suids::suid _suid;
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
#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
            std::string propertyKey;
            std::string propertyName;
            ExternalNodeId_t original_node_id;
            act_prob_vec_t act_prob_vec;
            bool using_condom;  // For notifying coital act observers
            IRelationshipManager * relMan;
            unsigned int total_coital_acts;
            bool has_migrated;
#pragma warning( pop )

    private:
        static void LogRelationship( unsigned int _id, suids::suid _male_partner, suids::suid _female_partner, RelationshipType::Enum _type );
        struct LogEntry {
            unsigned int id;
            suids::suid male_partner;
            suids::suid female_partner;
            RelationshipType::Enum type;
        };
#define REL_LOG_COUNT    0x100000   // 2^20 entries
        static unsigned int _head;
        static unsigned int _tail;
        static LogEntry _log[REL_LOG_COUNT];
    };

    class IDMAPI RelationshipFactory
    {
    public:
        static IRelationship* CreateRelationship( const suids::suid& rRelId,
                                                  IRelationshipManager* pRelMan,
                                                  IRelationshipParameters* pParams, 
                                                  IIndividualHumanSTI* male_partner, 
                                                  IIndividualHumanSTI* female_partner );
    };

    class IDMAPI TransitoryRelationship : public Relationship
    {
        public:
            friend class RelationshipFactory;
            DECLARE_QUERY_INTERFACE()

        protected:
            TransitoryRelationship();
            TransitoryRelationship( const suids::suid& rRelId,
                                    IRelationshipManager* pRelMan,
                                    IRelationshipParameters* pParams,
                                    IIndividualHumanSTI* male_partner, 
                                    IIndividualHumanSTI* female_partner );

            virtual Relationship* Clone() override;

#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
            DECLARE_SERIALIZABLE(TransitoryRelationship);
#pragma warning( pop )
    };

    class InformalRelationship : public Relationship
    {
        public:
            friend class RelationshipFactory;
            DECLARE_QUERY_INTERFACE()

        protected:
            InformalRelationship();
            InformalRelationship( const suids::suid& rRelId,
                                  IRelationshipManager* pRelMan,
                                  IRelationshipParameters* pParams, 
                                  IIndividualHumanSTI* male_partner, 
                                  IIndividualHumanSTI* female_partner );

            virtual Relationship* Clone() override;

#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
            DECLARE_SERIALIZABLE(InformalRelationship);
#pragma warning( pop )
    };

    class MarriageRelationship : public Relationship
    {
        public:
            friend class RelationshipFactory;
            DECLARE_QUERY_INTERFACE()

        protected:
            MarriageRelationship();
            MarriageRelationship( const suids::suid& rRelId,
                                  IRelationshipManager* pRelMan,
                                  IRelationshipParameters* pParams, 
                                  IIndividualHumanSTI* male_partner, 
                                  IIndividualHumanSTI* female_partner );

            virtual Relationship* Clone() override;

#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
            DECLARE_SERIALIZABLE(MarriageRelationship);
#pragma warning( pop )
    };
}
