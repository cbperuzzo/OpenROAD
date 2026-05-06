include("openroad")

set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Eigen3 REQUIRED)
find_package(OpenMP REQUIRED)

project(sap)

swig_lib(NAME      sap
         NAMESPACE sap
         I_FILE    src/SAplace.i
         SWIG_INCLUDES
             ${ODB_HOME}/src/swig/common
             ${ODB_HOME}/src/swig/tcl
             ${ODB_HOME}/include
         SCRIPTS   src/SAplace.tcl
)

target_sources(sap
  PRIVATE
    src/MakeSAplace.cpp
    src/SAplace.cpp
    src/Net.cpp
    src/Macro.cpp
    src/Pin.cpp
)

messages(TARGET sap)

target_include_directories(sap
  PUBLIC
    include
  PRIVATE
    src
      
)

target_link_libraries(sap
    OpenSTA            
    odb                                
    Eigen3::Eigen
    Threads::Threads
    OpenMP::OpenMP_CXX
)

if(ENABLE_TESTS)
  add_subdirectory(test)
endif()
