/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <string>
#include <map>

#include "WaningEffectMap.h"

namespace Kernel
{
    class WaningEffectMapCount : public WaningEffectMapPiecewise, public IWaningEffectCount
    {
        DECLARE_FACTORY_REGISTERED_EXPORT( WaningEffectFactory, WaningEffectMapCount, IWaningEffect )
        DECLARE_QUERY_INTERFACE()
    public:
        WaningEffectMapCount();
        WaningEffectMapCount( const WaningEffectMapCount& rOrig );
        virtual ~WaningEffectMapCount();


        // WaningEffectMapAbstract methods
        virtual IWaningEffect* Clone() override;
        virtual void  Update( float dt ) override;
        virtual float Current() const override;

        //IWaningEffectCount methods
        virtual int32_t AddRef( void ) { return WaningEffectMapPiecewise::AddRef(); }
        virtual int32_t Release( void ) { return WaningEffectMapPiecewise::Release(); }
        virtual void SetCount( uint32_t numCounts ) override;
        virtual bool IsValidConfiguration( uint32_t maxCount ) const override;

    protected:
        virtual bool ConfigureExpiration( const Configuration* config );
        virtual bool ConfigureReferenceTimer( const Configuration* config );

        bool m_SetCountCalled;

        DECLARE_SERIALIZABLE( WaningEffectMapCount );
    };
}