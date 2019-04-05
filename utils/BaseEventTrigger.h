/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "Configure.h"

namespace Kernel
{
    ENUM_DEFINE( EventType,
                 ENUM_VALUE_SPEC( INDIVIDUAL,  0 )
                 ENUM_VALUE_SPEC( NODE,        1 )
                 ENUM_VALUE_SPEC( COORDINATOR, 2 ) )

    // This class is an internal implementation is not meant to be used
    // outside the EventTrigger set of classes.
    class EventTriggerInternal
    {
    public:
        std::string m_Name; // string name of the event
        int m_Index;        // an index/id that is used by the class that is responding to events

        EventTriggerInternal( const std::string& rName, int index )
            : m_Name( rName )
            , m_Index( index )
        {
        }
    };

    class EventTrigger;
    class EventTriggerNode;
    class EventTriggerCoordinator;
    template<class Trig, class Fact> class BaseEventTrigger;
    template<class Trig, class Fact> class BaseEventTriggerFactory;

    // BaseEventTrigger is a template class that is used to identify a specific event.
    // These triggers could be things like NewInfection or Birth.
    // Triggers can be built-in or user defined.
    // Much of this implementation is done to reduce string copying and comparisons.
    template<class Trigger, class Factory>
    class BaseEventTrigger
    {
    public:
        Trigger& operator=( const Trigger& rTrigger );
        Trigger& operator=( const std::string& rTriggerStr );

        bool operator==( const Trigger& rThat ) const;
        bool operator!=( const Trigger& rThat ) const;

        // Returns true if the trigger has not been intialized
        bool IsUninitialized() const;

        // Returns the string value of the event
        const std::string& ToString() const;
        const char* c_str() const;

        // Returns the index/unique integer id of the event.  This is intended to be used
        // by NodeEventContextHost so that it can use a vector and not map.
        int GetIndex() const;

        static void serialize( Kernel::IArchive& ar, Trigger& obj );

    protected:
        template<class Trig, class Fact> friend class Kernel::BaseEventTriggerFactory;

        BaseEventTrigger();
        BaseEventTrigger( const std::string &init_str );
        BaseEventTrigger( const char *init_str );
        BaseEventTrigger( EventTriggerInternal* peti );
        ~BaseEventTrigger();


        EventTriggerInternal* m_pInternal;

    private:
        // Don't allow conversion from the subclasses to the base class
        BaseEventTrigger( const EventTrigger& );
        BaseEventTrigger( const EventTriggerNode& );
        BaseEventTrigger( const EventTriggerCoordinator& );
    };

    // Since these triggers need to be unique, we need a factory class to manage the
    // collection of triggers.  This class also is used to combine the ideas of
    // built-in and user-defined events.
    template<class Trigger, class Factory>
    class BaseEventTriggerFactory : public JsonConfigurable
    {
    public:
        ~BaseEventTriggerFactory();

        static const char* CONSTRAINT_SCHEMA_STRING;

        // static methods
        static Factory* GetInstance();
        static void DeleteInstance();
        static void SetBuiltIn();

        virtual bool Configure( const Configuration* inputJson ) override;

        // ISupports
        virtual QueryResult QueryInterface( iid_t iid, void** pinstance ) { return e_NOINTERFACE; }
        virtual int32_t AddRef() { return 1; }
        virtual int32_t Release() { return 1; }

        // Other
        int GetNumEventTriggers() const;
        std::vector<Trigger> GetAllEventTriggers();
        bool IsValidEvent( const std::string& candidateEvent ) const;
        const std::string& GetEventTriggerName( int eventIndex ) const;

        Trigger CreateUserEventTrigger( const std::string& name );
        Trigger CreateTrigger( const std::string& rParamName, const std::string& rTriggerName );
        std::vector<Trigger> CreateTriggerList( const std::string& rParamName, const std::vector<std::string>& rTriggerNameList );

        const std::vector<std::string>& GetBuiltInNames();

    protected:
        template<class Trig, class Fact> friend class Kernel::BaseEventTrigger;
        friend class EventTrigger;
        friend class EventTriggerNode;
        friend class EventTriggerCoordinator;
        friend class EventTriggerInternal;

        BaseEventTriggerFactory();
        EventTriggerInternal* GetEventTriggerInternal( const std::string& str );
        EventTriggerInternal* GetEventTriggerInternal( const char* str );
        EventTriggerInternal* Add( const std::string& str );

        std::vector<EventTriggerInternal*> m_VectorUser;
        std::vector<EventTriggerInternal*> m_VectorAll;
        std::map<std::string, EventTriggerInternal*> m_MapAll;
        std::vector<std::string> m_BuiltInNames;


        static const Trigger& CreateBuiltInEventTrigger( const char* name );

        static const EventType::Enum EVENT_TYPE;
        static const char* USER_EVENTS_PARAMETER_NAME;
        static const char* USER_EVENTS_PARAMETER_DESC;
        static std::vector<std::pair<std::string, Trigger*>> m_VectorBuiltIn;
    };
}
