// OpenVDB_Softimage
// VDB_Node_VolumeToMesh.h
// Volume to Mesh custom ICE node

#ifndef VDB_NODE_VOLUMETOMESH_H
#define VDB_NODE_VOLUMETOMESH_H

#include <vector>

#include <xsi_pluginregistrar.h>
#include <xsi_status.h>
#include <xsi_icenodecontext.h>
#include <xsi_vector3f.h>
#include <xsi_dataarray.h>
#include <xsi_dataarray2D.h>

#include <openvdb/openvdb.h>
#include <openvdb/tools/VolumeToMesh.h>

using openvdb::tools::PolygonPool;

class VDB_Node_VolumeToMesh
{
public:
   VDB_Node_VolumeToMesh();
   ~VDB_Node_VolumeToMesh();
   
   XSI::CStatus Cache(XSI::ICENodeContext& ctxt);
   XSI::CStatus Evaluate(XSI::ICENodeContext& ctxt);
   bool IsValid();
   
   static XSI::CStatus Register(XSI::PluginRegistrar& reg);

private:
   bool m_isValid;
   ULONG m_polygonArraySize;
   std::vector<openvdb::math::Vec3s> m_points;
   std::vector<openvdb::Vec4I> m_quads;
   std::vector<openvdb::Vec3I> m_triangles;
};

#endif

