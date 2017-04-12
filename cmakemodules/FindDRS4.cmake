#########################################################
# cmake module for finding the DRS4 code
#
#########################################################

# ---------- includes ---------------------------------------------------------
SET( DRS4_INCLUDE_DIRS DRS4_INCLUDE_DIRS-NOTFOUND )
MARK_AS_ADVANCED( DRS4_INCLUDE_DIRS )

FIND_PATH( DRS4_INCLUDE_DIRS
    NAMES DRS.h
    PATHS ${DRS4_DIR}/include
    NO_DEFAULT_PATH
)
IF( NOT DRS4_DIR )
    FIND_PATH( DRS4_INCLUDE_DIRS NAMES DRS.h )
ENDIF()

IF( DRS4_INCLUDE_DIRS )
    GET_FILENAME_COMPONENT( DRS4_ROOT ${DRS4_INCLUDE_DIRS} PATH )
ENDIF()

# ---------- final checking ---------------------------------------------------
INCLUDE( FindPackageHandleStandardArgs )
# set DRS4_FOUND to TRUE if all listed variables are TRUE and not empty
FIND_PACKAGE_HANDLE_STANDARD_ARGS( DRS4 DEFAULT_MSG DRS4_ROOT )

