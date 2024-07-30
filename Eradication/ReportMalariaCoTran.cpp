
#include "stdafx.h"

#include "ReportMalariaCoTran.h"
#include "MalariaContexts.h"
#include "VectorContexts.h"
#include "IIndividualHuman.h"
#include "INodeContext.h"
#include "NodeEventContext.h"
#include "StrainIdentityMalariaCoTran.h"
#include "MalariaCoTransmissionContexts.h"

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED( ReportMalariaCoTran, ReportMalaria )
        HANDLE_INTERFACE( IIndividualEventObserver )
        HANDLE_ISUPPORTS_VIA( IReport )
    END_QUERY_INTERFACE_DERIVED( ReportMalariaCoTran, ReportMalaria )

    ReportMalariaCoTran::ReportMalariaCoTran()
        : ReportMalaria()
        , m_IsRegisteredCoTran( false )
        , avg_num_cytokines_id()
        , num_transmissions_id()
        , avg_num_transmitted_id()
        , avg_num_acquired_id()
        , new_vector_infs_id()
        , avg_infs_in_new_vector_id()
        , infected_vectors_id()
        , infectious_vectors_id()
        , avg_infs_infected_id()
        , avg_infs_infectious_id()
    {
        avg_num_cytokines_id      = AddChannel( "Avg Num Cytokines"           );
        num_transmissions_id      = AddChannel( "Num Transmissions"           );
        avg_num_transmitted_id    = AddChannel( "Avg Num Infs Trans"          );
        avg_num_acquired_id       = AddChannel( "Avg Num Infs Acq"            );
        new_vector_infs_id        = AddChannel( "New Vector Infections"       );
        avg_infs_in_new_vector_id = AddChannel( "Avg Infs in New Vector Inf"  );
        infected_vectors_id       = AddChannel( "Num Infected Vectors"        );
        infectious_vectors_id     = AddChannel( "Num Infectious Vectors"      );
        avg_infs_infected_id      = AddChannel( "Avg Infs Infected Vectors"   );
        avg_infs_infectious_id    = AddChannel( "Avg Infs Infectious Vectors" );
    }

    void ReportMalariaCoTran::populateSummaryDataUnitsMap( std::map<std::string, std::string> &units_map )
    {
        ReportMalaria::populateSummaryDataUnitsMap( units_map );
        
        units_map[ avg_num_cytokines_id.GetName()      ] = "";
        units_map[ num_transmissions_id.GetName()      ] = "";
        units_map[ avg_num_transmitted_id.GetName()    ] = "";
        units_map[ avg_num_acquired_id.GetName()       ] = "";
        units_map[ new_vector_infs_id.GetName()        ] = "";
        units_map[ avg_infs_in_new_vector_id.GetName() ] = "";
        units_map[ infected_vectors_id.GetName()       ] = "";
        units_map[ infectious_vectors_id.GetName()     ] = "";
        units_map[ avg_infs_infected_id.GetName()      ] = "";
        units_map[ avg_infs_infectious_id.GetName()    ] = "";
    }

    void ReportMalariaCoTran::postProcessAccumulatedData()
    {
        ReportMalaria::postProcessAccumulatedData();

        normalizeChannel( avg_num_cytokines_id.GetName(),      stat_pop_id.GetName()          );
        normalizeChannel( avg_num_transmitted_id.GetName(),    num_transmissions_id.GetName() );
        normalizeChannel( avg_num_acquired_id.GetName(),       num_transmissions_id.GetName() );
        normalizeChannel( avg_infs_in_new_vector_id.GetName(), new_vector_infs_id.GetName()   );

        channelDataMap.RemoveChannel( num_transmissions_id.GetName() );
    }

    void ReportMalariaCoTran::LogIndividualData( Kernel::IIndividualHuman* individual )
    {
        ReportMalaria::LogIndividualData( individual );

        IMalariaHumanContext* p_human_malaria = nullptr;
        if( s_OK != individual->QueryInterface( GET_IID( IMalariaHumanContext ), (void**)&p_human_malaria ) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "individual", "IMalariaHumanContext", "IIndividualHuman" );
        }
        IMalariaSusceptibility* p_suscept = p_human_malaria->GetMalariaSusceptibilityContext();
        Accumulate( avg_num_cytokines_id, p_suscept->get_cytokines() );
    }

    void ReportMalariaCoTran::LogNodeData( INodeContext * pNC )
    {
        ReportMalaria::LogNodeData( pNC );

        INodeVector * pNodeVector = NULL;
        if (s_OK != pNC->QueryInterface(GET_IID(INodeVector), (void**)&pNodeVector) )
        {
            throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "pNC", "INodeVector", "INodeContext");
        }
        const VectorPopulationReportingList_t& vector_pop_list = pNodeVector->GetVectorPopulationReporting();
        float num_infected = 0.0;
        float num_infectious = 0.0;
        float num_infs_infected = 0.0;
        float num_infs_infectious = 0.0;
        for( auto vp : vector_pop_list )
        {
            num_infected        += vp->getCount( VectorStateEnum::STATE_INFECTED );
            num_infectious      += vp->getCount( VectorStateEnum::STATE_INFECTIOUS );
            num_infs_infected   += vp->getNumInfsCount( VectorStateEnum::STATE_INFECTED );
            num_infs_infectious += vp->getNumInfsCount( VectorStateEnum::STATE_INFECTIOUS );
        }
        float avg_infected = 0.0;
        if( num_infected > 0.0 )
        {
            avg_infected = num_infs_infected / num_infected;
        }
        float avg_infectious = 0.0;
        if( num_infectious > 0.0 )
        {
            avg_infectious = num_infs_infectious / num_infectious;
        }
        Accumulate( infected_vectors_id,    num_infected   );
        Accumulate( infectious_vectors_id,  num_infectious );
        Accumulate( avg_infs_infected_id,   avg_infected   );
        Accumulate( avg_infs_infectious_id, avg_infectious );
    }

    void ReportMalariaCoTran::UpdateEventRegistration( float currentTime, 
                                                       float dt, 
                                                       std::vector<INodeEventContext*>& rNodeEventContextList,
                                                       ISimulationEventContext* pSimEventContext )
    {
        ReportMalaria::UpdateEventRegistration( currentTime, dt, rNodeEventContextList, pSimEventContext );

        if( !m_IsRegisteredCoTran )
        {
            for( auto pNEC : rNodeEventContextList )
            {
                IIndividualEventBroadcaster* broadcaster = pNEC->GetIndividualEventBroadcaster();
                broadcaster->RegisterObserver( this, EventTrigger::VectorToHumanTransmission );
                broadcaster->RegisterObserver( this, EventTrigger::HumanToVectorTransmission );
            }
            m_IsRegisteredCoTran = true;
        }
    }

    void ReportMalariaCoTran::BeginTimestep()
    {
        ReportMalaria::BeginTimestep();

        Accumulate( num_transmissions_id,      0 );
        Accumulate( avg_num_transmitted_id,    0 );
        Accumulate( avg_num_acquired_id,       0 );
        Accumulate( new_vector_infs_id,        0 );
        Accumulate( avg_infs_in_new_vector_id, 0 );
    }

    bool ReportMalariaCoTran::notifyOnEvent( IIndividualHumanEventContext *pHuman, const EventTrigger& trigger )
    {
        ReportMalaria::notifyOnEvent( pHuman, trigger );

        IMalariaHumanReport* p_human_report = nullptr;
        if( pHuman->QueryInterface(GET_IID(IMalariaHumanReport), (void **)&p_human_report) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__,
                                           "context", "IMalariaHumanReport", "IIndividualHumanEventContext" );
        }
        const StrainIdentityMalariaCoTran& r_si_malaria = p_human_report->GetRecentTransmissionInfo();

        if( trigger == EventTrigger::VectorToHumanTransmission )
        {
            Accumulate( num_transmissions_id, 1 );
            Accumulate( avg_num_transmitted_id, r_si_malaria.GetTransmittedInfections().size() );
            Accumulate( avg_num_acquired_id,    r_si_malaria.GetAcquiredInfections().size() );
        }
        else if( trigger == EventTrigger::HumanToVectorTransmission )
        {
            Accumulate( new_vector_infs_id, 1 );
            Accumulate( avg_infs_in_new_vector_id, r_si_malaria.GetTransmittedInfections().size() );
        }
        return true;
    }
}
