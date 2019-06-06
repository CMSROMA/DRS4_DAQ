add_library (drs4analysis SHARED src/Event.cxx )
add_executable (drs4convert src/drs4convert.cxx )

target_include_directories( drs4analysis PUBLIC include ${ROOT_INCLUDE_DIRS} )
target_link_libraries ( drs4analysis PRIVATE ${ROOT_LIBRARIES})
target_include_directories ( drs4convert PUBLIC include ${ROOT_INCLUDE_DIRS} )
target_link_libraries ( drs4convert PRIVATE drs4analysis ${ROOT_LIBRARIES} ${ROOT_COMPONENT_LIBRARIES})

install( TARGETS drs4convert  RUNTIME DESTINATION bin )
install( TARGETS drs4analysis LIBRARY DESTINATION lib )
