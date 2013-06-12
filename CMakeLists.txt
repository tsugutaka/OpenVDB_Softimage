cmake_minimum_required (VERSION 2.6)

project (openvdb_softimage)

set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} c:/dev_local/CMake/Modules)

find_package (Softimage)
find_package (boost)

if (Boost_FOUND)
 set (BOOST_INCLUDE_PATH ${Boost_INCLUDE_DIRS})
 set (BOOST_LIBRARIES ${Boost_LIBRARIES})
endif()

find_package (zlib)
set (ILMBASE_ROOT "C:/Program Files/ILMBase")
find_package (ilmbase REQUIRED)
find_package (tbb)

set (SEEXPR_ROOT "C:/Program Files/SeExpr")
set (SEEXPR_INCLUDE_DIR "${SEEXPR_ROOT}/include")
set (SEEXPR_LIBRARY_DIR "${SEEXPR_ROOT}/lib")
set (SEEXPR_LIBRARY "SeExpr.lib")

add_definitions(-DOPENVDB_STATICLIB -DNOMINMAX /bigobj)

message (STATUS "---configuring openvdb_softimage plugin---")

set (INCLUDE_DIRS
 "c:/program files/openvdb/include"
 ${ILMBASE_INCLUDE_DIRECTORY}
 ${BOOST_INCLUDE_PATH}
 ${TBB_INCLUDE_DIR}
 ${ZLIB_INCLUDE_DIRS}
 ${SEEXPR_INCLUDE_DIR}
)

set (LINK_DIRS
 "c:/program files/openvdb/lib"
 ${SEEXPR_LIBRARY_DIR}
)

include_directories (${INCLUDE_DIRS})

link_directories (${LINK_DIRS})

set (SOURCES
 OpenVDB_Softimage.cpp
 VDB_Node_FBM.cpp
 VDB_Node_MeshToVolume.cpp
 VDB_Node_Noise.cpp
 VDB_Node_TestCustomData.cpp
 VDB_Node_Turbulence.cpp
 VDB_Node_VolumeToMesh.cpp
 VDB_Node_Write.cpp
 VDB_Primitive.cpp
 VDB_Utils.cpp
)

set (HEADERS
 VDB_Node_FBM.h
 VDB_Node_MeshToVolume.h
 VDB_Node_Noise.h
 VDB_Node_TestCustomData.h
 VDB_Node_Turbulence.h
 VDB_Node_VolumeToMesh.h
 VDB_Node_Write.h
 VDB_Primitive.h
 VDB_Utils.h
)

set (LINK_LIBS
 openvdb
 ${ILMBASE_HALF_LIB}
 ${TBB_LIBRARIES}
 ${ZLIB_LIBRARIES}
 ${SEEXPR_LIBRARY}
)

set (LINK_DEBUG_LIBS
 openvdb
 ${ILMBASE_HALF_LIB}
 ${TBB_DEBUG_LIBRARIES}
 ${ZLIB_LIBRARIES}
 ${SEEXPR_LIBRARY}
)

add_softimage_plugin (openvdb_softimage ${SOURCES} ${HEADERS})

target_link_libraries (openvdb_softimage ${LINK_LIBS})
target_link_libraries (openvdb_softimage debug ${LINK_DEBUG_LIBS})
