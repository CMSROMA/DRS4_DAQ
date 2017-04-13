add_executable (drs4daq src/drs4daq.cxx)

target_include_directories ( drs4daq PUBLIC ../drs4data/include ${DRS4_INCLUDE_DIRS} ${ROOT_INCLUDE_DIRS} )
target_link_libraries ( drs4daq PRIVATE drs4 drs4data ${ROOT_LIBRARIES})

install( TARGETS drs4daq 
         RUNTIME DESTINATION bin )

