/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "StdAfx.h"
#include "MalariaPatientJSONReport.h"

#include "FileSystem.h"
#include "Environment.h"
#include "Exceptions.h"
#include "IndividualMalaria.h"
#include "../interventions/IDrug.h"
#include "ReportUtilities.h"

#include "DllInterfaceHelper.h"
#include "DllDefs.h"
#include "ProgVersion.h"

#include "FactorySupport.h"

using namespace Kernel ;

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!! CREATING NEW REPORTS
// !!! If you are creating a new report by copying this one, you will need to modify 
// !!! the values below indicated by "<<<"

// Module name for logging, CustomReport.json, and DLL GetType()
static const char * _module = "MalariaPatientJSONReport"; // <<< Name of this file

// You can put 0 or more valid Sim types into _sim_types but has to end with nullptr.
// "*" can be used if it applies to all simulation types.
static const char * _sim_types[] = {"MALARIA_SIM", nullptr}; // <<< Types of simulation the report is to be used with

// Output file name
static const std::string _report_name = "MalariaPatientReport.json"; // <<< Filename to put data into

report_instantiator_function_t rif = []()
{
    return (IReport*)(new MalariaPatientJSONReport()); // <<< Report to create
};

DllInterfaceHelper DLL_HELPER( _module, _sim_types, rif );

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

// ------------------------------
// --- DLL Interface Methods
// ---
// --- The DTK will use these methods to establish communication with the DLL.
// ------------------------------

#ifdef __cplusplus    // If used by C++ code, 
extern "C" {          // we need to export the C interface
#endif

DTK_DLLEXPORT char* __cdecl
GetEModuleVersion(char* sVer, const Environment * pEnv)
{
    return DLL_HELPER.GetEModuleVersion( sVer, pEnv );
}

DTK_DLLEXPORT void __cdecl
GetSupportedSimTypes(char* simTypes[])
{
    DLL_HELPER.GetSupportedSimTypes( simTypes );
}

DTK_DLLEXPORT const char * __cdecl
GetType()
{
    return DLL_HELPER.GetType();
}

DTK_DLLEXPORT void __cdecl
GetReportInstantiator( Kernel::report_instantiator_function_t* pif )
{
    DLL_HELPER.GetReportInstantiator( pif );
}

#ifdef __cplusplus
}
#endif

// ---------------------------
// --- MalariaPatent Methods
// ---------------------------

MalariaPatient::MalariaPatient(int id_, float age_, float birthday_)
    : id(id_)
    , initial_age(age_)
    , birthday(birthday_)
    , n_drug_treatments(0)
{
}

MalariaPatient::~MalariaPatient()
{
}

void MalariaPatient::JSerialize( IJsonObjectAdapter* root, JSerializer* helper )
{
    LOG_DEBUG("Serializing MalariaPatient\n");
    root->BeginObject();

    LOG_DEBUG("Inserting simple variables\n");
    root->Insert("id", id);
    root->Insert("initial_age", initial_age);
    root->Insert("birthday", birthday);

    LOG_DEBUG("Inserting array variables\n");
    SerializeChannel("true_asexual_parasites", true_asexual_density, root, helper);
    SerializeChannel("true_gametocytes", true_gametocyte_density, root, helper);
    SerializeChannel("asexual_parasites", asexual_parasite_density, root, helper);
    SerializeChannel("gametocytes", gametocyte_density, root, helper);
    SerializeChannel("infected_mosquito_fraction", infectiousness, root, helper);
    SerializeChannel("hemoglobin", hemoglobin, root, helper);
    SerializeChannel("temps", fever, root, helper);
    SerializeChannel("treatment", drug_treatments, root, helper);
    SerializeChannel("asexual_positive_fields", pos_fields_of_view, root, helper);
    SerializeChannel("gametocyte_positive_fields", gametocyte_pos_fields_of_view, root, helper);

    root->EndObject();
}

void MalariaPatient::SerializeChannel( std::string channel_name, std::vector<float> &channel_data,
                                       IJsonObjectAdapter* root, JSerializer* helper )
{
    root->Insert(channel_name.c_str());
    root->BeginArray();
    helper->JSerialize(channel_data, root);
    root->EndArray();
}

void MalariaPatient::SerializeChannel( std::string channel_name, std::vector<std::string> &channel_data,
                                       IJsonObjectAdapter* root, JSerializer* helper )
{
    root->Insert(channel_name.c_str());
    root->BeginArray();
    helper->JSerialize(channel_data, root);
    root->EndArray();
}

// ----------------------------------------
// --- MalariaPatientJSONReport Methods
// ----------------------------------------

MalariaPatientJSONReport::MalariaPatientJSONReport()
    : BaseReport()
    , report_name( _report_name )
    , simtime(0.0f)
    , ntsteps(0)
    , patient_map()
{
    LOG_DEBUG( "CTOR\n" );
}

MalariaPatientJSONReport::~MalariaPatientJSONReport()
{
    LOG_DEBUG( "DTOR\n" );
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // !!! Commenting out the code to delete all the patients because it can take
    // !!! a long time to delete them.  If we are doing this, we are trying to delete
    // !!! the report and exit the simulation.  Hurry up and exit.
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //for( auto& entry : patient_map )
    //{
    //    delete entry.second ;
    //    entry.second = nullptr ;
    //}
    patient_map.clear();
}

void MalariaPatientJSONReport::Initialize( unsigned int nrmSize )
{
    LOG_DEBUG( "Initialize\n" );
}

void MalariaPatientJSONReport::BeginTimestep()
{
    LOG_DEBUG( "BeginTimestep\n" );
    ntsteps += 1;
}

void MalariaPatientJSONReport::LogNodeData( INodeContext * pNC )
{
    LOG_DEBUG( "LogNodeData\n" );
    simtime = pNC->GetTime().time;
}

bool MalariaPatientJSONReport::IsCollectingIndividualData( float currentTime, float dt ) const
{
    return true;
}

void MalariaPatientJSONReport::LogIndividualData( IIndividualHuman* individual )
{
    LOG_DEBUG( "LogIndividualData\n" );

    // individual identifying info
    int id           = individual->GetSuid().data;
    double mc_weight = individual->GetMonteCarloWeight();
    double age       = individual->GetAge();

    // get malaria contexts
    const IndividualHumanMalaria* individual_malaria = static_cast<const IndividualHumanMalaria*>(individual);
    IMalariaSusceptibility* susceptibility_malaria = individual_malaria->GetMalariaSusceptibilityContext();

    // get the correct existing patient or insert a new one
    MalariaPatient* patient = NULL;
    patient_map_t::const_iterator it = patient_map.find(id);
    if ( it == patient_map.end() )
    {
        patient = new MalariaPatient(id, age, simtime-age);
        patient_map.insert( std::make_pair(id, patient) );
    }
    else
    {
        patient = it->second;
    }

    // Push back today's disease variables for infected individuals
    float max_fever = susceptibility_malaria->GetMaxFever();
    patient->fever.push_back( max_fever > 0 ? max_fever + 37.0f : -1.0f );
    patient->hemoglobin.push_back( susceptibility_malaria->GetHemoglobin() );
    patient->infectiousness.push_back( individual->GetInfectiousness() * 100.0f ); // (100.0f = turn fraction into percentage)

    // True values in model
    patient->true_asexual_density.push_back( susceptibility_malaria->get_parasite_density() ); // Getting this directly will make the value one day earlier than the other three densities
    patient->true_gametocyte_density.push_back( individual_malaria->GetGametocyteDensity() );

    // Values incorporating variability and sensitivity of blood test
    patient->asexual_parasite_density.push_back( individual_malaria->CheckParasiteCountWithTest( MALARIA_TEST_BLOOD_SMEAR ) );
    patient->gametocyte_density.push_back( individual_malaria->CheckGametocyteCountWithTest( MALARIA_TEST_BLOOD_SMEAR ) );

    // Positive fields of view (out of 200 views in Garki-like setup)
    int pos_fields = 0;
    int gam_pos_fields = 0;
    individual_malaria->CountPositiveSlideFields( DLL_HELPER.GetRandomNumberGenerator(), 200, 1.0f/400, pos_fields, gam_pos_fields);
    patient->pos_fields_of_view.push_back(float(pos_fields));
    patient->gametocyte_pos_fields_of_view.push_back(float(gam_pos_fields));

    // New drugs
    std::list<IDrug*> drug_list = ReportUtilities::GetDrugList( individual, std::string("class Kernel::AntimalarialDrug") );
    LOG_DEBUG_F( "Drug doses distributed = %d\n", drug_list.size() );

    int new_drugs = drug_list.size() - patient->n_drug_treatments;
    patient->n_drug_treatments += new_drugs;
    std::string new_drug_names = "";

    while(new_drugs > 0)
    {
        new_drug_names += drug_list.back()->GetDrugName();
        drug_list.pop_back();
        new_drugs--;
        if (new_drugs == 0) break;
        new_drug_names += " + ";
    }

    patient->drug_treatments.push_back(new_drug_names);
}

void MalariaPatientJSONReport::EndTimestep( float currentTime, float dt )
{
    LOG_DEBUG( "EndTimestep\n" );
}

// TODO: are we ever going to want to use this on multi-core?  Lot's of data output!
void MalariaPatientJSONReport::Reduce()
{
    LOG_DEBUG( "Reduce\n" );
}

std::string MalariaPatientJSONReport::GetReportName() const
{
    return report_name;
}

void MalariaPatientJSONReport::Finalize()
{
    // Open output file
    ofstream ofs;
    std::ostringstream output_file_name;
    output_file_name << _report_name;
    LOG_INFO_F( "Writing file: %s\n", output_file_name.str().c_str() );
    ofs.open( FileSystem::Concat( EnvPtr->OutputPath, output_file_name.str() ).c_str() );
    if (!ofs.is_open())
    {
        LOG_ERR("Failed to open output file for serialization.\n");
        throw FileNotFoundException( __FILE__, __LINE__, __FUNCTION__, output_file_name.str().c_str() );
    }

    // Accumulate array of patients as JSON
    int counter = 0;
    JSerializer js;
    LOG_DEBUG("Creating JSON object adaptor.\n");
    IJsonObjectAdapter* pIJsonObj = CreateJsonObjAdapter();
    LOG_DEBUG("Creating new JSON writer.\n");
    pIJsonObj->CreateNewWriter();
    LOG_DEBUG("Beginning malaria patient array.\n");
    pIJsonObj->BeginObject();
    pIJsonObj->Insert("patient_array");
    pIJsonObj->BeginArray();
    for(auto &id_patient_pair: patient_map)
    {
        MalariaPatient* patient = id_patient_pair.second;
        LOG_DEBUG_F("Serializing patient %d\n", counter);
        patient->JSerialize(pIJsonObj, &js);
        LOG_DEBUG_F("Finished serializing patient %d\n", counter);
    }
    pIJsonObj->EndArray();
    pIJsonObj->Insert("ntsteps", ntsteps);
    pIJsonObj->EndObject();

    // Write output to file
    // GetPrettyFormattedOutput() can be used for nicer indentation but bigger filesize
    // NOTE: This report can be quite large.  The pretty format can take this report
    //       from 60 MB to 300 MB.
    const char* sHumans;
    js.GetFormattedOutput(pIJsonObj, sHumans);
    if (sHumans)
    {
        ofs << sHumans << endl;
        LOG_DEBUG("Done inserting\n");
    }
    else
    {
        LOG_ERR("Failed to get prettyHumans\n");
        throw FileIOException( __FILE__, __LINE__, __FUNCTION__, output_file_name.str().c_str() );
    }
    if (ofs.is_open())
    {
        ofs.close();
        LOG_DEBUG("Done writing\n");
    }
    pIJsonObj->FinishWriter();
    delete pIJsonObj ;
}

