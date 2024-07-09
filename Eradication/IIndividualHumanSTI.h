
#pragma once

#include <set>

#include "IdmApi.h"
#include "ISupports.h"
#include "Types.h"
#include "suids.hpp"
#include "IRelationship.h"
#include "CoitalAct.h"

namespace Kernel 
{
    class IPKeyValueContainer;
    struct IRelationshipParameters;
    struct IRelationship;
    struct IdmDateTime;
    struct IIndividualHuman;

    struct IDMAPI IIndividualHumanSTI : public ISupports
    {
        virtual suids::suid GetSuid() const = 0; // pass-through to base
        virtual bool IsInfected() const = 0; //  pass-through to base
        virtual IIndividualHuman* GetIndividualHuman() = 0;
        virtual const IPKeyValueContainer& GetPropertiesConst() const = 0;
        virtual suids::suid GetNodeSuid() const = 0;
        virtual bool IsBehavioralSuperSpreader() const = 0;
        virtual unsigned int GetExtrarelationalFlags() const = 0;
        virtual bool IsCircumcised() const = 0;
        virtual bool IsPostDebut() const = 0;
        virtual bool EnterPfaNow() const = 0;
        virtual bool EnterRelationshipNow() const = 0;
        virtual void UpdateHistory( const IdmDateTime& rCurrentTime, float dt ) = 0;
        virtual void UpdatePausedRelationships( const IdmDateTime& rCurrentTime, float dt ) = 0;

        virtual float GetCoitalActRiskAcquisitionFactor() const = 0;
        virtual float GetCoitalActRiskTransmissionFactor() const = 0;

        virtual float GetCoInfectiveTransmissionFactor() const = 0;
        virtual float GetCoInfectiveAcquisitionFactor() const = 0;
        virtual bool HasSTICoInfection() const = 0;
        virtual void SetStiCoInfectionState() = 0;
        virtual void ClearStiCoInfectionState() = 0;
        virtual void UpdateInfectiousnessSTI( CoitalAct& rCoitalAct ) = 0;
        virtual void SetSexualDebutAge( float age ) = 0;

        // Availability
        virtual bool AvailableForRelationship(RelationshipType::Enum) const = 0;
        virtual std::vector<unsigned int> GetQueuedRelationships() const = 0;
        virtual std::vector<bool>GetAvailableRelationships() const = 0;

        // Relationships
        virtual void UpdateEligibility() = 0;
        virtual void UpdateSTINetworkParams(const char *prop = nullptr, const char* new_value = nullptr) = 0;
        virtual void ConsiderRelationships(float dt) = 0;
        virtual void AddRelationship( IRelationship* pNewRelationship ) = 0;
        virtual void RemoveRelationship( IRelationship* pNewRelationship ) = 0;
        virtual std::vector<IRelationship*>& GetRelationships() = 0;
        virtual const std::vector<IRelationship*>& GetRelationshipsTerminated() = 0;
        virtual void onEmigrating() = 0;
        virtual void onImmigrating() = 0;
        virtual IRelationship* GetMigratingRelationship() = 0;

        virtual std::string toString() const = 0; // serialization, for logging
        virtual unsigned int GetOpenRelationshipSlot() const = 0; // change name
        virtual NaturalNumber GetLast6MonthRels() const = 0;
        virtual NaturalNumber GetLast6MonthRels( RelationshipType::Enum ofType ) const = 0;
        virtual NaturalNumber GetLast12MonthRels( RelationshipType::Enum ofType ) const = 0;
        virtual NaturalNumber GetNumUniquePartners( int itp, int irel ) const = 0;
        virtual NaturalNumber GetLifetimeRelationshipCount() const = 0;
        virtual NaturalNumber GetLifetimeRelationshipCount( RelationshipType::Enum ofType ) const = 0;
        virtual float GetSimDayBorn() const = 0;
        virtual float GetDebutAge() const = 0;
        virtual float GetAgeAtInfection() const = 0;
        virtual void NotifyPotentialExposure() = 0;
        virtual ProbabilityNumber getProbabilityUsingCondomThisAct( const IRelationshipParameters* pRelParams ) const = 0;
        virtual void UpdateNumCoitalActs( uint32_t numActs ) = 0;
        virtual uint32_t GetTotalCoitalActs() const = 0;
        virtual IRelationship* GetExitingRelationship() const = 0;
        virtual const CoitalAct& GetCurrentCoitalAct() const = 0;

        virtual void ClearAssortivityIndexes() = 0;
        virtual int GetAssortivityIndex( RelationshipType::Enum type ) const = 0;
        virtual void SetAssortivityIndex( RelationshipType::Enum type, int index ) = 0;
    };

}
