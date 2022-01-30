/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "MalariaTransmissionReport.h"

#include <algorithm>
#include <numeric>

#include "FileSystem.h"
#include "Environment.h"
#include "Exceptions.h"
#include "DllInterfaceHelper.h"
#include "DllDefs.h"
#include "ProgVersion.h"
#include "IdmMpi.h"
#include "ReportUtilities.h"
#include "Serializer.h"

#include "ISimulationContext.h"
#include "MalariaContexts.h"
#include "VectorContexts.h"
#include "IIndividualHuman.h"
#include "NodeEventContext.h"
#include "VectorCohortIndividual.h"
#include "IMigrate.h"
#include "RANDOM.h"

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!! CREATING NEW REPORTS
// !!! If you are creating a new report by copying this one, you will need to modify 
// !!! the values below indicated by "<<<"

// Module name for logging, CustomReport.json, and DLL GetType()
SETUP_LOGGING("MalariaTransmissionReport") // <<< Name of this file

namespace Kernel
{
// You can put 0 or more valid Sim types into _sim_types but has to end with nullptr.
// "*" can be used if it applies to all simulation types.
static const char * _sim_types[] = {"MALARIA_SIM", nullptr}; // <<< Types of simulation the report is to be used with

report_instantiator_function_t rif = []()
{
    return (IReport*)(new MalariaTransmissionReport()); // <<< Report to create
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
GetReportInstantiator( report_instantiator_function_t* pif )
{
    DLL_HELPER.GetReportInstantiator( pif );
}

#ifdef __cplusplus
}
#endif

    MigratingVector::MigratingVector(uint64_t id, ExternalNodeId_t from_node_id, ExternalNodeId_t to_node_id)
        : id(id)
        , from_node_id(from_node_id)
        , to_node_id(to_node_id)
    {
    }

    MigratingVector::MigratingVector(IVectorCohortIndividual* pivci, ISimulationContext* pSim, const suids::suid& nodeSuid)
    {
        IMigrate * pim = nullptr;
        if (s_OK != pivci->QueryInterface(GET_IID(IMigrate), (void**)&pim))
        {
            throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "pivci", "IMigrate", "IVectorCohortIndividual");
        }

        id = pivci->GetID();
        from_node_id = pSim->GetNodeExternalID(nodeSuid);
        to_node_id = pSim->GetNodeExternalID(pim->GetMigrationDestination());
    }

// ----------------------------------------
// --- MalariaTransmissionReport Methods
// ----------------------------------------

    MalariaTransmissionReport::MalariaTransmissionReport() 
        : BaseEventReport( _module )
        , m_PrettyFormat(true)
        , outputWritten(false)
        , timeStep(0)
    {
        LOG_DEBUG( "CTOR\n" );
    }

    MalariaTransmissionReport::~MalariaTransmissionReport()
    {
        LOG_DEBUG( "DTOR\n" );
    }

    bool MalariaTransmissionReport::Configure( const Configuration* inputJson )
    {
        initConfigTypeMap( "Pretty_Format", &m_PrettyFormat, "True implies pretty JSON format, false saves space.", false );

        return BaseEventReport::Configure( inputJson );
    }

    void MalariaTransmissionReport::LogVectorMigration(ISimulationContext* pSim, float currentTime, const suids::suid& nodeSuid, IVectorCohort* pvc)
    {
        IVectorCohortIndividual * pivci = NULL;
        if (s_OK != pvc->QueryInterface(GET_IID(IVectorCohortIndividual), (void**)&pivci))
        {
            throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "pvc", "IVectorCohortIndividual", "IVectorCohort");
        }

        VectorStateEnum::Enum state = pvc->GetState();

        if (state == VectorStateEnum::STATE_INFECTIOUS)
        {
            // Need to track infectious mosquitos who are migrating
            MigratingVector mv(pivci, pSim, nodeSuid);
            LOG_DEBUG_F("Infectious mosquito (id=%d) migrating from node %d to %d\n", mv.id, mv.from_node_id, mv.to_node_id);

            auto found = infectious_mosquitos_current.find(mv.to_node_id);
            if (found == infectious_mosquitos_current.end())
            {
                infectious_mosquitos_current[mv.to_node_id] = InfectiousMosquitos_t();
            }

            infectious_mosquitos_current[mv.to_node_id].push_back(mv.id);
        }
        else if (state == VectorStateEnum::STATE_INFECTED && pvc->GetProgress() == 0)
        {
            // Also need to track newly infected mosquitos who are migrating
            MigratingVector mv(pivci, pSim, nodeSuid);
            LOG_DEBUG_F("Newly infected mosquito (id=%d) migrating from node %d to %d\n", mv.id, mv.from_node_id, mv.to_node_id);

            Location location = std::make_pair(mv.from_node_id, timeStep);
            infected_mosquito_buffer[mv.id] = location;
        }
    }

    bool MalariaTransmissionReport::notifyOnEvent( IIndividualHumanEventContext *context, const EventTrigger& trigger )
    {
        if( HaveUnregisteredAllEvents() )
        {
            return false ;
        }

        const std::string& StateChange = trigger.ToString();

        uint32_t id = context->GetSuid().data;
        float age = context->GetAge();

        ExternalNodeId_t node_id = context->GetNodeEventContext()->GetExternalId();
        Location location = std::make_pair(node_id, timeStep);

        LOG_DEBUG_F("==> Notified of %s event by %d-year old individual (suid=%d).\n",
            StateChange.c_str(), (int)(age / DAYSPERYEAR), id);

        if(trigger == EventTrigger::NewInfectionEvent)
        {
            // ---------- Find ID of acquired infection
            MalariaIndividualInfo info = MalariaIndividualInfo(context);
            if (info.infections.empty())
            {
                throw IllegalOperationException(
                    __FILE__, __LINE__, __FUNCTION__,
                    "Notified of NewInfectionEvent by individual with empty infections list.");
            }
            uint32_t acInfId_ = info.infections.front()->id;  // most recent: IndividualHuman::AcquireNewInfection does push_front

            // ---------- Sample transmitting infectious mosquito from buffer
            uint32_t txId_ = -1;
            std::vector<uint32_t> txInfIds_;
            std::vector<float> txGams_;

            InfectiousMosquitos_t infectious_mosquitos;
            auto found = infectious_mosquitos_previous.find(node_id);
            if (found == infectious_mosquitos_previous.end())
            {
                LOG_WARN_F("No infectious mosquitos at t=%d, node_id=%d.\n",
                    location.second, location.first);
                transmissions.push_back(_new_ Transmission(location, txId_, txInfIds_, txGams_, id, acInfId_));
                return false;
            }
            else
            {
                infectious_mosquitos = found->second;
            }
            LOG_DEBUG_F("%d infectious mosquitos in buffer at t=%d, node_id=%d.\n",
                infectious_mosquitos.size(), location.second, location.first);
            uint64_t mosq_id = infectious_mosquitos.at(DLL_HELPER.GetRandomNumberGenerator()->uniformZeroToN16(infectious_mosquitos.size())); // TODO: SuidGenerator + Reduce() for multicore
            LOG_DEBUG_F("Sampled infectious mosquito (id=%d) at t=%d, node_id=%d.\n",
                mosq_id, location.second, location.first);

            // ---------- Sample location of transmitting mosquito's original infection from buffer
            auto infected_found = infected_mosquito_buffer.find(mosq_id);
            if (infected_found == infected_mosquito_buffer.end())
            {
                LOG_WARN_F("No newly infected mosquito with id=%d.\n", mosq_id);
                transmissions.push_back(_new_ Transmission(location, txId_, txInfIds_, txGams_, id, acInfId_));
                return false;
            }
            Location infected_location = infected_found->second;
            LOG_DEBUG_F("Sampled mosquito (id=%d) infected at t=%d, node_id=%d.\n",
                mosq_id, infected_location.second, infected_location.first);

            // ---------- Sample transmitting infectious human from buffer
            auto humans_found = infected_human_buffer.find(infected_location);
            if (humans_found == infected_human_buffer.end())
            {
                LOG_WARN_F("No infectious humans at t=%d, node_id=%d.\n",
                    infected_location.second, infected_location.first);
                transmissions.push_back(_new_ Transmission(location, txId_, txInfIds_, txGams_, id, acInfId_));
                return false;
            }
            const auto& inf_humans = humans_found->second;
            LOG_DEBUG_F("%d infectious humans in buffer at t=%d, node_id=%d.\n",
                inf_humans.size(), infected_location.second, infected_location.first);

            // weighted sampling by infectiousness
            std::vector<float> weights(inf_humans.size(), 0);
            std::transform(inf_humans.begin(), inf_humans.end(),
                weights.begin(), [](const auto& m){return m->infectiousness;});
            std::vector<float> cum_weights(weights.size(), 0);
            std::partial_sum(weights.begin(), weights.end(), cum_weights.begin());
            float total = cum_weights.back();
            LOG_DEBUG_F("Total infectiousness = %0.2f.  Normalizing weights...\n", total);
            std::for_each(cum_weights.begin(), cum_weights.end(), [total](float &w){w /= total;});

            auto up = std::upper_bound(cum_weights.begin(), cum_weights.end(), DLL_HELPER.GetRandomNumberGenerator()->e());
            int idx = std::distance(cum_weights.begin(), up);
            const MalariaIndividualInfo& txHuman = *(inf_humans.at(idx));
            txId_ = txHuman.id;
            LOG_DEBUG_F("Transmitting human ID = %d\n", txId_);

            //// ---------- Sample infecting strains from human buffer

            std::vector<MalariaInfectionInfo*> txInfections = txHuman.infections;
            int n_infections = txInfections.size();

            if (n_infections == 0)
            {
               throw IllegalOperationException(__FILE__, __LINE__, __FUNCTION__,
                   "Infectious individual with no infections.");
            }

            std::ostringstream ss;
            for (int idx = 0; idx < n_infections; idx++)
            {
                txInfIds_.push_back(txInfections[idx]->id);
                txGams_.push_back(txInfections[idx]->gametocyte_density);
                ss << txInfIds_[idx] << ":" << int(txGams_[idx]) << " ";
            }
            LOG_DEBUG_F("Transmitting infection densities = [ %s]\n", ss.str().c_str());

            Transmission *t = _new_ Transmission(location, txId_, txInfIds_, txGams_, id, acInfId_);
            transmissions.push_back(t);
        }
        else
        {
            // Infection-sampling events, e.g. Received_Treatment or Received_RCD_Drugs
            MalariaIndividualInfo info = MalariaIndividualInfo(context);
            int n_infections = info.infections.size();

            if (n_infections == 0)
            {
                LOG_DEBUG_F("Notified of %s event by person id=%d with no infections.\n", StateChange.c_str(), id);
                return false;
            }

            std::vector<uint32_t> infIds_;
            std::vector<float> densities_;

            std::ostringstream ss;
            for (int idx = 0; idx < n_infections; idx++)
            {
                infIds_.push_back(info.infections[idx]->id);
                densities_.push_back(info.infections[idx]->asexual_density);
                ss << infIds_[idx] << ":" << int(densities_[idx]) << " ";
            }
            LOG_DEBUG_F("Sampled '%s' individual (suid=%d): infection densities = [ %s]\n",
                StateChange.c_str(), id, ss.str().c_str());

            ClinicalSample *s = _new_ ClinicalSample(location, StateChange, id, infIds_, densities_);
            samples.push_back(s);
        }

        return true;
    }

    bool MalariaTransmissionReport::IsActive() const
    {
        // Same conditions for LogData as the notifyEvent
        return HaveRegisteredAllEvents() && !HaveUnregisteredAllEvents();
    }

    bool MalariaTransmissionReport::IsFinished() const
    {
        return HaveRegisteredAllEvents() && HaveUnregisteredAllEvents();
    }

    bool MalariaTransmissionReport::IsCollectingIndividualData( float currentTime, float dt ) const
    {
        return IsActive();
    }

    void MalariaTransmissionReport::LogIndividualData( IIndividualHuman* individual )
    {
        if (!individual->IsInfected())
            return;

        //LOG_VALID_F("\tLogIndividualData for infected %d-year old (suid=%d).\n",
        //    (int)(individual->GetAge() / DAYSPERYEAR), individual->GetSuid().data);

        ExternalNodeId_t node_id = individual->GetParent()->GetExternalID();
        Location location = std::make_pair(node_id, timeStep);

        infected_human_buffer[location].push_back(
            std::unique_ptr<MalariaIndividualInfo>(_new_ MalariaIndividualInfo(individual)));
    }

    InfectiousMosquitos_t MalariaTransmissionReport::BufferInfectiousVectors( INodeEventContext *context )
    {
        INodeContext* pNC = NULL;
        if (s_OK != context->QueryInterface(GET_IID(INodeContext), (void**)&pNC) )
        {
            throw QueryInterfaceException(
                __FILE__, __LINE__, __FUNCTION__,
                "context", "INodeContext", "INodeEventContext");
        }

        return BufferInfectiousVectors(pNC);
    }

    InfectiousMosquitos_t MalariaTransmissionReport::BufferInfectiousVectors(INodeContext *pNC)
    {
        ExternalNodeId_t node_id = pNC->GetExternalID();
        Location location = std::make_pair(node_id, timeStep);

        INodeVector* inv = NULL;
        if (s_OK != pNC->QueryInterface(GET_IID(INodeVector), (void**)&inv))
        {
            throw QueryInterfaceException(
                __FILE__, __LINE__, __FUNCTION__,
                "pNC", "INodeVector", "INodeContext");
        }

        InfectiousMosquitos_t tmp_infectious;
        for (auto pop : inv->GetVectorPopulationReporting())
        {
            // Get infectious mosquitoes
            std::vector<uint64_t> infectious_suids = pop->GetInfectiousVectorIds();
            if (!infectious_suids.empty())
            {
                LOG_DEBUG_F("%d infectious %s mosquitos, node_id=%d\n", infectious_suids.size(), pop->get_SpeciesID().c_str(), node_id);
                tmp_infectious.insert(tmp_infectious.end(), infectious_suids.begin(), infectious_suids.end());
            }
        }

        if (!tmp_infectious.empty())
        {
            auto found = infectious_mosquitos_current.find(node_id);
            if (found == infectious_mosquitos_current.end())
            {
                LOG_DEBUG_F("Adding first %d mosquitos to node-%d buffer\n", tmp_infectious.size(), node_id);
                infectious_mosquitos_current.insert(std::make_pair(node_id, tmp_infectious));
            }
            else
            {
                LOG_DEBUG_F("Adding %d mosquitos to existing node-%d buffer (size=%d)\n", tmp_infectious.size(), node_id, found->second.size());
                found->second.insert(found->second.end(), tmp_infectious.begin(), tmp_infectious.end());
            }
        }

        return tmp_infectious;
    }

    void MalariaTransmissionReport::LogNodeData( INodeContext * pNC )
    {
        if (!IsActive())
            return;

        ExternalNodeId_t node_id = pNC->GetExternalID();
        Location location = std::make_pair(node_id, timeStep + 1);  // notifyOnEvent called before LogNodeData, so offset by 1.
        LOG_DEBUG_F("LogNodeData for suid=%d (externalId=%d).\n", pNC->GetSuid().data, node_id);

        INodeVector* inv = NULL;
        if (s_OK != pNC->QueryInterface(GET_IID(INodeVector), (void**)&inv) )
        {
            throw QueryInterfaceException(
                __FILE__, __LINE__, __FUNCTION__,
                "pNC", "INodeVector", "INodeContext");
        }

        for (auto pop : inv->GetVectorPopulationReporting())
        {
            // Get newly infected mosquitos
            std::vector<uint64_t> newly_infected_mosquitos = pop->GetNewlyInfectedVectorIds();
            LOG_DEBUG_F("%d newly infected %s mosquitos\n", newly_infected_mosquitos.size(), pop->get_SpeciesID().c_str());
            for (auto mosquito_id : newly_infected_mosquitos)
            {
                infected_mosquito_buffer[mosquito_id] = location;
            }
        }

        BufferInfectiousVectors(pNC);
    }

    void MalariaTransmissionReport::EndTimestep( float currentTime, float dt )
    {
        timeStep++;

        // Write outputs when the reporting interval is finished
        if (IsFinished() && !outputWritten)
        {
            WriteOutput(currentTime);
            outputWritten = true;
        }

        // Infectious mosquito buffer only needs to carry over for one extra timestep
        infectious_mosquitos_current.swap(infectious_mosquitos_previous);
        infectious_mosquitos_current.clear();

        // Let's only clear the infected mosquito and human buffers periodically
        if (timeStep % 10)
            return;

        int threshold = timeStep - 40;  // erase info older than longest plausible mosquito lifetime

        for (auto it = infected_mosquito_buffer.cbegin(); it != infected_mosquito_buffer.cend();)
        {
            Location location = it->second;
            int ts = location.second; // time-step of newly infected mosquito by ID
            if (ts < threshold)
            {
                //LOG_VALID_F("Clearing mosquito (id=%d) infected at t=%d, node_id=%d.\n", it->first, ts, location.first);
                infected_mosquito_buffer.erase(it++);
            }
            else
            {
                ++it;
            }
        }

        for (auto it = infected_human_buffer.cbegin(); it != infected_human_buffer.cend();)
        {
            Location location = it->first;
            int ts = location.second; // time-step of Location of infected human
            if (ts < threshold)
            {
                //LOG_VALID_F("Clearing infected human buffer at t=%d, node_id=%d.\n", ts, location.first);
                infected_human_buffer.erase(it++);
            }
            else
            {
                ++it;
            }
        }
    }

    void MalariaTransmissionReport::SerializeTransmissions(IJsonObjectAdapter& pIJsonObj,
                                                           JSerializer& js )
    {
        // transmitted infections (new infections linked to infecting human via infectious vector)
        pIJsonObj.Insert("transmissions");
        pIJsonObj.BeginArray();
        for(auto &transmission : transmissions)
        {
            transmission->Serialize(pIJsonObj, js);
        }
        pIJsonObj.EndArray();

        // sampled infections (treated clinical cases, reactive follow-up, etc.)
        pIJsonObj.Insert("samples");
        pIJsonObj.BeginArray();
        for (auto &sample : samples)
        {
            sample->Serialize(pIJsonObj, js);
        }
        pIJsonObj.EndArray();
    }

    void MalariaTransmissionReport::WriteOutput( float currentTime )
    {
        // Open output file
        ofstream ofs;
        std::ostringstream output_file_name;
        output_file_name << GetBaseOutputFilename() << ".json";
        LOG_INFO_F( "Writing file: %s\n", output_file_name.str().c_str() );
        ofs.open( FileSystem::Concat( EnvPtr->OutputPath, output_file_name.str() ).c_str() );
        if (!ofs.is_open())
        {
            throw FileIOException( __FILE__, __LINE__, __FUNCTION__, output_file_name.str().c_str() );
        }

        // Accumulate array of transmissions as JSON
        JSerializer js;
        LOG_DEBUG("Creating JSON object adaptor.\n");
        IJsonObjectAdapter* pIJsonObj = CreateJsonObjAdapter();
        LOG_DEBUG("Creating new JSON writer.\n");
        pIJsonObj->CreateNewWriter();
        LOG_DEBUG("Beginning transmission array.\n");
        pIJsonObj->BeginObject();
        SerializeTransmissions(*pIJsonObj, js);
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
            throw FileIOException( __FILE__, __LINE__, __FUNCTION__, output_file_name.str().c_str() );
        }

        if (ofs.is_open())
        {
            ofs.close();
        }
        pIJsonObj->FinishWriter();
        delete pIJsonObj ;
    }
}
