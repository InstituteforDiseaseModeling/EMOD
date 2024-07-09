
#pragma once

#include <vector>
#include <map>

#include "SqlReport.h"
#include "IReportMalariaDiagnostics.h"
#include "MalariaEnums.h"

namespace Kernel
{
    struct IArchive;

    struct PatientDrugStatus
    {
        PatientDrugStatus();
        ~PatientDrugStatus();

        std::string drug_name;
        float current_efficacy;
        int num_remaining_doses;

        static void serialize( IArchive& ar, PatientDrugStatus& obj );
    };

    struct PatientInfectionMalaria : PatientInfection
    {
        PatientInfectionMalaria();
        ~PatientInfectionMalaria();

        int64_t infected_red_blood_cells;
        int64_t num_mature_gametocytes_female;
        int64_t num_mature_gametocytes_male;

        DECLARE_QUERY_INTERFACE()
        DECLARE_SERIALIZABLE(PatientInfectionMalaria);
    };

    struct SqlPatientMalaria : SqlPatient
    {
        SqlPatientMalaria();
        ~SqlPatientMalaria();

        float relative_biting_rate;
        bool is_clinical_case;
        bool is_severe_case;
        SevereCaseTypesEnum::Enum severe_case_type;
        float inv_microliters_blood;
        int64_t red_blood_cell_count;
        float cytokines;
        float hrp2;
        float pfemp1_major_variant_fraction;

        std::vector<PatientDrugStatus> drug_status;

        DECLARE_QUERY_INTERFACE()
        DECLARE_SERIALIZABLE(SqlPatientMalaria);
    };

    class SqlReportMalaria : public SqlReport, public IReportMalariaDiagnostics
    {
    public:
        DECLARE_QUERY_INTERFACE()
        IMPLEMENT_NO_REFERENCE_COUNTING()
        DECLARE_FACTORY_REGISTERED( ReportFactory, SqlReportMalaria, IReport )
    public:
        static IReport* CreateReport();
        SqlReportMalaria();
        SqlReportMalaria( const std::string& rReportName );
        virtual ~SqlReportMalaria();

        virtual bool Configure( const Configuration* inputJson ) override;
        virtual void Initialize( unsigned int nrmSize ) override; // public because Simulation::Populate will call this function, passing in NodeRankMap size

        virtual void Finalize() override;

        // IReportMalariaDiagnostics
        virtual void SetDetectionThresholds( const std::vector<float>& rDetectionThresholds ) override;

    protected:
        virtual PatientInfection* CreatePatientInfection() override;
        virtual SqlPatient* CreateSqlPatient() override;
        virtual void ExtractPatientInfo( IIndividualHuman* pInd, SqlPatient* pPatient ) override;
        virtual void ExtractInfectionInfo( IInfection* pInfection, PatientInfection* pPatientInfection ) override;

        virtual void AddInterventions( float currentTime, const SqlPatient* pPatient ) override;
        virtual int BindToHealth(        float currentTime, int nextIndex, const SqlPatient* pPatient ) override;
        virtual int BindToInfectionData( float currentTime, int nextIndex, const PatientInfection* pInfection ) override;

        void FillSevereCaseTypeTable();

        Table m_TableDrugStatus;
        Table m_TableSeverCaseType;

        sqlite3_stmt* m_pStatementInsertDrugStatus;

        bool m_IncludeTableDrugStatus;
    };
}
