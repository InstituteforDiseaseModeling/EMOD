/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "ControllerFactory.h"
#include "Exceptions.h"
#include "Sugar.h"
#include "Configuration.h"

using namespace std;

//SETUP_LOGGING( "ControllerFactory" )

IController* ControllerFactory::CreateController( const Configuration *config )
{
    string ControllerName;
    if (!CONFIG_PARAMETER_EXISTS(config, "ControllerName"))
        ControllerName = "Default";
    else 
        ControllerName = GET_CONFIG_STRING(EnvPtr->Config, "ControllerName");

    if ("Default"== ControllerName)
        return _new_ DefaultController();
#ifdef _UNIT_TESTS
    else if ("UnitTests" == ControllerName)
        return _new_ UnitTestController();
#endif
    else
    {
        std::stringstream s ;
        s << "Controller with name '" << ControllerName << "' is not implemented.  Check ControllerName parameter in the config file." << std::endl ;
        throw Kernel::GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, s.str().c_str() );
    }
}
