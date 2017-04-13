########################################################
# cmake file for building drs4daq
# @author Strahinja Lukic
CMAKE_MINIMUM_REQUIRED( VERSION 2.8 FATAL_ERROR )
########################################################

# project name
PROJECT( DRS4_DAQ )

# project version
SET( ${PROJECT_NAME}_VERSION_MAJOR 0 )
SET( ${PROJECT_NAME}_VERSION_MINOR 1 )
SET( ${PROJECT_NAME}_VERSION_PATCH 0 )

IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  SET (DOS "OS_LINUX")
ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  SET (DOS "OS_DARWIN")
ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  SET (DOS "OS_WINDOWS")
ELSE()
  SET (DOS "OS_UNKNOWN")
ENDIF()

MESSAGE( STATUS "OS: " ${DOS} )

### DEPENDENCIES ############################################################

SET( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmakemodules )

FIND_PACKAGE( DRS4 REQUIRED )
FIND_PACKAGE( LIBUSB10 REQUIRED )
FIND_PACKAGE( WXWidgets REQUIRED )
FIND_PACKAGE( ROOT REQUIRED )

SET( CMAKE_C_FLAGS  "-D${DOS} -DHAVE_USB -DHAVE_LIBUSB10 -DUSE_DRS_MUTEX ${WXFLAGS} ${CMAKE_C_FLAGS}" )
SET( CMAKE_CXX_FLAGS  "-w -ansi -pedantic -pthread -std=c++11 -D${DOS} -DHAVE_USB -DHAVE_LIBUSB10 -DUSE_DRS_MUTEX ${WXFLAGS} ${CMAKE_CXX_FLAGS}" )
####################################################################################################

### SUBDIRECTORIES #####################################

add_subdirectory (drs4data)
SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
add_subdirectory (drs4)
add_subdirectory (drs4daq)

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/drs4data/include ${DRS4_INCLUDE_DIRS} )

