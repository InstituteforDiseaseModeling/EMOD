/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once
#include <string>
#include <fstream>
#include <functional>

#include "IController.h"
#include "ISimulation.h"
#include "Environment.h"

/*
** The Controller class serves as the bridge between the Simulation and the outside environment
** It handles reading configuration files and creating the simulation object. It is also the locus
** of implementation of high level simulation plans involving averaging, serialization, etc
*/

template<class ControllerT>
class ControllerExecuteFunctor
{
public:
    ControllerT* controller;

    ControllerExecuteFunctor(ControllerT *c) : controller(c) { }

    template<class SimulationT>
    bool call()
    {
        return controller->template execute_internal<SimulationT>();
    }
};

class DefaultController : public IController
{
public:
    virtual bool Execute();
    virtual ~DefaultController() {}

protected:

    template <class SimulationT> 
    bool execute_internal();

    // Non-template version
    bool execute_internal();

    friend class ControllerExecuteFunctor<DefaultController>;
};

