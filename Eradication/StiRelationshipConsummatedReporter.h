
#pragma once

#include "BaseTextReportEvents.h"
#include "ISimulation.h"
#include "Properties.h"
#include "IRelationship.h"
#include "ReportFilterRelationship.h"
#include "HIVEnums.h"

namespace Kernel
{
    ENUM_DEFINE(ReportRelationshipType,
        ENUM_VALUE_SPEC( NA         , -1)
        ENUM_VALUE_SPEC( TRANSITORY , RelationshipType::TRANSITORY )
        ENUM_VALUE_SPEC( INFORMAL   , RelationshipType::INFORMAL   )
        ENUM_VALUE_SPEC( MARITAL    , RelationshipType::MARITAL    )
        ENUM_VALUE_SPEC( COMMERCIAL , RelationshipType::COMMERCIAL )
    )

    class CoitalAct;

    struct ConsummatedIndividulatData
    {
        uint32_t human_id;
        int gender;
        double age_years;
        std::vector<std::string> ip_values;
        std::vector<bool> has_intervention;
        bool is_infected;
        uint32_t num_current_relationships;
        bool is_circumcised;
        bool has_sti_coinfection;
        HIVInfectionStage::Enum HIV_infection_stage;
        bool is_on_art;
        std::vector<uint32_t> num_partners_with_IP;

        ConsummatedIndividulatData()
            : human_id( 0 )
            , gender( 0 )
            , age_years( 0.0 )
            , ip_values()
            , has_intervention( false )
            , is_infected( false )
            , num_current_relationships( 0 )
            , is_circumcised( false )
            , has_sti_coinfection( false )
            , HIV_infection_stage( HIVInfectionStage::NOT_INFECTED )
            , is_on_art( false )
            , num_partners_with_IP()
        {
        }
    };

    struct ConsummatedReportData
    {
        float time;
        float year;
        uint32_t node_id;
        uint32_t coital_act_id;
        uint32_t rel_id;
        RelationshipType::Enum rel_type;
        bool is_outside_pfa;
        bool using_condom;
        float risk_mult;
        float prob_tran;
        float prob_acq;
        bool infection_was_transmitted;
        ConsummatedIndividulatData individual_data[ 2 ];

        ConsummatedReportData()
            : time(0.0)
            , year(0.0)
            , node_id(0)
            , coital_act_id(0)
            , rel_id(0)
            , rel_type(RelationshipType::TRANSITORY)
            , is_outside_pfa(false)
            , using_condom( false )
            , risk_mult( 0.0 )
            , prob_tran( 0.0 )
            , prob_acq( 0.0 )
            , infection_was_transmitted( false )
            , individual_data()
        {
        }
    };

    class StiRelationshipConsummatedReporter : public BaseTextReportEvents
    {
        GET_SCHEMA_STATIC_WRAPPER( StiRelationshipConsummatedReporter )
    public:
        static IReport* Create(ISimulation* simulation);

        // IReport
        virtual void BeginTimestep();
        virtual void EndTimestep( float currentTime, float dt );

    protected:
        StiRelationshipConsummatedReporter(ISimulation* simulation=nullptr);
        virtual ~StiRelationshipConsummatedReporter();

        // BaseTextReport
        virtual bool Configure( const Configuration* inputJson ) override;
        virtual void Initialize( unsigned int nrmSize ) override;
        virtual void CheckForValidNodeIDs( const std::vector<ExternalNodeId_t>& demographicNodeIds ) override;
        virtual void UpdateEventRegistration( float currentTime,
                                              float dt,
                                              std::vector<INodeEventContext*>& rNodeEventContextList,
                                              ISimulationEventContext* pSimEventContext ) override;
        virtual bool notifyOnEvent( IIndividualHumanEventContext *context, const EventTrigger& trigger ) override;
        virtual std::string GetHeader() const ;

        void onNewNode(Kernel::INodeContext* node);
        void onCoitalAct( IRelationship* relationship, const CoitalAct& rCoitalAct );
        ConsummatedReportData GatherDataFromRelationship( const IRelationship* pRel, const CoitalAct& rCoitalAct );
        std::string CreateReportLine( const ConsummatedReportData& rData );

        void ExtractData( ConsummatedIndividulatData& rIndividualData, IIndividualHumanSTI* pHumanSTI );

        ISimulation* m_pSimulation;
        std::map<uint32_t,ConsummatedReportData> m_ReportData;
        bool m_IsCollectingData;
        std::vector<InterventionName> m_InterventionNames;
        std::vector<std::string> m_IPKeyNames;
        std::vector<IPKey>       m_IPKeys;
        std::vector<std::string> m_PartnersWithIPKeyValueNames;
        std::vector<IPKeyValue>  m_PartnersWithIPKeyValue;
        ReportFilterRelationship m_ReportFilter;
        ReportRelationshipType::Enum m_RelationshipType;
    };
}
