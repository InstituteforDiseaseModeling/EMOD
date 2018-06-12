/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#ifdef WIN32
#if defined(IDM_EXPORT)
//#pragma message( "Exporting IDM API." )
//#define IDMAPI  __declspec(dllexport)
#define IDMAPI
#define IDM_EXTERN
#else
//#pragma message( "Importing IDM API." )
//#define IDMAPI  __declspec(dllimport)
//#define IDM_EXTERN extern
#define IDMAPI
#define IDM_EXTERN
#endif
#else
#define IDMAPI 
#endif

// ---------------------------------------------------------------------------------------
// --- DMB 8/13/2014
// --- When I tried exporting std::string (like below), I ended up with multiply defined
// --- occurrences of basic_string when I linked the unit test exe.
// ---
// --- #pragma warning( disable: 4251 )
// --- This turns off the warning that the string will not be exported and will not be accessible
// --- by DLL users.   If the data is private, it is easy to ignore.  If it is public, 
// --- create getters/setters.
// ---------------------------------------------------------------------------------------
//#include <string>
//#include <vector>

//IDM_EXTERN template class IDMAPI std::allocator<char>;
//IDM_EXTERN template class IDMAPI std::basic_string<char>;
//IDM_EXTERN template class IDMAPI std::vector<std::string> ;
//IDM_EXTERN template class IDMAPI std::vector<int> ;
//IDM_EXTERN template class IDMAPI std::vector<float> ;
