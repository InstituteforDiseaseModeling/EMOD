
#pragma once

// If buf is given, its size should be big enough to hold multiple function names with scopes
// suggestion of value, 1024 per call, for to dump callstacks with 10 calls, 10K bytes is needed
void CaptureCallstack(char* buf = nullptr);
void WalkCallstack(char* buf = nullptr);
