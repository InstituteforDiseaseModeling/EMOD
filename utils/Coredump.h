/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

// If buf is given, its size should be big enough to hold multiple function names with scopes
// suggestion of value, 1024 per call, for to dump callstacks with 10 calls, 10K bytes is needed
void CaptureCallstack(char* buf = nullptr);
void WalkCallstack(char* buf = nullptr);
