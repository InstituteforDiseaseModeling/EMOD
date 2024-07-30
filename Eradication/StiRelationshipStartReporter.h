
#pragma once

#include "BaseTextReport.h"
#include "ISimulation.h"
#include "RelationshipReporting.h"
#include "ReportFilterRelationship.h"
#include "Properties.h"

namespace Kernel
{
    struct  IIndividualHumanSTI ;
    struct  IIndividualHumanEventContext ;

    class StiRelationshipStartReporter : public BaseTextReport
    {
        GET_SCHEMA_STATIC_WRAPPER( StiRelationshipStartReporter )
    public:
        static IReport* Create(ISimulation* simulation);

        // IReport
        virtual bool Configure( const Configuration* inputJson ) override;
        virtual void Initialize( unsigned int nrmSize ) override;
        virtual void CheckForValidNodeIDs( const std::vector<ExternalNodeId_t>& demographicNodeIds ) override;
        virtual void BeginTimestep() override;
        virtual void EndTimestep( float currentTime, float dt ) override;

    protected:
        StiRelationshipStartReporter( ISimulation* simulation = nullptr );
        virtual ~StiRelationshipStartReporter();

        // BaseTextReport
        virtual std::string GetHeader() const override;

        void onNewNode(INodeContext* node);
        void onNewRelationship(IRelationship* relationship);

        virtual void ClearData();
        virtual void CollectOtherData( unsigned int relationshipID,
                                       IIndividualHumanSTI* pPartnerA,
                                       IIndividualHumanSTI* pPartnerB ) {} ;
        virtual std::string GetOtherData( unsigned int relationshipID ) { return ""; };
        void ExtractPartnerData( IIndividualHumanEventContext* pHuman,
                                 IIndividualHumanSTI* pHumanSTI,
                                 ParticipantInfo& rParticipant );
        void AddPartnerColumnHeaders( const char* pPartnerLabel, std::stringstream& rHeader ) const;
        void WriteParticipant( ParticipantInfo& rParticipant );

        ISimulation* simulation;
        std::vector<RelationshipStartInfo> report_data;
        std::vector<std::string> m_IPKeyNames;
        std::vector<IPKey> m_IPKeys;
        bool m_IncludeOther;
        ReportFilterRelationship m_ReportFilter;
    };
}
