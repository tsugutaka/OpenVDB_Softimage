// OpenVDB_Softimage
// VDB_Node_MeshToVolume.h
// Mesh to Volume custom ICE node

#ifndef VDB_NODE_MESHTOVOLUME_H
#define VDB_NODE_MESHTOVOLUME_H

#include <xsi_pluginregistrar.h>
#include <xsi_status.h>
#include <xsi_icenodecontext.h>

#include <openvdb/openvdb.h>

enum VDB_Node_MeshToVolumePorts
{
   kMeshToVolumeGroup1 = 100,
   kMeshToVolumeGeometry = 0,
   kMeshToVolumeVoxelSize = 1,
   kMeshToVolumeExteriorWidth = 2,
   kMeshToVolumeInteriorWidth = 3,
   kMeshToVolumeVDBGrid = 200
};

struct vdb_grid
{
   openvdb::GridBase::ConstPtr m_grid;
};

class VDB_Node_MeshToVolume
{
public:
   VDB_Node_MeshToVolume();
   ~VDB_Node_MeshToVolume();

   XSI::CStatus Evaluate(XSI::ICENodeContext& ctxt);
   
   static XSI::CStatus Register(XSI::PluginRegistrar& reg);
private:
   openvdb::math::Transform::Ptr m_transform;
   //openvdb::FloatGrid::Ptr distGrid;
};

#endif

