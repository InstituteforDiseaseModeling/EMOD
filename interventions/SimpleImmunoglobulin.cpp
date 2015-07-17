/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"
#include "SimpleImmunoglobulin.h"

#include "Common.h"             // for INFINITE_TIME
#include "InterventionEnums.h"  // for InterventionDurabilityProfile, ImmunoglobulinType, etc.

static const char * _module = "SimpleImmunoglobulin";

namespace Kernel
{
    IMPLEMENT_FACTORY_REGISTERED(SimpleImmunoglobulin)

    SimpleImmunoglobulin::SimpleImmunoglobulin()
    {
        // done in base class ctor, not needed here.
        // primary_decay_time_constant...
        // cost_per_unit...
    }

    bool
    SimpleImmunoglobulin::Configure(
        const Configuration * inputJson
    )
    {
        vaccine_take = 1.0; // immunoglobulin always takes in the model

        initConfig( "Durability_Time_Profile", durability_time_profile, inputJson, MetadataDescriptor::Enum("Durability_Time_Profile", SI_Durability_Time_Profile_DESC_TEXT, MDD_ENUM_ARGS(InterventionDurabilityProfile) ) );
        if ( durability_time_profile == InterventionDurabilityProfile::BOXDECAYDURABILITY || JsonConfigurable::_dryrun )
        {
            initConfigTypeMap("Secondary_Decay_Time_Constant", &secondary_decay_time_constant, SI_Secondary_Decay_Time_Constant_DESC_TEXT, 0, INFINITE_TIME);
        }
        initConfig( "Vaccine_Type", vaccine_type, inputJson, MetadataDescriptor::Enum("immunoglobulin_type", SI_Vaccine_Type_DESC_TEXT, MDD_ENUM_ARGS(ImmunoglobulinType))); // required? 
    
        initConfigTypeMap("Reduced_Acquire", &current_reducedacquire, SI_Reduced_Acquire_DESC_TEXT, 0, 1, 1);
        initConfigTypeMap("Reduced_Transmit", &current_reducedtransmit, SI_Reduced_Transmit_DESC_TEXT, 0, 1, 1);

        return JsonConfigurable::Configure( inputJson );
    }
}

#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
BOOST_CLASS_EXPORT(Kernel::SimpleImmunoglobulin)
namespace Kernel {
    template<class Archive>
    void serialize(Archive &ar, SimpleImmunoglobulin &vacc, const unsigned int v)
    {
        ar & boost::serialization::base_object<SimpleVaccine>(vacc);
    }
}

#endif