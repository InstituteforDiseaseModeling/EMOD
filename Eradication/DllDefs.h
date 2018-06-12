/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

// Common Dll definitions

#define DISEASE_EMODULES         "disease_plugins"
#define REPORTER_EMODULES        "reporter_plugins"
#define INTERVENTION_EMODULES    "interventions"

// for implementation
#if defined(WIN32)
#define DISEASE_PLUGINS         L"disease_plugins"
#define REPORTER_PLUGINS        L"reporter_plugins"
#define INTERVENTION_PLUGINS    L"interventions"
#else
#define DISEASE_PLUGINS         "disease_plugins"
#define REPORTER_PLUGINS        "reporter_plugins"
#define INTERVENTION_PLUGINS    "interventions"
#endif

// Maximum number of supported simulation types in DTK
#define SIMTYPES_MAXNUM         25
