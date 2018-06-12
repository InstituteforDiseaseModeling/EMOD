/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "MosquitoRelease.h"

#include "Exceptions.h"
#include "InterventionFactory.h"
#include "SimulationConfig.h"
#include "NodeVectorEventContext.h"  // for IMosquitoReleaseConsumer methods
#include "VectorParameters.h"

SETUP_LOGGING( "MosquitoRelease" )

namespace Kernel
{
    void
    ResistanceHegGenetics::ConfigureFromJsonAndKey(
        const Configuration * inputJson,
        const std::string& key
    )
    {
        try
        {
            initConfig( "Pesticide_Resistance", pesticideResistance, &(*inputJson)[key], MetadataDescriptor::Enum("Pesticide_Resistance", MR_Released_Pesticide_Resistance_DESC_TEXT , MDD_ENUM_ARGS(VectorAllele)));
        }
        catch( Kernel::DetailedException &e )
        {
            //throw JsonTypeConfigurationException( __FILE__, __LINE__, __FUNCTION__, "Pesticide_Resistance", (*inputJson)[key], e.GetMsg() );
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, e.GetMsg() );
        }
        try
        {
            initConfig( "HEG", HEG, &(*inputJson)[key], MetadataDescriptor::Enum("HEG", MR_Released_HEGs_DESC_TEXT, MDD_ENUM_ARGS(VectorAllele)));
        }
        catch( Kernel::DetailedException &e )
        { 
            //throw JsonTypeConfigurationException( __FILE__, __LINE__, __FUNCTION__, "HEG", (*inputJson)[key], e.GetMsg() );
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, e.GetMsg() );
        }
        LOG_INFO_F( "pesticideResistance = %s, HEG = %s\n", VectorAllele::pairs::lookup_key(pesticideResistance), VectorAllele::pairs::lookup_key(HEG) );
    }

    json::QuickBuilder
    ResistanceHegGenetics::GetSchema()
    {
        json::QuickBuilder schema( GetSchemaBase() );
        auto tn = JsonConfigurable::_typename_label();
        auto ts = JsonConfigurable::_typeschema_label();

        // 2 blocks needed for correct schema text
        {
            auto enum_md = MetadataDescriptor::Enum("VectorAllele", MR_Released_Pesticide_Resistance_DESC_TEXT, MDD_ENUM_ARGS(VectorAllele));
            MetadataDescriptor::Enum * pEnumMd = const_cast<MetadataDescriptor::Enum *>(&enum_md);
            json::Element *elem_copy = _new_ json::Element(pEnumMd->GetSchemaElement());
            auto enumSchema = json::QuickBuilder( *elem_copy ); 
            schema[ ts ][ "Pesticide_Resistance" ] = enumSchema.As< json::Object >();
            delete elem_copy;
        }

        {
            auto enum_md = MetadataDescriptor::Enum("VectorAllele", MR_Released_HEGs_DESC_TEXT, MDD_ENUM_ARGS(VectorAllele));
            MetadataDescriptor::Enum * pEnumMd = const_cast<MetadataDescriptor::Enum *>(&enum_md);
            json::Element *elem_copy = _new_ json::Element(pEnumMd->GetSchemaElement());
            auto enumSchema = json::QuickBuilder( *elem_copy ); 
            schema[ ts ][ "HEG" ] = enumSchema.As< json::Object >();
            schema[ tn ] = json::String( "idmType:ResistanceHegGenetics" );
            delete elem_copy;
        }
        return schema;
    }

    BEGIN_QUERY_INTERFACE_BODY(MosquitoRelease)
        HANDLE_INTERFACE(IConfigurable)
        //HANDLE_INTERFACE(IDistributableIntervention)
        HANDLE_INTERFACE(IBaseIntervention)
        HANDLE_INTERFACE(INodeDistributableIntervention)
        //HANDLE_INTERFACE(IMosquitoRelease)
        HANDLE_ISUPPORTS_VIA(INodeDistributableIntervention)
    END_QUERY_INTERFACE_BODY(MosquitoRelease)

    IMPLEMENT_FACTORY_REGISTERED(MosquitoRelease)

    MosquitoRelease::MosquitoRelease()
    : BaseNodeIntervention()
    , releasedSpecies()
    , vector_genetics()
    , self()
    , mate()
    , releasedNumber(10000)
    {
    }

    MosquitoRelease::MosquitoRelease( const MosquitoRelease& master )
    : BaseNodeIntervention( master )
    , releasedSpecies( master.releasedSpecies )
    , vector_genetics( master.vector_genetics )
    , self( master.self )
    , mate( master.mate )
    , releasedNumber( master.releasedNumber )
    {
    }

    bool
    MosquitoRelease::Configure(
        const Configuration * inputJson
    )
    {
        releasedSpecies.constraints = "<configuration>:Vector_Species_Params.*";
        if( GET_CONFIGURABLE(SimulationConfig) != nullptr )
        {
            releasedSpecies.constraint_param = &GET_CONFIGURABLE(SimulationConfig)->vector_params->vector_species_names;
        }

        initSimTypes( 2, "VECTOR_SIM", "MALARIA_SIM" );
        initConfigTypeMap( "Released_Number",  &releasedNumber,  MR_Released_Number_DESC_TEXT, 1, 1e8, 10000);
        initConfigTypeMap( "Released_Species", &releasedSpecies, MR_Released_Species_DESC_TEXT);
        initConfigTypeMap( "Cost_To_Consumer", &cost_per_unit,   MR_Cost_To_Consumer_DESC_TEXT, 0, 999999, 0.0f);
        mate.pesticideResistance = VectorAllele::NotMated;
        mate.HEG = VectorAllele::NotMated;

        VectorGender::Enum              gender;
        VectorSterility::Enum           sterility;
        VectorWolbachia::Enum           Wolbachia;

        initConfig( "Released_Gender", gender, inputJson, MetadataDescriptor::Enum("Released_Gender", MR_Released_Gender_DESC_TEXT, MDD_ENUM_ARGS(VectorGender)));

        initConfigComplexType( "Released_Genetics", &self, MR_Released_Genetics_DESC_TEXT );
        if( gender == VectorGender::VECTOR_FEMALE || JsonConfigurable::_dryrun )
        {
            initConfigComplexType( "Mated_Genetics", &mate, MR_Mated_Genetics_DESC_TEXT, "Released_Gender", "VECTOR_FEMALE" );
        }

        BaseNodeIntervention::Configure( inputJson );
        initConfig( "Released_Sterility", sterility, inputJson, MetadataDescriptor::Enum("Released_Sterility", MR_Released_Sterility_DESC_TEXT, MDD_ENUM_ARGS(VectorSterility)));
        initConfig( "Released_Wolbachia", Wolbachia, inputJson, MetadataDescriptor::Enum("Released_Wolbachia", MR_Released_Wolbachia_DESC_TEXT, MDD_ENUM_ARGS(VectorWolbachia))); 
        //initVectorConfig( "Released_HEGs", HEGstatus, inputJson, MetadataDescriptor::VectorOfEnum("Released_HEGs", MR_Released_HEGs_DESC_TEXT, MDD_ENUM_ARGS(VectorAllele))); 

        if (!JsonConfigurable::_dryrun)
        {
            if ( gender == VectorGender::VECTOR_MALE && ( mate.pesticideResistance != VectorAllele::NotMated || mate.HEG != VectorAllele::NotMated ) )
            {
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, 
                    "The HEG value in the 'Released_Pesticide_Resistance' and 'Released_HEGs' parameter arrays, namely the status of the mosquito's mate, \
                    must be set to 'NotMated' for male mosquito releases. The following values are only valid for mated females: 'FULL', 'HALF', 'WILD'." );
            }

            vector_genetics = VectorMatingStructure(gender, sterility, Wolbachia);
            vector_genetics.SetPesticideResistance( self.pesticideResistance, mate.pesticideResistance );
            vector_genetics.SetHEG( self.HEG, mate.HEG );
        }

        return true;
    }

    bool MosquitoRelease::Distribute(INodeEventContext *context, IEventCoordinator2* pEC)
    {
        parent = context;

        if( AbortDueToDisqualifyingInterventionStatus( context ) )
        {
            return false;
        }

        IMosquitoReleaseConsumer *imrc;
        if (s_OK != context->QueryInterface(GET_IID(IMosquitoReleaseConsumer), (void**)&imrc))
        {
            throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "context", "IMosquitoReleaseConsumer", "INodeEventContext");
        }

        bool wasDistributed = false;
        if( getNumber() > 0)
        {
            imrc->ReleaseMosquitoes( cost_per_unit, getSpecies(), getVectorGenetics(), getNumber() );
            wasDistributed = true;
        }

        return wasDistributed;
    }

    void MosquitoRelease::Update( float dt )
    {
        // Distribute() doesn't call GiveIntervention() for this intervention, so it isn't added to the NodeEventContext's list of NDI
        throw IllegalOperationException(__FILE__, __LINE__, __FUNCTION__, "MosquitoRelease::Update() should not be called.");
    }

    const std::string& MosquitoRelease::getSpecies() const
    {
        return releasedSpecies;
    }

    const VectorMatingStructure& MosquitoRelease::getVectorGenetics() const
    {
        return vector_genetics;
    }

    uint32_t MosquitoRelease::getNumber() const
    {
        return releasedNumber;
    }
}

#if 0
namespace Kernel {
    template<class Archive>
    void serialize_inner(Archive &ar, const unsigned int v)
    {
        ar & releasedSpecies;
        ar & vector_genetics;
        ar & releasedNumber;
    }
}
#endif
