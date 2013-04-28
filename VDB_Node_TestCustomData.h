// OpenVDB_Softimage
// VDB_Node_TestCustomData.h
// ICE node to test the custom data typ

#ifndef VDB_NODE_TESTCUSTOMDATA_H
#define VDB_NODE_TESTCUSTOMDATA_H

#include <xsi_pluginregistrar.h>
#include <xsi_status.h>
#include <xsi_icenodecontext.h>

#include <openvdb/openvdb.h>

class VDB_Node_TestCustomData
{
public:
   VDB_Node_TestCustomData();
   ~VDB_Node_TestCustomData();

   XSI::CStatus Evaluate(XSI::ICENodeContext& ctxt);
   
   static XSI::CStatus Register(XSI::PluginRegistrar& reg);
};

#endif
