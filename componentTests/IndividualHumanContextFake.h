/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "Contexts.h"
#include "IIndividualHumanHIV.h"
#include "RANDOM.h"
#include "IHIVInterventionsContainer.h"
#include "InfectionHIV.h"
#include "IIndividualHumanSTI.h"
#include "IIndividualHuman.h"
#include "IndividualEventContext.h"

using namespace Kernel;

class IndividualHumanContextFake : public IIndividualHumanContext,
                                   public IIndividualHumanHIV,
                                   public IGlobalContext,
                                   public IIndividualHumanEventContext,
                                   public IInfectionHIV,
                                   public IIndividualHumanSTI,
                                   public IIndividualHuman
{
public:
    IndividualHumanContextFake( IIndividualHumanInterventionsContext* pIHIC,
                                INodeContext* pNC,
                                INodeEventContext* pNEC,
                                ISusceptibilityHIV* pISusceptibilityHIV )
        : IIndividualHumanContext() 
        , m_RefCount(0)
        , m_Id()
        , m_pInterventionsContext(pIHIC)
        , m_pNodeContext(pNC)
        , m_pNodeEventContext(pNEC)
        , m_pISusceptibilityHIV( pISusceptibilityHIV )
        , m_Rand(0)
        , m_IntendsToBreastfeed(false)
        , m_IsPregnant(false)
        , m_Age(0.0)
        , m_WhoStage(0.0)
        , m_HasSTI(false)
        , m_HasHIV(false)
        , m_HasCoSTI(false)
        , m_Relationships()
        , m_Properties()
    {
        m_Id.data = 1 ;
    }

    virtual ~IndividualHumanContextFake() {};

    // ---------------------
    // --- ISupport Methods
    // ---------------------
    virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override
    {
        *ppvObject = nullptr ;
        if ( iid == GET_IID(IIndividualHumanHIV)) 
            *ppvObject = static_cast<IIndividualHumanHIV*>(this);
        else if ( iid == GET_IID(IGlobalContext)) 
            *ppvObject = static_cast<IGlobalContext*>(this);
        else if ( iid == GET_IID(IIndividualHumanEventContext)) 
            *ppvObject = static_cast<IIndividualHumanEventContext*>(this);
        else if ( iid == GET_IID(IIndividualHumanSTI)) 
            *ppvObject = static_cast<IIndividualHumanSTI*>(this);
        else if ( iid == GET_IID(IIndividualHuman)) 
            *ppvObject = static_cast<IIndividualHuman*>(this);

        if( *ppvObject != nullptr )
        {
            return QueryResult::s_OK ;
        }
        else
            return QueryResult::e_NOINTERFACE ;
    }

    virtual int32_t AddRef() override
    {
        m_RefCount++ ;
        return m_RefCount;
    }

    virtual int32_t Release() override
    {
        m_RefCount-- ;
        return m_RefCount;
    }

    // ------------------------------------
    // --- IIndividualHumanContext Methods
    // ------------------------------------
    virtual suids::suid GetSuid() const override
    {
        return m_Id ;
    }

    virtual IIndividualHumanInterventionsContext *GetInterventionsContext() const override
    {
        return m_pInterventionsContext ;
    }
        
    virtual IIndividualHumanEventContext* GetEventContext() override
    {
        return static_cast<IIndividualHumanEventContext*>(this) ;
    }

    virtual ::RANDOMBASE* GetRng() override
    { 
        return &m_Rand ; 
    }

    virtual suids::suid   GetNextInfectionSuid()                    override { throw std::exception("The method or operation is not implemented."); }
    virtual void          UpdateGroupMembership()                   override { throw std::exception("The method or operation is not implemented."); }
    virtual void          UpdateGroupPopulation(float size_changes) override { throw std::exception("The method or operation is not implemented."); }

    virtual ISusceptibilityContext* GetSusceptibilityContext() const override { throw std::exception("The method or operation is not implemented."); }
    virtual const NodeDemographics* GetDemographics()          const override { throw std::exception("The method or operation is not implemented."); }

    virtual IIndividualHumanInterventionsContext* GetInterventionsContextbyInfection(Infection* infection)       override { throw std::exception("The method or operation is not implemented."); }
    virtual const NodeDemographicsDistribution*   GetDemographicsDistribution(std::string)                 const override { throw std::exception("The method or operation is not implemented."); }        

    virtual const std::string& GetPropertyReportString() const     override { throw std::exception("The method or operation is not implemented."); }
    virtual void SetPropertyReportString( const std::string& str ) override { throw std::exception("The method or operation is not implemented."); }

    // --------------------------------
    // --- IIndividualHuman Methods
    // --------------------------------
    //virtual suids::suid GetSuid() const = 0;
    //virtual double GetAge() const  = 0 ;
    //virtual int GetGender() const = 0;
    //virtual double GetMonteCarloWeight() const = 0;
    //virtual bool IsInfected() const = 0;
    //virtual HumanStateChange GetStateChange() const = 0;
    //virtual IIndividualHumanInterventionsContext* GetInterventionsContext() const = 0;
    //virtual tProperties* GetProperties() = 0;
    virtual NewInfectionState::_enum GetNewInfectionState() const override { throw std::exception("The method or operation is not implemented."); }
    virtual void Update(float current_time, float dt)             override { throw std::exception("The method or operation is not implemented."); }

    virtual INodeContext* GetParent() const override
    {
        return m_pNodeContext ;
    }

    virtual bool IsAdult() const
    {
        return m_Age/DAYSPERYEAR > 15.0 ;
    }

    virtual void setupMaternalAntibodies(Kernel::IIndividualHumanContext *,Kernel::INodeContext *) override { throw std::exception("The method or operation is not implemented."); }
    virtual void AcquireNewInfection(Kernel::StrainIdentity *,int)                                 override { throw std::exception("The method or operation is not implemented."); }
    virtual void SetInitialInfections(int)                                                         override { throw std::exception("The method or operation is not implemented."); }
    virtual void SetParameters(float,float,float,float)                                            override { throw std::exception("The method or operation is not implemented."); }
    virtual void InitializeHuman(void)                                                             override { throw std::exception("The method or operation is not implemented."); }
    virtual void UpdateInfectiousness(float)                                                       override { throw std::exception("The method or operation is not implemented."); }
    virtual float GetAcquisitionImmunity(void) const                                               override { throw std::exception("The method or operation is not implemented."); }
    virtual void UpdateMCSamplingRate(float)                                                       override { throw std::exception("The method or operation is not implemented."); }
    virtual bool UpdatePregnancy(float)                                                            override { throw std::exception("The method or operation is not implemented."); }
    virtual void InitiatePregnancy(float)                                                          override { throw std::exception("The method or operation is not implemented."); }
    virtual ProbabilityNumber getProbMaternalTransmission(void) const                              override { throw std::exception("The method or operation is not implemented."); }
    virtual suids::suid GetParentSuid(void) const                                                  override { throw std::exception("The method or operation is not implemented."); }
    virtual bool IsMigrating(void)                                                                 override { throw std::exception("The method or operation is not implemented."); }
    virtual void ClearNewInfectionState(void)                                                      override { throw std::exception("The method or operation is not implemented."); }
    virtual const infection_list_t& GetInfections(void) const                                      override { throw std::exception("The method or operation is not implemented."); }
    virtual float GetImmunityReducedAcquire(void) const                                            override { throw std::exception("The method or operation is not implemented."); }
    virtual float GetInterventionReducedAcquire(void) const                                        override { throw std::exception("The method or operation is not implemented."); }
    virtual const suids::suid& GetMigrationDestination(void)                                       override { throw std::exception("The method or operation is not implemented."); }
    virtual void SetContextTo(Kernel::INodeContext *)                                              override { throw std::exception("The method or operation is not implemented."); }
    virtual void SetMigrationModifier( float modifier )                                            override { throw std::exception("The method or operation is not implemented."); }

    virtual void SetGoingOnFamilyTrip( suids::suid migrationDestination, 
                                        MigrationType::Enum migrationType, 
                                        float timeUntilTrip, 
                                        float timeAtDestination,
                                        bool isDestinationNewHome ) override { throw std::exception("The method or operation is not implemented."); }

    virtual void SetWaitingToGoOnFamilyTrip() override { throw std::exception("The method or operation is not implemented."); }
    virtual void GoHome()                     override { throw std::exception("The method or operation is not implemented."); }

    // --------------------------------
    // --- IIndividualHumanHIV Methods
    // --------------------------------
    virtual ISusceptibilityHIV* GetHIVSusceptibility() const override
    {
        return m_pISusceptibilityHIV ;
    }

    virtual bool IntendsToBreastfeed() const
    {
        return m_IntendsToBreastfeed ;
    }

    virtual IInfectionHIV* GetHIVInfection() const override
    {
        return (IInfectionHIV*)this ;
    }

    virtual bool HasHIV() const override
    {
        return m_HasHIV ;
    }

    virtual bool HasSTICoInfection() const override
    {
        return m_HasCoSTI ;
    }

    virtual void SetStiCoInfectionState() override
    { 
        m_HasCoSTI = true ;
        release_assert( !(m_HasSTI && (m_HasCoSTI || m_HasHIV)) );
    }

    virtual void ClearStiCoInfectionState() override
    { 
        m_HasCoSTI = false ;
        release_assert( !(m_HasSTI && (m_HasCoSTI || m_HasHIV)) );
    }

    virtual IHIVInterventionsContainer* GetHIVInterventionsContainer() const override
    {
        return reinterpret_cast<IHIVInterventionsContainer*>(m_pInterventionsContext) ;
    }

    virtual std::string toString() const override { throw std::exception("The method or operation is not implemented."); }

    // --------------------------
    // --- IGlobalContext Methods
    // --------------------------
    virtual const SimulationConfig*     GetSimulationConfigObj() const override { throw std::exception("The method or operation is not implemented."); }
    virtual const IInterventionFactory* GetInterventionFactory() const override { throw std::exception("The method or operation is not implemented."); }

    // -----------------------------------------
    // --- IIndividualHumanEventContext Methods
    // -----------------------------------------
    virtual bool IsPregnant() const override
    {
        return m_IsPregnant ;
    }

    virtual double GetAge() const override
    {
        return m_Age ;
    }

    virtual bool IsInfected() const override
    {
        return m_HasSTI || m_HasHIV ;
    }

    virtual int GetGender() const override
    {
        return m_Gender ;
    }

    virtual int              GetAbovePoverty()     const override { throw std::exception("The method or operation is not implemented."); }
    virtual double           GetMonteCarloWeight() const override { throw std::exception("The method or operation is not implemented."); }
    virtual bool             IsPossibleMother()    const override { throw std::exception("The method or operation is not implemented."); }
    virtual float            GetInfectiousness()   const override { throw std::exception("The method or operation is not implemented."); }
    virtual HumanStateChange GetStateChange(void)  const override { throw std::exception("The method or operation is not implemented."); }
    virtual void             Die( HumanStateChange )     override { throw std::exception("The method or operation is not implemented."); }
    virtual bool             AtHome()              const override { throw std::exception("The method or operation is not implemented."); }
    virtual bool             IsOnFamilyTrip()      const override { throw std::exception("The method or operation is not implemented."); }
    virtual const suids::suid& GetHomeNodeId()     const override { throw std::exception("The method or operation is not implemented."); }
    virtual bool IsDead()                          const override { throw std::exception("The method or operation is not implemented."); }

    virtual tProperties* GetProperties() override
    {
        return &m_Properties ;
    }

    virtual INodeEventContext* GetNodeEventContext() override
    {
        return m_pNodeEventContext ;
    }

    // ------------------
    // --- IInfectionHIV Methods 
    // ------------------
    virtual float GetWHOStage() const override
    {
        return m_WhoStage ;
    }

    virtual NaturalNumber GetViralLoad()              const override { throw std::exception("The method or operation is not implemented."); }
    virtual float GetPrognosis()                      const override { throw std::exception("The method or operation is not implemented."); }
    virtual float GetTimeInfected()                   const override { throw std::exception("The method or operation is not implemented."); }
    virtual float GetDaysTillDeath()                  const override { throw std::exception("The method or operation is not implemented."); }
    virtual const HIVInfectionStage::Enum& GetStage() const override { throw std::exception("The method or operation is not implemented."); }
    virtual void SetupSuppressedDiseaseTimers()             override { throw std::exception("The method or operation is not implemented."); }
    virtual void ApplySuppressionDropout()                  override { throw std::exception("The method or operation is not implemented."); }
    virtual void ApplySuppressionFailure()                  override { throw std::exception("The method or operation is not implemented."); }


    // ------------------
    // --- IIndividualHumanSTI Methods 
    // ------------------
    //virtual suids::suid GetSuid() const = 0; // pass-through to base
    //virtual bool IsInfected() const = 0; //  pass-through to base
    virtual bool IsBehavioralSuperSpreader()       const override { throw std::exception("The method or operation is not implemented."); }
    virtual unsigned int GetExtrarelationalFlags() const override { throw std::exception("The method or operation is not implemented."); }
    virtual bool IsCircumcised()                   const override { throw std::exception("The method or operation is not implemented."); }
    virtual float GetCoInfectiveFactor()           const override { throw std::exception("The method or operation is not implemented."); }
    
    virtual void UpdateSTINetworkParams(const char *prop = nullptr, const char* new_value = nullptr) override {throw std::exception("The method or operation is not implemented.");}

    virtual void UpdateInfectiousnessSTI(std::vector<act_prob_t> &act_prob_vec, unsigned int rel_id) override { throw std::exception("The method or operation is not implemented."); }



    virtual bool AvailableForRelationship(RelationshipType::Enum) const override { throw std::exception("The method or operation is not implemented."); }
    virtual void UpdateEligibility()                                    override { throw std::exception("The method or operation is not implemented."); }
    virtual void ConsiderRelationships(float dt)                        override { throw std::exception("The method or operation is not implemented."); }
    virtual void RemoveRelationship( IRelationship* pNewRelationship )  override { throw std::exception("The method or operation is not implemented."); }
    //virtual std::string toString() const = 0 ;
    virtual unsigned int GetOpenRelationshipSlot() const       override { throw std::exception("The method or operation is not implemented."); }
    virtual NaturalNumber GetLast6MonthRels() const            override { throw std::exception("The method or operation is not implemented."); }
    virtual NaturalNumber GetLifetimeRelationshipCount() const override { throw std::exception("The method or operation is not implemented."); }
    virtual NaturalNumber GetNumRelationshipsAtDeath() const   override { throw std::exception("The method or operation is not implemented."); }
    virtual void NotifyPotentialExposure()                     override { throw std::exception("The method or operation is not implemented."); }
    virtual void onEmigrating()                                override { throw std::exception("The method or operation is not implemented."); }
    virtual void onImmigrating()                               override { throw std::exception("The method or operation is not implemented."); }
    virtual suids::suid GetNodeSuid() const                    override { throw std::exception("The method or operation is not implemented."); }

    virtual ProbabilityNumber getProbabilityUsingCondomThisAct( const IRelationshipParameters* pRelParams ) const override { throw std::exception("The method or operation is not implemented."); }

    virtual RelationshipSet_t& GetRelationshipsAtDeath() override { throw std::exception("The method or operation is not implemented."); }

    virtual float GetDebutAge() const override
    {
        return 13*365 ;
    }

    virtual RelationshipSet_t& GetRelationships() override
    {
        return m_Relationships ;
    }

    virtual void AddRelationship( IRelationship* pNewRelationship ) override
    {
        m_Relationships.insert( pNewRelationship );
    }

    // ------------------
    // --- Other Methods 
    // ------------------
    void SetId( int id )
    {
        m_Id.data = id ;
    }

    void SetIntendsToBreastfeed( bool feed )
    {
        m_IntendsToBreastfeed = feed ;
    }

    void SetIsPregnant( bool isPregnant )
    {
        m_IsPregnant = isPregnant ;
    }

    void SetGender( int gender )
    {
        m_Gender = gender ;
    }

    void SetAge( float age )
    {
        m_Age = age ;
    }

    void SetWhoStage( float stage )
    {
        m_WhoStage = stage ;
    }

    void SetHasSTI( bool hasSTI )
    {
        m_HasSTI = hasSTI ;
        release_assert( !(m_HasSTI && (m_HasCoSTI || m_HasHIV)) );
    }

    void SetHasHIV( bool hasHIV )
    {
        m_HasHIV = hasHIV ;
        release_assert( !(m_HasSTI && (m_HasCoSTI || m_HasHIV)) );
    }

private:
    int m_RefCount ;
    suids::suid m_Id ;
    IIndividualHumanInterventionsContext* m_pInterventionsContext ;
    INodeContext* m_pNodeContext ;
    INodeEventContext* m_pNodeEventContext ;
    ISusceptibilityHIV* m_pISusceptibilityHIV ;
    PSEUDO_DES m_Rand ;
    bool m_IntendsToBreastfeed ;
    bool m_IsPregnant ;
    int m_Gender ;
    float m_Age ;
    float m_WhoStage ;
    bool m_HasSTI ;
    bool m_HasHIV ;
    bool m_HasCoSTI ;
    RelationshipSet_t m_Relationships ;
    tProperties m_Properties ;
};


