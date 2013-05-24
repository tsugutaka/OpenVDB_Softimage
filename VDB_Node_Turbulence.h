// OpenVDB_Softimage
// VDB_Node_Turbulence.h
// ICE node that applies a mutli-frequency noise function,
// FBM (fractal browning motion) to a level set represented by OpenVDB.
// Absolute value of each noise term is taken. This gives billowy appearance.
// The noise can optionally be masked by another level set

#ifndef VDB_NODE_Turbulence_H
#define VDB_NODE_Turbulence_H

#include <xsi_pluginregistrar.h>
#include <xsi_status.h>
#include <xsi_icenodecontext.h>

#include <openvdb/openvdb.h>

class VDB_Node_Turbulence
{
public:
   VDB_Node_Turbulence();
   ~VDB_Node_Turbulence();

   XSI::CStatus Evaluate(XSI::ICENodeContext& ctxt);
   
   static XSI::CStatus Register(XSI::PluginRegistrar& reg);
};

#endif
