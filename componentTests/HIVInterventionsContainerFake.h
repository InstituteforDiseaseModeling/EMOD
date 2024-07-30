#pragma once

#include "IHIVInterventionsContainer.h"
#include "HIVEnums.h"

class HIVInterventionsContainerFake : public IHIVInterventionsContainer
                                    , public IHIVMedicalHistory
{
public:
    virtual bool OnArtQuery(void) const { return (m_ArtStatus == ARTStatus::ON_VL_SUPPRESSED); }
    virtual const ARTStatus::Enum &GetArtStatus(void) const { return m_ArtStatus; }
    virtual bool ShouldReconstituteCD4(void) const { return false; }
    virtual const ProbabilityNumber GetInfectivitySuppression(void) const { return 0.0f; }
    virtual float GetDurationSinceLastStartingART(void) const { return m_DurationSinceLastStartingArt; }
    virtual const ProbabilityNumber &GetProbMaternalTransmissionModifier(void) const { return m_ProbMaternalTransmissionModifier; }

    // --------------------------
    // --- IHIVMedicalHistory
    // --------------------------
    // updates medical chart
    virtual void OnTestForHIV(bool test_result)                override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual void OnReceivedTestResultForHIV(bool test_result)  override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual void OnStageForART(bool stagedForART)              override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual void OnAssessWHOStage(float WHOStage)              override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual void OnTestCD4(float CD4count)                     override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual void OnBeginART()                                  override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }

    // queries medical chart
    virtual bool EverTested()                 const { return m_EverTested; }
    virtual bool EverTestedPastYear()         const { return false; }
    virtual bool EverTestedHIVPositive()      const { return m_EverTestedPositive; }
    virtual bool EverStaged()                 const { return false; }
    virtual bool EverStagedForART()           const { return false; }
    virtual bool EverReceivedCD4()            const { return false; }
    virtual bool EverBeenOnART()              const { return false; }
    virtual float TimeOfMostRecentTest()      const { return 0.0f; }
    virtual float TimeOfMostRecentCD4()       const { return 0.0f; }
    virtual float TimeLastSeenByHealthcare()  const { return 0.0f; }
    virtual float TimeFirstStartedART()       const { return 0.0f; }
    virtual float TimeLastStartedART()        const { return 0.0f; }
    virtual float TotalTimeOnART()            const { return 0.0f; }
    virtual unsigned int NumTimesStartedART() const { return 0; }
    virtual float LastRecordedWHOStage()      const { return 0.0f; }
    virtual float LowestRecordedCD4()         const { return 0.0f; }
    virtual float FirstRecordedCD4()          const { return 0.0f; }
    virtual float LastRecordedCD4()           const { return 0.0f; }
    virtual ReceivedTestResultsType::Enum ReceivedTestResultForHIV() const { return m_Results; }

    virtual NaturalNumber GetTotalARTInitiations()        const { return 0.0f; }
    virtual NonNegativeFloat GetTotalYearsOnART()         const { return 0.0f; }
    virtual NonNegativeFloat GetYearsSinceFirstARTInit()  const { return 0.0f; }
    virtual NonNegativeFloat GetYearsSinceLatestARTInit() const { return 0.0f; }

    // ---------------------
    // --- ISupport Methods
    // ---------------------
    virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override
    {
        *ppvObject = nullptr;
        if (iid == GET_IID(IHIVInterventionsContainer))
            *ppvObject = static_cast<IHIVInterventionsContainer*>(this);
        else if (iid == GET_IID(IHIVMedicalHistory))
            *ppvObject = static_cast<IHIVMedicalHistory*>(this);

        if (*ppvObject != nullptr)
        {
            return QueryResult::s_OK;
        }
        else
            return QueryResult::e_NOINTERFACE;
    }

    virtual int32_t AddRef() override
    {
        m_RefCount++;
        return m_RefCount;
    }

    virtual int32_t Release() override
    {
        m_RefCount--;
        return m_RefCount;
    }

    // ----------------------------------------------
    // --- Methods for setting state - i.e. TEST CODE
    // ----------------------------------------------
    void SetDurationSinceLastStartingArt(float duration)
    {
        m_DurationSinceLastStartingArt = duration;
    }

    void SetArtStatus( ARTStatus::Enum status )
    {
        m_ArtStatus = status;
    }

    void SetEverTested( bool tested )
    {
        m_EverTested = tested;
    }

    void SetEverTestedPositive( bool testedPositive )
    {
        m_EverTestedPositive = testedPositive;
    }

    void SetReceivedResults( ReceivedTestResultsType::Enum results )
    {
        m_Results = results;
    }

    // ----------------
    // --- Constructor
    // ----------------
    HIVInterventionsContainerFake()
        : m_RefCount(0)
        , m_ArtStatus( ARTStatus::UNDEFINED)
        , m_ProbMaternalTransmissionModifier(0.0f)
        , m_DurationSinceLastStartingArt(0.0f)
        , m_EverTested( false )
        , m_EverTestedPositive( false )
        , m_Results( ReceivedTestResultsType::UNKNOWN )
    {
    }
    
private:
    int m_RefCount;
    ARTStatus::Enum m_ArtStatus;
    ProbabilityNumber m_ProbMaternalTransmissionModifier;
    float m_DurationSinceLastStartingArt;
    bool m_EverTested;
    bool m_EverTestedPositive;
    ReceivedTestResultsType::Enum m_Results;
};