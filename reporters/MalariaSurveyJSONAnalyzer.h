
#pragma once

#include <vector>
#include <map>

#include "BaseEventReportIntervalOutput.h"
#include "IReportMalariaDiagnostics.h"

#ifndef _REPORT_DLL
#include "ReportFactory.h"
#endif

namespace Kernel
{
    struct Patient
    {
        Patient(int id_, float age_, float local_birthday_);
        virtual ~Patient();

        uint32_t id;
        uint32_t node_id;
        float initial_age;
        float local_birthday;
        std::vector< std::pair<int,int> > strain_ids;
        std::vector<std::string> ip_data;
        std::vector<double> true_asexual_density;
        std::vector<double> true_gametocyte_density;
        std::vector<double> smeared_true_asexual_density;
        std::vector<double> smeared_true_gametocyte_density;
        std::vector<double> asexual_parasite_density;
        std::vector<double> gametocyte_density;
        std::vector<double> pcr_parasite_density;
        std::vector<double> pcr_gametocyte_density;
        std::vector<double> pfhrp2;
        std::vector<double> smeared_asexual_parasite_density;
        std::vector<double> smeared_gametocyte_density;
        std::vector<double> infectiousness;
        std::vector<double> infectiousness_smeared;
        std::vector<double> infectiousness_age_scaled;
        std::vector<double> pos_asexual_fields;
        std::vector<double> pos_gametocyte_fields;
        std::vector<double> fever;

        virtual void Serialize( IJsonObjectAdapter& root, JSerializer& helper );
        virtual void Deserialize( IJsonObjectAdapter& root );

    };

    class PatientMap : public IIntervalData
    {
    public:
        PatientMap();
        virtual ~PatientMap();

        // IIntervalData methods
        virtual void Clear() override;
        virtual void Update( const IIntervalData& rOther ) override;
        virtual void Serialize( IJsonObjectAdapter& rjoa, JSerializer& js ) override;
        virtual void Deserialize( IJsonObjectAdapter& rjoa ) override;

        // other methods
        Patient* FindPatient( uint32_t id );
        void Add( Patient* pPatient );

    protected:
        std::map<uint32_t,Patient*> m_Map;
    };

    class MalariaSurveyJSONAnalyzer : public BaseEventReportIntervalOutput, public IReportMalariaDiagnostics
    {
#ifndef _REPORT_DLL
        DECLARE_FACTORY_REGISTERED( ReportFactory, MalariaSurveyJSONAnalyzer, IReport )
#endif
    public:
        // needed for IReportMalariaDiagnostics
        DECLARE_QUERY_INTERFACE()
        IMPLEMENT_NO_REFERENCE_COUNTING()

        MalariaSurveyJSONAnalyzer();
        virtual ~MalariaSurveyJSONAnalyzer();

        // BaseEventReportIntervalOutput
        virtual void Initialize( unsigned int nrmSize ) override;
        virtual bool Configure( const Configuration* ) override;
        virtual bool notifyOnEvent( IIndividualHumanEventContext *context, const EventTrigger& trigger ) override;

        // IReportMalariaDiagnostics
        virtual void SetDetectionThresholds( const std::vector<float>& rDetectionThresholds ) override;

    protected:
        // BaseEventReportIntervalOutput
        virtual void SerializeOutput( float currentTime, IJsonObjectAdapter& output, JSerializer& js ) override;

        std::string m_IPKeyToCollect;
        PatientMap* m_pPatientMap;
        std::vector<float> m_DetectionThresholds;
        RANDOMBASE* m_pRNG;
    };
}