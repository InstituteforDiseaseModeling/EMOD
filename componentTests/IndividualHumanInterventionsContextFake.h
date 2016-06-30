/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "Contexts.h"
#include "Interventions.h"
#include "IHIVInterventionsContainer.h"

using namespace Kernel;

class IndividualHumanInterventionsContextFake : public IIndividualHumanInterventionsContext,
                                                public IHIVCascadeOfCare,
                                                public IInterventionConsumer,
                                                public IHIVMedicalHistory
{
public:
    IndividualHumanInterventionsContextFake()
        : IIndividualHumanInterventionsContext()
        , m_Parent(nullptr)
        , m_CascadeState("")
        , m_CD4Count(0.0)
        , m_CD4CountLowest(FLT_MAX)
        , m_ReceivedCD4(false)
        , m_EverStaged(false)
        , m_OnStageForArt(false)
        , m_HasTested(false)
        , m_HasTestedHIVPositive(false)
        , m_ReceivedTestResultForHIV(ReceivedTestResultsType::UNKNOWN)
        , m_WhoStage(0.0)
    {
    }
    virtual ~IndividualHumanInterventionsContextFake() {}

    // -------------------------------------------------
    // --- IIndividualHumanInterventionsContext Methods
    // -------------------------------------------------
    virtual void SetContextTo(IIndividualHumanContext *context)
    {
        m_Parent = context ;
    }

    virtual IIndividualHumanContext* GetParent()
    {
        return m_Parent ;
    }

    virtual std::list<IDistributableIntervention*> GetInterventionsByType(const std::string &type_name)
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual std::list<IDistributableIntervention*> GetInterventionsByName(const std::string &intervention_name)
    { 
        throw std::exception("The method or operation is not implemented.");
    }

    virtual std::list<void*> GetInterventionsByInterface( iid_t iid )
    { 
        throw std::exception("The method or operation is not implemented."); 
    }

    virtual void PurgeExisting( const std::string &iv_name )
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual bool ContainsExisting( const std::string &iv_name )
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual void CheckSTINetworkParams(const char *prop = nullptr, const char* new_value = nullptr) {}

    // ---------------------
    // --- ISupport Methods
    // ---------------------
    virtual QueryResult QueryInterface(iid_t iid, void **ppvObject)
    {
        *ppvObject = nullptr ;
        if ( iid == GET_IID(IHIVCascadeOfCare)) 
            *ppvObject = static_cast<IHIVCascadeOfCare*>(this);
        else if ( iid == GET_IID(IInterventionConsumer)) 
            *ppvObject = static_cast<IInterventionConsumer*>(this);
        else if ( iid == GET_IID(IHIVMedicalHistory)) 
            *ppvObject = static_cast<IHIVMedicalHistory*>(this);

        if( *ppvObject != nullptr )
        {
            return QueryResult::s_OK ;
        }
        else
            return QueryResult::e_NOINTERFACE ;
    }

    virtual int32_t AddRef()
    {
        throw std::exception("The method or operation is not implemented.");
    }
    virtual int32_t Release()
    {
        throw std::exception("The method or operation is not implemented.");
    }

    // ------------------------------
    // --- IHIVCascadeOfCare Methods
    // ------------------------------
    virtual std::string getCascadeState() const
    {
        return m_CascadeState ;
    }

    virtual void setCascadeState(std::string state)
    {
        m_CascadeState = state ;
    }

    // ------------------------------
    // --- IInterventionConsumer Methods
    // ------------------------------
    virtual bool GiveIntervention( IDistributableIntervention * pIV )
    {
        return true ;
    }

    // ------------------------------
    // --- IHIVMedicalHistory Methods
    // ------------------------------

    virtual void OnTestCD4(float cd4count)
    {
        m_CD4Count = cd4count ;
        if( m_CD4Count < m_CD4CountLowest )
        {
            m_CD4CountLowest = m_CD4Count ;
        }
        m_ReceivedCD4 = true ;
    }

    virtual float FirstRecordedCD4() const 
    {
        return -1.0 ;
    }

    virtual float LastRecordedCD4() const 
    {
        return m_CD4Count ;
    }

    virtual float LowestRecordedCD4() const
    {
        return m_CD4CountLowest ;
    }

    virtual bool EverReceivedCD4() const
    {
        return m_ReceivedCD4 ;
    }

    virtual void OnStageForART(bool stagedForART)
    {
        m_EverStaged = true ;
        m_OnStageForArt = stagedForART ;
    }

    virtual bool EverStaged() const
    {
        return m_EverStaged ;
    }

    virtual bool EverStagedForART() const
    {
        return m_OnStageForArt ;
    }

    virtual void OnAssessWHOStage(float WHOStage)
    {
        m_WhoStage = WHOStage ;
    }

    virtual float LastRecordedWHOStage() const
    {
        return m_WhoStage ;
    }

    virtual bool EverTested() const
    {
        return m_HasTested ;
    }

    virtual bool EverTestedPastYear() const { throw std::exception("The method or operation is not implemented."); }

    virtual bool EverTestedHIVPositive() const
    {
        return m_HasTestedHIVPositive ;
    }

    virtual ReceivedTestResultsType::Enum ReceivedTestResultForHIV() const
    {
        return m_ReceivedTestResultForHIV ;
    }

    virtual void OnTestForHIV(bool test_result)
    {
        m_HasTested = true ;
        if( test_result )
        {
            m_HasTestedHIVPositive = true ;
        }
    }

    virtual void OnReceivedTestResultForHIV(bool test_result)
    {
        if( test_result )
            m_ReceivedTestResultForHIV = ReceivedTestResultsType::POSITIVE ;
        else
            m_ReceivedTestResultForHIV = ReceivedTestResultsType::NEGATIVE ;
    }

    virtual void OnBeginART()                     { throw std::exception("The method or operation is not implemented."); }
    virtual void OnBeginPreART()                  { throw std::exception("The method or operation is not implemented."); }
    virtual void OnEndPreART()                    { throw std::exception("The method or operation is not implemented."); }

    virtual bool EverBeenOnPreART()      const { throw std::exception("The method or operation is not implemented."); }
    virtual bool EverBeenOnART()         const { throw std::exception("The method or operation is not implemented."); }

    virtual float TimeOfMostRecentTest()      const { throw std::exception("The method or operation is not implemented."); }
    virtual float TimeOfMostRecentCD4()       const { throw std::exception("The method or operation is not implemented."); }
    virtual float TimeLastSeenByHealthcare()  const { throw std::exception("The method or operation is not implemented."); }
    virtual float TimeFirstStartedART()       const { throw std::exception("The method or operation is not implemented."); }
    virtual float TimeLastStartedART()        const { throw std::exception("The method or operation is not implemented."); }
    virtual float TotalTimeOnART()            const { throw std::exception("The method or operation is not implemented."); }
    virtual unsigned int NumTimesStartedART() const { throw std::exception("The method or operation is not implemented."); }

    virtual NaturalNumber    GetTotalARTInitiations()     const { throw std::exception("The method or operation is not implemented."); }
    virtual NonNegativeFloat GetTotalYearsOnART()         const { throw std::exception("The method or operation is not implemented."); }
    virtual NonNegativeFloat GetYearsSinceFirstARTInit()  const { throw std::exception("The method or operation is not implemented."); }
    virtual NonNegativeFloat GetYearsSinceLatestARTInit() const { throw std::exception("The method or operation is not implemented."); }

private:
    IIndividualHumanContext* m_Parent ;
    std::string m_CascadeState ;
    float m_CD4Count ;
    float m_CD4CountLowest ;
    bool m_ReceivedCD4 ;
    bool m_EverStaged ;
    bool m_OnStageForArt ;
    bool m_HasTested ;
    bool m_HasTestedHIVPositive ;
    ReceivedTestResultsType::Enum m_ReceivedTestResultForHIV ;
    float m_WhoStage ;
};
