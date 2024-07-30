
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
