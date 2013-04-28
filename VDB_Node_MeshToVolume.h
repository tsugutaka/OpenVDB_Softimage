// OpenVDB_Softimage
// VDB_Node_MeshToVolume.h
// Mesh to Volume custom ICE node

#ifndef VDB_NODE_MESHTOVOLUME_H
#define VDB_NODE_MESHTOVOLUME_H

#include <xsi_pluginregistrar.h>
#include <xsi_status.h>
#include <xsi_icenodecontext.h>

#include <openvdb/openvdb.h>

class VDB_Node_MeshToVolume
{
public:
   VDB_Node_MeshToVolume();
   ~VDB_Node_MeshToVolume();

   XSI::CStatus Evaluate(XSI::ICENodeContext& ctxt);
   
   static XSI::CStatus Register(XSI::PluginRegistrar& reg);
private:
   bool m_isDirty;
   openvdb::math::Transform::Ptr m_transform;
};

#endif

