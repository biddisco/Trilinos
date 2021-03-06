#include <Kokkos_Core.hpp>

#include <Kokkos_Qthread.hpp>
#include <Qthread/Kokkos_Qthread_TaskPolicy.hpp>

#include "Teuchos_CommandLineProcessor.hpp"

using namespace std;

typedef double value_type;
typedef int    ordinal_type;
typedef int    size_type;

typedef Kokkos::Qthread exec_space;

#define __USE_FIXED_TEAM_SIZE__ 1
#include "example_chol_performance.hpp"

using namespace Tacho;

int main (int argc, char *argv[]) {

  Teuchos::CommandLineProcessor clp;
  clp.setDocString("This example program measure the performance of Chol algorithms on Kokkos::Threads execution space.\n");

  int nthreads = 1;
  clp.setOption("nthreads", &nthreads, "Number of threads");

  int max_task_dependence = 10;
  clp.setOption("max-task-dependence", &max_task_dependence, "Max number of task dependence");

  int team_size = 1;
  clp.setOption("team-size", &team_size, "Team size");

  bool team_interface = false;
  clp.setOption("enable-team-interface", "disable-team-interface",
                &team_interface, "Flag for team interface");

  int stack_size = 8192;
  clp.setOption("stack-size", &stack_size, "Stack size");

  bool verbose = false;
  clp.setOption("enable-verbose", "disable-verbose", &verbose, "Flag for verbose printing");

  string file_input = "test.mtx";
  clp.setOption("file-input", &file_input, "Input file (MatrixMarket SPD matrix)");

  int niter = 10;
  clp.setOption("niter", &niter, "Number of iterations for testing");

  clp.recogniseAllOptions(true);
  clp.throwExceptions(false);

  Teuchos::CommandLineProcessor::EParseCommandLineReturn r_parse= clp.parse( argc, argv );

  if (r_parse == Teuchos::CommandLineProcessor::PARSE_HELP_PRINTED) return 0;
  if (r_parse != Teuchos::CommandLineProcessor::PARSE_SUCCESSFUL  ) return -1;
  
  int r_val = 0;
  {
    const bool overwrite = true;
    const int nshepherds = (team_interface ? nthreads/team_size : nthreads);
    const int nworker_per_shepherd = nthreads/nshepherds;

    setenv("QT_HWPAR",                    to_string(nthreads).c_str(),             overwrite);
    setenv("QT_NUM_SHEPHERDS",            to_string(nshepherds).c_str(),           overwrite);
    setenv("QT_NUM_WORKERS_PER_SHEPHERD", to_string(nworker_per_shepherd).c_str(), overwrite);
    setenv("QT_STACK_SIZE",               to_string(stack_size).c_str(),           overwrite);

    exec_space::initialize(nthreads);
    exec_space::print_configuration(cout, true);
    
    // r_val = exampleCholPerformance
    //   <value_type,ordinal_type,size_type,exec_space,void>
    //   (file_input, niter, nthreads, max_task_dependence, team_size, team_interface, (nthreads != 1), verbose);

    exec_space::finalize();

    unsetenv("QT_HWPAR");
    unsetenv("QT_NUM_SHEPHERDS");
    unsetenv("QT_NUM_WORKERS_PER_SHEPHERD");
    unsetenv("QT_STACK_SIZE");
  }

  return r_val;
}
