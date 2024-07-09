
#include "stdafx.h"

#include "ReportUtilitiesSTI.h"
#include "Debug.h"
#include "IIndividualHuman.h"
#include "IIndividualHumanSTI.h"
#include "IndividualEventContext.h"
#include "StrainIdentity.h"
#include "IInfectable.h"
#include "Sugar.h"

// Module name for logging
static const char * _module = "ReportUtilitiesSTI"; 

using namespace Kernel;

namespace ReportUtilitiesSTI
{
    std::string GetRelationshipTypeColumnHeader()
    {
        std::stringstream ss;
        ss << "Rel_type (";
        for( int i = 0; i < RelationshipType::COUNT; ++i )
        {
            ss << i << " = " << RelationshipType::pairs::get_keys()[ i ];
            if( (i + 1) < RelationshipType::COUNT )
            {
                ss << "; ";
            }
        }
        ss << ")";

        return ss.str();
    }

    IRelationship* GetTransmittingRelationship( IIndividualHumanEventContext* pRecipientContext )
    {
        const IIndividualHuman* p_recipient = pRecipientContext->GetIndividualHumanConst();

        IIndividualHumanSTI* p_recipient_sti = nullptr;
        if (pRecipientContext->QueryInterface(GET_IID(IIndividualHumanSTI), (void**)&p_recipient_sti) != s_OK)
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__,
                                           "pRecipientContext", "IIndividualHumanSTI", "IIndividualHuman" );
        }

        // Let's figure out who the Infector was. This info is stored in the HIVInfection's strainidentity object
        //auto infections = dynamic_cast<IInfectable*>(individual)->GetInfections();
        IInfectable* p_infectable = nullptr;
        if (pRecipientContext->QueryInterface(GET_IID(IInfectable), (void**)&p_infectable) != s_OK)
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "p_recipient", "IInfectable", "IIndividualHuman" );
        }
        auto infections = p_infectable->GetInfections();
        if( infections.size() == 0 )
        {
            // This person cleared their infection on this timestep already! Nothing to report.
            return nullptr;
        }

        auto infection = infections.front(); // Assuming no super-infections here obviously.

        const IStrainIdentity& si = infection->GetInfectiousStrainID();

        // NOTE: Right now we re-use the Generic StrainIdentity object and store 
        // the InfectorID as the AntigenID (which is really just the arbitrary 
        // major id of the strain to my thinking). In the future, each disease 
        // could sub-class StrainIdentity (like we do with other major classes)
        // and put InfectorID in it's own field for HIV's purposes.
        auto infector = si.GetAntigenID();
        if( infector == 0 )
        {
            // could be outbreak or maternal transmission
            return nullptr;
        }

        IRelationship* p_migration_rel =  p_recipient_sti->GetMigratingRelationship();
        if( p_migration_rel != nullptr )
        {
            release_assert( p_migration_rel->GetState() == RelationshipState::MIGRATING );

            IIndividualHumanSTI* p_transmitter_sti = p_migration_rel->GetPartner( p_recipient_sti );
            release_assert( p_transmitter_sti != nullptr );
            if (p_transmitter_sti->GetSuid().data == infector )
            {
                return p_migration_rel;
            }
        }

        std::vector<IRelationship*> relationships = p_recipient_sti->GetRelationships();
        const std::vector<IRelationship*>& r_rels = p_recipient_sti->GetRelationshipsTerminated();
        relationships.insert( relationships.end(), r_rels.begin(), r_rels.end() );

        if( relationships.size() == 0 )
        {
            std::stringstream ss;
            ss << "Must have had a relationship to get here.\n";
            ss << "Rank=" << EnvPtr->MPI.Rank << "  pRecipient-ID=" << pRecipientContext->GetSuid().data << "  p_recipient->GetStateChange()=" << int(p_recipient->GetStateChange());
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }

        for (auto relationship : relationships)
        {
            IIndividualHumanSTI* p_transmitter_sti = relationship->GetPartner( p_recipient_sti );

            // ------------------------------------------------------------------------------------
            // --- The partner could be null when the relationship is paused and the individual has
            // --- moved nodes.  We don't want to check RelationshipState because the couple could
            // --- have consumated and then had one of the partners decide to move/migrate.
            // ------------------------------------------------------------------------------------
            if( p_transmitter_sti != nullptr )
            {
                //GH-611 if( relationship->GetPartnerId( p_recipient_sti->GetSuid() ).data == infector )
                if (p_transmitter_sti->GetSuid().data == infector )
                {
                    return relationship;
                }
            }
            else
            {
                // if the partner is null, then the relationship had better be paused or terminated
                release_assert( (relationship->GetState() == RelationshipState::PAUSED) || (relationship->GetState() == RelationshipState::TERMINATED) );
            }
        }
        std::stringstream ss;
        ss << "Could not find transmitting parter (" << infector << ") for infected partner (" << p_recipient->GetSuid().data << ").";
        throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
    }
}
