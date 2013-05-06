// OpenVDB_Softimage
// VDB_Node_Noise.h
// ICE node that applies noise to a level set represented by OpenVDB
// The noise can optionally be masked by another level set

#ifndef VDB_NODE_NOISE_H
#define VDB_NODE_NOISE_H

#include <xsi_pluginregistrar.h>
#include <xsi_status.h>
#include <xsi_icenodecontext.h>

#include <openvdb/openvdb.h>

class VDB_Node_Noise
{
public:
   VDB_Node_Noise();
   ~VDB_Node_Noise();

   XSI::CStatus Evaluate(XSI::ICENodeContext& ctxt);
   
   static XSI::CStatus Register(XSI::PluginRegistrar& reg);
};

#endif
