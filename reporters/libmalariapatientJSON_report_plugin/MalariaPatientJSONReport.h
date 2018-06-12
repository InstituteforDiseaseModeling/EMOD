/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <vector>
#include <map>

#include "IReport.h"
#include "Configuration.h"

namespace Kernel
{
    struct IJsonObjectAdapter;
    class JSerializer;
}

struct MalariaPatient
{
    MalariaPatient(int id_, float age_, float birthday_);
    ~MalariaPatient();

    int id;
    float initial_age;
    float birthday;
    std::vector<float> true_asexual_density;
    std::vector<float> true_gametocyte_density;
    std::vector<float> asexual_parasite_density;
    std::vector<float> gametocyte_density;
    std::vector<float> infectiousness;
    std::vector<float> hemoglobin;
    std::vector<float> fever;
    std::vector<float> pos_fields_of_view;
    std::vector<float> gametocyte_pos_fields_of_view;

    int n_drug_treatments;
    std::vector<std::string> drug_treatments;

    virtual void JSerialize( Kernel::IJsonObjectAdapter* root, Kernel::JSerializer* helper );

protected:
    void SerializeChannel( std::string channel_name, std::vector<float> &channel_data,
                           Kernel::IJsonObjectAdapter* root, Kernel::JSerializer* helper );
    void SerializeChannel( std::string channel_name, std::vector<std::string> &channel_data,
                           Kernel::IJsonObjectAdapter* root, Kernel::JSerializer* helper );

};

class MalariaPatientJSONReport : public Kernel::BaseReport
{
public:
    static IReport* CreateReport();
    MalariaPatientJSONReport();
    virtual ~MalariaPatientJSONReport();

    virtual void Initialize( unsigned int nrmSize ) override; // public because Simulation::Populate will call this function, passing in NodeRankMap size

    virtual void BeginTimestep() override;
    virtual void LogNodeData( Kernel::INodeContext * pNC ) override;
    virtual bool IsCollectingIndividualData( float currentTime, float dt ) const override;
    virtual void LogIndividualData( Kernel::IIndividualHuman* individual ) override;
    virtual void EndTimestep( float currentTime, float dt ) override;

    // TODO: are we ever going to want to use this on multi-core?  Lot's of data output!
    virtual void Reduce() override;

    virtual std::string GetReportName() const override;
    virtual void Finalize() override;

protected:
    std::string report_name;
    float simtime;
    int ntsteps;

    typedef std::map<int, MalariaPatient*> patient_map_t;
    patient_map_t patient_map; // TODO: lots of patients --> unordered_map (!)
};

