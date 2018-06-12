/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "ISupports.h"

namespace Kernel
{

    class IIndividualHumanTB : public ISupports //this is the simple TB interface, use for TB/HIV 
    {
    public:
        virtual bool HasActiveInfection() const = 0;
        virtual bool HasLatentInfection() const = 0;
        virtual bool IsImmune() const = 0;
        virtual bool IsMDR() const = 0;
        virtual bool IsSmearPositive() const = 0;
        virtual bool HasActivePresymptomaticInfection() const = 0;
        virtual int GetTime() const = 0;

        virtual bool HasFailedTreatment() const = 0;
        virtual bool HasEverRelapsedAfterTreatment() const = 0;
        virtual bool IsExtrapulmonary() const = 0;
        virtual bool IsEvolvedMDR() const = 0;
    
        virtual bool HasPendingRelapseInfection() const = 0;
        virtual bool IsFastProgressor() const = 0;
        virtual bool IsTreatmentNaive() const = 0;
        virtual bool IsOnTreatment() const = 0;
        virtual float GetDurationSinceInitInfection() const = 0;
    };

    class IIndividualHumanTB2 : public IIndividualHumanTB 
    {
        public:
            virtual void onInfectionIncidence() = 0;
            virtual void onInfectionMDRIncidence() = 0;   
    };

    class INodeTB : public ISupports
    {
    public:
        virtual float GetIncidentCounter()           const = 0;
        virtual float GetMDRIncidentCounter()         const = 0;
        virtual float GetMDREvolvedIncidentCounter() const = 0;
        virtual float GetMDRFastIncidentCounter()     const = 0;
    };

}
