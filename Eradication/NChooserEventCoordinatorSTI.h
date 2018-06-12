/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "NChooserEventCoordinator.h"

namespace Kernel
{
    // ------------------------------------------------------------------------
    // --- TargetedDistributionSTI
    // ------------------------------------------------------------------------

    // Extend/Override to use year instead of time
    class IDMAPI TargetedDistributionSTI : public TargetedDistribution
    {
    public:
        DECLARE_QUERY_INTERFACE()

        TargetedDistributionSTI( NChooserObjectFactory* pObjectFactory );
        virtual ~TargetedDistributionSTI();

        virtual bool operator<( const TargetedDistribution& rThat ) const override;

        virtual void CheckOverlaped( const TargetedDistribution& rPrev ) const override;
        virtual bool IsPastStart( const IdmDateTime& rDateTime ) const override;
        virtual bool IsPastEnd( const IdmDateTime& rDateTime ) const override;

    protected:
        virtual void AddTimeConfiguration() override;
        virtual void CheckTimePeriod() const override;
        virtual float GetStartInDays() const override;
        virtual float GetEndInDays() const override;
        virtual float GetCurrentInDays( const IdmDateTime& rDateTime ) const override;

#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        float m_StartYear;
        float m_EndYear;
#pragma warning( pop )
    };

    // ------------------------------------------------------------------------
    // --- NChooserObjectFactorySTI
    // ------------------------------------------------------------------------
    // Extend so we can create TargetedDistributionSTI objects
    class IDMAPI NChooserObjectFactorySTI : public NChooserObjectFactory
    {
    public:
        NChooserObjectFactorySTI();
        virtual ~NChooserObjectFactorySTI();

        virtual TargetedDistribution* CreateTargetedDistribution() override;
    };

    // ------------------------------------------------------------------------
    // --- NChooserEventCoordinatorSTI
    // ------------------------------------------------------------------------

    // Extend so we can override the object factory
    class IDMAPI NChooserEventCoordinatorSTI : public NChooserEventCoordinator
    {
        DECLARE_FACTORY_REGISTERED_EXPORT(EventCoordinatorFactory, NChooserEventCoordinatorSTI, IEventCoordinator)    
    public:
        DECLARE_QUERY_INTERFACE()

        NChooserEventCoordinatorSTI();
        NChooserEventCoordinatorSTI( NChooserObjectFactory* pObjectFactory );
        virtual ~NChooserEventCoordinatorSTI();

        virtual bool Configure( const Configuration * inputJson ) override;
    };
}
