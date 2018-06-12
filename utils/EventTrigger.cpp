/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "stdafx.h"
#include "EventTrigger.h"

namespace Kernel
{
    // -----------------------------------------------------------------------------------------------
    // --- These built-in triggers are static variables that need to be in sync with the 
    // --- same static varaibles in the EXE and DLLs.  To do this, they are first created
    // --- such that the variable is a reference to an empty/uninitialized EventTrigger
    // --- that is maintained in m_VectorBuiltIn.  During Configure(), the EventTriggerInternal
    // --- is created for each of these built-in triggers.  Hence, one must call the Configure()
    // --- before attempting to use any triggers and it should only be called in the EXE.
    // ---
    // --- The DLLs get the EventTriggerFactory instance via the Environment instance being
    // --- set in DllInterfaceHelper::GetEModuleVersion().  This method calls Environment::setInstance()
    // --- which calls EventTriggerFactory::SetBuiltIn().  SetBuiltIn() assigns the
    // --- EventTriggerInternal's of these statics with those from the EXE.
    // -----------------------------------------------------------------------------------------------
    std::vector<std::pair<std::string,EventTrigger*>> EventTriggerFactory::m_VectorBuiltIn;

    const EventTrigger& EventTrigger::NoTrigger                  = EventTriggerFactory::CreateBuiltInEventTrigger( "NoTrigger"                 );
    const EventTrigger& EventTrigger::Births                     = EventTriggerFactory::CreateBuiltInEventTrigger( "Births"                    );
    const EventTrigger& EventTrigger::EveryUpdate                = EventTriggerFactory::CreateBuiltInEventTrigger( "EveryUpdate"               );
    const EventTrigger& EventTrigger::EveryTimeStep              = EventTriggerFactory::CreateBuiltInEventTrigger( "EveryTimeStep"             );
    const EventTrigger& EventTrigger::NewInfectionEvent          = EventTriggerFactory::CreateBuiltInEventTrigger( "NewInfectionEvent"         );
    const EventTrigger& EventTrigger::TBActivation               = EventTriggerFactory::CreateBuiltInEventTrigger( "TBActivation"              );
    const EventTrigger& EventTrigger::NewClinicalCase            = EventTriggerFactory::CreateBuiltInEventTrigger( "NewClinicalCase"           );
    const EventTrigger& EventTrigger::NewSevereCase              = EventTriggerFactory::CreateBuiltInEventTrigger( "NewSevereCase"             );
    const EventTrigger& EventTrigger::DiseaseDeaths              = EventTriggerFactory::CreateBuiltInEventTrigger( "DiseaseDeaths"             );
    const EventTrigger& EventTrigger::OpportunisticInfectionDeath = EventTriggerFactory::CreateBuiltInEventTrigger("OpportunisticInfectionDeath");
    const EventTrigger& EventTrigger::NonDiseaseDeaths           = EventTriggerFactory::CreateBuiltInEventTrigger( "NonDiseaseDeaths"          );
    const EventTrigger& EventTrigger::TBActivationSmearPos       = EventTriggerFactory::CreateBuiltInEventTrigger( "TBActivationSmearPos"      );
    const EventTrigger& EventTrigger::TBActivationSmearNeg       = EventTriggerFactory::CreateBuiltInEventTrigger( "TBActivationSmearNeg"      );
    const EventTrigger& EventTrigger::TBActivationExtrapulm      = EventTriggerFactory::CreateBuiltInEventTrigger( "TBActivationExtrapulm"     );
    const EventTrigger& EventTrigger::TBActivationPostRelapse    = EventTriggerFactory::CreateBuiltInEventTrigger( "TBActivationPostRelapse"   );
    const EventTrigger& EventTrigger::TBPendingRelapse           = EventTriggerFactory::CreateBuiltInEventTrigger( "TBPendingRelapse"          );
    const EventTrigger& EventTrigger::TBActivationPresymptomatic = EventTriggerFactory::CreateBuiltInEventTrigger( "TBActivationPresymptomatic");
    const EventTrigger& EventTrigger::TestPositiveOnSmear        = EventTriggerFactory::CreateBuiltInEventTrigger( "TestPositiveOnSmear"       );
    const EventTrigger& EventTrigger::ProviderOrdersTBTest       = EventTriggerFactory::CreateBuiltInEventTrigger( "ProviderOrdersTBTest"      );
    const EventTrigger& EventTrigger::TBTestPositive             = EventTriggerFactory::CreateBuiltInEventTrigger( "TBTestPositive"            );
    const EventTrigger& EventTrigger::TBTestNegative             = EventTriggerFactory::CreateBuiltInEventTrigger( "TBTestNegative"            );
    const EventTrigger& EventTrigger::TBTestDefault              = EventTriggerFactory::CreateBuiltInEventTrigger( "TBTestDefault"             );
    const EventTrigger& EventTrigger::TBRestartHSB               = EventTriggerFactory::CreateBuiltInEventTrigger( "TBRestartHSB"              );
    const EventTrigger& EventTrigger::TBMDRTestPositive          = EventTriggerFactory::CreateBuiltInEventTrigger( "TBMDRTestPositive"         );
    const EventTrigger& EventTrigger::TBMDRTestNegative          = EventTriggerFactory::CreateBuiltInEventTrigger( "TBMDRTestNegative"         );
    const EventTrigger& EventTrigger::TBMDRTestDefault           = EventTriggerFactory::CreateBuiltInEventTrigger( "TBMDRTestDefault"          );
    const EventTrigger& EventTrigger::TBFailedDrugRegimen        = EventTriggerFactory::CreateBuiltInEventTrigger( "TBFailedDrugRegimen"       );
    const EventTrigger& EventTrigger::TBRelapseAfterDrugRegimen  = EventTriggerFactory::CreateBuiltInEventTrigger( "TBRelapseAfterDrugRegimen" );
    const EventTrigger& EventTrigger::TBStartDrugRegimen         = EventTriggerFactory::CreateBuiltInEventTrigger( "TBStartDrugRegimen"        );
    const EventTrigger& EventTrigger::TBStopDrugRegimen          = EventTriggerFactory::CreateBuiltInEventTrigger( "TBStopDrugRegimen"         );
    const EventTrigger& EventTrigger::PropertyChange             = EventTriggerFactory::CreateBuiltInEventTrigger( "PropertyChange"            );
    const EventTrigger& EventTrigger::STIDebut                   = EventTriggerFactory::CreateBuiltInEventTrigger( "STIDebut"                  );
    const EventTrigger& EventTrigger::StartedART                 = EventTriggerFactory::CreateBuiltInEventTrigger( "StartedART"                );
    const EventTrigger& EventTrigger::StoppedART                 = EventTriggerFactory::CreateBuiltInEventTrigger( "StoppedART"                );
    const EventTrigger& EventTrigger::InterventionDisqualified   = EventTriggerFactory::CreateBuiltInEventTrigger( "InterventionDisqualified"  );
    const EventTrigger& EventTrigger::HIVNewlyDiagnosed          = EventTriggerFactory::CreateBuiltInEventTrigger( "HIVNewlyDiagnosed"         );
    const EventTrigger& EventTrigger::GaveBirth                  = EventTriggerFactory::CreateBuiltInEventTrigger( "GaveBirth"                 );
    const EventTrigger& EventTrigger::Pregnant                   = EventTriggerFactory::CreateBuiltInEventTrigger( "Pregnant"                  );
    const EventTrigger& EventTrigger::Emigrating                 = EventTriggerFactory::CreateBuiltInEventTrigger( "Emigrating"                );
    const EventTrigger& EventTrigger::Immigrating                = EventTriggerFactory::CreateBuiltInEventTrigger( "Immigrating"               );
    const EventTrigger& EventTrigger::HIVTestedNegative          = EventTriggerFactory::CreateBuiltInEventTrigger( "HIVTestedNegative"         );
    const EventTrigger& EventTrigger::HIVTestedPositive          = EventTriggerFactory::CreateBuiltInEventTrigger( "HIVTestedPositive"         );
    const EventTrigger& EventTrigger::HIVSymptomatic             = EventTriggerFactory::CreateBuiltInEventTrigger( "HIVSymptomatic"            );
    const EventTrigger& EventTrigger::TwelveWeeksPregnant        = EventTriggerFactory::CreateBuiltInEventTrigger( "TwelveWeeksPregnant"       );
    const EventTrigger& EventTrigger::FourteenWeeksPregnant      = EventTriggerFactory::CreateBuiltInEventTrigger( "FourteenWeeksPregnant"     );
    const EventTrigger& EventTrigger::SixWeeksOld                = EventTriggerFactory::CreateBuiltInEventTrigger( "SixWeeksOld"               );
    const EventTrigger& EventTrigger::EighteenMonthsOld          = EventTriggerFactory::CreateBuiltInEventTrigger( "EighteenMonthsOld"         );
    const EventTrigger& EventTrigger::STIPreEmigrating           = EventTriggerFactory::CreateBuiltInEventTrigger( "STIPreEmigrating"          );
    const EventTrigger& EventTrigger::STIPostImmigrating         = EventTriggerFactory::CreateBuiltInEventTrigger( "STIPostImmigrating"        );
    const EventTrigger& EventTrigger::STINewInfection            = EventTriggerFactory::CreateBuiltInEventTrigger( "STINewInfection"           ); 
    const EventTrigger& EventTrigger::NewExternalHIVInfection    = EventTriggerFactory::CreateBuiltInEventTrigger( "NewExternalHIVInfection"   );
    const EventTrigger& EventTrigger::NodePropertyChange         = EventTriggerFactory::CreateBuiltInEventTrigger( "NodePropertyChange"        );
    const EventTrigger& EventTrigger::HappyBirthday              = EventTriggerFactory::CreateBuiltInEventTrigger( "HappyBirthday"             );
    const EventTrigger& EventTrigger::EnteredRelationship        = EventTriggerFactory::CreateBuiltInEventTrigger( "EnteredRelationship"       );
    const EventTrigger& EventTrigger::ExitedRelationship         = EventTriggerFactory::CreateBuiltInEventTrigger( "ExitedRelationship"        );
    const EventTrigger& EventTrigger::FirstCoitalAct             = EventTriggerFactory::CreateBuiltInEventTrigger( "FirstCoitalAct"            );

    const char* EventTriggerFactory::CONSTRAINT_SCHEMA_STRING = "'<configuration>:Listed_Events.*' or Built-in";
    const char* EventTriggerFactory::PARAMETER_NAME = "Listed_Events";

    // ------------------------------------------------------------------------
    // --- EventTriggerInternal
    // ------------------------------------------------------------------------
    class EventTriggerInternal
    {
    public:
        std::string m_Name;
        int m_Index;

        EventTriggerInternal( const std::string& rName, int index )
            : m_Name( rName )
            , m_Index( index )
        {
        }

    };

    // ------------------------------------------------------------------------
    // --- EventTrigger
    // ------------------------------------------------------------------------

    EventTrigger::EventTrigger()
    : m_pInternal( nullptr )
    {
    }

    EventTrigger::EventTrigger( std::string &init_str )
   : m_pInternal( nullptr )
    {
        m_pInternal = EventTriggerFactory::GetInstance()->GetEventTriggerInternal( init_str );
    }

    EventTrigger::EventTrigger( const char *init_str )
    : m_pInternal( nullptr )
    {
        m_pInternal = EventTriggerFactory::GetInstance()->GetEventTriggerInternal( init_str );
    }

    EventTrigger::EventTrigger( EventTriggerInternal* peti )
    : m_pInternal( peti )
    {
    }

    EventTrigger& EventTrigger::operator=( const EventTrigger& rTrigger )
    {
        this->m_pInternal = rTrigger.m_pInternal;
        return *this;
    }

    EventTrigger& EventTrigger::operator=( const std::string& rTriggerStr )
    {
        m_pInternal = EventTriggerFactory::GetInstance()->GetEventTriggerInternal( rTriggerStr );
        return *this;
    }

    bool EventTrigger::operator==( const EventTrigger& rThat ) const
    {
        return (this->m_pInternal == rThat.m_pInternal);
    }

    bool EventTrigger::operator!=( const EventTrigger& rThat ) const
    {
        return (this->m_pInternal != rThat.m_pInternal);
    }

    bool EventTrigger::IsUninitialized() const
    {
        return (m_pInternal == nullptr) || (m_pInternal == NoTrigger.m_pInternal);
    }

    const std::string& EventTrigger::ToString() const
    {
        if( m_pInternal == nullptr )
        {
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "Cannot use ToString().  EventTrigger is uninitialized." );
        }
        return m_pInternal->m_Name;
    }

    const char* EventTrigger::c_str() const
    {
        if( m_pInternal == nullptr )
        {
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "Cannot use ToString().  EventTrigger is uninitialized." );
        }
        return m_pInternal->m_Name.c_str();
    }
    int EventTrigger::GetIndex() const
    {
        if( m_pInternal == nullptr )
        {
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "Cannot use ToString().  EventTrigger is uninitialized." );
        }
        return m_pInternal->m_Index;
    }

    void EventTrigger::serialize( Kernel::IArchive& ar, EventTrigger& obj )
    {
        std::string tmp;
        if( obj.IsUninitialized() )
            tmp = NoTrigger.ToString();
        else
            tmp = obj.ToString();

        ar & tmp;

        obj = tmp;
    }


    // ------------------------------------------------------------------------
    // --- EventTriggerFactory
    // ------------------------------------------------------------------------

    GET_SCHEMA_STATIC_WRAPPER_IMPL( EventTriggerFactory, EventTriggerFactory )
    BEGIN_QUERY_INTERFACE_BODY(EventTriggerFactory)
    END_QUERY_INTERFACE_BODY(EventTriggerFactory)

    EventTriggerFactory::EventTriggerFactory()
    : JsonConfigurable()
    , m_VectorUser()
    , m_VectorAll()
    , m_MapAll()
    , m_BuiltInNames()
    {
    }

    EventTriggerFactory::~EventTriggerFactory()
    {
    }

    const std::vector<std::string>& EventTriggerFactory::GetBuiltInNames()
    {
        if( m_BuiltInNames.size() == 0 )
        {
            for( auto entry : m_VectorBuiltIn )
            {
                m_BuiltInNames.push_back( entry.first );
            }
        }
        return m_BuiltInNames;
    }

    bool EventTriggerFactory::Configure( const Configuration* inputJson )
    {
        std::vector<std::string> user_events;
        initConfigTypeMap( "Listed_Events", &user_events, Listed_Events_DESC_TEXT );

        bool ret = JsonConfigurable::Configure( inputJson );

        // ---------------------------------------------------------------------------
        // --- NOTE: we want to execute the following code even if we ar egenerating
        // --- the schema so that we create the internal triggers.
        // ---------------------------------------------------------------------------
        if( ret )
        {
            if (ret && std::find(user_events.begin(), user_events.end(), "") != user_events.end())
            {
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, "Invalid Event in Listed_Events.  Empty string is an invalid event." );
            }

            // ----------------------------------------------------
            // --- See the top of the file about Built-in triggers
            // ----------------------------------------------------
            release_assert( m_VectorBuiltIn.size() > 0 );
            bool create_built_in = ((m_VectorBuiltIn.size() > 0) && (m_VectorBuiltIn[ 0 ].second->m_pInternal == nullptr));
            if( create_built_in )
            {
                for( auto& entry : m_VectorBuiltIn )
                {
                    entry.second->m_pInternal = Add( entry.first );
                }
            }

            for( auto& ev : user_events )
            {
                CreateUserEventTrigger( ev );
            }
        }

        return ret;
    }

    int EventTriggerFactory::GetNumEventTriggers() const
    {
        return m_VectorAll.size();
    }

    std::vector<EventTrigger> EventTriggerFactory::GetAllEventTriggers()
    {
        std::vector<EventTrigger> trigger_list;
        for( auto trigger : m_VectorAll )
        {
            trigger_list.push_back( EventTrigger( trigger ) );
        }
        return trigger_list;
    }

    bool EventTriggerFactory::IsValidEvent( const std::string& candidateEvent ) const
    {
        return (m_MapAll.count( candidateEvent ) > 0);
    }

    const std::string& EventTriggerFactory::GetEventTriggerName( int eventIndex ) const
    {
        release_assert( eventIndex < m_VectorAll.size() );
        return m_VectorAll[ eventIndex ]->m_Name;
    }

    EventTrigger EventTriggerFactory::CreateUserEventTrigger( const std::string& name )
    {
        EventTriggerInternal* p_eti = Add( std::string( name ) );
        m_VectorUser.push_back( p_eti );
        EventTrigger et( p_eti );
        return et;
    }

    EventTriggerInternal* EventTriggerFactory::GetEventTriggerInternal( const std::string& str )
    {
        if( m_MapAll.count( str ) == 0 )
        {
            std::stringstream ss;
            ss << "Undefined EventTrigger=" << str << ".  Not built-in or found in " << EventTriggerFactory::CONSTRAINT_SCHEMA_STRING << ".";
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
        return m_MapAll.at( str );
    }

    EventTriggerInternal* EventTriggerFactory::GetEventTriggerInternal( const char* str )
    {
        return GetEventTriggerInternal( std::string(str) );
    }

    EventTriggerInternal*  EventTriggerFactory::Add( const std::string& str )
    {
        if( m_MapAll.count( str ) != 0 )
        {
            std::stringstream ss;
            if (std::find(GetBuiltInNames().begin(), GetBuiltInNames().end(), str) != GetBuiltInNames().end())
            {
                ss << "Duplicate event = '" << str << "'. This is a Built-in Event. You do not need to define it. The Built-in events are:" << std::endl;
                for (std::string name : GetBuiltInNames())
                {
                    ss << name << std::endl;
                }
            }
            else
            {
                ss << "Duplicate event = '" << str << "'.  Events names must be unique.";
            }
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
        int index = m_VectorAll.size();
        EventTriggerInternal* p_eti = new EventTriggerInternal( str, index );
        m_VectorAll.push_back( p_eti );
        m_MapAll[ str ] = p_eti;
        return p_eti;
    }

    const EventTrigger& EventTriggerFactory::CreateBuiltInEventTrigger( const char* name )
    {
        // -----------------------------------------------------------------------------------
        // --- See the top of the file for more info, but notice that we do not create the
        // --- EventTriggerInternal, just the EventTrigger shell.  This allows us to update
        // --- the shell in the EXE when we create the internal or update the shell in the DLL
        // --- with the pointer in the EXE.
        // -----------------------------------------------------------------------------------
        std::pair<std::string, EventTrigger*> entry;
        entry.first = name;
        entry.second = new EventTrigger();
        m_VectorBuiltIn.push_back( entry );
        return *(m_VectorBuiltIn.back().second);
    }

    void EventTriggerFactory::SetBuiltIn()
    {
        // ----------------------------------------------------
        // --- See the top of the file about Built-in triggers.
        // --- Should only be called from DLLs and triggers should
        // --- not be used before hand.
        // ----------------------------------------------------
        release_assert( (m_VectorBuiltIn.size() > 0) && (m_VectorBuiltIn[ 0 ].second->m_pInternal == nullptr) );

        EventTriggerFactory* p_factory = (EventTriggerFactory*)Environment::getEventTriggerFactory();
        release_assert( p_factory != nullptr );

        for( int i = 0 ; i < m_VectorBuiltIn.size() ; ++i )
        {
            if( p_factory->m_MapAll.count( m_VectorBuiltIn[ i ].first ) == 0 )
            {
                std::stringstream ss;
                ss << "DLL Built-in trigger = " << m_VectorBuiltIn[ i ].first << " is missing in the EXE.";
                throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }

            m_VectorBuiltIn[i].second->m_pInternal = p_factory->m_MapAll.at( m_VectorBuiltIn[ i ].first );
        }
    }


    void EventTriggerFactory::DeleteInstance()
    {
        EventTriggerFactory* p_factory = (EventTriggerFactory*)Environment::getEventTriggerFactory();
        if( p_factory != nullptr )
        {
            // ---------------------------------------------------------------------------------
            // --- The Built-In events only get created once per execution.  For componentTests,
            // --- we need to get rid of the user defined events but maintain the built-in ones.
            // ---------------------------------------------------------------------------------
            p_factory->m_VectorAll.clear();
            p_factory->m_MapAll.clear();
            for( EventTriggerInternal* p_eti : p_factory->m_VectorUser )
            {
                delete p_eti;
            }
            p_factory->m_VectorUser.clear();
            delete p_factory;
        }
        Environment::setEventTriggerFactory( nullptr );
    }

    EventTriggerFactory* EventTriggerFactory::GetInstance()
    {
        EventTriggerFactory* p_factory = (EventTriggerFactory*)Environment::getEventTriggerFactory();
        if( p_factory == nullptr )
        {
            p_factory = new EventTriggerFactory();

            release_assert( m_VectorBuiltIn.size() > 0 );
            bool built_in_created = ((m_VectorBuiltIn.size() > 0) && (m_VectorBuiltIn[0].second->m_pInternal != nullptr));

            // ---------------------------------------------------------------------------------
            // --- The Built-In events only get created once per execution.  For componentTests,
            // --- we need to get rid of the user defined events but maintain the built-in ones.
            // ---------------------------------------------------------------------------------
            if( built_in_created )
            {
                for( auto& entry : m_VectorBuiltIn )
                {
                    p_factory->m_VectorAll.push_back( entry.second->m_pInternal );
                    p_factory->m_MapAll[ entry.first ] = entry.second->m_pInternal;
                }
            }
            Environment::setEventTriggerFactory( p_factory );
        }
        return p_factory;
    }
}
