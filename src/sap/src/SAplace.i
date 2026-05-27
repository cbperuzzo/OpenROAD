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
saplace_simulated_annealing_simple_cmd(int iterations_per_T, double initial_T, double alpha, int halo_size)
{
  SAplace* saplace = getSAplace();
  int threads = ord::OpenRoad::openRoad()->getThreadCount();
  saplace->simulatedAnnealing(threads, iterations_per_T, initial_T, alpha, halo_size);
}


%}