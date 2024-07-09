
///////////////////////////////////////////////////////////////////////////////
// Sugar (Syntactic)

#pragma once

// Environment helpers
#include "Environment.h"
#define EnvPtr Environment::getInstance() // make code a bit more readable using Dll-boundary-crossable Env pointer

//////////////////////////////////////////////////////////
// helpers for config variables 

#define CONFIG_PARAMETER_EXISTS(config_ptr, name) \
 (config_ptr->As<json::Object>().Find(name) != config_ptr->As<json::Object>().End())
