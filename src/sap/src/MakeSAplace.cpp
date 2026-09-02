#include "sap/MakeSAplace.h"

#include "tcl.h"
#include "utl/decode.h"

extern "C" {
extern int Sap_Init(Tcl_Interp* interp);
}

namespace sap {

// Tcl files encoded into strings.
extern const char* sap_tcl_inits[];

void initSAplace(Tcl_Interp* tcl_interp)
{
  Sap_Init(tcl_interp);
  utl::evalTclInit(tcl_interp, sap_tcl_inits);
}

}