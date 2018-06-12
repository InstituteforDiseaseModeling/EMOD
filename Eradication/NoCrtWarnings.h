/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#ifdef WIN32
#else
#define sscanf_s    sscanf
#define fscanf_s    fscanf
#define strcpy_s(_dst, _count, _src)   strcpy((_dst), (_src))
#define sprintf_s(buff,buffsize,format,...)  sprintf((buff),(format),##__VA_ARGS__)
#define fopen_s(pfile, filename, mode)  ((*(pfile)) = fopen((filename), (mode)), errno)
#define gmtime_s(ptm, ptimer)  ((*(ptm)) = *gmtime(ptimer))
#endif
