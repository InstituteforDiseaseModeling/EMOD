/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "ReferenceTrackingEventCoordinator.h"

namespace Kernel
{
    ENUM_DEFINE(TargetDiseaseStateType,
        ENUM_VALUE_SPEC( Everyone                      , 1) 
        ENUM_VALUE_SPEC( HIV_Positive                  , 2) 
        ENUM_VALUE_SPEC( HIV_Negative                  , 3) 
        ENUM_VALUE_SPEC( Tested_Positive               , 4) 
        ENUM_VALUE_SPEC( Tested_Negative               , 5)
        ENUM_VALUE_SPEC( Not_Tested_Or_Tested_Negative , 6))

    class ReferenceTrackingEventCoordinatorHIV : public ReferenceTrackingEventCoordinator 
    {
        DECLARE_FACTORY_REGISTERED_EXPORT(EventCoordinatorFactory, ReferenceTrackingEventCoordinatorHIV, IEventCoordinator)    

    public:
        DECLARE_CONFIGURED(ReferenceTrackingEventCoordinatorHIV)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

        ReferenceTrackingEventCoordinatorHIV();
        virtual ~ReferenceTrackingEventCoordinatorHIV() { } 

        virtual bool qualifiesDemographically( const IIndividualHumanEventContext * pIndividual );

#if USE_JSON_SERIALIZATION
    public:
        // IJsonSerializable Interfaces
        virtual void JSerialize( IJsonObjectAdapter* root, JSerializer* helper ) const;
        virtual void JDeserialize( IJsonObjectAdapter* root, JSerializer* helper );
#endif    
    protected:
        TargetDiseaseStateType::Enum target_disease_state;

#if USE_BOOST_SERIALIZATION
    private:
        friend class ::boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive &ar, StandardInterventionDistributionEventCoordinator &ec, const unsigned int v);
#endif
    };
}
