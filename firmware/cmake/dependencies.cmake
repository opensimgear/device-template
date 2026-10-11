include(FetchContent)
FetchContent_Declare(firmware_common GIT_REPOSITORY https://github.com/opensimgear/firmware-common.git
  GIT_TAG 399e2f6762fcc8d3c6614dabdb8630af01c46277 GIT_SUBMODULES "")
set(OSG_EMBEDDED ON CACHE BOOL "" FORCE)
set(OSG_USB_CONFIG_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
FetchContent_MakeAvailable(firmware_common)
