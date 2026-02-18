cmake_minimum_required(VERSION 3.16)
project(GeomProcessor VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt MOC/UIC/RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# -----------------------------------------------------------------------
# Qt 6
# -----------------------------------------------------------------------
set(Qt6_DIR "D:/ProgramFiles/Qt/6.7.3/msvc2022_64/lib/cmake/Qt6" CACHE PATH "Qt6 cmake path")
set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};D:/ProgramFiles/Qt/6.7.3/msvc2022_64/lib/cmake")

find_package(Qt6 6.7 REQUIRED COMPONENTS Core Widgets OpenGL)
set(QT_LIBS Qt6::Widgets Qt6::OpenGL)
message(STATUS "Qt6 found: ${Qt6_VERSION}")

# -----------------------------------------------------------------------
# OpenCASCADE 7.9.3
# -----------------------------------------------------------------------
set(OCC_ROOT "D:/SDK/opencascade-7.9.3-vc14-64" CACHE PATH "OCCT root")
set(OCC_LIB_DIR "${OCC_ROOT}/win64/vc14/lib")
set(OCC_BIN_DIR "${OCC_ROOT}/win64/vc14/bin")

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${OCC_ROOT}/inc
)

# Core OCC libs
set(_OCC_LIB_NAMES
    TKernel TKMath TKBRep TKGeomBase TKGeomAlgo TKG3d TKG2d TKTopAlgo TKPrim
    TKXSBase TKV3d TKService TKOpenGl
    TKDESTEP TKDE
    # ShapeFix
    TKShHealing
    # Boolean / Defeaturing
    TKBO TKBool
    # Offset
    TKOffset TKFeat
    # STEP write
    TKXSBase
)
list(REMOVE_DUPLICATES _OCC_LIB_NAMES)

set(_OCC_LIBS_FOUND)
foreach(_lib ${_OCC_LIB_NAMES})
    find_library(_OCC_${_lib} NAMES ${_lib} PATHS ${OCC_LIB_DIR} NO_DEFAULT_PATH)
    if(_OCC_${_lib})
        list(APPEND _OCC_LIBS_FOUND ${_OCC_${_lib}})
    else()
        message(WARNING "OCCT lib not found: ${_lib} in ${OCC_LIB_DIR}")
    endif()
endforeach()
message(STATUS "OCCT libs: ${_OCC_LIBS_FOUND}")

# -----------------------------------------------------------------------
# Sources
# -----------------------------------------------------------------------
set(SOURCES
    src/main.cpp
    src/OccViewWidget.cpp
    src/GeomReceiver.cpp
    src/GeomProcessor.cpp
    src/GeomProcessorWindow.cpp
)

set(HEADERS
    include/OccViewWidget.h
    include/GeomIPC.h
    include/GeomReceiver.h
    include/GeomProcessor.h
    include/GeomProcessorWindow.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${OCC_ROOT}/inc
)

target_link_libraries(${PROJECT_NAME}
    ${QT_LIBS}
    ${_OCC_LIBS_FOUND}
)

# -----------------------------------------------------------------------
# Post-build: copy DLLs
# -----------------------------------------------------------------------
if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE ON)

    # OCCT DLLs
    if(EXISTS "${OCC_BIN_DIR}")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${OCC_BIN_DIR}"
                    "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
            COMMENT "Copying OCCT DLLs")
    endif()
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${OCC_LIB_DIR}"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
        COMMENT "Copying OCCT .lib-side DLLs")

    # Third-party DLLs (same as SimulationTool)
    set(_SDK_BASE "D:/SDK/3rdparty-vc14-64")
    set(_EXTRA_DLLS
        "${_SDK_BASE}/openvr-1.14.15-64/bin/win64/openvr_api.dll"
        "${_SDK_BASE}/freeimage-3.18.0-x64/bin/FreeImage.dll"
        "${_SDK_BASE}/ffmpeg-3.3.4-64/bin/avcodec-57.dll"
        "${_SDK_BASE}/ffmpeg-3.3.4-64/bin/avformat-57.dll"
        "${_SDK_BASE}/ffmpeg-3.3.4-64/bin/swscale-4.dll"
        "${_SDK_BASE}/ffmpeg-3.3.4-64/bin/avutil-55.dll"
        "${_SDK_BASE}/tbb-2021.13.0-x64/bin/tbb12.dll"
        "${_SDK_BASE}/tbb-2021.13.0-x64/bin/tbbmalloc.dll"
        "${_SDK_BASE}/jemalloc-vc14-64/bin/jemalloc.dll"
        "${_SDK_BASE}/freetype-2.13.3-x64/bin/freetype.dll"
    )
    foreach(_dll ${_EXTRA_DLLS})
        if(EXISTS "${_dll}")
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "${_dll}"
                        "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
                COMMENT "Copying: ${_dll}")
        endif()
    endforeach()

    # Qt DLLs
    set(_QT_BIN "D:/ProgramFiles/Qt/6.7.3/msvc2022_64")
    foreach(_qtdll Qt6Core Qt6Gui Qt6Widgets Qt6OpenGL Qt6Network Qt6Svg)
        if(EXISTS "${_QT_BIN}/bin/${_qtdll}.dll")
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${_QT_BIN}/bin/${_qtdll}.dll"
                    "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
                COMMENT "Copying Qt DLL: ${_qtdll}.dll")
        endif()
    endforeach()
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/platforms"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${_QT_BIN}/plugins/platforms/qwindows.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/platforms/qwindows.dll"
        COMMENT "Copying Qt platform plugin")
endif()
