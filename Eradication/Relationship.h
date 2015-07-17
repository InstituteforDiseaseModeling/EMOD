/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include "Types.h"
#include "Node.h"
#include "IRelationship.h"
#include "IRelationshipManager.h"
#include "IIndividualHumanSTI.h"

namespace Kernel {

    class IDMAPI Relationship : public IRelationship
    {
        friend class IndividualHumanSTI;
        friend class RelationshipFactory;

        public:
            virtual ~Relationship();

            // public non-const
            virtual void pause( IIndividualHumanSTI* departee );
            virtual void resume( IIndividualHumanSTI* returnee );
            virtual void terminate( IRelationshipManager* );
            virtual void Initialize( IRelationshipManager* );
            virtual bool Update( float dt = 1.0f );
            virtual void notifyIndividuals();
            virtual void Consummate( float dt );

            // public const
            virtual IIndividualHumanSTI* MalePartner() const;
            virtual IIndividualHumanSTI* FemalePartner() const;
            virtual IIndividualHumanSTI* GetPartner( IIndividualHumanSTI* pIndiv ) const;
            virtual unsigned long int GetId() const;
            virtual void SetSuid(suids::suid);
            virtual suids::suid GetSuid();
            virtual const std::string& GetPropertyKey();
            virtual const std::string& GetPropertyName() const;
            virtual const tRelationshipMembers GetMembers() const;
            virtual bool IsDiscordant() const;
            virtual float GetTimer() const;
            virtual float GetDuration() const;
            virtual float GetStartTime() const;
            virtual float GetScheduledEndTime() const;
            virtual bool GetUsingCondom() const;
            virtual RelationshipType::Enum GetType() const;
            virtual suids::suid GetMalePartnerId() const;
            virtual suids::suid GetFemalePartnerId() const;
            virtual suids::suid GetOriginalNodeId() const;

            virtual float GetCoitalRate() const;
            virtual ProbabilityNumber getProbabilityUsingCondomThisAct() const;

            static unsigned long int counter; // mpi-node level, maybe not that great an idea

        protected:
            Relationship( IIndividualHumanSTI* male_partner, IIndividualHumanSTI* female_partner, RelationshipType::Enum type = RelationshipType::TRANSITORY );

            IIndividualHumanSTI* male_partner; // change to male_partner and female_partner at some point, but sounds so sterile...
            IIndividualHumanSTI* female_partner;
            suids::suid absent_male_partner_id;
            suids::suid absent_female_partner_id;
            unsigned long int _id;
            suids::suid _suid;
            float rel_timer;
            float rel_duration;
            float start_time;
            float scheduled_end_time;
#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
            std::string propertyKey;
            std::string propertyName;
            bool force_finish;
            RelationshipType::Enum relationship_type;
            suids::suid original_node_id;
            act_prob_vec_t act_prob_vec;
#pragma warning( pop )

            bool using_condom;  // For notifying coital act observers
            IRelationshipManager * relMan;


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
        static IRelationship* CreateRelationship( RelationshipType::Enum relType, IIndividualHumanSTI* male_partner, IIndividualHumanSTI* female_partner );
    };

    class IDMAPI TransitoryRelationship : public Relationship
    {
        public:
            friend class RelationshipFactory;
        protected:
            TransitoryRelationship( IIndividualHumanSTI* male_partner, IIndividualHumanSTI* female_partner );
    };

    class InformalRelationship : public Relationship
    {
        public:
            friend class RelationshipFactory;
        protected:
            InformalRelationship( IIndividualHumanSTI* male_partner, IIndividualHumanSTI* female_partner );
    };

    class MarriageRelationship : public Relationship
    {
        public:
            friend class RelationshipFactory;
        protected:
            MarriageRelationship( IIndividualHumanSTI* male_partner, IIndividualHumanSTI* female_partner );
    };
}
