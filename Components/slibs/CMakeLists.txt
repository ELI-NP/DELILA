cmake_minimum_required(VERSION 3.0)

set(project_name libVME)
project(${project_name})

message("${project_name}")
set(CMAKE_CXX_FLAGS "-O2 -Wall")
set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR}) # Stupid!

include_directories(${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/../include)

file(GLOB headers ${PROJECT_SOURCE_DIR}/../include/VME*.hpp ${PROJECT_SOURCE_DIR}/../include/includesiseth/*.h)
#file(GLOB headers ${PROJECT_SOURCE_DIR}/../include/includesiseth/*.h)
file(GLOB sources ${PROJECT_SOURCE_DIR}/*.cpp)
file(GLOB vme_controller ${PROJECT_SOURCE_DIR}/../src/VMEController.cpp)
file(GLOB vme_device ${PROJECT_SOURCE_DIR}/../src/VMEDevice.cpp)
file(GLOB sis_vme_class ${PROJECT_SOURCE_DIR}/../src/srcsiseth/sis3153ETH_vme_class.cpp)

foreach(source IN LISTS sources)
    get_filename_component(name ${source} NAME_WE)
    set(lib_name "")
    string(APPEND lib_name "_" ${name})
    list(REMOVE_ITEM ${source} "sis3153ETH_vme_class.cpp")
    add_library(${lib_name} SHARED ${source} ${headers} ${vme_controller} ${vme_device} ${sis_vme_class})
    target_link_libraries(${lib_name} CAENComm)
    install(TARGETS ${lib_name} DESTINATION ${PROJECT_SOURCE_DIR}) # Not making lib directory

    # target_link_libraries(nameless ${CMAKE_DL_LIBS})
endforeach()