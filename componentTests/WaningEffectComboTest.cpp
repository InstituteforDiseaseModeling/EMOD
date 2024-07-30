
#include "stdafx.h"
#include <iostream>
#include <fstream>
#include <memory> // unique_ptr
#include "UnitTest++.h"
#include "componentTests.h"

#include "WaningEffectCombo.h"
#include "IndividualHumanContextFake.h"

using namespace Kernel;

SUITE( WaningEffectComboTest )
{
    TEST( TestRead )
    {
        unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/WaningEffectComboTest/TestRead.json" ) );

        WaningEffectCombo combo;

        try
        {
            combo.Configure( p_config.get() );
        }
        catch( DetailedException& re )
        {
            PrintDebug( re.GetMsg() );
            CHECK( false );
        }

        unique_ptr<IndividualHumanContextFake> p_human( new IndividualHumanContextFake( nullptr, nullptr, nullptr, nullptr ) );

        combo.SetContextTo( p_human.get() );

        p_human->SetAge( 12.0*DAYSPERYEAR );
        combo.Update( 1.0 );
        float current = combo.Current();
        CHECK_CLOSE( 0.0, current, 0.000001 ); // zero because age is < 13

        p_human->SetAge( 15.0*DAYSPERYEAR );
        combo.Update( 1.0 );
        current = combo.Current();
        CHECK_CLOSE( 0.734694, current, 0.000001 ); // age > 13 & second day of exponential with 7-day decay

        combo.Update( 1.0 );
        current = combo.Current();
        CHECK_CLOSE( 0.629738, current, 0.000001 ); // age > 13 & third day of exponential with 7-day decay

        combo.Update( 1.0 );
        current = combo.Current();
        CHECK_CLOSE( 0.539775, current, 0.000001 ); // age > 13 & fourth day of exponential with 7-day decay

        combo.Update( 1.0 );
        current = combo.Current();
        CHECK_CLOSE( 0.462664, current, 0.000001 ); // age > 13 & fifth day of exponential with 7-day decay
    }

    TEST( TestExpireFirst )
    {
        unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/WaningEffectComboTest/TestExpireFirst.json" ) );

        WaningEffectCombo combo;

        try
        {
            combo.Configure( p_config.get() );
        }
        catch( DetailedException& re )
        {
            PrintDebug( re.GetMsg() );
            CHECK( false );
        }

        unique_ptr<IndividualHumanContextFake> p_human( new IndividualHumanContextFake( nullptr, nullptr, nullptr, nullptr ) );
        p_human->SetAge( 12.0*DAYSPERYEAR );

        combo.SetContextTo( p_human.get() );

        combo.Update( 1.0 );
        float current = combo.Current();
        CHECK_CLOSE( 0.1, current, 0.000001 ); // 0.2 from Linear times 0.5 from Box
        CHECK_EQUAL( false, combo.Expired() ); 

        combo.Update( 1.0 );
        current = combo.Current();
        CHECK_CLOSE( 0.2, current, 0.000001 ); // 0.4 from Linear times 0.5 from Box
        CHECK_EQUAL( false, combo.Expired() );

        combo.Update( 1.0 );
        current = combo.Current();
        CHECK_CLOSE( 0.3, current, 0.000001 ); // 0.6 from Linear times 0.5 from Box
        CHECK_EQUAL( true, combo.Expired() ); // end of Linear is time 2
    }

    TEST( TestExpireAll )
    {
        unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/WaningEffectComboTest/TestExpireAll.json" ) );

        WaningEffectCombo combo;

        try
        {
            combo.Configure( p_config.get() );
        }
        catch( DetailedException& re )
        {
            PrintDebug( re.GetMsg() );
            CHECK( false );
        }

        unique_ptr<IndividualHumanContextFake> p_human( new IndividualHumanContextFake( nullptr, nullptr, nullptr, nullptr ) );
        p_human->SetAge( 12.0*DAYSPERYEAR );

        combo.SetContextTo( p_human.get() );

        combo.Update( 1.0 );
        float current = combo.Current();
        CHECK_CLOSE( 0.7, current, 0.000001 ); // 0.2 from Linear times 0.5 from Piecewise
        CHECK_EQUAL( false, combo.Expired() );

        combo.Update( 1.0 );
        current = combo.Current();
        CHECK_CLOSE( 0.9, current, 0.000001 ); // 0.4 from Linear times 0.5 from Piecewise
        CHECK_EQUAL( false, combo.Expired() );

        combo.Update( 1.0 );
        current = combo.Current();
        CHECK_CLOSE( 1.0, current, 0.000001 ); // 0.6 from Linear times 0.5 from Piecewise, capped at 1.0
        CHECK_EQUAL( false, combo.Expired() ); // end of Linear is time 2 but Box expires after 5

        combo.Update( 1.0 );
        current = combo.Current();
        CHECK_CLOSE( 0.5, current, 0.000001 ); // 0.0 from Linear times 0.5 from Piecewise
        CHECK_EQUAL( false, combo.Expired() ); // end of Linear is time 2 but Box expires after 5

        combo.Update( 1.0 );
        current = combo.Current();
        CHECK_CLOSE( 0.5, current, 0.000001 ); // 0.0 from Linear times 0.5 from Piecewise
        CHECK_EQUAL( false, combo.Expired() ); // end of Linear is time 2 but Box expires after 5

        combo.Update( 1.0 );
        current = combo.Current();
        CHECK_CLOSE( 0.0, current, 0.000001 ); // 0.0 from Linear times 0.0 from Piecewise
        CHECK_EQUAL( true, combo.Expired() ); // end of Linear is time 2 but Box expires after 5
    }

    TEST( TestAdd )
    {
        unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/WaningEffectComboTest/TestAdd.json" ) );

        WaningEffectCombo combo;

        try
        {
            combo.Configure( p_config.get() );
        }
        catch( DetailedException& re )
        {
            PrintDebug( re.GetMsg() );
            CHECK( false );
        }

        unique_ptr<IndividualHumanContextFake> p_human( new IndividualHumanContextFake( nullptr, nullptr, nullptr, nullptr ) );
        p_human->SetAge( 12.0*DAYSPERYEAR );

        combo.SetContextTo( p_human.get() );

        combo.Update( 1.0 );
        float current = combo.Current();
        CHECK_CLOSE( 0.7, current, 0.000001 ); // 0.2 from Linear plus 0.5 from Box
        CHECK_EQUAL( false, combo.Expired() );

        combo.Update( 1.0 );
        current = combo.Current();
        CHECK_CLOSE( 0.9, current, 0.000001 ); // 0.4 from Linear plus 0.5 from Box
        CHECK_EQUAL( false, combo.Expired() );

        combo.Update( 1.0 );
        current = combo.Current();
        CHECK_CLOSE( 1.0, current, 0.000001 ); // 0.5 from Linear plus 0.5 from Box
        CHECK_EQUAL( false, combo.Expired() );

        combo.Update( 1.0 );
        current = combo.Current();
        CHECK_CLOSE( 1.0, current, 0.000001 ); // 0.6 from Linear plus 0.5 from Box = capped at 1.0
        CHECK_EQUAL( false, combo.Expired() );

        combo.Update( 1.0 );
        current = combo.Current();
        CHECK_CLOSE( 1.0, current, 0.000001 ); // 0.6 from Linear plus 0.5 from Box = capped at 1.0
        CHECK_EQUAL( true, combo.Expired() );

        combo.Update( 1.0 );
        current = combo.Current();
        CHECK_CLOSE( 0.0, current, 0.000001 ); // 0.6 from Linear plus 0.5 from Box = capped at 1.0
        CHECK_EQUAL( true, combo.Expired() );
    }

    TEST( TestSetInitial )
    {
        unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/WaningEffectComboTest/TestSetInitial.json" ) );

        WaningEffectCombo combo;

        try
        {
            combo.Configure( p_config.get() );
        }
        catch( DetailedException& re )
        {
            PrintDebug( re.GetMsg() );
            CHECK( false );
        }

        unique_ptr<IndividualHumanContextFake> p_human( new IndividualHumanContextFake( nullptr, nullptr, nullptr, nullptr ) );
        p_human->SetAge( 12.0*DAYSPERYEAR );

        combo.SetContextTo( p_human.get() );

        combo.SetInitial( 0.25 );

        combo.Update( 1.0 );
        float current = combo.Current();
        CHECK_CLOSE( 0.3, current, 0.000001 ); // 0.2*0.25 from Linear plus 0.25 from Box
        CHECK_EQUAL( false, combo.Expired() );

        combo.Update( 1.0 );
        current = combo.Current();
        CHECK_CLOSE( 0.35, current, 0.000001 ); // 0.4*0.25 from Linear plus 0.25 from Box
        CHECK_EQUAL( false, combo.Expired() );

        combo.Update( 1.0 );
        current = combo.Current();
        CHECK_CLOSE( 0.4, current, 0.000001 ); // 0.6*0.25 from Linear plus 0.25 from Box
        CHECK_EQUAL( true, combo.Expired() );
    }

    TEST( TestSetCurrentTime )
    {
        unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/WaningEffectComboTest/TestSetCurrentTime.json" ) );

        WaningEffectCombo combo;

        try
        {
            combo.Configure( p_config.get() );
        }
        catch( DetailedException& re )
        {
            PrintDebug( re.GetMsg() );
            CHECK( false );
        }

        unique_ptr<IndividualHumanContextFake> p_human( new IndividualHumanContextFake( nullptr, nullptr, nullptr, nullptr ) );
        p_human->SetAge( 12.0*DAYSPERYEAR );

        combo.SetContextTo( p_human.get() );

        combo.SetCurrentTime( 8.0 );

        combo.Update( 1.0 );
        float current = combo.Current();
        CHECK_CLOSE( 0.21, current, 0.000001 ); // 0.6 from Linear1, 0.7 from Linear 2, and 0.5 from Box
        CHECK_EQUAL( false, combo.Expired() );

        combo.Update( 1.0 );
        current = combo.Current();
        CHECK_CLOSE( 0.21, current, 0.000001 ); // 0.0 from Linear1, 1.0 from Linear 2, and 0.5 from Box
        CHECK_EQUAL( false, combo.Expired() );

        combo.Update( 1.0 );
        current = combo.Current();
        CHECK_CLOSE( 0.2, current, 0.000001 ); // 0.0 from Linear1, 1.0 from Linear 2, and 0.5 from Box
        CHECK_EQUAL( true, combo.Expired() );
    }

    TEST( TestUpdateCount )
    {
        unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/WaningEffectComboTest/TestUpdateCount.json" ) );

        WaningEffectCombo combo;

        try
        {
            combo.Configure( p_config.get() );
        }
        catch( DetailedException& re )
        {
            PrintDebug( re.GetMsg() );
            CHECK( false );
        }

        unique_ptr<IndividualHumanContextFake> p_human( new IndividualHumanContextFake( nullptr, nullptr, nullptr, nullptr ) );
        p_human->SetAge( 12.0*DAYSPERYEAR );

        combo.SetContextTo( p_human.get() );

        combo.Update( 1.0 );
        combo.SetCount( 2 );
        float current = combo.Current();
        CHECK_CLOSE( 0.032, current, 0.000001 ); // 0.2 from Count1, 0.8 from Count2, and 0.2 from Linear
        CHECK_EQUAL( false, combo.Expired() );

        combo.Update( 1.0 );
        combo.SetCount( 6 );
        current = combo.Current();
        CHECK_CLOSE( 0.048, current, 0.000001 ); // 0.6 from Count1, 0.4 from Count2, and 0.2 from Linear
        CHECK_EQUAL( false, combo.Expired() );

        combo.Update( 1.0 );
        combo.SetCount( 7 );
        current = combo.Current();
        CHECK_CLOSE( 0.042, current, 0.000001 ); // 0.7 from Count1, 0.3 from Count2, and 0.2 from Linear
        CHECK_EQUAL( false, combo.Expired() );

        combo.Update( 1.0 );
        combo.SetCount( 8 );
        current = combo.Current();
        CHECK_CLOSE( 0.064, current, 0.000001 ); // 0.8 from Count1, 0.2 from Count2, and 0.4 from Linear
        CHECK_EQUAL( false, combo.Expired() );

        combo.Update( 1.0 );
        combo.SetCount( 9 );
        current = combo.Current();
        CHECK_CLOSE( 0.036, current, 0.000001 ); // 0.9 from Count1, 0.1 from Count2, and 0.4 from Linear
        CHECK_EQUAL( true, combo.Expired() );
    }
}
