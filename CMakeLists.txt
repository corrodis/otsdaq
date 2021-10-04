# ======================================================================
#  otsdaq main build file
#
#  cd .../path/to/build/directory
#  source .../path/to/otsdaq/ups/setup_for_development
#  buildtool
# ======================================================================


# use cmake 3.17 or later
cmake_minimum_required (VERSION 3.17)

find_package(cetmodules)
set(CET_CETBUILDTOOLS_COMPAT TRUE)
include(Compatibility)
set(CET_WARN_DEPRECATED)

project(otsdaq VERSION 2.06.03)

set(CMAKE_MODULE_PATH
        $ENV{XDAQ_DIR}/Modules
        $ENV{ARTDAQ_UTILITIES_DIR}/Modules
        ${CMAKE_MODULE_PATH})
message(STATUS "CMAKE_MODULE_PATH is ${CMAKE_MODULE_PATH}")

include(CetCMakeEnv)
cet_cmake_env()
include(CetFindPkgConfigPackage)

cet_set_compiler_flags(
 DIAGS 
 VIGILANT
 WERROR
 NO_UNDEFINED
 EXTRA_FLAGS -pedantic -D_LIBCPP_ENABLE_CXX17_REMOVED_FEATURES=1
 )
 
cet_remove_compiler_flags(-std=c++98)
 
#string(TOUPPER ${CMAKE_BUILD_TYPE} BTYPE_UC )
#if( ${BTYPE_UC} MATCHES "DEBUG" )
#  cet_add_compiler_flags(-fsanitize=address)
#endif()

cet_report_compiler_flags()

# these are minimum required versions, not the actual product versions
find_ups_product( cetbuildtools v3_03_00 )
find_ups_product( artdaq v3_09_00 )
find_ups_product( artdaq_core v3_06_00 )
find_ups_product( artdaq_utilities v1_00_00 )
find_ups_product( art v1_06_00 )
find_ups_product( canvas v1_00_00 )
find_ups_product( messagefacility v1_10_00 )
find_ups_product( cetlib v1_03_00 )
find_ups_product( artdaq_database v1_03_07 )
find_ups_product( TRACE v3_03_00 )
find_ups_product( xdaq v16_7_0_1)
find_ups_root( v5_34_00 )
find_ups_boost( v1_50_0 filesystem
                        program_options
                        unit_test_framework
 )

 # XDAQ Extra setup
 include_directories($ENV{XDAQ_INC}/linux)
link_directories($ENV{XDAQ_LIB})
link_directories($ENV{ARTDAQ_DATABASE_LIB})
 set(XDAQ_BASIC_LIB_LIST 
   xcept
   xoap
   xdaq
   toolbox
   xerces-c
   cgicc
 )
 set(XDAQ_XERCES-C xerces-c)

string(REGEX MATCHALL "[0-9][0-9]?[0-9]?" ART_VERSION_LIST "$ENV{ART_VERSION}")
list(GET ART_VERSION_LIST 0 ART_MAJOR_VERSION)
list(GET ART_VERSION_LIST 1 ART_MINOR_VERSION)
list(GET ART_VERSION_LIST 2 ART_PATCH_VERSION)

message("-DART_HEX_VERSION=0x${ART_MAJOR_VERSION}${ART_MINOR_VERSION}${ART_PATCH_VERSION}")
set(ART_HEX_VERSION 0x${ART_MAJOR_VERSION}${ART_MINOR_VERSION}${ART_PATCH_VERSION})
add_definitions(-DART_HEX_VERSION=0x${ART_MAJOR_VERSION}${ART_MINOR_VERSION}${ART_PATCH_VERSION})

string(REGEX MATCHALL "[0-9][0-9]?[0-9]?" MESSAGEFACILITY_VERSION_LIST "$ENV{MESSAGEFACILITY_VERSION}")
list(GET MESSAGEFACILITY_VERSION_LIST 0 MESSAGEFACILITY_MAJOR_VERSION)
list(GET MESSAGEFACILITY_VERSION_LIST 1 MESSAGEFACILITY_MINOR_VERSION)
list(GET MESSAGEFACILITY_VERSION_LIST 2 MESSAGEFACILITY_PATCH_VERSION)

# as of fhiclcpp v4_15_00 (s108), ParameterSet::make is the preferred way to make a ParameterSet
check_ups_version(fhiclcpp $ENV{FHICLCPP_VERSION} v4_15_00 PRODUCT_MATCHES_VAR PSET_MAKE)
if(PSET_MAKE)
  add_definitions(-DSIMPLER_PSET_MAKE)
endif()

message("-DMESSAGEFACILITY_HEX_VERSION=0x${MESSAGEFACILITY_MAJOR_VERSION}${MESSAGEFACILITY_MINOR_VERSION}${MESSAGEFACILITY_PATCH_VERSION}")
add_definitions(-DMESSAGEFACILITY_HEX_VERSION=0x${MESSAGEFACILITY_MAJOR_VERSION}${MESSAGEFACILITY_MINOR_VERSION}${MESSAGEFACILITY_PATCH_VERSION})

# For version.h
message("PROJECT_VERSION ${PROJECT_VERSION}")
string(REGEX MATCHALL "[0-9][0-9]?" OTSDAQ_VERSION_LIST "${PROJECT_VERSION}")
list(GET OTSDAQ_VERSION_LIST 0 OTSDAQ_MAJOR_VERSION)
list(GET OTSDAQ_VERSION_LIST 1 OTSDAQ_MINOR_VERSION)
list(GET OTSDAQ_VERSION_LIST 2 OTSDAQ_PATCH_VERSION)
add_definitions(-DOTSDAQ_MAJOR_VERSION=${OTSDAQ_MAJOR_VERSION})
add_definitions(-DOTSDAQ_MINOR_VERSION=${OTSDAQ_MINOR_VERSION})
add_definitions(-DOTSDAQ_PATCH_VERSION=${OTSDAQ_PATCH_VERSION})

# macros for art_dictionary and simple_plugin

include(CetMake)
include(ArtMake)
include(BuildPlugins)
include(CetTest)

# source
add_subdirectory(artdaq-ots)
add_subdirectory(otsdaq)

# tools
add_subdirectory(tools)

# tools
FILE(COPY tools DESTINATION .)
INSTALL(DIRECTORY tools DESTINATION ${product}/${version})

# Documentation (to comment out if not needed DOXYGEN locally)
if( $ENV{OTS_DOXY} MATCHES "DOIT" )
	add_subdirectory(doc)
endif()

# test programs
add_subdirectory(test)

# tools
FILE(COPY test DESTINATION .)
INSTALL(DIRECTORY test DESTINATION ${product}/${version})

# data-core
FILE(COPY data-core DESTINATION .)
INSTALL(DIRECTORY data-core DESTINATION ${product}/${version})

cet_cmake_config()
