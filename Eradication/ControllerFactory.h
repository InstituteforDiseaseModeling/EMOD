
#pragma once

#include "Controller.h"

class ControllerFactory
{
public:
    static IController *CreateController(const Configuration *config);
};
