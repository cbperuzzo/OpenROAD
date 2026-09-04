%{
#include "ord/OpenRoad.hh"
#include "sap/SAplace.h"
#include "odb/db.h"

namespace ord {
OpenRoad*
getOpenRoad();

sap::SAplace*
getSAplace();
}

using ord::getOpenRoad;
using ord::getSAplace;
using sap::SAplace;

%}

%inline %{


void
saplace_simulated_annealing_simple_cmd(
  int iterations_per_T, double initial_T, double alpha, int halo_width,
  int halo_height, float boundry_const, float boundry_coef, float bool worst_hpwl
)
{
  SAplace* saplace = getSAplace();
  saplace->init(halo_width, halo_height, boundry_const, boundry_coef);
  saplace->run(iterations_per_T, initial_T, alpha, worst_hpwl);
}


%}