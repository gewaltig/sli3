#ifndef SLICONFIG_H
#define SLICONFIG_H
/* 
    Configuration header.
    sliconfig.h is automatically generated from sliconfig.h.in
    during the configuration process. 

    Please make all changes in sliconfig.h.in.
*/

#include "config.h"

#define SLI_PRGNAME "@SLI_PRGNAME@"

#define SLI_MAJOR_REVISION @SLI_MAJOR@
#define SLI_MINOR_REVISION @SLI_MINOR@
#define SLI_PATCHLEVEL "@SLI_PATCHLEVEL@"

#define SLI_HOST "@host@"
#define SLI_HOSTOS "@host_os@"
#define SLI_HOSTCPU "@host_cpu@"
#define SLI_HOSTVENDOR "@host_vendor@"

#define SLI_SOURCEDIR "@PKGSRCDIR@"
#define SLI_BUILDDIR "@PKGBUILDDIR@"
#define SLI_PREFIX "@SLI_PREFIX@"

#define SLI_EXITCODE_ABORT @SLI_EXITCODE_ABORT@
#define SLI_EXITCODE_SEGFAULT @SLI_EXITCODE_SEGFAULT@

/* Do not delete the empty line above ! Configure will not work correctly
   on some systems without it!
*/
#endif
/** end of file **/

