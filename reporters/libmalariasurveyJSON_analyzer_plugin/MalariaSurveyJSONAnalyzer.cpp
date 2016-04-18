/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "StdAfx.h"
#include "MalariaSurveyJSONAnalyzer.h"

#include "FileSystem.h"
#include "Environment.h"
#include "Exceptions.h"
#include "IndividualMalaria.h"
#include "SusceptibilityMalaria.h"
#include "NodeEventContext.h"

#include "DllInterfaceHelper.h"
#include "DllDefs.h"
#include "ProgVersion.h"
#include "IdmMpi.h"

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!! CREATING NEW REPORTS
// !!! If you are creating a new report by copying this one, you will need to modify 
// !!! the values below indicated by "<<<"

// Module name for logging, CustomReport.json, and DLL GetType()
static const char * _module = "MalariaSurveyJSONAnalyzer"; // <<< Name of this file

namespace Kernel
{
// You can put 0 or more valid Sim types into _sim_types but has to end with nullptr.
// "*" can be used if it applies to all simulation types.
static const char * _sim_types[] = {"MALARIA_SIM", nullptr}; // <<< Types of simulation the report is to be used with

Kernel::report_instantiator_function_t rif = []()
{
    return (Kernel::IReport*)(new MalariaSurveyJSONAnalyzer()); // <<< Report to create
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

    MalariaPatient::MalariaPatient(int id_, float age_, float local_birthday_)
        : id(id_)
        , node_id(-1)
        , initial_age(age_)
        , local_birthday(local_birthday_)
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
        root->Insert("id",             id);
        root->Insert("node_id",        node_id);
        root->Insert("initial_age",    initial_age);
        root->Insert("local_birthday", local_birthday);

        root->Insert("strain_ids");
        root->BeginArray();
        for (auto& inner : strain_ids)
        {
            // TODO: add helper function for serializing pair<int,int> ??
            root->BeginArray();
            root->Add(inner.first);
            root->Add(inner.second);
            root->EndArray();
        }
        root->EndArray();

        LOG_DEBUG("Inserting array variables\n");
        SerializeChannel("true_asexual_parasites", true_asexual_density,     root, helper);
        SerializeChannel("true_gametocytes",       true_gametocyte_density,  root, helper);
        SerializeChannel("asexual_parasites",      asexual_parasite_density, root, helper);
        SerializeChannel("gametocytes",            gametocyte_density,       root, helper);
        SerializeChannel("infectiousness",         infectiousness,           root, helper);
        SerializeChannel("pos_asexual_fields",     pos_asexual_fields,       root, helper);
        SerializeChannel("pos_gametocyte_fields",  pos_gametocyte_fields,    root, helper);
        SerializeChannel("temps",                  fever,                    root, helper);
        SerializeChannel("rdt",                    rdt,                      root, helper);

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

    std::vector<float> JDeserializeFloatVector( const std::string& name, IJsonObjectAdapter* root )
    {
        std::vector<float> value_list;

        IJsonObjectAdapter* p_outer_array = root->GetJsonArray( name.c_str() );
        release_assert( p_outer_array->GetSize() == 1 );

        IJsonObjectAdapter* p_json_array = (*p_outer_array)[IndexType(0)];
        for( unsigned int i = 0 ; i < p_json_array->GetSize(); ++i )
        {
            float value = (*p_json_array)[IndexType(i)]->AsFloat();
            value_list.push_back( value );
        }
        return value_list;
    }

    std::vector<std::string> JDeserializeStringVector( const std::string& name, IJsonObjectAdapter* root )
    {
        std::vector<std::string> value_list;

        IJsonObjectAdapter* p_outer_array = root->GetJsonArray( name.c_str() );
        release_assert( p_outer_array->GetSize() == 1 );

        IJsonObjectAdapter* p_json_array = (*p_outer_array)[IndexType(0)];
        for( unsigned int i = 0 ; i < p_json_array->GetSize(); ++i )
        {
            std::string value = (*p_json_array)[IndexType(i)]->AsString();
            value_list.push_back( value );
        }
        return value_list;
    }

    std::vector< std::pair<int,int> > JDeserializeStrainIds( IJsonObjectAdapter* root )
    {
        std::vector< std::pair<int,int> > value_list;

        IJsonObjectAdapter* p_json_array = root->GetJsonArray( "strain_ids" );
        for( unsigned int i = 0 ; i < p_json_array->GetSize(); ++i )
        {
            IJsonObjectAdapter* p_inner_array = (*p_json_array)[IndexType(i)];
            int first  = (*p_inner_array)[IndexType(0)]->AsInt();
            int second = (*p_inner_array)[IndexType(1)]->AsInt();
            value_list.push_back( std::make_pair( first, second ) );
        }
        return value_list;
    }

    void MalariaPatient::JDeserialize( IJsonObjectAdapter* root )
    {
        id             = root->GetInt(   "id"             );
        node_id        = root->GetInt(   "node_id"        );
        initial_age    = root->GetFloat( "initial_age"    );
        local_birthday = root->GetFloat( "local_birthday" );

        strain_ids = JDeserializeStrainIds( root );

        true_asexual_density      = JDeserializeFloatVector(  "true_asexual_parasites" , root );
        true_gametocyte_density   = JDeserializeFloatVector(  "true_gametocytes"       , root );
        asexual_parasite_density  = JDeserializeFloatVector(  "asexual_parasites"      , root );
        gametocyte_density        = JDeserializeFloatVector(  "gametocytes"            , root );
        infectiousness            = JDeserializeFloatVector(  "infectiousness"         , root );
        pos_asexual_fields        = JDeserializeFloatVector(  "pos_asexual_fields"     , root );
        pos_gametocyte_fields     = JDeserializeFloatVector(  "pos_gametocyte_fields"  , root );
        fever                     = JDeserializeFloatVector(  "temps"                  , root );
        rdt                       = JDeserializeFloatVector(  "rdt"                    , root );
    }

// ----------------------------------------
// --- MalariaSurveyJSONAnalyzer Methods
// ----------------------------------------

    MalariaSurveyJSONAnalyzer::MalariaSurveyJSONAnalyzer() 
        : BaseEventReportIntervalOutput( _module )
        , m_patient_map()
        , m_PrettyFormat(true)
    {
        LOG_DEBUG( "CTOR\n" );
    }

    MalariaSurveyJSONAnalyzer::~MalariaSurveyJSONAnalyzer()
    {
        LOG_DEBUG( "DTOR\n" );
        if( m_patient_map.size() > 0 )
        {
            WriteOutput( -999.0f );
        }
        ClearOutputData();
    }

    bool MalariaSurveyJSONAnalyzer::Configure( const Configuration* inputJson )
    {
        initConfigTypeMap( "Pretty_Format", &m_PrettyFormat, "True implies pretty JSON format, false saves space.", true );

        return BaseEventReportIntervalOutput::Configure( inputJson );
    }

    bool MalariaSurveyJSONAnalyzer::notifyOnEvent( IIndividualHumanEventContext *context, const std::string& StateChange )
    {
        if( HaveUnregisteredAllEvents() )
        {
            return false ;
        }

        LOG_DEBUG_F( "MalariaSurveyAnalyzer notified of event by %d-year old individual.\n", (int) (context->GetAge() / DAYSPERYEAR) );

        // individual context for suid
        IIndividualHumanContext * iindividual = NULL;
        if (s_OK != context->QueryInterface(GET_IID(IIndividualHumanContext), (void**)&iindividual) )
        {
            throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "context", "IIndividualHumanContext", "IIndividualHumanEventContext");
        }

        int id           = iindividual->GetSuid().data;
        double mc_weight = context->GetMonteCarloWeight();
        double age       = context->GetAge();

        // get malaria contexts
        IMalariaHumanContext * individual_malaria = NULL;
        if (s_OK != context->QueryInterface(GET_IID(IMalariaHumanContext), (void**)&individual_malaria) )
        {
            throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "context", "IMalariaHumanContext", "IIndividualHumanEventContext");
        }
        IMalariaSusceptibility* susceptibility_malaria = individual_malaria->GetMalariaSusceptibilityContext();

        // get the correct existing patient or insert a new one
        MalariaPatient* patient = NULL;
        patient_map_t::const_iterator it = m_patient_map.find(id);
        if ( it == m_patient_map.end() )
        {
            patient = new MalariaPatient(id, age, m_interval_timer-age); // birthday relative to current interval (i.e. output file)
            m_patient_map.insert( std::make_pair(id, patient) );

            patient->strain_ids = individual_malaria->GetInfectingStrainIds();
            INodeEventContext* node_context = context->GetNodeEventContext();
            patient->node_id = node_context->GetExternalId();
        }
        else
        {
            patient = it->second;
        }

        // Push back today's disease variables
        float max_fever = susceptibility_malaria->GetMaxFever();
        patient->fever.push_back( max_fever > 0 ? max_fever + 37.0f : -1.0f );
        // GetInfectiousness is an inline function of IndividualHuman (not belonging to any queriable interface presently)
        patient->infectiousness.push_back( static_cast<IndividualHuman*>(context)->GetInfectiousness() );

        // True values in model
        patient->true_asexual_density.push_back( susceptibility_malaria->get_parasite_density() );
        patient->true_gametocyte_density.push_back( individual_malaria->GetGametocyteDensity() );

        // Values incorporating variability and sensitivity of blood tes
        patient->asexual_parasite_density.push_back( individual_malaria->CheckParasiteCountWithTest( MALARIA_TEST_BLOOD_SMEAR ) );
        patient->gametocyte_density.push_back( individual_malaria->CheckGametocyteCountWithTest( MALARIA_TEST_BLOOD_SMEAR ) );
        patient->rdt.push_back( individual_malaria->CheckParasiteCountWithTest( MALARIA_TEST_NEW_DIAGNOSTIC ) );

        // Positive fields of view (out of 200 views in Garki-like setup)
        int positive_asexual_fields = 0;
        int positive_gametocyte_fields = 0;
        individual_malaria->CountPositiveSlideFields( DLL_HELPER.GetRandomNumberGenerator(), 200, (float)(1.0/400.0), positive_asexual_fields, positive_gametocyte_fields);
        patient->pos_asexual_fields.push_back( (float)positive_asexual_fields );
        patient->pos_gametocyte_fields.push_back( (float)positive_gametocyte_fields );
        LOG_DEBUG_F("(a,g) = (%d,%d)\n", (int)positive_asexual_fields, (int)positive_gametocyte_fields);

        return true;
    }

    void MalariaSurveyJSONAnalyzer::EndTimestep( float currentTime, float dt )
    {
        Reduce();
        BaseEventReportIntervalOutput::EndTimestep( currentTime, dt );
    }

    template <typename T>
    void UpdateVector( std::vector<T>& existing_vector, std::vector<T>& new_vector )
    {
        for( int i = existing_vector.size(); i < new_vector.size() ; ++i )
        {
            existing_vector.push_back( new_vector[i] );
        }
    }

    void SendData( const std::string& rToSend )
    {
        uint32_t size = rToSend.size();

        IdmMpi::Request size_request;
        EnvPtr->MPI.p_idm_mpi->SendIntegers( &size, 1, 0, &size_request );


        IdmMpi::Request data_request;
        EnvPtr->MPI.p_idm_mpi->SendChars( rToSend.c_str(), size, 0, &data_request );

        IdmMpi::RequestList request_list;
        request_list.Add( size_request );
        request_list.Add( data_request );

        EnvPtr->MPI.p_idm_mpi->WaitAll( request_list );
    }

    void GetData( int fromRank, std::vector<char>& rReceive )
    {
        int32_t size = 0;
        EnvPtr->MPI.p_idm_mpi->ReceiveIntegers( &size, 1, fromRank );

        rReceive.resize( size + 1 );

        EnvPtr->MPI.p_idm_mpi->ReceiveChars( rReceive.data(), size, fromRank );
        rReceive[size] = '\0';
    }

    void MalariaSurveyJSONAnalyzer::Reduce()
    {
        if( !HaveRegisteredAllEvents() )
        {
            return;
        }

        if( EnvPtr->MPI.Rank != 0 )
        {
            IJsonObjectAdapter* pIJsonObj = CreateJsonObjAdapter();
            pIJsonObj->CreateNewWriter();
            pIJsonObj->BeginObject();

            JSerializer js;
            SerializePatients( pIJsonObj, js );

            pIJsonObj->EndObject();

            std::string json_data = pIJsonObj->ToString();

            SendData( json_data );
            delete pIJsonObj;
        }
        else
        {
            for( int fromRank = 1 ; fromRank < EnvPtr->MPI.NumTasks ; ++fromRank )
            {
                std::vector<char> received;
                GetData( fromRank, received );

                IJsonObjectAdapter* pIJsonObj = CreateJsonObjAdapter();
                pIJsonObj->Parse( received.data() );

                IJsonObjectAdapter* p_json_array = pIJsonObj->GetJsonArray("patient_array");

                for( unsigned int i = 0 ; i < p_json_array->GetSize() ; ++i )
                {
                    MalariaPatient* new_patient = new MalariaPatient(-1,-1,-1);
                    new_patient->JDeserialize( (*p_json_array)[i] );

                    patient_map_t::const_iterator it = m_patient_map.find( new_patient->id );
                    if( it == m_patient_map.end() )
                    {
                        m_patient_map.insert( std::make_pair( new_patient->id, new_patient ) );
                    }
                    else
                    {
                        MalariaPatient* existing_patient = it->second;

                        existing_patient->node_id        = new_patient->node_id;
                        existing_patient->initial_age    = new_patient->initial_age;
                        existing_patient->local_birthday = new_patient->local_birthday;

                        UpdateVector( existing_patient->true_asexual_density     , new_patient->true_asexual_density     );
                        UpdateVector( existing_patient->true_gametocyte_density  , new_patient->true_gametocyte_density  );
                        UpdateVector( existing_patient->asexual_parasite_density , new_patient->asexual_parasite_density );
                        UpdateVector( existing_patient->gametocyte_density       , new_patient->gametocyte_density       );
                        UpdateVector( existing_patient->infectiousness           , new_patient->infectiousness           );
                        UpdateVector( existing_patient->pos_asexual_fields       , new_patient->pos_asexual_fields       );
                        UpdateVector( existing_patient->pos_gametocyte_fields    , new_patient->pos_gametocyte_fields    );
                        UpdateVector( existing_patient->fever                    , new_patient->fever                    );
                        UpdateVector( existing_patient->rdt                      , new_patient->rdt                      );

                        delete new_patient;
                        new_patient = nullptr;
                    }
                }
                delete pIJsonObj;
            }
        }
    }

    void MalariaSurveyJSONAnalyzer::SerializePatients( IJsonObjectAdapter* pIJsonObj,
                                                       JSerializer& js )
    {
        pIJsonObj->Insert("patient_array");
        pIJsonObj->BeginArray();
        for(auto &id_patient_pair: m_patient_map)
        {
            MalariaPatient* patient = id_patient_pair.second;
            patient->JSerialize(pIJsonObj, &js);
        }
        pIJsonObj->EndArray();
    }

    void MalariaSurveyJSONAnalyzer::WriteOutput( float currentTime )
    {
        // Open output file
        ofstream ofs;
        std::ostringstream output_file_name;
        output_file_name << GetBaseOutputFilename() << "_" << (m_report_count-1) << ".json";
        LOG_INFO_F( "Writing file: %s\n", output_file_name.str().c_str() );
        ofs.open( FileSystem::Concat( EnvPtr->OutputPath, output_file_name.str() ).c_str() );
        if (!ofs.is_open())
        {
            LOG_ERR("Failed to open output file for serialization.\n");
            throw Kernel::FileNotFoundException( __FILE__, __LINE__, __FUNCTION__, output_file_name.str().c_str() );
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
        pIJsonObj->Insert("ntsteps", m_reporting_interval);

        SerializePatients( pIJsonObj, js );

        pIJsonObj->EndObject();

        // Write output to file
        // GetFormattedOutput() could be used for a smaller but less human readable file
        char* sHumans = nullptr;
        if( m_PrettyFormat )
        {
            js.GetPrettyFormattedOutput(pIJsonObj, sHumans);
        }
        else
        {
            const char* const_humans = nullptr;
            js.GetFormattedOutput( pIJsonObj, const_humans );
            sHumans = const_cast<char*>(const_humans);
        }

        if (sHumans)
        {
            ofs << sHumans << endl;
            if( m_PrettyFormat )
            {
                delete sHumans ;
            }
            sHumans = nullptr ;
        }
        else
        {
            throw Kernel::FileIOException( __FILE__, __LINE__, __FUNCTION__, output_file_name.str().c_str() );
        }

        if (ofs.is_open())
        {
            ofs.close();
        }
        pIJsonObj->FinishWriter();
        delete pIJsonObj ;
    }

    void MalariaSurveyJSONAnalyzer::ClearOutputData()
    {
        for( auto &patient: m_patient_map )
        {
            delete patient.second;
        }
        m_patient_map.clear();
    }
}