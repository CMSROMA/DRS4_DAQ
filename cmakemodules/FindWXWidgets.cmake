#########################################################
# cmake module for finding the LIBUSB10 code
#
#########################################################

FIND_PATH( WXCONFIG_DIR
    NAMES wx-config
    PATHS ${WXWidgets_DIR}/bin
    NO_DEFAULT_PATH
)
IF( NOT WXWidgets_DIR )
    FIND_PATH( WXCONFIG_DIR NAMES wx-config )
ENDIF()

IF( WXCONFIG_DIR )
    GET_FILENAME_COMPONENT( WXWidgets_ROOT ${WXCONFIG_DIR} PATH )
    SET( WXCONFIG ${WXCONFIG_DIR}/wx-config )
ENDIF()

find_library( WXCORE_LIB wx_gtk2u_core-3.0 ${WXWidgets_ROOT}/lib )

if( (NOT WXCONFIG) OR (NOT WXCORE_LIB) )
  message( FATAL_ERROR "Cannot find WXWidgets. If WXWidgets is present on the system,\
  specify the location where it is installed using variable WXWidgets_DIR.")
endif()

execute_process(COMMAND ${WXCONFIG} --cxxflags OUTPUT_VARIABLE WXFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)

# ---------- final checking ---------------------------------------------------
INCLUDE( FindPackageHandleStandardArgs )
# set WXWidgets_FOUND to TRUE if all listed variables are TRUE and not empty
FIND_PACKAGE_HANDLE_STANDARD_ARGS( WXWidgets DEFAULT_MSG WXWidgets_ROOT WXCORE_LIB )

