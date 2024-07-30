
#include "stdafx.h"
#include "ReportMalaria.h" // for base class
#include "MalariaContexts.h" // for base class
#include "IIndividualHuman.h"
#include "INodeContext.h"
#include "ReportUtilitiesMalaria.h"
#include "IndividualEventContext.h"
#include "NodeEventContext.h"
#include <numeric> //for std::accumulate

namespace Kernel
{
    GET_SCHEMA_STATIC_WRAPPER_IMPL(ReportMalaria,ReportMalaria)

    BEGIN_QUERY_INTERFACE_DERIVED( ReportMalaria, Report )
        HANDLE_INTERFACE( IReportMalariaDiagnostics )
        HANDLE_ISUPPORTS_VIA( IReport )
    END_QUERY_INTERFACE_DERIVED( ReportMalaria, Report )

    ReportMalaria::ReportMalaria()
        : ReportVector()
        , num_people_infected_id()
        , avg_num_inf_id()
        , avg_infection_dur_id()
        , major_variant_fraction_id()
        , mean_parasitemia_id()
        , new_clinical_cases_id()
        , new_severe_cases_id()
        , m_PrevalenceByDiagnosticChannelIDs()
        , m_DetectionThresholds()
        , m_Detected()
        , m_MeanParasitemia(0.0)
        , m_NumInfectionsClearedQueue()
        , m_InfectionClearedDurationQueue()
        , m_Include30DayAvg(true)
    {
        num_people_infected_id    = AddChannel( "NumPeopleInfected"             );// temporary - not in output
        avg_num_inf_id            = AddChannel( "Avg Num Infections"            );
        major_variant_fraction_id = AddChannel( "Variant Fraction-PfEMP1 Major" );
        mean_parasitemia_id       = AddChannel( "Mean Parasitemia"              );
        new_clinical_cases_id     = AddChannel( "New Clinical Cases"            );
        new_severe_cases_id       = AddChannel( "New Severe Cases"              );

        m_PrevalenceByDiagnosticChannelIDs.push_back( AddChannel( "Blood Smear Parasite Prevalence"   ) );
        m_PrevalenceByDiagnosticChannelIDs.push_back( AddChannel( "Blood Smear Gametocyte Prevalence" ) );
        m_PrevalenceByDiagnosticChannelIDs.push_back( AddChannel( "PCR Parasite Prevalence"           ) );
        m_PrevalenceByDiagnosticChannelIDs.push_back( AddChannel( "PCR Gametocyte Prevalence"         ) );
        m_PrevalenceByDiagnosticChannelIDs.push_back( AddChannel( "PfHRP2 Prevalence"                 ) );
        m_PrevalenceByDiagnosticChannelIDs.push_back( AddChannel( "True Prevalence"                   ) );
        m_PrevalenceByDiagnosticChannelIDs.push_back( AddChannel( "Fever Prevalence"                  ) );

        release_assert( MalariaDiagnosticType::pairs::count() == m_PrevalenceByDiagnosticChannelIDs.size() );

        m_Detected.resize( MalariaDiagnosticType::pairs::count(), 0.0 );
    }

    bool ReportMalaria::Configure( const Configuration * inputJson )
    {
        initConfigTypeMap( GetParameterNameFor30DayAvg(),
                           &m_Include30DayAvg,
                           GetDescTextFor30DayAvg(),
                           true,
                           GetDependsOnFor30DayAvg() );

        bool ret = ReportVector::Configure( inputJson );
        if( ret && !JsonConfigurable::_dryrun )
        {
            if( m_Include30DayAvg )
            {
                m_EventTriggerList.push_back( EventTrigger::InfectionCleared );
                avg_infection_dur_id = AddChannel( "30-day Avg Infection Duration" );
            }
        }
        return ret;
    }

    const char* ReportMalaria::GetParameterNameFor30DayAvg() const
    {
        return "Inset_Chart_Reporting_Include_30Day_Avg_Infection_Duration";
    }

    const char* ReportMalaria::GetDescTextFor30DayAvg() const
    {
        return Inset_Chart_Reporting_Include_30Day_Avg_Infection_Duration_DESC_TEXT;
    }

    const char* ReportMalaria::GetDependsOnFor30DayAvg() const
    {
        return "Enable_Default_Reporting";
    }

    void ReportMalaria::SetDetectionThresholds( const std::vector<float>& rDetectionThresholds )
    {
        m_DetectionThresholds = rDetectionThresholds;
    }

    void
    ReportMalaria::populateSummaryDataUnitsMap(
        std::map<std::string, std::string> &units_map
    )
    {
        ReportVector::populateSummaryDataUnitsMap(units_map);
        
        // Additional malaria channels
        for( auto& r_pbd_id : m_PrevalenceByDiagnosticChannelIDs )
        {
            units_map[ r_pbd_id.GetName()] = "% Detected Infected";
        }
        if( m_Include30DayAvg )
        {
            units_map[ avg_infection_dur_id.GetName() ] = "";
        }
        units_map[ num_people_infected_id.GetName()    ] = "";
        units_map[ avg_num_inf_id.GetName()            ] = "";
        units_map[ major_variant_fraction_id.GetName() ] = "";
        units_map[ mean_parasitemia_id.GetName()       ] = "Geo. mean parasites/microliter";
        units_map[ new_clinical_cases_id.GetName()     ] = "";
        units_map[ new_severe_cases_id.GetName()       ] = "";
    }

    void
    ReportMalaria::postProcessAccumulatedData()
    {
        ReportVector::postProcessAccumulatedData();

        // make sure to normalize Mean Parasitemia BEFORE Parasite Prevalence, then it is exponentiated
        normalizeChannel( mean_parasitemia_id.GetName(),
                          m_PrevalenceByDiagnosticChannelIDs[MalariaDiagnosticType::BLOOD_SMEAR_PARASITES].GetName() );
        channelDataMap.ExponentialValues( mean_parasitemia_id.GetName() );

        // now normalize rest of channels
        for( auto& r_pbd_id : m_PrevalenceByDiagnosticChannelIDs )
        {
            normalizeChannel( r_pbd_id.GetName(), Report::stat_pop_id.GetName() );
        }
        normalizeChannel( major_variant_fraction_id.GetName(), Report::stat_pop_id.GetName());

        normalizeChannel( avg_num_inf_id.GetName(), num_people_infected_id.GetName() );
        channelDataMap.RemoveChannel( num_people_infected_id.GetName() );
    }

    void ReportMalaria::LogIndividualData( Kernel::IIndividualHuman* individual )
    {
        ReportVector::LogIndividualData( individual );

        if( individual->GetInfections().size() > 0 )
        {
            Accumulate( num_people_infected_id, 1);

            Accumulate( avg_num_inf_id, individual->GetInfections().size() );
        }

        release_assert( m_DetectionThresholds.size() == m_PrevalenceByDiagnosticChannelIDs.size() );

        ReportUtilitiesMalaria::LogIndividualMalariaInfectionAssessment( individual,
                                                                         m_DetectionThresholds,
                                                                         m_Detected,
                                                                         m_MeanParasitemia );

        const IMalariaSusceptibility* p_malaria_sus = nullptr;
        if( individual->GetSusceptibilityContext()->QueryInterface( GET_IID(IMalariaSusceptibility), (void**)&p_malaria_sus ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, 
                                           "individual->GetSusceptibilityContext()", "IMalariaSusceptibility", "ISusceptibilityContext" );
        }
        
        Accumulate( major_variant_fraction_id, p_malaria_sus->get_fraction_of_variants_with_antibodies( MalariaAntibodyType::PfEMP1_major ) );
    }

    void
    ReportMalaria::LogNodeData(
        INodeContext * pNC
    )
    {
        ReportVector::LogNodeData( pNC );

        for( int i = 0; i < m_PrevalenceByDiagnosticChannelIDs.size(); ++i )
        {
            Accumulate( m_PrevalenceByDiagnosticChannelIDs[ i ], m_Detected[ i ] );
            m_Detected[ i ] = 0.0;
        }

        Accumulate( mean_parasitemia_id, m_MeanParasitemia );
        m_MeanParasitemia = 0.0;

        const INodeMalaria* pMalariaNode = nullptr;
        if( pNC->QueryInterface( GET_IID(INodeMalaria), (void**)&pMalariaNode ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "pNC", "INodeMalaria", "INodeContext" );
        }

        Accumulate( new_clinical_cases_id, pMalariaNode->GetNewClinicalCases() );
        Accumulate( new_severe_cases_id,   pMalariaNode->GetNewSevereCases()   );
    }

    void ReportMalaria::BeginTimestep()
    {
        ReportVector::BeginTimestep();

        Accumulate( num_people_infected_id, 0 );
        Accumulate( avg_num_inf_id, 0 );

        if( m_Include30DayAvg )
        {
            m_NumInfectionsClearedQueue.push_back( 0 );
            m_InfectionClearedDurationQueue.push_back( 0.0 );
            while( m_NumInfectionsClearedQueue.size() > 30 )
            {
                m_NumInfectionsClearedQueue.pop_front();
                m_InfectionClearedDurationQueue.pop_front();
            }
        }
    }

    void ReportMalaria::EndTimestep( float currentTime, float dt )
    {
        if( m_Include30DayAvg )
        {
            uint32_t num_cleared = std::accumulate( m_NumInfectionsClearedQueue.begin(), m_NumInfectionsClearedQueue.end(), uint32_t(0) );
            float sum_duration = std::accumulate( m_InfectionClearedDurationQueue.begin(), m_InfectionClearedDurationQueue.end(), float( 0.0 ) );
            float avg_duration = 0.0;
            if( num_cleared > 0 )
            {
                avg_duration = sum_duration / float( num_cleared );
            }

            Accumulate( avg_infection_dur_id, avg_duration );
        }
        ReportVector::EndTimestep( currentTime, dt );
    }

    bool ReportMalaria::notifyOnEvent( IIndividualHumanEventContext *pEntity, const EventTrigger& trigger )
    {
        ReportVector::notifyOnEvent( pEntity, trigger );

        if( m_Include30DayAvg && (trigger == EventTrigger::InfectionCleared) )
        {
            const IIndividualHuman* individual = pEntity->GetIndividualHumanConst();
            bool found = false;
            for( auto p_inf : individual->GetInfections() )
            {
                if( p_inf->GetStateChange() == InfectionStateChange::Cleared )
                {
                    found = true;
                    m_NumInfectionsClearedQueue.back() += 1;
                    m_InfectionClearedDurationQueue.back() += p_inf->GetDuration();
                    break;
                }
            }
            release_assert( found );
        }
        return true;
    }
}
