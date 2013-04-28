// OpenVDB_Softimage
// VDB_Node_VolumeToMesh.h
// Volume to Mesh custom ICE node

#ifndef VDB_NODE_VOLUMETOMESH_H
#define VDB_NODE_VOLUMETOMESH_H

#include <xsi_pluginregistrar.h>
#include <xsi_status.h>
#include <xsi_icenodecontext.h>
#include <xsi_vector3f.h>
#include <xsi_dataarray.h>
#include <xsi_dataarray2D.h>

#include <openvdb/openvdb.h>

class VDB_Node_VolumeToMesh
{
public:
   VDB_Node_VolumeToMesh();
   ~VDB_Node_VolumeToMesh();
   
   //XSI::CStatus BeginEvaluate(XSI::ICENodeContext& ctxt);
   XSI::CStatus Evaluate(XSI::ICENodeContext& ctxt);
   
   static XSI::CStatus Register(XSI::PluginRegistrar& reg);

};

#endif

