/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

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
    virtual QueryResult QueryInterface(iid_t iid, void **ppvObject)
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

    virtual int32_t AddRef()
    {
        m_RefCount++ ;
        return m_RefCount;
    }

    virtual int32_t Release()
    {
        m_RefCount-- ;
        return m_RefCount;
    }

    // ------------------------------------
    // --- IIndividualHumanContext Methods
    // ------------------------------------
    virtual suids::suid GetSuid() const
    {
        return m_Id ;
    }

    virtual IIndividualHumanInterventionsContext *GetInterventionsContext() const
    {
        return m_pInterventionsContext ;
    }
        
    virtual IIndividualHumanEventContext* GetEventContext()
    {
        return (IIndividualHumanEventContext*)this ;
    }

    virtual ::RANDOMBASE* GetRng()
    { 
        return &m_Rand ; 
    }

    virtual suids::suid   GetNextInfectionSuid()                    { throw std::exception("The method or operation is not implemented."); }
    virtual void          UpdateGroupMembership()                   { throw std::exception("The method or operation is not implemented."); }
    virtual void          UpdateGroupPopulation(float size_changes) { throw std::exception("The method or operation is not implemented."); }

    virtual ISusceptibilityContext* GetSusceptibilityContext() const { throw std::exception("The method or operation is not implemented."); }
    virtual const NodeDemographics* GetDemographics()          const { throw std::exception("The method or operation is not implemented."); }

    virtual IIndividualHumanInterventionsContext* GetInterventionsContextbyInfection(Infection* infection)       { throw std::exception("The method or operation is not implemented."); }
    virtual const NodeDemographicsDistribution*   GetDemographicsDistribution(std::string)                 const { throw std::exception("The method or operation is not implemented."); }        

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
    virtual NewInfectionState::_enum GetNewInfectionState() const { throw std::exception("The method or operation is not implemented."); }
    virtual void Update(float current_time, float dt)             { throw std::exception("The method or operation is not implemented."); }

    virtual INodeContext* GetParent() const
    {
        return m_pNodeContext ;
    }

    // --------------------------------
    // --- IIndividualHumanHIV Methods
    // --------------------------------
    virtual	ISusceptibilityHIV* GetHIVSusceptibility() const 
    {
        return m_pISusceptibilityHIV ;
    }

    virtual bool IntendsToBreastfeed() const
    {
        return m_IntendsToBreastfeed ;
    }

    virtual	IInfectionHIV* GetHIVInfection() const
    {
        return (IInfectionHIV*)this ;
    }

    virtual	bool HasHIV() const
    {
        return m_HasHIV ;
    }

    virtual bool HasSTICoInfection() const
    {
        return m_HasCoSTI ;
    }

    virtual void SetStiCoInfectionState()   
    { 
        m_HasCoSTI = true ;
        release_assert( !(m_HasSTI && (m_HasCoSTI || m_HasHIV)) );
    }

    virtual void ClearStiCoInfectionState() 
    { 
        m_HasCoSTI = false ;
        release_assert( !(m_HasSTI && (m_HasCoSTI || m_HasHIV)) );
    }

    virtual IHIVInterventionsContainer* GetHIVInterventionsContainer() const
    {
        return (IHIVInterventionsContainer*)m_pInterventionsContext ;
    }

    virtual std::string toString()                                     const { throw std::exception("The method or operation is not implemented."); }

    // --------------------------
    // --- IGlobalContext Methods
    // --------------------------
    virtual const SimulationConfig*     GetSimulationConfigObj() const { throw std::exception("The method or operation is not implemented."); }
    virtual const IInterventionFactory* GetInterventionFactory() const { throw std::exception("The method or operation is not implemented."); }

    // -----------------------------------------
    // --- IIndividualHumanEventContext Methods
    // -----------------------------------------
    virtual bool IsPregnant() const
    {
        return m_IsPregnant ;
    }

    virtual double GetAge() const
    {
        return m_Age ;
    }

    virtual bool IsInfected() const
    {
        return m_HasSTI || m_HasHIV ;
    }

    virtual int GetGender() const 
    {
        return m_Gender ;
    }

    virtual int              GetAbovePoverty()     const { throw std::exception("The method or operation is not implemented."); }
    virtual double           GetMonteCarloWeight() const { throw std::exception("The method or operation is not implemented."); }
    virtual bool             IsPossibleMother()    const { throw std::exception("The method or operation is not implemented."); }
    virtual float            GetInfectiousness()   const { throw std::exception("The method or operation is not implemented."); }
    virtual HumanStateChange GetStateChange(void)  const { throw std::exception("The method or operation is not implemented."); }
    virtual void             Die( HumanStateChange )     { throw std::exception("The method or operation is not implemented."); }

    virtual tProperties* GetProperties()
    {
        return &m_Properties ;
    }

    virtual INodeEventContext* GetNodeEventContext()
    {
        return m_pNodeEventContext ;
    }

    // ------------------
    // --- IInfectionHIV Methods 
    // ------------------
    virtual float GetWHOStage() const
    {
        return m_WhoStage ;
    }

    virtual NaturalNumber GetViralLoad()              const { throw std::exception("The method or operation is not implemented."); }
    virtual float GetPrognosis()                      const { throw std::exception("The method or operation is not implemented."); }
    virtual float GetTimeInfected()                   const { throw std::exception("The method or operation is not implemented."); }
    virtual float GetDaysTillDeath()                  const { throw std::exception("The method or operation is not implemented."); }
    virtual const HIVInfectionStage::Enum& GetStage() const { throw std::exception("The method or operation is not implemented."); }
    virtual void SetupSuppressedDiseaseTimers()             { throw std::exception("The method or operation is not implemented."); }
    virtual void ApplySuppressionDropout()                  { throw std::exception("The method or operation is not implemented."); }
    virtual void ApplySuppressionFailure()                  { throw std::exception("The method or operation is not implemented."); }


    // ------------------
    // --- IIndividualHumanSTI Methods 
    // ------------------
    //virtual suids::suid GetSuid() const = 0; // pass-through to base
    //virtual bool IsInfected() const = 0; //  pass-through to base
    virtual bool IsBehavioralSuperSpreader()       const { throw std::exception("The method or operation is not implemented."); }
    virtual unsigned int GetExtrarelationalFlags() const { throw std::exception("The method or operation is not implemented."); }
    virtual bool IsCircumcised()                   const { throw std::exception("The method or operation is not implemented."); }
    virtual float GetCoInfectiveFactor()           const { throw std::exception("The method or operation is not implemented."); }
    
    virtual void UpdateSTINetworkParams(const char *prop = NULL, const char* new_value = NULL) {throw std::exception("The method or operation is not implemented.");}

    virtual void UpdateInfectiousnessSTI(std::vector<act_prob_t> &act_prob_vec, unsigned int rel_id) { throw std::exception("The method or operation is not implemented."); }



    virtual bool AvailableForRelationship(RelationshipType::Enum) const { throw std::exception("The method or operation is not implemented."); }
    virtual void UpdateEligibility()                                    { throw std::exception("The method or operation is not implemented."); }
    virtual void ConsiderRelationships(float dt)                        { throw std::exception("The method or operation is not implemented."); }
    virtual void RemoveRelationship( IRelationship* pNewRelationship )  { throw std::exception("The method or operation is not implemented."); }
    virtual void VacateRelationship( IRelationship* relationship )      { throw std::exception("The method or operation is not implemented."); }
    virtual void RejoinRelationship( IRelationship* relationship )      { throw std::exception("The method or operation is not implemented."); }
    //virtual std::string toString() const = 0 ;
    virtual unsigned int GetOpenRelationshipSlot() const       { throw std::exception("The method or operation is not implemented."); }
    virtual NaturalNumber GetLast6MonthRels() const            { throw std::exception("The method or operation is not implemented."); }
    virtual NaturalNumber GetLifetimeRelationshipCount() const { throw std::exception("The method or operation is not implemented."); }
    virtual NaturalNumber GetNumRelationshipsAtDeath() const   { throw std::exception("The method or operation is not implemented."); }
    virtual void NotifyPotentialExposure()                     { throw std::exception("The method or operation is not implemented."); }

    virtual ProbabilityNumber getProbabilityUsingCondomThisAct( RelationshipType::Enum ) const { throw std::exception("The method or operation is not implemented."); }

    virtual RelationshipSet_t& GetRelationshipsAtDeath() { throw std::exception("The method or operation is not implemented."); }

    virtual float GetDebutAge() const
    {
        return 13*365 ;
    }

    virtual RelationshipSet_t& GetRelationships()
    {
        return m_Relationships ;
    }

    virtual void AddRelationship( IRelationship* pNewRelationship )
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


