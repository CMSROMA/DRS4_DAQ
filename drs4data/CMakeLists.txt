add_library (drs4data SHARED src/DRS4_fifo.cxx src/DRS4_writer.cxx src/DRS4_reader.cxx)

target_include_directories(drs4data PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include )

install( TARGETS drs4data LIBRARY DESTINATION lib )

