
#pragma once

#include <functional>
#include "FactorySupport.h"
#include "Interventions.h"
#include "ObjectFactory.h"

namespace Kernel
{
    struct IInterventionFactory
    {
        virtual void Register( const char *classname, instantiator_function_t _if ) = 0;
    };

    class InterventionFactory : public ObjectFactory<IDistributableIntervention,InterventionFactory>
                              , public IInterventionFactory
    {
    public:
        virtual void Register( const char *classname, instantiator_function_t _if ) override;

        // returns NULL if could not create a distributable intervention with the specified definition
        IDistributableIntervention* CreateIntervention( const json::Element& rJsonElement,
                                                        const std::string& rDataLocation,
                                                        const char* parameterName,
                                                        bool throwIfNull=false );
        void CreateInterventionList( const json::Element& rJsonElement,
                                     const std::string& rDataLocation,
                                     const char* parameterName,
                                     std::vector<IDistributableIntervention*>& interventionsList );

        // returns NULL if could not create a node distributable intervention with the specified definition
        INodeDistributableIntervention* CreateNDIIntervention( const json::Element& rJsonElement,
                                                               const std::string& rDataLocation,
                                                               const char* parameterName,
                                                               bool throwIfNull=false );
        void CreateNDIInterventionList( const json::Element& rJsonElement,
                                        const std::string& rDataLocation,
                                        const char* parameterName,
                                        std::vector<INodeDistributableIntervention*>& interventionsList );

        void SetUseDefaults( bool useDefaults );
        bool IsUsingDefaults() const;

    protected:
        template<class IObject, class Factory> friend class Kernel::ObjectFactory;

        InterventionFactory();
        virtual void ModifySchema( json::QuickBuilder& rSchema, ISupports*pObject );

        bool m_UseDefaults;
    };
}
