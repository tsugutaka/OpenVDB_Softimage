// OpenVDB_Softimage
// VDB_Node_Write.h
// ICE node to write grids to disk

#ifndef VDB_NODE_WRITE_H
#define VDB_NODE_WRITE_H

#include <xsi_pluginregistrar.h>
#include <xsi_status.h>
#include <xsi_icenodecontext.h>

#include <openvdb/openvdb.h>

class VDB_Node_Write
{
public:
   VDB_Node_Write();
   ~VDB_Node_Write();

   XSI::CStatus Evaluate(XSI::ICENodeContext& ctxt);
   
   static XSI::CStatus Register(XSI::PluginRegistrar& reg);
};

#endif