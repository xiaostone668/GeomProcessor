cmake_minimum_required(VERSION 3.16)
project(GeomProcessor VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt MOC/UIC/RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# -----------------------------------------------------------------------
# Qt 5.6.3
# -----------------------------------------------------------------------
set(Qt5_DIR "D:/Qt/Qt5.6.3/5.6.3/msvc2015_64/lib/cmake/Qt5" CACHE PATH "Qt5 cmake path")
set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};D:/Qt/Qt5.6.3/5.6.3/msvc2015_64/lib/cmake")

find_package(Qt5 5.6 REQUIRED COMPONENTS Core Widgets OpenGL)
set(QT_LIBS Qt5::Widgets Qt5::OpenGL)
message(STATUS "Qt5 found: ${Qt5_VERSION}")

# -----------------------------------------------------------------------
# OpenCASCADE 7.7.0
# -----------------------------------------------------------------------
set(OCC_ROOT "D:/OpenCASCADE-7.7.0" CACHE PATH "OCCT root")
# Auto-detect lib dir: try win64/vc14/lib then lib
set(_OCC_LIB_CANDIDATES "${OCC_ROOT}/win64/vc14/lib" "${OCC_ROOT}/lib")
set(OCC_LIB_DIR "${OCC_ROOT}/opencascade-7.7.0/win64/vc14/lib" CACHE PATH "OCCT library directory")
foreach(_dir ${_OCC_LIB_CANDIDATES})
  if(EXISTS "${_dir}/TKernel.lib")
    set(OCC_LIB_DIR "${_dir}" CACHE PATH "OCCT library directory" FORCE)
    message(STATUS "Found OCCT libs in: ${_dir}")
    break()
  endif()
endforeach()
# OCCT bin dir (DLLs)
set(OCC_BIN_DIR "${OCC_ROOT}/opencascade-7.7.0/win64/vc14/bin")

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${OCC_ROOT}/inc
)

# Core OCC libs
set(_OCC_LIB_NAMES
    TKernel TKMath TKBRep TKGeomBase TKGeomAlgo TKG3d TKG2d TKTopAlgo TKPrim
    TKXSBase TKV3d TKService TKOpenGl
    # STEP (OCCT 7.7.0 uses legacy STEP libs)
    TKSTEP TKSTEPAttr TKSTEPBase TKSTEP209
    # ShapeFix
    TKShHealing
    # Boolean / Defeaturing
    TKBO TKBool
    # Offset
    TKOffset TKFeat
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
    src/GeomChecker.cpp
)

set(HEADERS
    include/OccViewWidget.h
    include/GeomIPC.h
    include/GeomReceiver.h
    include/GeomProcessor.h
    include/GeomProcessorWindow.h
    include/GeomChecker.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${OCC_ROOT}/inc
)

# Set UTF-8 encoding for MSVC compiler to fix Chinese character display
if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)
endif()

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

    # Third-party DLLs (from OpenCASCADE installation)
    set(_OCC_BASE "D:/OpenCASCADE-7.7.0")
    set(_EXTRA_DLLS
        "${_OCC_BASE}/freeimage-3.17.0-vc14-64/bin/FreeImage.dll"
        "${_OCC_BASE}/ffmpeg-3.3.4-64/bin/avcodec-57.dll"
        "${_OCC_BASE}/ffmpeg-3.3.4-64/bin/avformat-57.dll"
        "${_OCC_BASE}/ffmpeg-3.3.4-64/bin/swscale-4.dll"
        "${_OCC_BASE}/ffmpeg-3.3.4-64/bin/avutil-55.dll"
        "${_OCC_BASE}/tbb_2021.5-vc14-64/bin/tbb12.dll"
        "${_OCC_BASE}/freetype-2.5.5-vc14-64/bin/freetype.dll"
    )
    foreach(_dll ${_EXTRA_DLLS})
        if(EXISTS "${_dll}")
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "${_dll}"
                        "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
                COMMENT "Copying: ${_dll}")
        endif()
    endforeach()

    # Qt5 DLLs
    set(_QT_BIN "D:/Qt/Qt5.6.3/5.6.3/msvc2015_64")
    foreach(_qtdll Qt5Core Qt5Gui Qt5Widgets Qt5OpenGL)
        if(EXISTS "${_QT_BIN}/bin/${_qtdll}.dll")
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${_QT_BIN}/bin/${_qtdll}.dll"
                    "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
                COMMENT "Copying Qt5 DLL: ${_qtdll}.dll")
        endif()
    endforeach()
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/platforms"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${_QT_BIN}/plugins/platforms/qwindows.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/platforms/qwindows.dll"
        COMMENT "Copying Qt5 platform plugin")
endif()