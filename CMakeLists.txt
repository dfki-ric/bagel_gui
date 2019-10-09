project(bagel_gui)
set(PROJECT_VERSION 1.0)
set(PROJECT_DESCRIPTION "A")
cmake_minimum_required(VERSION 2.6)
include(FindPkgConfig)

find_package(lib_manager)
lib_defaults()
define_module_info()

include(CheckIncludeFileCXX)

CHECK_INCLUDE_FILE_CXX("tr1/functional" USE_TR1)
if(${USE_TR1})
    ADD_DEFINITIONS(-DUSE_TR1)
else(${USE_TR1})
    ADD_DEFINITIONS(-DNO_TR1)
endif()

pkg_check_modules(OSGQT openscenegraph-osgQt5)
if(OSGQT_FOUND)
find_package(OpenSceneGraph REQUIRED osgManipulator osgViewer osgFX osgShadow osgParticle osgTerrain osgDB osgGA osgWidget osgText osgUtil)

include_directories(${OSGQT_INCLUDE_DIRS})
link_directories(${OSGQT_LIBRARY_DIRS})
add_definitions(${OSGQT_CFLAGS_OTHER})  #cflags without -I

else()
find_package(OpenSceneGraph REQUIRED osgManipulator osgViewer osgFX osgShadow osgParticle osgTerrain osgDB osgGA osgWidget osgText osgUtil osgQt)
endif()

include_directories(${OPENSCENEGRAPH_INCLUDE_DIRS})
link_directories(${OPENSCENEGRAPH_LIBRARY_DIRS})

set (QT_USE_QTOPENGL TRUE)
setup_qt()

set (CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES} ${OPENSCENEGRAPH_INCLUDE_DIRS})
CHECK_INCLUDE_FILE_CXX("osg/Version" HAVE_OSG_VERSION_H)
if(${HAVE_OSG_VERSION_H})
    ADD_DEFINITIONS(-DHAVE_OSG_VERSION_H)
endif()

pkg_check_modules(BGM BehaviorGraphMARS)
if(BGM_FOUND)
  add_definitions("-DBGM")
  include_directories(${BGM_INCLUDE_DIRS})
  link_directories(${BGM_LIBRARY_DIRS})
endif(BGM_FOUND)

pkg_check_modules(PKGCONFIG REQUIRED
          lib_manager
          mars_utils
          osg_graph_viz
          osg_text
          config_map_gui
          main_gui
          configmaps
)

include_directories(${PKGCONFIG_INCLUDE_DIRS})
link_directories(${PKGCONFIG_LIBRARY_DIRS})
add_definitions(${PKGCONFIG_CFLAGS_OTHER})  #flags excluding the ones with -I

include_directories(
	src
)

set(SOURCES 
	src/BagelGui.cpp
	src/SlotWrapper.cpp
	src/NodeTypeWidget.cpp
	src/NodeInfoWidget.cpp
	src/HistoryWidget.cpp
  src/GraphicsTimer.cpp
  src/BagelLoader.cpp
  src/BagelModel.cpp
  src/View.cpp
)

set(HEADERS
	src/BagelGui.hpp
	src/SlotWrapper.hpp
	src/NodeTypeWidget.hpp
	src/NodeInfoWidget.hpp
	src/HistoryWidget.hpp
  src/GraphicsTimer.hpp
  src/NodeLoader.hpp
  src/BagelLoader.hpp
  src/ModelInterface.hpp
  src/PluginInterface.hpp
  src/BagelModel.hpp
  src/View.hpp
  src/ForceLayout.hpp
)

set (QT_MOC_HEADER
	src/SlotWrapper.hpp
  src/NodeTypeWidget.hpp
  src/NodeInfoWidget.hpp
  src/HistoryWidget.hpp
  src/GraphicsTimer.hpp
  src/View.hpp
)

if (${USE_QT5})
qt5_wrap_cpp ( QT_MOC_HEADER_SRC ${QT_MOC_HEADER} )
else (${USE_QT5})
qt4_wrap_cpp ( QT_MOC_HEADER_SRC ${QT_MOC_HEADER} )
endif (${USE_QT5})

add_library(${PROJECT_NAME} SHARED ${SOURCES} ${QT_MOC_HEADER_SRC})

if (${USE_QT5})
qt5_use_modules(${PROJECT_NAME} Widgets OpenGL)
endif (${USE_QT5})

if(OSGQT_FOUND)
  SET(EXTRA_LIBS ${OSGQT_LIBRARIES})
endif()

target_link_libraries(${PROJECT_NAME}
                      ${PKGCONFIG_LIBRARIES}
                      ${OPENSCENEGRAPH_LIBRARIES}
                      ${QT_LIBRARIES}
                      ${BGM_LIBRARIES}
                      ${EXTRA_LIBS}
)

if(WIN32)
  set(LIB_INSTALL_DIR bin) # .dll are in PATH, like executables
else(WIN32)
  set(LIB_INSTALL_DIR lib)
endif(WIN32)


set(_INSTALL_DESTINATIONS
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION ${LIB_INSTALL_DIR}
	ARCHIVE DESTINATION lib
)


# Install the library into the lib folder
install(TARGETS ${PROJECT_NAME} ${_INSTALL_DESTINATIONS})

# Install headers into mars include directory
install(FILES ${HEADERS} DESTINATION include/${PROJECT_NAME})

# Prepare and install necessary files to support finding of the library 
# using pkg-config
configure_file(${PROJECT_NAME}.pc.in ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pc @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pc DESTINATION lib/pkgconfig)
install(DIRECTORY configuration/ DESTINATION configuration/)
INSTALL(PROGRAMS bin/bagel_gui DESTINATION bin)
install(DIRECTORY resources/ DESTINATION share/bagel_gui/resources/)
