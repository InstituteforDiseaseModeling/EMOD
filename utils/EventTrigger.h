/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "BaseEventTrigger.h"

namespace Kernel
{
    class EventTriggerInternal;
    class EventTriggerFactory;
    class EventTrigger;

    // An EventTrigger is the name of an event that occurs to an individual.
    // EventTriggers can be built-in or defined by the user.  They are broadcasted by
    // a IIndividualEventBroadcaster (which is implemented by NodeEventContextHost)
    // to objects that have registered to be notified of these events.
    // This implementation of EventTrigger attempts to reduce string processing and
    // improve performance.
    class EventTrigger : public BaseEventTrigger<EventTrigger,EventTriggerFactory>
    {
    public:
        static const EventTrigger& Births;
        static const EventTrigger& EveryUpdate;
        static const EventTrigger& EveryTimeStep;
        static const EventTrigger& NewInfectionEvent;
        static const EventTrigger& TBActivation;
        static const EventTrigger& NewClinicalCase;
        static const EventTrigger& NewSevereCase;
        static const EventTrigger& DiseaseDeaths;
        static const EventTrigger& OpportunisticInfectionDeath;
        static const EventTrigger& NonDiseaseDeaths;
        static const EventTrigger& TBActivationSmearPos;
        static const EventTrigger& TBActivationSmearNeg;
        static const EventTrigger& TBActivationExtrapulm;
        static const EventTrigger& TBActivationPostRelapse;
        static const EventTrigger& TBPendingRelapse;
        static const EventTrigger& TBActivationPresymptomatic;
        static const EventTrigger& TestPositiveOnSmear;
        static const EventTrigger& ProviderOrdersTBTest;
        static const EventTrigger& TBTestPositive;
        static const EventTrigger& TBTestNegative;
        static const EventTrigger& TBTestDefault;
        static const EventTrigger& TBRestartHSB;
        static const EventTrigger& TBMDRTestPositive;
        static const EventTrigger& TBMDRTestNegative;
        static const EventTrigger& TBMDRTestDefault;
        static const EventTrigger& TBFailedDrugRegimen;
        static const EventTrigger& TBRelapseAfterDrugRegimen;
        static const EventTrigger& TBStartDrugRegimen;
        static const EventTrigger& TBStopDrugRegimen;
        static const EventTrigger& PropertyChange;
        static const EventTrigger& STIDebut;
        static const EventTrigger& StartedART;
        static const EventTrigger& StoppedART;
        static const EventTrigger& InterventionDisqualified;
        static const EventTrigger& HIVNewlyDiagnosed;
        static const EventTrigger& GaveBirth;
        static const EventTrigger& Pregnant;
        static const EventTrigger& Emigrating;
        static const EventTrigger& Immigrating;
        static const EventTrigger& HIVTestedNegative;
        static const EventTrigger& HIVTestedPositive;
        static const EventTrigger& NewlySymptomatic;
        static const EventTrigger& SymptomaticCleared;
        static const EventTrigger& TwelveWeeksPregnant;
        static const EventTrigger& FourteenWeeksPregnant;
        static const EventTrigger& SixWeeksOld;
        static const EventTrigger& EighteenMonthsOld;
        static const EventTrigger& STIPreEmigrating;
        static const EventTrigger& STIPostImmigrating;
        static const EventTrigger& STINewInfection;
        static const EventTrigger& NewExternalHIVInfection;
        static const EventTrigger& NodePropertyChange;
        static const EventTrigger& HappyBirthday;
        static const EventTrigger& EnteredRelationship;
        static const EventTrigger& ExitedRelationship;
        static const EventTrigger& FirstCoitalAct;
        static const EventTrigger& ExposureComplete;

    public:
        EventTrigger();
        explicit EventTrigger( const std::string &init_str );
        explicit EventTrigger( const char *init_str );
        ~EventTrigger();

    protected:
        template<class Trigger, class Factory> friend class Kernel::BaseEventTriggerFactory;

        explicit EventTrigger( EventTriggerInternal* peti );
    };

    class EventTriggerFactory : public BaseEventTriggerFactory<EventTrigger, EventTriggerFactory>
    {
    public:
        GET_SCHEMA_STATIC_WRAPPER( EventTriggerFactory )

        ~EventTriggerFactory();

    private:
        template<class Trigger, class Factory> friend class Kernel::BaseEventTriggerFactory;

        EventTriggerFactory();
    };
}
