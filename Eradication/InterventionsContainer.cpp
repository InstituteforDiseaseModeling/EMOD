/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"

#include "SimpleTypemapRegistration.h"
#include "Debug.h"
#include "Exceptions.h"
#include "Sugar.h"
#include "Environment.h"
#include "InterventionsContainer.h"
#include "Individual.h"                // for implementation of IIndividualHumanContext functions e.g. GetEventContext()
#include <typeinfo>
#include "NodeEventContext.h"

// TBD: currently included for JDeserialize only. Once we figure out how to wrap the deserialize 
// into rapidjsonimpl class, then this is not needed
#include "RapidJsonImpl.h"

static const char* _module = "InterventionsContainer";
    
namespace Kernel
{
    QueryResult
    InterventionsContainer::QueryInterface( iid_t iid, void** ppinstance )
    {
        assert(ppinstance); // todo: add a real message: "QueryInterface requires a non-NULL destination!");

        if ( !ppinstance )
            return e_NULL_POINTER;

        ISupports* foundInterface;

        GET_IID(IIndividualHumanInterventionsContext);
        iid_t retVal = GET_IID(IIndividualHumanInterventionsContext);
        if (iid == retVal)
            foundInterface = static_cast<IIndividualHumanInterventionsContext*>(this);
        else if (iid == GET_IID(IInterventionConsumer))
            foundInterface = static_cast<IInterventionConsumer*>(this);
        else if (iid == GET_IID(IVaccineConsumer))
            foundInterface = static_cast<IVaccineConsumer*>(this);
        else if (iid == GET_IID(IDrugVaccineInterventionEffects))
            foundInterface = static_cast<IDrugVaccineInterventionEffects*>(this);
        else if (iid == GET_IID(IPropertyValueChangerEffects))
            foundInterface = static_cast<IPropertyValueChangerEffects*>(this);
        else
            foundInterface = 0;

        QueryResult status;
        if ( !foundInterface )
        {
            status = e_NOINTERFACE;
            // if we had a base class we would QI down into it, but we don't.
        }
        else
        {
            //foundInterface->AddRef();           // not implementing this yet!
            status = s_OK;
        }

        *ppinstance = foundInterface;
        return status;
    }

    InterventionsContainer::~InterventionsContainer()
    {
        for (auto intervention : interventions)
        {
            delete intervention;
        }
    }

    void InterventionsContainer::PropagateContextToDependents()
    {
        IIndividualHumanContext *context = GetParent();
        for (auto intervention : interventions)
        {
            intervention->SetContextTo(context);
        }
    }

    std::list<IDistributableIntervention*> InterventionsContainer::GetInterventionsByType(const std::string &type_name)
    {
        std::list<IDistributableIntervention*> interventions_of_type;
        LOG_DEBUG_F( "Looking for intervention of type %s\n", type_name.c_str() );
        for (auto intervention : interventions)
        {
            std::string cur_iv_type_name = typeid( *intervention ).name();
            LOG_DEBUG_F("intervention name = %s\n", cur_iv_type_name.c_str());
            if( cur_iv_type_name == type_name )
            {
                LOG_DEBUG("Found one...\n");
                interventions_of_type.push_back( intervention );
            }
            /*else
            {
                LOG_INFO_F("No match: you asked about %s but I have %s\n", type_name, cur_iv_type_name);
            }*/
        }

        return interventions_of_type;
    }

    void InterventionsContainer::PurgeExisting(
        const std::string &iv_name
    )
    {
        for (auto intervention : interventions)
        {
            std::string cur_iv_type_name = typeid( *intervention ).name();
            if( cur_iv_type_name == iv_name )
            {
                LOG_DEBUG_F("Found an existing intervention by that name (%s) which we are purging\n", iv_name.c_str());
                interventions.remove( intervention );
                delete intervention;
                break;
            }
        }
    }
 
    void InterventionsContainer::Update(float dt)
    {
        drugVaccineReducedAcquire   = 1.0;
        drugVaccineReducedTransmit  = 1.0;
        drugVaccineReducedMortality = 1.0;

        if ( !interventions.empty() )
        {
            for (auto intervention : interventions)
            {
                intervention->Update(dt);
            }

            // TODO: appears that it might be more efficient to remove on the fly
            std::list<IDistributableIntervention*> dead_ivs;
            for (auto intervention : interventions)
            {
                if( intervention->Expired() )
                {
                    LOG_DEBUG("Found an expired intervention\n");
                    dead_ivs.push_back( intervention );
                }
            }

            // Remove any expired interventions (e.g., calendars). Don't want these things accumulating forever.
            for (auto intervention : dead_ivs)
            {
                LOG_DEBUG("Destroying an expired intervention.\n");
                interventions.remove( intervention );
                //pIV->Release(); // is refcounting implemented on interventions?
                delete intervention;
            }
        }
    }

    InterventionsContainer::InterventionsContainer() :
        parent(NULL),
        drugVaccineReducedAcquire(1.0f), 
        drugVaccineReducedTransmit(1.0f),
        drugVaccineReducedMortality(1.0f)
    {
    }

    bool InterventionsContainer::GiveIntervention(
        IDistributableIntervention* iv
    )
    {
        interventions.push_back( iv );
        // We need to increase the reference counter here to represent fact that interventions container
        // is keeping a pointer to the intervention. (Otherwise when event coordinator calls Release,
        // and ref counter is decremented, the intervention object will delete itself.)
        iv->AddRef();
        iv->SetContextTo( parent );
        // TODO: For vaccine, now vaccine intervention needs to call ApplyVaccineTake on itself (???)
        LOG_DEBUG_F("InterventionsContainer has %d interventions now\n", interventions.size());
        return true;
    }

    void InterventionsContainer::UpdateVaccineAcquireRate(
        float acquire
    )
    {
        drugVaccineReducedAcquire *= (1.0f-acquire);
    }

    void InterventionsContainer::UpdateVaccineTransmitRate(
        float xmit
    )
    {
        drugVaccineReducedTransmit *= (1.0f-xmit);
    }

    void InterventionsContainer::UpdateVaccineMortalityRate(
        float mort
    )
    {
        drugVaccineReducedMortality *= (1.0f-mort);
    }
    
    void InterventionsContainer::ChangeProperty(
        const char * property,
        const char * new_value
    )
    {
/*
        LOG_DEBUG_F( "ChangeProperty: property = %s, new_value = %s\n", property, new_value );
        if( strlen( property ) == 0 )
        {
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "ChangeProperty called with empty property string." );
        }
        if( strlen( new_value ) == 0 )
        {
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "ChangeProperty called with empty value string." );
        }
*/
        // Get parent property (remove need for casts)
        tProperties* pProps = parent->GetEventContext()->GetProperties();
        release_assert( pProps );
        // Check that property exists, except Age_Bins which are special case. We bootstrap individuals into age_bins at t=1,
        // with no prior existing age bin property.
        if( ( std::string( property ) != "Age_Bin" ) && ( pProps->find( property ) == pProps->end() ) )
        {
            throw BadMapKeyException( __FILE__, __LINE__, __FUNCTION__, "properties", property );
        }

        INodeContext* pNode = NULL;
        if ( s_OK != parent->GetEventContext()->GetNodeEventContext()->QueryInterface(GET_IID(INodeContext), (void**)&pNode) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent->GetEventContext()->GetNodeEventContext()", "INodeContext", "INodeEventContext" );
        }
        pNode->checkValidIPValue( property, new_value );
        //dynamic_cast<INodeContext*>(parent->GetEventContext()->GetNodeEventContext())->checkValidIPValue( property, new_value );

        if( (*pProps)[ property ] != new_value )
        {
            LOG_DEBUG_F( "Moving individual (%lu) property %s from %s to %s\n", parent->GetSuid().data, property, (*pProps)[ property ].c_str(), new_value );
            parent->UpdateGroupPopulation(-1.0f);
            (*pProps)[ property ] = new_value;
            parent->UpdateGroupMembership();
            parent->UpdateGroupPopulation(1.0f);
        }
        else
        {
            LOG_WARN_F( "ChangeProperty found that individual %lu already has property value %s.\n", parent->GetSuid().data, new_value );
        }
    }

    void
    InterventionsContainer::SetContextTo(
        IIndividualHumanContext* context
    )
    {
        parent = context;
        if (parent)
        {
            PropagateContextToDependents();
        }
    }

    IIndividualHumanContext*
    InterventionsContainer::GetParent()
    {
        return parent;
    }

    float InterventionsContainer::GetInterventionReducedAcquire()   const { return drugVaccineReducedAcquire; }
    float InterventionsContainer::GetInterventionReducedTransmit()  const { 
        return drugVaccineReducedTransmit;
    }
    float InterventionsContainer::GetInterventionReducedMortality() const { return drugVaccineReducedMortality; }
}

#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
BOOST_CLASS_EXPORT(Kernel::InterventionsContainer)
namespace Kernel
{
    template<class Archive>
    void serialize(Archive &ar, InterventionsContainer &cont, const unsigned int v)
    {
        //ar.template register_type<Kernel::PolioInterventionsContainer>();
        ar & cont.drugVaccineReducedAcquire;
        ar & cont.drugVaccineReducedTransmit;
        ar & cont.drugVaccineReducedMortality;
        ar & cont.interventions;
    }

// Stupid boost serialization crap that suddenly is needed when you move code around!
template void serialize(boost::archive::binary_iarchive & ar, InterventionsContainer&, const unsigned int file_version);
template void serialize(boost::archive::binary_oarchive & ar, InterventionsContainer&, const unsigned int file_version);
template void serialize(boost::mpi::packed_skeleton_oarchive&, InterventionsContainer&, unsigned int);
template void serialize(boost::mpi::detail::content_oarchive&, InterventionsContainer&, unsigned int);
template void serialize(boost::mpi::packed_skeleton_iarchive&, InterventionsContainer&, unsigned int);
template void serialize(boost::mpi::detail::mpi_datatype_oarchive&, InterventionsContainer&, unsigned int);
template void serialize(boost::mpi::packed_oarchive&, InterventionsContainer&, unsigned int);
template void serialize(boost::mpi::packed_iarchive&, InterventionsContainer&, unsigned int);

}
#endif

#if USE_JSON_SERIALIZATION || USE_JSON_MPI
namespace Kernel {

    // IJsonSerializable Interfaces
    void InterventionsContainer::JSerialize( IJsonObjectAdapter* root, JSerializer* helper ) const
    {
        root->BeginObject();

        root->Insert("drugVaccineReducedAcquire", drugVaccineReducedAcquire);
        root->Insert("drugVaccineReducedTransmit", drugVaccineReducedTransmit);
        root->Insert("drugVaccineReducedMortality", drugVaccineReducedMortality);

        root->Insert("interventions");
        root->BeginArray();
        for (auto intervention : interventions)
        {
            static_cast<BaseIntervention*>(intervention)->JSerialize(root, helper);
        }
        root->EndArray();
        root->EndObject();
    }

    void InterventionsContainer::JDeserialize( IJsonObjectAdapter* root, JSerializer* helper )
    {
        rapidjson::Document * doc = (rapidjson::Document*) root; // total hack to get around build path issues with rapid json and abstraction

        drugVaccineReducedAcquire   = (*doc)["drugVaccineReducedAcquire"].GetDouble();
        drugVaccineReducedTransmit  = (*doc)["drugVaccineReducedTransmit"].GetDouble();
        drugVaccineReducedMortality = (*doc)["drugVaccineReducedMortality"].GetDouble();

        // interventions is a list, so get the size first
        unsigned int intervention_count = (*doc)["interventions"].Size();

        if (intervention_count > 0)
            LOG_INFO_F( "num_interventions = %d, %f, %f, %f\n", intervention_count, drugVaccineReducedAcquire, drugVaccineReducedTransmit, drugVaccineReducedTransmit);

        for (unsigned int iid = 0; iid < intervention_count; iid++ )
        {
            std::string class_name = (*doc)["interventions"][iid]["class"].GetString();

            LOG_INFO_F( "Deserializating intenventions container iid=%d class_name=%s.\n",iid, class_name.c_str() );
           
            // Create the intervention and push it into the list
            json::Object tmpJson;
            tmpJson["class"] = json::String(class_name);
            Configuration * tmpConfig = Configuration::CopyFromElement( tmpJson );

            // TBD: Handle Node-targeted interventions (CreateNDIIntervention)
            IDistributableIntervention* newintven = InterventionFactory::getInstance()->CreateIntervention((const Configuration*)tmpConfig);
            delete tmpConfig;
            interventions.push_back(newintven);
            dynamic_cast<BaseIntervention *>(newintven)->JDeserialize((IJsonObjectAdapter*)&(*doc)["interventions"][iid],helper);
        }
    }
} // namespace Kernel

#endif
