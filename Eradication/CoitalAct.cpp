
#include "stdafx.h"

#include "CoitalAct.h"

//SETUP_LOGGING( "CoitalAct" )

namespace Kernel
{
    CoitalAct::CoitalAct()
        : m_ActID()
        , m_RelID()
        , m_UnInfectedPartnerID()
        , m_InfectedPartnerID()
        , m_IsUsingCondom( false )
        , m_RiskMultiplier( 1.0 )
        , m_TransmissionProbability( 0.0 )
        , m_AcquisitionProbability( 0.0 )
        , m_WasTransmitted( false )
    {
    }

    CoitalAct::CoitalAct( const suids::suid& actID,
                          const suids::suid& relID,
                          const suids::suid& unInfectedPartnerID,
                          const suids::suid& infectedPartnerID,
                          bool isUsingCondom,
                          float riskMult )
        : m_ActID( actID )
        , m_RelID( relID )
        , m_UnInfectedPartnerID( unInfectedPartnerID )
        , m_InfectedPartnerID( infectedPartnerID )
        , m_IsUsingCondom( isUsingCondom )
        , m_RiskMultiplier( riskMult )
        , m_TransmissionProbability( 0.0 )
        , m_AcquisitionProbability( 0.0 )
        , m_WasTransmitted( false )
    {
    }

    CoitalAct::~CoitalAct()
    {
    }

    suids::suid CoitalAct::GetSuid() const
    {
        return m_ActID;
    }

    suids::suid CoitalAct::GetRelationshipID() const
    {
        return m_RelID;
    }

    suids::suid CoitalAct::GetUnInfectedPartnerID() const
    {
        return m_UnInfectedPartnerID;
    }

    suids::suid CoitalAct::GetInfectedPartnerID() const
    {
        return m_InfectedPartnerID;
    }

    bool CoitalAct::IsUsingCondoum() const
    {
        return m_IsUsingCondom;
    }

    float CoitalAct::GetRiskMultiplier() const
    {
        return m_RiskMultiplier;
    }

    float CoitalAct::GetTransmissionProbability() const
    {
        return m_TransmissionProbability;
    }

    void CoitalAct::SetTransmissionProbability( float prob )
    {
        m_TransmissionProbability = prob;
    }

    float CoitalAct::GetAcquisitionProbability() const
    {
        return m_AcquisitionProbability;
    }

    void CoitalAct::SetAcquisitionProbability( float prob )
    {
        m_AcquisitionProbability = prob;
    }

    bool CoitalAct::WasTransmitted() const
    {
        return m_WasTransmitted;
    }

    void CoitalAct::SetWasTransmitted()
    {
        m_WasTransmitted = true;
    }

    void CoitalAct::ClearProbabilities()
    {
        m_RiskMultiplier = 0.0;
        m_TransmissionProbability = 0.0;
        m_AcquisitionProbability = 0.0;
    }
}
