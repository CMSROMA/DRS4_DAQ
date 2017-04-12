#########################################################
# cmake module for finding the LIBUSB10 code
#
#########################################################

# ---------- includes ---------------------------------------------------------
SET( LIBUSB10_INCLUDE_DIRS LIBUSB10_INCLUDE_DIRS-NOTFOUND )
MARK_AS_ADVANCED( LIBUSB10_INCLUDE_DIRS )

FIND_PATH( LIBUSB10_INCLUDE_DIRS
    NAMES libusb-1.0/libusb.h
    PATHS ${LIBUSB10_DIR}/include
    NO_DEFAULT_PATH
)
IF( NOT LIBUSB10_DIR )
    FIND_PATH( LIBUSB10_INCLUDE_DIRS NAMES libusb-1.0/libusb.h )
ENDIF()

IF( LIBUSB10_INCLUDE_DIRS )
    GET_FILENAME_COMPONENT( LIBUSB10_ROOT ${LIBUSB10_INCLUDE_DIRS} PATH )
ENDIF()

find_library( LIBUSB10_LIB usb-1.0 ${LIBUSB10_ROOT}/lib )

if( (NOT LIBUSB10_INCLUDE_DIRS) OR (NOT LIBUSB10_LIB) )
  message( FATAL_ERROR "Cannot find libusb-1.0. If libusb-1.0 is present on the system, \
  specify the location where it is installed using variable LIBUSB10_DIR." )
endif()

# ---------- final checking ---------------------------------------------------
INCLUDE( FindPackageHandleStandardArgs )
# set LIBUSB10_FOUND to TRUE if all listed variables are TRUE and not empty
FIND_PACKAGE_HANDLE_STANDARD_ARGS( LIBUSB10 DEFAULT_MSG LIBUSB10_ROOT LIBUSB10_LIB )

