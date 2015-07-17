/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include <string>
#include <list>
#include <vector>

#include "ISTIInterventionsContainer.h"
#include "Interventions.h"
#include "InterventionsContainer.h"
#include "MaleCircumcision.h" // for ICircumcision interface
#include "SimpleTypemapRegistration.h"
#include "IRelationship.h"

namespace Kernel
{
    // this container becomes a help implementation member of the IndividualHumanSTI class 
    // it needs to implement consumer interfaces for all the relevant intervention types


    class STIInterventionsContainer
        : public InterventionsContainer
        , public ISTIInterventionsContainer
        , public ISTIBarrierConsumer
        , public ISTICircumcisionConsumer 
        , public ISTICoInfectionStatusChangeApply
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()

    public:
        STIInterventionsContainer();
        virtual ~STIInterventionsContainer();

        // ISupports
        virtual QueryResult QueryInterface(iid_t iid, void** pinstance);
        virtual bool GiveIntervention( IDistributableIntervention * pIV );

        // ISTIBarrierConsumer 
        virtual void UpdateSTIBarrierProbabilitiesByType( RelationshipType::Enum rel_type, SigmoidConfig config_overrides );
        virtual SigmoidConfig GetSTIBarrierProbabilitiesByRelType( RelationshipType::Enum rel_type ) const;

        // ISTICircumcisionConsumer 
        virtual bool IsCircumcised( void ) const;
        virtual bool ApplyCircumcision( ICircumcision *);

        // IPropertyValueChangerEffects
        virtual void ChangeProperty( const char *property, const char* new_value);

        virtual void Update(float dt); // hook to update interventions if they need it

        virtual float GetInterventionReducedAcquire() const;
        virtual float GetInterventionReducedTransmit() const;

        // ISTICoInfectionStatusChangeApply 
        virtual void SpreadStiCoInfection();
        virtual void CureStiCoInfection();

    protected:
        bool is_circumcised;

        std::map< RelationshipType::Enum, SigmoidConfig > STI_blocking_overrides;

    private:
#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
        friend class ::boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive &ar, STIInterventionsContainer& container, const unsigned int v);
#endif
    };
}
