/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "RelationshipReporting.h"
#include "IIndividualHuman.h"
#include "IIndividualHumanSTI.h"
#include "IIndividualHumanHIV.h"
#include "InfectionHIV.h"
#include "IHIVInterventionsContainer.h"

static const char* _module = "RelationshipReporting";

namespace Kernel
{
    ParticipantInfo::ParticipantInfo()
        : id(0)
        , is_infected(false)
        , gender(-1)
        , age(0)
        , active_relationship_count(0)
        , cumulative_lifetime_relationships(0)
        , relationships_in_last_six_months(0)
        , extrarelational_flags(0)
        , is_circumcised( false )
        , has_sti( false )
        , is_superspreader( false )
        , transitory_relationship_count(0)
        , informal_relationship_count(0)
        , marital_relationship_count(0)
        , props()
    {}

    std::string CoitalActInfo::GetHeader() const
    {
        std::ostringstream header;
        header << "Time,"
               << "Node_ID,"
               << "Rel_ID,"
               << "A_ID,"
               << "B_ID,"
               << "A_Gender,"
               << "B_Gender," 
               << "A_Age,"
               << "B_Age,"
               << "A_Is_Infected,"
               << "B_Is_Infected,"
               << "Did_Use_Condom,"
               << "A_Num_Current_Rels,"
               << "B_Num_Current_Rels,"
               << "A_Is_Circumcised,"
               << "B_Is_Circumcised,"
               << "A_Has_CoInfection,"
               << "B_Has_CoInfection,"
               // The following are HIV-Specific
               << "A_HIV_Infection_Stage,"
               << "B_HIV_Infection_Stage,"
               << "A_Is_On_ART,"
               << "B_Is_On_ART" ;
        std::string retLine = header.str();
        return retLine;
    }

    void
    CoitalActInfo::GatherLineFromRelationship(
        const IRelationship* pRel
    )
    {
        release_assert( pRel );
        std::ostringstream line;

        IIndividualHumanSTI *sti_A = pRel->MalePartner();
        IIndividualHumanSTI *sti_B = pRel->FemalePartner();

        IIndividualHuman *ih_A;
        if( sti_A->QueryInterface( GET_IID( IIndividualHuman ), (void**)&ih_A ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "sti_A", "IndividualHumanSTI", "IIndividualHuman" );
        }

        IIndividualHuman *ih_B;
        if( sti_B->QueryInterface( GET_IID( IIndividualHuman ), (void**)&ih_B ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "sti_B", "IndividualHumanSTI", "IIndividualHuman" );
        }

        // --------------------------------------------------------
        // --- Assuming that the individuals in a relationship
        // --- must be in the same node.
        // --------------------------------------------------------
        ExternalNodeId_t node_id = ih_A->GetParent()->GetExternalID();

        bool infected_A = sti_A->IsInfected();
        int hiv_infection_stage_A = -1;
        bool On_ART_A = false;
        bool stiCoInfection_A = false;
        IIndividualHumanHIV *hiv_A;
        if( infected_A && sti_A->QueryInterface( GET_IID( IIndividualHumanHIV ), (void**)&hiv_A ) == s_OK )
        {
            hiv_infection_stage_A = hiv_A->GetHIVInfection()->GetStage();
            On_ART_A = hiv_A->GetHIVInterventionsContainer()->OnArtQuery();
            stiCoInfection_A = sti_A->HasSTICoInfection();
        }

        bool infected_B = sti_B->IsInfected();
        int hiv_infection_stage_B = -1;
        bool On_ART_B = false;
        bool stiCoInfection_B = false;
        IIndividualHumanHIV *hiv_B;
        if( infected_B && sti_B->QueryInterface( GET_IID( IIndividualHumanHIV ), (void**)&hiv_B ) == s_OK )
        {
            hiv_infection_stage_B = hiv_B->GetHIVInfection()->GetStage();
            On_ART_B = hiv_B->GetHIVInterventionsContainer()->OnArtQuery();
            stiCoInfection_B = sti_B->HasSTICoInfection();
        }

        const char* gender_A = Gender::pairs::lookup_key( ih_A->GetGender() );
        const char* gender_B = Gender::pairs::lookup_key( ih_B->GetGender() );

        line << time << ','                             // Time
             << node_id << ','                          // Node_ID
             << pRel->GetSuid().data << ','             // Rel_ID 
             << pRel->GetMalePartnerId().data << ','    // A_ID
             << pRel->GetFemalePartnerId().data << ','  // B_ID
             << gender_A << ','                         // A_Gender 
             << gender_B << ','                         // B_Gender
             << ih_A->GetAge()/DAYSPERYEAR << ','       // A_Age
             << ih_B->GetAge()/DAYSPERYEAR << ','       // B_Age
             << infected_A << ','                       // A_Is_Infected
             << infected_B << ','                       // B_Is_Infected
             << pRel->GetUsingCondom() << ','           // Did_Use_Condom
             << sti_A->GetRelationships().size() << ',' // A_Num_Current_Rels 
             << sti_B->GetRelationships().size() << ',' // B_Num_Current_Rels 
             << sti_A->IsCircumcised() << ','           // "A_Is_Circumcised,"
             << sti_B->IsCircumcised() << ','           // "B_Is_Circumcised,"
             << stiCoInfection_A << ','                 // "A_Has_CoInfection,"
             << stiCoInfection_B << ','                 // "B_Has_CoInfection,"
             << hiv_infection_stage_A << ','            // "A_HIV_Infection_Stage,"
             << hiv_infection_stage_B << ','            // "B_HIV_Infection_Stage,"
             << On_ART_A << ','                         // "A_Is_On_ART,"
             << On_ART_B                                // "B_Is_On_ART"
             << endl;
        _line = line.str();
    }

}
