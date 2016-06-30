/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "DemographicRestrictions.h"
#include "IndividualEventContext.h"
#include "Node.h"
#include "IIndividualHuman.h"

static const char * _module = "DemographicRestrictions";

#define DEFAULT_DEMOGRAPHIC_COVERAGE (1.0)

namespace Kernel
{
    DemographicRestrictions::DemographicRestrictions()
    : allow_age_restrictions(true)
    , demographic_coverage(0)
    , default_target_demographic(TargetDemographicType::Everyone)
    , target_demographic(default_target_demographic)
    , target_age_min(0)
    , target_age_max(0)
    , target_gender(TargetGender::All)
    , property_restrictions_set()
    , property_restrictions()
    , target_residents_only( false )
    {
    }

    DemographicRestrictions::DemographicRestrictions( bool age_restrictions, TargetDemographicType::Enum defaultTargetDemographic )
    : allow_age_restrictions(age_restrictions)
    , demographic_coverage(0)
    , default_target_demographic(defaultTargetDemographic)
    , target_demographic(default_target_demographic)
    , target_age_min(0)
    , target_age_max(0)
    , target_gender(TargetGender::All)
    , property_restrictions_set()
    , property_restrictions()
    , target_residents_only( false )
    {
    }

    void DemographicRestrictions::ConfigureRestrictions( JsonConfigurable* pParent, const Configuration * inputJson )
    {
        pParent->initConfigTypeMap( "Demographic_Coverage", 
                                    &demographic_coverage,
                                    Demographic_Coverage_DESC_TEXT,
                                    0.0, 
                                    1.0, 
                                    DEFAULT_DEMOGRAPHIC_COVERAGE, 
                                    "Intervention_Config.*.iv_type", 
                                    "IndividualTargeted" );

        if( JsonConfigurable::_dryrun ||
            (default_target_demographic == TargetDemographicType::Everyone) || 
            inputJson->Exist("Target_Demographic") )
        {
            pParent->initConfig( "Target_Demographic", 
                                 target_demographic, 
                                 inputJson,
                                 MetadataDescriptor::Enum( "target_demographic", 
                                                           Target_Demographic_DESC_TEXT, 
                                                           MDD_ENUM_ARGS(TargetDemographicType)),
                                 "Intervention_Config.*.iv_type", 
                                 "IndividualTargeted");
        }
        
        if( (target_demographic == TargetDemographicType::ExplicitAgeRanges         ) || 
            (target_demographic == TargetDemographicType::ExplicitAgeRangesAndGender) ||
            JsonConfigurable::_dryrun )
        {
            if( !allow_age_restrictions && !JsonConfigurable::_dryrun )
            {
                std::ostringstream msg;
                msg << typeid(*pParent).name() << " does not allow age restrictions.  'Target_Demographic' cannot be 'ExplicitAgeRanges' or 'ExplicitAgeRangesAndGender'";
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
            }

            pParent->initConfigTypeMap( "Target_Age_Min", &target_age_min, Target_Age_Min_DESC_TEXT, 0.0f, FLT_MAX,    0.0f, "Target_Demographic", "ExplicitAgeRanges" );
            pParent->initConfigTypeMap( "Target_Age_Max", &target_age_max, Target_Age_Max_DESC_TEXT, 0.0f, FLT_MAX, FLT_MAX, "Target_Demographic", "ExplicitAgeRanges" );
            if( (target_demographic == TargetDemographicType::ExplicitAgeRangesAndGender) || JsonConfigurable::_dryrun)
            {
                pParent->initConfig( "Target_Gender", target_gender, inputJson, MetadataDescriptor::Enum("target_gender", Target_Gender_DESC_TEXT, MDD_ENUM_ARGS(TargetGender)) ); 
            }
        }
        else if( (target_demographic == TargetDemographicType::ExplicitGender) || JsonConfigurable::_dryrun )
        {
            pParent->initConfig( "Target_Gender", target_gender, inputJson, MetadataDescriptor::Enum("target_gender", Target_Gender_DESC_TEXT, MDD_ENUM_ARGS(TargetGender)) ); 
        }

        // xpath-y way of saying that the possible values for prop restrictions comes from demographics file IP's.
        property_restrictions_set.value_source = "<demographics>::Defaults.Individual_Properties.*.Property.<keys>:<demographics>::Defaults.Individual_Properties.*.Value.<keys>"; 
        pParent->initConfigTypeMap("Property_Restrictions", &property_restrictions_set, Property_Restriction_DESC_TEXT, "Intervention_Config.*.iv_type", "IndividualTargeted" );

        pParent->initConfigComplexType("Property_Restrictions_Within_Node", &property_restrictions, Property_Restriction_DESC_TEXT, "Intervention_Config.*.iv_type", "IndividualTargeted" );

        pParent->initConfigTypeMap( "Target_Residents_Only", &target_residents_only, Target_Residents_Only_DESC_TEXT, false );
    }

    void DemographicRestrictions::CheckConfiguration()
    {
        if( property_restrictions_set.size() > 0 )
        {
            if( property_restrictions.Size() > 0 )
            {
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, "Cannot have both 'Property_Restrictions' and 'Property_Restrictions_Within_Node'." );
            }

            std::map< std::string, std::string > restriction_map;

            std::set< std::string > dupKeyHelperSet;
            for (const auto& prop : property_restrictions_set)
            {
                // parse, pre-colon is prop key, post is value
                size_t sep = prop.find( ':' );
                const std::string& szKey = prop.substr( 0, sep );
                const std::string& szVal = prop.substr( sep+1, prop.size()-sep );
                if( dupKeyHelperSet.count( szKey ) != 0 )
                {
                    std::string msg;
                    msg += "Duplicate keys in 'Property_Restrictions'. Since entries are AND-ed together, ";
                    msg += "this will always be an empty set since an individual can only have a single ";
                    msg += "value of a given key. Use 'Property_Restrictions_Within_Node' instead.";
                    throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.c_str() );
                }
                dupKeyHelperSet.insert( szKey );
                restriction_map.insert( make_pair( szKey, szVal ) );
            }
            property_restrictions.Add( restriction_map );
        }
    }

    bool DemographicRestrictions::HasDefaultRestrictions() const
    {
        if( demographic_coverage         != DEFAULT_DEMOGRAPHIC_COVERAGE    ) return false;
        if( target_demographic           != TargetDemographicType::Everyone ) return false;
        if( target_residents_only        != false                           ) return false;
        if( property_restrictions.Size() >  0                               ) return false;

        return true;
    }

    bool DemographicRestrictions::IsQualified( const IIndividualHumanEventContext* pIndividual )
    {
        bool retQualifies = true;

        if( target_residents_only )
        {
            IIndividualHuman* p_human = nullptr;
            if (s_OK != const_cast<IIndividualHumanEventContext*>(pIndividual)->QueryInterface(GET_IID(IIndividualHuman), (void**)&p_human) )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "pIndividual", "IIndividualHuman", "IIndividualHumanEventContext" );
            }
            if( !p_human->AtHome() )
            {
                return false ;
            }
        }

        if( (target_demographic == TargetDemographicType::PossibleMothers) && !pIndividual->IsPossibleMother() )
        {
            LOG_DEBUG("Individual not given intervention because not possible mother\n");
            retQualifies = false;
        }
        else if( (target_demographic == TargetDemographicType::ExplicitAgeRanges         ) ||
                 (target_demographic == TargetDemographicType::ExplicitAgeRangesAndGender) )
        {
            if( !allow_age_restrictions )
            {
                std::ostringstream msg;
                msg << "Age Restrictions are not allowed.  'Target_Demographic' cannot be 'ExplicitAgeRanges' or 'ExplicitAgeRangesAndGender'";
                throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
            }

            if( pIndividual->GetAge() < target_age_min * DAYSPERYEAR )
            {
                LOG_DEBUG_F("Individual %lu not given intervention because too young (age=%f) for intervention min age (%f)\n", 
                            pIndividual->GetSuid().data, pIndividual->GetAge(), target_age_min* DAYSPERYEAR);
                retQualifies = false;
            }
            else if( pIndividual->GetAge() > target_age_max * DAYSPERYEAR )
            {
                LOG_DEBUG_F("Individual %lu not given intervention because too old (age=%f) for intervention max age (%f)\n", 
                    pIndividual->GetSuid().data, pIndividual->GetAge(), target_age_max* DAYSPERYEAR);
                retQualifies = false;
            }

            if( retQualifies && (target_demographic == TargetDemographicType::ExplicitAgeRangesAndGender) )
            {
                // Gender = 0 is MALE, Gender = 1 is FEMALE.  Should use Gender::Enum throughout code
                if( (pIndividual->GetGender() == 0) && (target_gender == TargetGender::Female) )
                {
                    retQualifies = false;
                }
                else if( (pIndividual->GetGender() == 1) && (target_gender == TargetGender::Male) )
                {
                    retQualifies = false;
                }
            }
        }
        else if( target_demographic == TargetDemographicType::ExplicitGender )
        {
            // Gender = 0 is MALE, Gender = 1 is FEMALE.  Should use Gender::Enum throughout code
            if( (pIndividual->GetGender() == 0) && (target_gender == TargetGender::Female) )
            {
                retQualifies = false;
            }
            else if( (pIndividual->GetGender() == 1) && (target_gender == TargetGender::Male) )
            {
                retQualifies = false;
            }
        }

        if( retQualifies && (property_restrictions.Size() > 0) )
            {
            retQualifies = property_restrictions.Qualifies( pIndividual );
        }
        else
        {
            LOG_DEBUG( "No property restrictions in event coordiantor applied.\n" );
        }
        LOG_DEBUG_F( "Returning %d from %s\n", retQualifies, __FUNCTION__ );
        return retQualifies;
    }

    void DemographicRestrictions::SetDemographicCoverage( float coverage )
    {
        demographic_coverage = coverage;
    }

    float DemographicRestrictions::GetDemographicCoverage() const
    {
        return demographic_coverage;
    }

    TargetDemographicType::Enum DemographicRestrictions::GetTargetDemographic() const
    {
        return target_demographic;
    }

    TargetGender::Enum DemographicRestrictions::GetTargetGender() const
    {
        return target_gender;
    }

    float DemographicRestrictions::GetMinimumAge() const
    {
        return target_age_min;
    }

    float DemographicRestrictions::GetMaximumAge() const
    {
        return target_age_max;
    }

    std::string DemographicRestrictions::GetPropertyRestrictionsAsString() const
    {
        return property_restrictions.GetAsString();
    }

#if USE_JSON_SERIALIZATION

    // IJsonSerializable Interfaces
    void DemographicRestrictions::JSerialize( IJsonObjectAdapter* root, JSerializer* helper ) const
    {
        root->BeginObject();

        root->Insert("demographic_coverage", demographic_coverage);
        root->Insert("target_demographic", target_demographic);
        root->Insert("target_age_min", target_age_min);
        root->Insert("target_age_max", target_age_max);
        root->Insert("target_gender", target_gender);

        root->Insert("property_restrictions_map");
        root->BeginArray();
        //for (auto& restriction : property_restrictions)
        //{
        //}
        root->EndArray();

        root->EndObject();
    }

    void DemographicRestrictions::JDeserialize( IJsonObjectAdapter* root, JSerializer* helper )
    {
    }
#endif
}

#if USE_BOOST_SERIALIZATION
BOOST_CLASS_EXPORT(Kernel::DemographicRestrictions);
namespace Kernel
{

    template<class Archive>
    void serialize(Archive &ar, DemographicRestrictions &ec, const unsigned int v)
    {
        ar & ec.demographic_coverage;
        ar & ec.property_restrictions_map;
        ar & ec.target_demographic;
        ar & ec.target_age_min;
        ar & ec.target_age_max;
        ar & ec.target_gender;
    }
}
#endif
