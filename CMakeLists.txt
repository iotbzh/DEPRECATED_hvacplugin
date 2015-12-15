PROJECT(hvacplugin CXX)
SET(CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS}")

include(FindPkgConfig)
pkg_check_modules(gio gio-2.0)
pkg_check_modules(gio-unix gio-unix-2.0)
pkg_check_modules(json-c json-c)
pkg_check_modules(amb automotive-message-broker)
set(Boost_INCLUDE_DIRS /usr/include/boost)

include_directories(${CMAKE_SOURCE_DIR}/lib ${include_dirs} ${gio_INCLUDE_DIRS} ${gio-unix_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/plugins/common ${Boost_INCLUDE_DIRS} ${json-c_INCLUDE_DIRS} ${amb_INCLUDE_DIRS}) 

set(hvacplugin_headers hvacplugin.h)
set(hvacplugin_sources hvacplugin.cpp)

add_library(hvacplugin MODULE ${hvacplugin_sources} ${hvacplugin_headers})
set_target_properties(hvacplugin PROPERTIES PREFIX "")
target_link_libraries(hvacplugin amb amb-plugins-common -L${CMAKE_CURRENT_BINARY_DIR}/plugins/common ${link_libraries} -lrt ${json-c_LIBRARIES})

install(TARGETS hvacplugin LIBRARY DESTINATION ${PLUGIN_INSTALL_PATH})


