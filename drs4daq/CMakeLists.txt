add_executable (drs4daq src/drs4daq.cxx)

target_link_libraries ( drs4daq PRIVATE drs4data)

install( TARGETS drs4daq 
         RUNTIME DESTINATION bin )

