/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <vector>
#include <map>

#include "BaseEventReportIntervalOutput.h"

namespace Kernel
{
    struct MalariaPatient
    {
        MalariaPatient(int id_, float age_, float local_birthday_);
        virtual ~MalariaPatient();

        uint32_t id;
        uint32_t node_id;
        float initial_age;
        float local_birthday;
        std::vector< std::pair<int,int> > strain_ids;
        std::vector<double> true_asexual_density;
        std::vector<double> true_gametocyte_density;
        std::vector<double> asexual_parasite_density;
        std::vector<double> gametocyte_density;
        std::vector<double> infectiousness;
        std::vector<double> pos_asexual_fields;
        std::vector<double> pos_gametocyte_fields;
        std::vector<double> fever;
        std::vector<double> rdt;

        virtual void Serialize( IJsonObjectAdapter& root, JSerializer& helper );
        virtual void Deserialize( IJsonObjectAdapter& root );

    };

    class MalariaPatientMap : public IIntervalData
    {
    public:
        MalariaPatientMap();
        virtual ~MalariaPatientMap();

        // IIntervalData methods
        virtual void Clear() override;
        virtual void Update( const IIntervalData& rOther ) override;
        virtual void Serialize( IJsonObjectAdapter& rjoa, JSerializer& js ) override;
        virtual void Deserialize( IJsonObjectAdapter& rjoa ) override;

        // other methods
        MalariaPatient* FindPatient( uint32_t id );
        void Add( MalariaPatient* pPatient );

    protected:
        std::map<uint32_t,MalariaPatient*> m_Map;
    };

    class MalariaSurveyJSONAnalyzer : public BaseEventReportIntervalOutput
    {
    public:
        MalariaSurveyJSONAnalyzer();
        virtual ~MalariaSurveyJSONAnalyzer();

        // BaseEventReportIntervalOutput
        virtual bool Configure( const Configuration* ) override;
        virtual bool notifyOnEvent( IIndividualHumanEventContext *context, const EventTrigger& trigger ) override;

    protected:
        // BaseEventReportIntervalOutput
        virtual void SerializeOutput( float currentTime, IJsonObjectAdapter& output, JSerializer& js ) override;

        MalariaPatientMap* m_pPatientMap;
    };
}