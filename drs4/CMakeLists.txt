# Build shared library from source provided by PSI

add_library (drs4 SHARED ${DRS4_ROOT}/src/DRS.cpp
                         ${DRS4_ROOT}/src/averager.cpp
                         ${DRS4_ROOT}/src/mxml.c
                         ${DRS4_ROOT}/src/strlcpy.c
                         ${DRS4_ROOT}/src/musbstd.c
                         )

target_include_directories(drs4 PUBLIC ${DRS4_INCLUDE_DIRS} )
target_link_libraries ( drs4 PUBLIC ${WXLIBS} ${LIBUSB10_LIB} )

install( TARGETS drs4 LIBRARY DESTINATION lib )

