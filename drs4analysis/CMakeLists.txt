add_executable (drs4analysis src/drs4analysis.cxx )

target_include_directories ( drs4analysis PUBLIC include ../drs4data/include ../drs4monitor/include ${DRS4_INCLUDE_DIRS} ${ROOT_INCLUDE_DIRS} )
target_link_libraries ( drs4analysis PRIVATE drs4 drs4data drs4monitor ${ROOT_LIBRARIES} ${ROOT_COMPONENT_LIBRARIES})

install( TARGETS drs4analysis 
         RUNTIME DESTINATION bin )

