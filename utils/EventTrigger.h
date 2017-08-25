/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "Configure.h"

namespace Kernel
{
    class EventTriggerInternal;

    // An EventTrigger is the name of an event that occurs to an individual (node in future).
    // EventTriggers can be built-in or defined by the user.  They are broadcasted by
    // a INodeTriggeredInterventionConsumer (which is implemented by NodeEventContextHost)
    // to objects that have registered to be notified of these events.
    // This implementation of EventTrigger attempts to reduce string processing and
    // improve performance.
    class IDMAPI EventTrigger
    {
    public:
        static const EventTrigger& NoTrigger;
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
        static const EventTrigger& HIVSymptomatic;
        static const EventTrigger& HIVPreARTToART;
        static const EventTrigger& HIVNonPreARTToART;
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

    public:
        EventTrigger();
        EventTrigger( std::string &init_str );
        EventTrigger( const char *init_str );

        EventTrigger& operator=( const EventTrigger& rTrigger );
        EventTrigger& operator=( const std::string& rTriggerStr );

        bool operator==( const EventTrigger& rThat ) const;
        bool operator!=( const EventTrigger& rThat ) const;

        // Returns true if the trigger has not been intialized
        bool IsUninitialized() const;

        // Returns the string value of the event
        const std::string& ToString() const;
        const char* c_str() const;

        // Returns the index/unique integer id of the event.  This is intended to be used
        // by NodeEventContextHost so that it can use a vector and not map.
        int GetIndex() const;

        static void serialize( Kernel::IArchive& ar, EventTrigger& obj );

    protected:
        friend class EventTriggerFactory;

        EventTrigger( EventTriggerInternal* );

    private:
        EventTriggerInternal* m_pInternal;
    };


    class IDMAPI EventTriggerFactory : public JsonConfigurable
    {
    public:
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

        static const char* CONSTRAINT_SCHEMA_STRING;

        static EventTriggerFactory* GetInstance();
        static void DeleteInstance();
        static void SetBuiltIn();

        ~EventTriggerFactory();

        virtual bool Configure( const Configuration* inputJson ) override;
        int GetNumEventTriggers() const;
        std::vector<EventTrigger> GetAllEventTriggers();
        bool IsValidEvent( const std::string& candidateEvent ) const;
        const std::string& GetEventTriggerName( int eventIndex ) const;

        EventTrigger CreateUserEventTrigger( const std::string& name );

        const std::vector<std::string>& GetBuiltInNames();

    protected:
        friend class EventTrigger;
        friend class EventTriggerInternal;

        static const EventTrigger& CreateBuiltInEventTrigger( const char* name );

        EventTriggerInternal* GetEventTriggerInternal( const std::string& str );
        EventTriggerInternal* GetEventTriggerInternal( const char* str );
        EventTriggerInternal* Add( const std::string& str );

    private:
        static const char* PARAMETER_NAME;

        EventTriggerFactory();

#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        static std::vector<std::pair<std::string, EventTrigger*>> m_VectorBuiltIn;

        std::vector<EventTriggerInternal*> m_VectorUser;
        std::vector<EventTriggerInternal*> m_VectorAll;
        std::map<std::string, EventTriggerInternal*> m_MapAll;
        std::vector<std::string> m_BuiltInNames;
#pragma warning( pop )
    };
}
