/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include <sstream>

#include "GroupEventCoordinator.h"
#include "InterventionFactory.h"
#include "TBContexts.h" //for querying to IIndividualHumanTB

SETUP_LOGGING( "GroupEventCoordinator" )


namespace Kernel
{

    IMPLEMENT_FACTORY_REGISTERED(GroupInterventionDistributionEventCoordinator)

    IMPL_QUERY_INTERFACE2(GroupInterventionDistributionEventCoordinator, IEventCoordinator, IConfigurable)

    // ctor
    GroupInterventionDistributionEventCoordinator::GroupInterventionDistributionEventCoordinator() 
        : StandardInterventionDistributionEventCoordinator()
    {
        LOG_DEBUG("GroupInterventionDistributionEventCoordinator ctor\n");
    }

    bool
    GroupInterventionDistributionEventCoordinator::Configure(
        const Configuration * inputJson
    )
    {
        JsonConfigurable::_useDefaults = InterventionFactory::useDefaults;

        bool ret = StandardInterventionDistributionEventCoordinator::Configure(inputJson);
        if( ret )
        {
            if( (demographic_restrictions.GetTargetDemographic() == TargetDemographicType::ExplicitDiseaseState) || JsonConfigurable::_dryrun )
            {
                initConfig( "Target_Disease_State", target_disease_state, inputJson, MetadataDescriptor::Enum("target_disease_state", Target_Disease_State_DESC_TEXT, MDD_ENUM_ARGS(TargetGroupType)));
            }
        }

        return ret;
    }

 
    bool
    GroupInterventionDistributionEventCoordinator::qualifiesDemographically(
        const IIndividualHumanEventContext * const pIndividual
    )
    {
        bool retQualifies = true;

        if( demographic_restrictions.GetTargetDemographic() == TargetDemographicType::ExplicitDiseaseState )
        {
            //TB SPECIFIC DISEASE STATES
            IIndividualHumanTB* tb_ind = nullptr;
            if(const_cast<IIndividualHumanEventContext*>(pIndividual)->QueryInterface( GET_IID( IIndividualHumanTB ), (void**)&tb_ind ) != s_OK)
            { 
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "pIndividual", "IIndividualHumanTB", "IIndividualHumanEventContext" );
            }

            if ( target_disease_state == TargetGroupType::Infected )
            {
                if ( !pIndividual->IsInfected() ) { return false; } // Only infected people in this target group
            }
            else if( target_disease_state == TargetGroupType::ActiveInfection )
            {
                if ( !( tb_ind->HasActiveInfection() ) ) {return false;} // Only active infection people in this target group
            }
            else if( target_disease_state == TargetGroupType::LatentInfection )
            {
                if ( !( tb_ind->HasLatentInfection() ) ) {return false;} // Only latent infection people in this target group
            }
            else if( target_disease_state == TargetGroupType::MDR )
            {
                if ( !( tb_ind->IsMDR() ) ) {return false;} // Only MDR+ people in this target group
            }
            else if( target_disease_state == TargetGroupType::TreatmentNaive )
            {
                if ( !( tb_ind->IsTreatmentNaive() ) ) {return false;} // Only treatment naive in this target group
            }
            else if( target_disease_state == TargetGroupType::HasFailedTreatment )
            {
                if ( !( tb_ind->HasFailedTreatment() ) ) {return false;} // Only people who have failed a treatment before in this target group (should be mostly active infection?)
            } 
            else if( target_disease_state == TargetGroupType::ActiveHadTreatment )
            {
                if ( !( tb_ind->HasActiveInfection() ))  {LOG_DEBUG("individual doesn't have active infection \n"); return false;} // Only active infection people who have gotten a treatment in this target group
                if ( !( tb_ind->HasFailedTreatment() ))  {LOG_DEBUG("individual never had a treatment before \n"); return false;} // Only active infection people who have gotten a treatment in this target group
            }
        }

        //also check the base class
        retQualifies = StandardInterventionDistributionEventCoordinator::qualifiesDemographically( pIndividual );

        return retQualifies;
    }


}


