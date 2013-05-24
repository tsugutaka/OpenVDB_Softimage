// OpenVDB_Softimage
// VDB_Node_FBM.h
// ICE node that applies a mutli-frequency noise function,
// FBM (fractal browning motion) to a level set represented by OpenVDB.
// The noise can optionally be masked by another level set

#ifndef VDB_NODE_FBM_H
#define VDB_NODE_FBM_H

#include <xsi_pluginregistrar.h>
#include <xsi_status.h>
#include <xsi_icenodecontext.h>

#include <openvdb/openvdb.h>

class VDB_Node_FBM
{
public:
   VDB_Node_FBM();
   ~VDB_Node_FBM();

   XSI::CStatus Evaluate(XSI::ICENodeContext& ctxt);
   
   static XSI::CStatus Register(XSI::PluginRegistrar& reg);
};

#endif
