
#pragma once

#include "suids.hpp"

namespace Kernel
{
    class CoitalAct
    {
    public:
        CoitalAct();
        CoitalAct( const suids::suid& actID,
                   const suids::suid & relID,
                   const suids::suid& unInfectedPartnerID,
                   const suids::suid& infectedPartnerID,
                   bool isUsingCondom,
                   float riskMult );
        virtual ~CoitalAct();

        suids::suid GetSuid() const;
        suids::suid GetRelationshipID() const;
        suids::suid GetUnInfectedPartnerID() const;
        suids::suid GetInfectedPartnerID() const;
        bool IsUsingCondoum() const;
        float GetRiskMultiplier() const;
        float GetTransmissionProbability() const;
        float GetAcquisitionProbability() const;
        bool WasTransmitted() const;

        void SetTransmissionProbability( float prob );
        void SetAcquisitionProbability( float prob );
        void SetWasTransmitted();
        void ClearProbabilities();

    protected:
        suids::suid m_ActID;
        suids::suid m_RelID;
        suids::suid m_UnInfectedPartnerID;
        suids::suid m_InfectedPartnerID;
        bool m_IsUsingCondom;
        float m_RiskMultiplier;
        float m_TransmissionProbability;
        float m_AcquisitionProbability;
        bool m_WasTransmitted;
    };
}
