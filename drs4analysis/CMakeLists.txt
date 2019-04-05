add_library (drs4analysis SHARED src/Event.cxx )
add_executable (drs4convert src/drs4convert.cxx )

#add_executable (drs4analysis src/drs4analysis.cxx )
#add_executable (drs4baseline src/drs4baseline.cxx )
#target_include_directories ( drs4analysis PUBLIC include ../drs4data/include ../drs4monitor/include ${DRS4_INCLUDE_DIRS} ${ROOT_INCLUDE_DIRS} )
#target_link_libraries ( drs4analysis PRIVATE drs4 drs4data drs4monitor ${ROOT_LIBRARIES} ${ROOT_COMPONENT_LIBRARIES})
#target_include_directories ( drs4baseline PUBLIC include ../drs4data/include ../drs4monitor/include ${DRS4_INCLUDE_DIRS} ${ROOT_INCLUDE_DIRS} )
#target_link_libraries ( drs4baseline PRIVATE drs4data drs4monitor ${ROOT_LIBRARIES} ${ROOT_COMPONENT_LIBRARIES})

target_include_directories( drs4analysis PUBLIC include ${DRS4_INCLUDE_DIRS} ${ROOT_INCLUDE_DIRS} )
target_link_libraries ( drs4analysis PRIVATE ${ROOT_LIBRARIES})
target_include_directories ( drs4convert PUBLIC include ../drs4data/include ../drs4monitor/include ${DRS4_INCLUDE_DIRS} ${ROOT_INCLUDE_DIRS} )
target_link_libraries ( drs4convert PRIVATE drs4 drs4data drs4monitor drs4analysis ${ROOT_LIBRARIES} ${ROOT_COMPONENT_LIBRARIES})

install( TARGETS drs4convert  RUNTIME DESTINATION bin )
install( TARGETS drs4analysis LIBRARY DESTINATION lib )


