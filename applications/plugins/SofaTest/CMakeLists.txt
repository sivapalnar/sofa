cmake_minimum_required(VERSION 3.1)
project(SofaTest)

set(SOFATEST_VERSION 0.1)

set(HEADER_FILES
    BroadPhase_test.h
    DataEngine_test.h
    Elasticity_test.h
    Elasticity_test.inl
    ForceField_test.h
    InitPlugin_test.h
    Mapping_test.h
    MultiMapping_test.h
    Multi2Mapping_test.h
    PrimitiveCreation.h
    Sofa_test.h
    TestMessageHandler.h
    #LogMessage.h
)

set(SOURCE_FILES
    Elasticity_test.cpp
    InitPlugin_test.cpp
    PrimitiveCreation.cpp
    TestMessageHandler.cpp
    Sofa_test.cpp
)

set(PYTHON_FILES "")

find_package(SofaFramework REQUIRED)
find_package(SofaMisc REQUIRED)
find_package(SceneCreator REQUIRED)

find_package(SofaPython QUIET)
if(SofaPython_FOUND)
    list(APPEND HEADER_FILES "Python_test.h")
    list(APPEND SOURCE_FILES "Python_test.cpp")
    set(PYTHON_FILES
        python/SofaTest/Macro.py
        python/SofaTest/__init__.py
    )
    message(STATUS "SofaTest: optional dependency SofaPython found, Python_test will be built")
else()
    message(STATUS "SofaTest: optional dependency SofaPython NOT found, Python_test won't be built")
endif()

# Config files and install rules for pythons scripts
sofa_set_python_directory(${PROJECT_NAME} "python")

add_library(${PROJECT_NAME} SHARED ${HEADER_FILES} ${SOURCE_FILES} ${PYTHON_FILES})
if(SofaPython_FOUND)
    target_link_libraries(${PROJECT_NAME} SofaPython)
endif()
target_link_libraries(${PROJECT_NAME} SceneCreator SofaComponentBase SofaComponentCommon SofaComponentGeneral SofaComponentAdvanced SofaComponentMisc)
target_link_libraries(${PROJECT_NAME} gtest)
target_include_directories(${PROJECT_NAME} PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>")
target_include_directories(${PROJECT_NAME} PUBLIC "$<INSTALL_INTERFACE:include>")
set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "-DSOFA_BUILD_TEST -DSOFA_SRC_DIR=\"${CMAKE_SOURCE_DIR}\"")
set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER "${HEADER_FILES}")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/..")

## Install rules for the library and headers; CMake package configurations files
sofa_create_package(SofaTest ${SOFATEST_VERSION} ${PROJECT_NAME} SofaTest)
