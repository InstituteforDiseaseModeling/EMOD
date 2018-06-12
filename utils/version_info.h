/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#define STRINGIFY(ver)       #ver
#define XSTRINGIFY(ver)       STRINGIFY(ver)

#ifdef WIN32
static const char* BUILD_DATE = __DATE__ " " __TIME__;
#else
#define BUILD_DATE __DATE__
#endif

#ifndef SCCS_BRANCH
#define    SCCS_BRANCH      "2.18-milestone (920f85b)"
#endif
#ifndef SCCS_DATE
#define    SCCS_DATE        "2018-06-04 13:55:11 -0700"
#endif

#define    MAJOR_VERSION    2
#define    MINOR_VERSION    18
#ifndef REVISION_NUMBER
#define    REVISION_NUMBER  4269
#endif
#define    BUILD_NUMBER     0

#ifndef BUILDER_NAME
#define    BUILDER_NAME     "dlukacevic"
#endif

#define FULL_VERSION_WITH_SVN       XSTRINGIFY(MAJOR_VERSION) "." XSTRINGIFY(MINOR_VERSION) "." XSTRINGIFY(REVISION_NUMBER)
#define FULL_VERSION_WITH_SVN_NQ    MAJOR_VERSION,MINOR_VERSION,REVISION_NUMBER,BUILD_NUMBER
