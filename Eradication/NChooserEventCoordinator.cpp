/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "NChooserEventCoordinator.h"
#include "InterventionFactory.h"
#include "Environment.h"
#include "RANDOM.h"
#include "IIndividualHumanSTI.h"
#include "IIndividualHumanHIV.h"
#include "IHIVInterventionsContainer.h"

static const char * _module = "NChooserEventCoordinator";

namespace Kernel
{
    // ------------------------------------------------------------------------
    // --- AgeRange
    // ------------------------------------------------------------------------
    BEGIN_QUERY_INTERFACE_BODY(AgeRange)
    END_QUERY_INTERFACE_BODY(AgeRange)


    AgeRange::AgeRange( float minYears, float maxYears )
    : JsonConfigurable()
    , m_MinYears(minYears)
    , m_MaxYears(maxYears)
    {
    }

    AgeRange::~AgeRange()
    {
    }

    bool AgeRange::operator<( const AgeRange& rThat ) const
    {
        return (this->m_MinYears < rThat.m_MinYears);
    }

    bool AgeRange::Configure( const Configuration * inputJson )
    {
        initConfigTypeMap("Min", &m_MinYears, NC_AR_Min_DESC_TEXT, 0.0, MAX_HUMAN_AGE, 0.0 );
        initConfigTypeMap("Max", &m_MaxYears, NC_AR_Max_DESC_TEXT, 0.0, MAX_HUMAN_AGE, MAX_HUMAN_AGE );

        bool ret = JsonConfigurable::Configure( inputJson );
        if( ret && !JsonConfigurable::_dryrun )
        {
            if( m_MinYears >= m_MaxYears )
            {
                throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, "Min", m_MinYears, "Max", m_MaxYears, "Min must be < Max" );
            }
        }
        return ret;
    }

    float AgeRange::GetMinYear() const
    {
        return m_MinYears;
    }

    float AgeRange::GetMaxYear() const
    {
        return m_MaxYears;
    }

    bool AgeRange::IsInRange( float ageYears ) const
    {
        return ((m_MinYears <= ageYears) && (ageYears < m_MaxYears));
    }

    // ------------------------------------------------------------------------
    // --- AgeRangeList
    // ------------------------------------------------------------------------

    BEGIN_QUERY_INTERFACE_BODY(AgeRangeList)
    END_QUERY_INTERFACE_BODY(AgeRangeList)

    AgeRangeList::AgeRangeList()
    : JsonConfigurable()
    , m_AgeRanges()
    {
    }

    AgeRangeList::~AgeRangeList()
    {
    }

    const AgeRange& AgeRangeList::operator[]( int index ) const
    {
        release_assert( (0 <= index) && (index < m_AgeRanges.size()) );
        return m_AgeRanges[index];
    }

    void AgeRangeList::ConfigureFromJsonAndKey( const Configuration* inputJson, const std::string& key )
    {
        // Temporary object created so we can 'operate' on json with the desired tools
        auto p_config = Configuration::CopyFromElement( (*inputJson)[key] );

        const auto& json_array = json_cast<const json::Array&>( (*p_config) );
        for( auto data = json_array.Begin(); data != json_array.End(); ++data )
        {
            Configuration* p_element_config = Configuration::CopyFromElement( *data );

            AgeRange ar;
            ar.Configure( p_element_config );

            Add( ar );

            delete p_element_config ;
        }
        delete p_config;
    }

    json::QuickBuilder AgeRangeList::GetSchema()
    {
        AgeRange ar;
        if( JsonConfigurable::_dryrun )
        {
            ar.Configure( nullptr );
        }

        json::QuickBuilder schema( GetSchemaBase() );
        auto tn = JsonConfigurable::_typename_label();
        auto ts = JsonConfigurable::_typeschema_label();
        schema[ tn ] = json::String( "idmType:AgeRangeList" );

        schema[ts] = json::Object();
        schema[ts]["<AgeRange Value>"] = ar.GetSchema().As<Object>();

        return schema;
    }

    void AgeRangeList::Add( const AgeRange& rar )
    {
        m_AgeRanges.push_back( rar );
        std::sort( m_AgeRanges.begin(), m_AgeRanges.end() );
    }

    int AgeRangeList::Size() const
    {
        return m_AgeRanges.size();
    }

    void AgeRangeList::CheckForOverlap()
    {
        if( m_AgeRanges.size() == 0 )
        {
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, "'Age_Ranges_Years' cannot have zero elements." );
        }

        float prev_min = m_AgeRanges[0].GetMinYear();
        float prev_max = m_AgeRanges[0].GetMaxYear();
        for( int i = 1 ; i < m_AgeRanges.size() ; ++i )
        {
            float this_min = m_AgeRanges[i].GetMinYear();
            float this_max = m_AgeRanges[i].GetMaxYear();

            if( prev_max > this_min )
            {
                std::stringstream ss;
                ss << "'Age_Ranges_Years' cannot have age ranges that overlap.  ";
                ss << "(" << prev_min << ", " << prev_max << ") vs (" << this_min << ", " << this_max << ")";
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }
        }
    }

    // ------------------------------------------------------------------------
    // --- TargetedByAgeAndGender
    // ------------------------------------------------------------------------

    TargetedByAgeAndGender::TargetedByAgeAndGender( const AgeRange& rar, 
                                                    Gender::Enum gender, 
                                                    int numTargeted, 
                                                    int numTimeSteps, 
                                                    int initialTimeStep )
    : m_AgeRange( rar )
    , m_Gender( gender )
    , m_NumTargeted( numTargeted )
    , m_TimeStep( initialTimeStep )
    , m_NumTargetedPerTimeStep()
    , m_QualifyingIndividuals()
    {
        release_assert( initialTimeStep < numTimeSteps );

        int avg_tgt = m_NumTargeted / numTimeSteps;
        int remainder = m_NumTargeted - (avg_tgt * numTimeSteps);

        for( int i = 0 ; i < numTimeSteps; ++i )
        {
            m_NumTargetedPerTimeStep.push_back( avg_tgt );
        }
        for( int i = 0 ; i < remainder; ++i )
        {
            m_NumTargetedPerTimeStep[i] += 1;
        }
    }

    TargetedByAgeAndGender::~TargetedByAgeAndGender()
    {
    }

    void TargetedByAgeAndGender::IncrementNextNumTargets()
    {
        ++m_TimeStep;
    }

    int TargetedByAgeAndGender::GetNumTargeted() const
    {
        release_assert( m_TimeStep < m_NumTargetedPerTimeStep.size() );
        return m_NumTargetedPerTimeStep[ m_TimeStep ];
    }

    void TargetedByAgeAndGender::FindQualifyingIndividuals( INodeEventContext* pNEC, 
                                                            const DiseaseQualifications& rDisease,
                                                            PropertyRestrictions& rPropertyRestrictions )
    {
        m_QualifyingIndividuals.clear();
        m_QualifyingIndividuals.reserve( pNEC->GetIndividualHumanCount() );

        INodeEventContext::individual_visit_function_t fn = 
            [ this, &rDisease, &rPropertyRestrictions ](IIndividualHumanEventContext *ihec)
        {
            if( !m_AgeRange.IsInRange( ihec->GetAge()/DAYSPERYEAR ) ) return;

            if( (m_Gender != Gender::COUNT) && (m_Gender != ihec->GetGender()) ) return;

            if( !rPropertyRestrictions.Qualifies( ihec ) ) return;

            if( !rDisease.Qualifies( ihec ) ) return;

            m_QualifyingIndividuals.push_back( ihec );
        };

        pNEC->VisitIndividuals( fn );
    }

    std::vector<IIndividualHumanEventContext*> TargetedByAgeAndGender::SelectIndividuals()
    {
        if( GetNumTargeted() >= m_QualifyingIndividuals.size() )
        {
            return m_QualifyingIndividuals;
        }

        // ----------------------------------------------------------------------------------
        // --- Robert Floyd's Algorithm for Sampling without Replacement
        // --- http://www.nowherenearithaca.com/2013/05/robert-floyds-tiny-and-beautiful.html
        // ----------------------------------------------------------------------------------

        std::set<int> selected_indexes;
        uint32_t N = m_QualifyingIndividuals.size();
        uint32_t M = GetNumTargeted();

        for( uint32_t j = (N - M) ; j < N ; j++ )
        {
            uint32_t index = EnvPtr->RNG->uniformZeroToN( j+1 );
            release_assert( index < N );
            if( selected_indexes.find( index ) == selected_indexes.end() )
            {
                selected_indexes.insert( index );
            }
            else
            {
                selected_indexes.insert( j );
            }
        }

        std::vector<IIndividualHumanEventContext*> selected_individuals;
        for( auto index : selected_indexes )
        {
            selected_individuals.push_back( m_QualifyingIndividuals[ index ] );
        }

        return selected_individuals;
    }

    bool TargetedByAgeAndGender::IsFinished() const
    {
        if( (m_TimeStep != 0) && (m_TimeStep >= (m_NumTargetedPerTimeStep.size()-1)) )
        {
            return true;
        }
        else
        {
            return false;
        }
    }


    // ------------------------------------------------------------------------
    // --- TargetedDistribution
    // ------------------------------------------------------------------------

    BEGIN_QUERY_INTERFACE_BODY(TargetedDistribution)
    END_QUERY_INTERFACE_BODY(TargetedDistribution)

    bool TargetedDistribution::LeftLessThanRight( const TargetedDistribution* pLeft, const TargetedDistribution* pRight )
    {
        return *pLeft < *pRight;
    }

    TargetedDistribution::TargetedDistribution( NChooserObjectFactory* pObjectFactory )
    : JsonConfigurable()
    , m_pObjectFactory( pObjectFactory )
    , m_pDiseaseQualifications( nullptr )
    , m_StartDay(0.0)
    , m_EndDay(FLT_MAX)
    , m_PropertyRestrictions()
    , m_AgeRangeList()
    , m_NumTargeted()
    , m_NumTargetedMales()
    , m_NumTargetedFemales()
    {
    }

    TargetedDistribution::~TargetedDistribution()
    {
    }

    bool TargetedDistribution::operator<( const TargetedDistribution& rThat ) const
    {
        return (this->m_StartDay < rThat.m_StartDay);
    }

    void TargetedDistribution::AddTimeConfiguration()
    {
        initConfigTypeMap("Start_Day", &m_StartDay, NC_TD_Start_Day_DESC_TEXT, 0.0,  FLT_MAX, 0.0 );
        initConfigTypeMap("End_Day",   &m_EndDay,   NC_TD_End_Day_DESC_TEXT,   0.0,  FLT_MAX, FLT_MAX );
    }

    void TargetedDistribution::CheckTimePeriod() const
    {
        if( m_StartDay >= m_EndDay )
        {
            throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, 
                "Start_Day", m_StartDay,
                "End_Day", m_EndDay,
                "Start_Day must be < End_Day" );
        }
    }

    bool TargetedDistribution::Configure( const Configuration * inputJson )
    {
        AddTimeConfiguration();
        AddDiseaseConfiguration();

        initConfigTypeMap("Num_Targeted",         &m_NumTargeted,        NC_TD_Num_Targeted_DESC_TEXT,              0, INT_MAX,      0 );
        initConfigTypeMap("Num_Targeted_Males",   &m_NumTargetedMales,   NC_TD_Num_Targeted_Males_DESC_TEXT,        0, INT_MAX,      0 );
        initConfigTypeMap("Num_Targeted_Females", &m_NumTargetedFemales, NC_TD_Num_Targeted_Females_DESC_TEXT,      0, INT_MAX,      0 );

        initConfigComplexType("Age_Ranges_Years",                  &m_AgeRangeList,         NC_TD_Age_Ranges_Years_DESC_TEXT );
        initConfigComplexType("Property_Restrictions_Within_Node", &m_PropertyRestrictions, NC_TD_Property_Restrictions_Within_Node_DESC_TEXT );

        bool ret = JsonConfigurable::Configure( inputJson );
        if( ret && !JsonConfigurable::_dryrun )
        {
            CheckTimePeriod();

            if( (m_NumTargeted.size() > 0) && ((m_NumTargetedMales.size() > 0) || (m_NumTargetedFemales.size() > 0)) )
            {
                std::stringstream ss;
                ss << "The number of elements in 'Num_Targeted' is " << m_NumTargeted.size() << ".\n";
                ss << "The number of elements in 'Num_Targeted_Males' is " << m_NumTargetedMales.size() << ".\n";
                ss << "The number of elements in 'Num_Targeted_Females' is " << m_NumTargetedFemales.size() << ".\n";
                ss << "If using Num_Targeted, then Num_Targeted_Males and Num_Targeted_Females must be empty.\nIf using Num_Targeted_Males and Num_Targeted_Females, then Num_Targeted must be empty.";
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }

            if( (m_NumTargeted.size() > 0) && (m_AgeRangeList.Size() != m_NumTargeted.size()) )
            {
                std::stringstream ss;
                ss << "The number of elements in 'Num_Targeted'(=" << m_NumTargeted.size() << ") is not the same as 'Age_Ranges_Years'(=" << m_AgeRangeList.Size() << ").\n";
                ss << "'Num_Targeted' and 'Age_Range_Years' must have the same number of elements, but not zero.  There must be one age range for each number targeted.";
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }

            if( ((m_NumTargetedMales.size() > 0) || (m_NumTargetedFemales.size() > 0)) && ((m_AgeRangeList.Size() != m_NumTargetedMales.size()) || (m_AgeRangeList.Size() != m_NumTargetedFemales.size())) )
            {
                std::stringstream ss;
                ss << "The number of elements in 'Num_Targeted_Males' is " << m_NumTargetedMales.size() << ".\n";
                ss << "The number of elements in 'Num_Targeted_Females' is " << m_NumTargetedFemales.size() << ".\n";
                ss << "The number of elements in 'Age_Range_Years' is " << m_AgeRangeList.Size() << ".\n";
                ss << "'Num_Targeted_Males', 'Num_Targeted_Females', and 'Age_Range_Years' must have the same number of elements, but not zero.  There must be one age range for each number targeted.";
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }

            if( (m_AgeRangeList.Size() == 0) && (m_NumTargeted.size() == 0) && (m_NumTargetedMales.size() == 0) && (m_NumTargetedFemales.size() == 0) )
            {
                std::stringstream ss;
                ss << "The arrays 'Age_Range_Years', 'Num_Targeted_Males', and 'Num_Targeted_Females' have zero elements.\n";
                ss << "Num_Targeted_Males, Num_Targeted_Females, and Age_Range_Years must have the same number of elements, but not zero.  There must be one age range for each number targeted.";
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }

            CheckForZeroTargeted();

            CheckDiseaseConfiguration();

            m_AgeRangeList.CheckForOverlap();
        }
        return ret;
    }


    void TargetedDistribution::ScaleTargets( float popScaleFactor )
    {
        if( popScaleFactor == 1.0 )
        {
            return;
        }
        if( m_NumTargeted.size() > 0 )
        {
            int num_total = 0;
            for( int i = 0 ; i < m_NumTargeted.size() ; ++i )
            {
                m_NumTargeted[i] *= popScaleFactor;
                num_total += m_NumTargeted[i];
            }
            if( num_total == 0 )
            {
                std::stringstream msg;
                msg << "The Base_Population_Scale_Factor (" << popScaleFactor << ") has scaled the values of Num_Targets all to zero so won't target anyone.";
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
            }
        }
        else
        {
            int num_total = 0;
            for( int i = 0 ; i < m_NumTargetedMales.size() ; ++i )
            {
                m_NumTargetedMales[i] *= popScaleFactor;
                m_NumTargetedFemales[i] *= popScaleFactor;
                num_total += m_NumTargetedMales[i];
                num_total += m_NumTargetedFemales[i];
            }
            if( num_total == 0 )
            {
                std::stringstream msg;
                msg << "The Base_Population_Scale_Factor (" << popScaleFactor << ") has scaled the values of Num_Targets_Males and Num_Target_Femailes all to zero so won't target anyone.";
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
            }
        }
    }

    void TargetedDistribution::CheckForZeroTargeted()
    {
        if( m_NumTargeted.size() > 0 )
        {
            int total = 0;
            for( auto num : m_NumTargeted )
            {
                total += num;
            }
            if( total == 0 )
            {
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, "'Num_Targeted' has zero values and won't target anyone." );
            }
        }
        else
        {
            release_assert( m_NumTargetedMales.size() == m_NumTargetedFemales.size() );

            int total_males = 0;
            int total_females = 0;
            for( int i = 0 ; i < m_NumTargetedMales.size() ; ++i )
            {
                total_males   += m_NumTargetedMales[i];
                total_females += m_NumTargetedFemales[i];
            }
            if( (total_males + total_females) == 0 )
            {
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, "'Num_Targeted_Males' and 'Num_Targeted_Females' have zero values and won't target anyone." );
            }
        }
    }

    float TargetedDistribution::GetStartInDays() const
    {
        return m_StartDay;
    }

    float TargetedDistribution::GetEndInDays() const
    {
        return m_EndDay;
    }

    float TargetedDistribution::GetCurrentInDays( const IdmDateTime& rDateTime ) const
    {
        return rDateTime.time;
    }

    void TargetedDistribution::CreateAgeAndGenderList( const IdmDateTime& rDateTime, float dt )
    {
        float start_days = GetStartInDays();
        float end_days   = GetEndInDays();
        int num_time_steps = int( (end_days - start_days) / dt );

        float current_days = GetCurrentInDays( rDateTime );
        int current_time_step = int( (current_days - start_days) / dt );

        for( int i = 0 ; i < m_AgeRangeList.Size(); ++i )
        {
            if( m_NumTargeted.size() > 0 )
            {
                if( m_NumTargeted[i] > 0 )
                {
                    TargetedByAgeAndGender* p_tbag = m_pObjectFactory->CreateTargetedByAgeAndGender( m_AgeRangeList[i],
                                                                                                     Gender::COUNT, 
                                                                                                     m_NumTargeted[i],
                                                                                                     num_time_steps,
                                                                                                     current_time_step );
                    m_AgeAndGenderList.push_back( p_tbag );
                }
            }
            else
            {
                if( m_NumTargetedMales[i] > 0 )
                {
                    TargetedByAgeAndGender* p_tbag = m_pObjectFactory->CreateTargetedByAgeAndGender( m_AgeRangeList[i],
                                                                                                     Gender::MALE, 
                                                                                                     m_NumTargetedMales[i],
                                                                                                     num_time_steps,
                                                                                                     current_time_step );
                    m_AgeAndGenderList.push_back( p_tbag );
                }
                if( m_NumTargetedFemales[i] > 0 )
                {
                    TargetedByAgeAndGender* p_tbag = m_pObjectFactory->CreateTargetedByAgeAndGender( m_AgeRangeList[i],
                                                                                                     Gender::FEMALE, 
                                                                                                     m_NumTargetedFemales[i],
                                                                                                     num_time_steps,
                                                                                                     current_time_step );
                    m_AgeAndGenderList.push_back( p_tbag );
                }
            }
        }
        release_assert( m_AgeAndGenderList.size() > 0 );
    }

    void TargetedDistribution::UpdateTargeting( const IdmDateTime& rDateTime, float dt )
    {
        if( m_AgeAndGenderList.size() == 0 )
        {
            CreateAgeAndGenderList( rDateTime, dt );
        }
        else
        {
            for( auto p_ag : m_AgeAndGenderList )
            {
                p_ag->IncrementNextNumTargets();
            }
        }
    }

    std::vector< IIndividualHumanEventContext* >
    TargetedDistribution::DetermineWhoGetsIntervention( const std::vector<INodeEventContext*> nodeList )
    {
        // ---------------------------------------------------------
        // --- Find the individuals for each age and gender
        // --- that meet the demographic restrictions
        // ---------------------------------------------------------
        if( m_pDiseaseQualifications == nullptr )
        {
            m_pDiseaseQualifications = m_pObjectFactory->CreateDiseaseQualifications( this );
            release_assert( m_pDiseaseQualifications );
        }

        for( auto p_ag : m_AgeAndGenderList )
        {
            for( auto p_nec : nodeList )
            {
                p_ag->FindQualifyingIndividuals( p_nec, *m_pDiseaseQualifications, m_PropertyRestrictions );
            }
        }

        // ----------------------------------------------------------------------
        // --- Of those that qualify, pick those that should get the intervention
        // ----------------------------------------------------------------------

        // Determine the total number of individuals that can receive the intervention
        int num_total = 0;
        for( auto p_ag : m_AgeAndGenderList )
        {
            num_total += p_ag->GetNumTargeted();
        }

        // Create the vector to return and allocate space for the individuals
        std::vector< IIndividualHumanEventContext* > distribute_to;
        distribute_to.reserve( num_total );

        for( auto p_ag : m_AgeAndGenderList )
        {
            std::vector< IIndividualHumanEventContext* > selected = p_ag->SelectIndividuals();
            distribute_to.insert( distribute_to.end(), selected.begin(), selected.end() );
        }
        return distribute_to;
    }

    bool TargetedDistribution::IsFinished() const
    {
        if( m_AgeAndGenderList.size() == 0 )
        {
            // haven't started yet
            return false;
        }
        bool is_finished = true;

        for( int i = 0 ; is_finished && (i < m_AgeAndGenderList.size()) ; ++i )
        {
            is_finished = m_AgeAndGenderList[i]->IsFinished();
        }
        return is_finished;
    }

    void TargetedDistribution::CheckOverlaped( const TargetedDistribution& rPrev ) const
    {
        if( rPrev.m_EndDay > this->m_StartDay )
        {
            std::stringstream ss;
            ss << "'Distributions' cannot have time periods that overlap.  ";
            ss << "(" << rPrev.m_StartDay << ", " << rPrev.m_EndDay << ") vs (" << this->m_StartDay << ", " << this->m_EndDay << ")";
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
    }

    bool TargetedDistribution::IsPastStart( const IdmDateTime& rDateTime ) const
    {
        return m_StartDay <= rDateTime.time;
    }

    bool TargetedDistribution::IsPastEnd( const IdmDateTime& rDateTime ) const
    {
        return m_EndDay <= rDateTime.time;
    }

    // ------------------------------------------------------------------------
    // --- TargetedDistributionList
    // ------------------------------------------------------------------------

    BEGIN_QUERY_INTERFACE_BODY(TargetedDistributionList)
    END_QUERY_INTERFACE_BODY(TargetedDistributionList)

    TargetedDistributionList::TargetedDistributionList( NChooserObjectFactory* pObjectFactory )
    : JsonConfigurable()
    , m_pObjectFactory( pObjectFactory )
    , m_CurrentIndex(0)
    , m_pCurrentTargets(nullptr)
    , m_TargetedDistributions()
    {
        release_assert( m_pObjectFactory );
    }

    TargetedDistributionList::~TargetedDistributionList()
    {
        for( auto td : m_TargetedDistributions )
        {
            delete td;
        }
        m_TargetedDistributions.clear();
    }

    void TargetedDistributionList::ConfigureFromJsonAndKey( const Configuration* inputJson, const std::string& key )
    {
        // Temporary object created so we can 'operate' on json with the desired tools
        auto p_config = Configuration::CopyFromElement( (*inputJson)[key] );

        const auto& json_array = json_cast<const json::Array&>( (*p_config) );
        for( auto data = json_array.Begin(); data != json_array.End(); ++data )
        {
            Configuration* p_element_config = Configuration::CopyFromElement( *data );

            TargetedDistribution* p_td = m_pObjectFactory->CreateTargetedDistribution();
            p_td->Configure( p_element_config );

            Add( p_td );

            delete p_element_config ;
        }
        delete p_config;
    }

    json::QuickBuilder TargetedDistributionList::GetSchema()
    {
        TargetedDistribution* p_td = m_pObjectFactory->CreateTargetedDistribution();
        if( JsonConfigurable::_dryrun )
        {
            p_td->Configure( nullptr );
        }

        json::QuickBuilder schema( GetSchemaBase() );
        auto tn = JsonConfigurable::_typename_label();
        auto ts = JsonConfigurable::_typeschema_label();
        schema[ tn ] = json::String( "idmType:TargetedDistributionList" );

        schema[ts] = json::Object();
        schema[ts]["<TargetedDistribution Value>"] = p_td->GetSchema().As<Object>();

        delete p_td;

        return schema;
    }

    void TargetedDistributionList::CheckForOverlap()
    {
        if( m_TargetedDistributions.size() == 0 )
        {
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, "'Distributions' cannot have zero elements." );
        }

        for( int i = 1 ; i < m_TargetedDistributions.size() ; ++i )
        {
            m_TargetedDistributions[i]->CheckOverlaped( *m_TargetedDistributions[i-1] );
        }
    }

    void TargetedDistributionList::Add( TargetedDistribution* ptd )
    {
        m_TargetedDistributions.push_back( ptd );
        std::sort( m_TargetedDistributions.begin(), m_TargetedDistributions.end(), TargetedDistribution::LeftLessThanRight );
    }

    void TargetedDistributionList::UpdateTargeting( const IdmDateTime& rDateTime, float dt )
    {
        // ----------------------------------------------------
        // --- Update where we are in the list of distributions
        // ----------------------------------------------------
        while( (m_CurrentIndex < m_TargetedDistributions.size()) &&
               m_TargetedDistributions[ m_CurrentIndex ]->IsPastEnd( rDateTime ) )
        {
            ++m_CurrentIndex;
        }

        // -----------------------------------------
        // --- Find the current distributino period
        // -----------------------------------------
        m_pCurrentTargets = nullptr;
        if( (m_CurrentIndex < m_TargetedDistributions.size()) &&
            (m_TargetedDistributions[ m_CurrentIndex ]->IsPastStart( rDateTime )) )
        {
            m_pCurrentTargets = m_TargetedDistributions[ m_CurrentIndex ];

            m_pCurrentTargets->UpdateTargeting( rDateTime, dt );
        }
    }

    TargetedDistribution* TargetedDistributionList::GetCurrentTargets()
    {
        return m_pCurrentTargets;
    }

    bool TargetedDistributionList::IsFinished( const IdmDateTime& rDateTime, float dt )
    {
        bool is_finished = false;
        while( (m_CurrentIndex < m_TargetedDistributions.size()) &&
               ( (m_TargetedDistributions[ m_CurrentIndex ]->IsPastEnd( rDateTime )) ||
                  m_TargetedDistributions[ m_CurrentIndex ]->IsFinished() ) )
        {
            ++m_CurrentIndex;
        }

        if( m_CurrentIndex >= m_TargetedDistributions.size() )
        {
            is_finished = true;
        }
        return is_finished;
    }

    void TargetedDistributionList::ScaleTargets( float popScaleFactor )
    {
        for( auto p_td : m_TargetedDistributions )
        {
            p_td->ScaleTargets( popScaleFactor );
        }
    }

    // ------------------------------------------------------------------------
    // --- NChooserObjectFactory
    // ------------------------------------------------------------------------

    NChooserObjectFactory::NChooserObjectFactory()
    {
    }

    NChooserObjectFactory::~NChooserObjectFactory()
    {
    }

    TargetedDistribution*   NChooserObjectFactory::CreateTargetedDistribution()
    {
        return new TargetedDistribution( this );
    }

    TargetedByAgeAndGender* NChooserObjectFactory::CreateTargetedByAgeAndGender( const AgeRange& rar, 
                                                                                 Gender::Enum gender, 
                                                                                 int numTargeted, 
                                                                                 int numTimeSteps, 
                                                                                 int initialTimeStep )
    {
        return new TargetedByAgeAndGender( rar, gender, numTargeted, numTimeSteps, initialTimeStep );
    }

    DiseaseQualifications*  NChooserObjectFactory::CreateDiseaseQualifications( TargetedDistribution* ptd )
    {
        return new DiseaseQualifications();
    }

    // ------------------------------------------------------------------------
    // --- NChooserEventCoordinator
    // ------------------------------------------------------------------------

    IMPLEMENT_FACTORY_REGISTERED(NChooserEventCoordinator)
    IMPL_QUERY_INTERFACE2(NChooserEventCoordinator, IEventCoordinator, IConfigurable)

    NChooserEventCoordinator::NChooserEventCoordinator()
    : m_Parent( nullptr )
    , m_pObjectFactory( new NChooserObjectFactory() )
    , m_CachedNodes()
    , m_InterventionName()
    , m_pIntervention( nullptr )
    , m_InterventionConfig()
    , m_TargetedDistributionList( m_pObjectFactory )
    , m_DistributionIndex(0)
    , m_IsFinished(false)
    , m_HasBeenScaled(false)
    {
        release_assert( m_pObjectFactory );
    }

    NChooserEventCoordinator::NChooserEventCoordinator( NChooserObjectFactory* pObjectFactory )
    : m_Parent( nullptr )
    , m_pObjectFactory( pObjectFactory )
    , m_CachedNodes()
    , m_InterventionName()
    , m_pIntervention( nullptr )
    , m_InterventionConfig()
    , m_TargetedDistributionList( m_pObjectFactory )
    , m_DistributionIndex(0)
    , m_IsFinished(false)
    , m_HasBeenScaled(false)
    {
        release_assert( m_pObjectFactory );
    }

    NChooserEventCoordinator::~NChooserEventCoordinator()
    {
        delete m_pObjectFactory;
    }


    bool NChooserEventCoordinator::Configure( const Configuration * inputJson )
    {
        initConfigComplexType(     "Distributions",   &m_TargetedDistributionList, NC_Distributions_DESC_TEXT       );
        initConfigComplexType( "Intervention_Config", &m_InterventionConfig,       NC_Intervention_Config_DESC_TEXT );

        bool retValue = JsonConfigurable::Configure( inputJson );

        if( retValue && !JsonConfigurable::_dryrun)
        {
            m_TargetedDistributionList.CheckForOverlap();

            InterventionValidator::ValidateIntervention( m_InterventionConfig._json );
        }

        return retValue;
    }

    void NChooserEventCoordinator::SetContextTo( ISimulationEventContext *isec )
    {
        m_Parent = isec;
    }

    void NChooserEventCoordinator::AddNode( const suids::suid& node_suid )
    {
        INodeEventContext* pNEC = m_Parent->GetNodeEventContext( node_suid );
        if( !m_HasBeenScaled )
        {
            m_TargetedDistributionList.ScaleTargets( pNEC->GetNodeContext()->GetBasePopulationScaleFactor() );
        }
        m_CachedNodes.push_back( pNEC );
    }

    void NChooserEventCoordinator::UpdateInterventionToBeDistributed( const IdmDateTime& rDateTime, float dt )
    {
        if( m_pIntervention == nullptr )
        {
            // intervention class names for informative logging
            std::ostringstream intervention_name;
            intervention_name << std::string( json::QuickInterpreter(m_InterventionConfig._json)["class"].As<json::String>() );
            m_InterventionName = intervention_name.str();

            auto qi_as_config = Configuration::CopyFromElement( m_InterventionConfig._json );
            m_pIntervention = InterventionFactory::getInstance()->CreateIntervention( qi_as_config );
            delete qi_as_config;
            qi_as_config = nullptr;
        }
    }

    void NChooserEventCoordinator::Update( float dt )
    {
        // --------------------------------------------------------------------------------
        // --- Update the intervention to be distributed.  This is probably only done once.
        // --------------------------------------------------------------------------------
        UpdateInterventionToBeDistributed( m_Parent->GetSimulationTime(), dt );

        // --------------------------------------------------------------------------------------
        // --- Update who is to be targeted
        // --- NOTE: We can't determine who gets the intervention because things could change
        // --- before UpdateNodes() is called.  However, we can determine what we are targeting.
        // --------------------------------------------------------------------------------------
        m_TargetedDistributionList.UpdateTargeting( m_Parent->GetSimulationTime(), dt );
    }

    void NChooserEventCoordinator::UpdateNodes( float dt )
    {
        // ---------------------------------------
        // --- Determine who gets the intervention
        // ---------------------------------------
        TargetedDistribution* p_current_targets = m_TargetedDistributionList.GetCurrentTargets();

        if( p_current_targets != nullptr ) // can be nullptr if in between periods
        {
            std::vector< IIndividualHumanEventContext *> individual_list = p_current_targets->DetermineWhoGetsIntervention( m_CachedNodes );

            // ------------------------------------------------------------------------------------------------------
            // --- Distribute the intervention
            // --- WARNING: This does not use the IVisitIndividual interface method of distributing the intervention
            // --- like StandardInterventionDistributionEventCoordinator.  This was done for performance reasons.
            // ------------------------------------------------------------------------------------------------------
            for( auto pIHEC : individual_list )
            {
                IDistributableIntervention *di = m_pIntervention->Clone();
                release_assert(di);
                if (di)
                {
                    ICampaignCostObserver* p_icco = nullptr;
                    if (s_OK != pIHEC->GetNodeEventContext()->QueryInterface(GET_IID(ICampaignCostObserver), (void**)&p_icco))
                    {
                        throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "pIHEC->GetNodeEventContext()", "ICampaignCostObserver", "INodeEventContext" );
                    }

                    di->AddRef();
                    di->Distribute( pIHEC->GetInterventionsContext(), p_icco );
                    di->Release();
                }
            }

            std::stringstream ss;
            ss << "UpdateNodes() gave out " << individual_list.size() << " '" << m_InterventionName.c_str() << "' interventions\n";
            LOG_INFO( ss.str().c_str() );
        }

        // ----------------------------------------------------
        // --- Determine if finished distributing interventions
        // ----------------------------------------------------
        m_IsFinished = m_TargetedDistributionList.IsFinished(  m_Parent->GetSimulationTime(), dt );
    }

    bool NChooserEventCoordinator::IsFinished()
    {
        return m_IsFinished;
    }
}

