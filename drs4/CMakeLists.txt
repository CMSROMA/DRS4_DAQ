# Build shared library from source provided by PSI

add_library (drs4 SHARED ${DRS4_ROOT}/src/DRS.cpp
                         ${DRS4_ROOT}/src/averager.cpp
                         ${DRS4_ROOT}/src/mxml.c
                         ${DRS4_ROOT}/src/strlcpy.c
                         )

target_include_directories(drs4 PUBLIC ${DRS4_INCLUDE_DIRS} )

install( TARGETS drs4 LIBRARY DESTINATION lib )

