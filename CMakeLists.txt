########################################################
# cmake file for building drs4daq
# @author Strahinja Lukic
CMAKE_MINIMUM_REQUIRED( VERSION 2.8 FATAL_ERROR )
########################################################

SET( CMAKE_CXX_FLAGS  "-w -ansi -pedantic -pthread -std=c++11 ${CMAKE_CXX_FLAGS}" )
####################################################################################################

# project name
PROJECT( DRS4_DAQ )

# project version
SET( ${PROJECT_NAME}_VERSION_MAJOR 0 )
SET( ${PROJECT_NAME}_VERSION_MINOR 1 )
SET( ${PROJECT_NAME}_VERSION_PATCH 0 )

SET( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmakemodules )

### DEPENDENCIES ############################################################

FIND_PACKAGE( DRS4 REQUIRED )
MESSAGE( STATUS "DRS4_ROOT = " ${DRS4_ROOT} )

### SUBDIRECTORIES #####################################

add_subdirectory (drs4)
add_subdirectory (drs4data)
SET(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/lib)
add_subdirectory (drs4daq)

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/drs4data/include ${DRS4_INCLUDE_DIRS} )

