// OpenVDB_Softimage
// VDB_Node_Write.cpp
// ICE node to write grids to disk

#include <xsi_application.h>
#include <xsi_icenodedef.h>
#include <xsi_factory.h>
#include <xsi_dataarray.h>
#include <xsi_dataarray2D.h>
#include <xsi_icegeometry.h>
#include <xsi_doublearray.h>
#include <xsi_longarray.h>
#include <xsi_iceportstate.h>

#include "VDB_Node_Write.h"
#include "VDB_Primitive.h"

// port values
static const ULONG kGroup1 = 100;
static const ULONG kGroup2 = 101;
static const ULONG kFilepath = 200;
static const ULONG kVDBGrid = 201;
static const ULONG kSuccess = 300;

using namespace XSI;

VDB_Node_Write::VDB_Node_Write()
{
}

VDB_Node_Write::~VDB_Node_Write()
{
}

CStatus VDB_Node_Write::Evaluate(ICENodeContext& ctxt)
{
   Application().LogMessage(L"[VDB_Node_Write] Evaluate");

   CDataArrayString filePath(ctxt, kFilepath);
   Application().LogMessage(L"[VDB_Node_Write] " + filePath[0]);

   // The current output port being evaluated...
   ULONG evaluatedPort = ctxt.GetEvaluatedOutputPortID();

   switch (evaluatedPort)
   {
      case kSuccess:
      {
         CDataArrayBool output(ctxt);
         CIndexSet indexSet(ctxt);

         ULONG portCount;
         ctxt.GetGroupInstanceCount(kGroup2, portCount);
         Application().LogMessage(L"[VDB_Node_Write] port count=" + CValue(portCount).GetAsText());

         openvdb::io::File file(filePath[0].GetAsciiString());
         openvdb::GridPtrVec grids;

         for (ULONG portIdx=0; portIdx<portCount; portIdx++)
			{
            CDataArrayCustomType VDBGridPort(ctxt, kVDBGrid, portIdx);

            for (CIndexSet::Iterator it = indexSet.Begin(); it.HasNext(); it.Next())
            {
               Application().LogMessage(L"[VDB_Node_Write] iterator index = " + CValue(it.GetIndex()).GetAsText());
               
               ULONG dataSize;
               VDB_Primitive* VDBPrim;
               VDBGridPort.GetData(it, (const CDataArrayCustomType::TData**)&VDBPrim, dataSize);
               if (!dataSize)
               {
                  Application().LogMessage(L"[VDB_Node_Write] data size is invalid!", siErrorMsg);
                  output.Set(it, false);
                  return CStatus::OK;
               }
               Application().LogMessage(L"[VDB_Node_Write] previous data size = " + CValue(dataSize).GetAsText());
               grids.push_back(VDBPrim->GetGridPtr());
               //openvdb::GridBase::Ptr grid = VDBPrim->GetGridPtr();
               //openvdb::FloatGrid::Ptr outputGrid;
               //outputGrid = openvdb::gridPtrCast<openvdb::FloatGrid>(grid);
               //openvdb::math::Transform::Ptr transform = outputGrid->getTransform();

               //VDB_Primitive* outVDBPrim = (VDB_Primitive*)output.Resize(it, sizeof(VDB_Primitive));
               //::memcpy(outVDBPrim, inVDBPrim, inDataSize);

               Application().LogMessage(L"[VDB_Node_Write] memcpy succeeded");
               Application().LogMessage(L"[VDB_Node_Write] grid type is " + CString(VDBPrim->GetTypeName()));
               output.Set(it, true);
            }
         }
         file.write(grids);
         file.close();

         break;
      }
      default:
         break;
   };

   return CStatus::OK;
}

CStatus VDB_Node_Write::Register(PluginRegistrar& reg)
{
   ICENodeDef nodeDef;
   Factory factory = Application().GetFactory();
   nodeDef = factory.CreateICENodeDef(L"VDB_Node_Write", L"VDB Write");

   CStatus st;
   st = nodeDef.PutColor(110, 110, 110);
   st.AssertSucceeded();

   st = nodeDef.PutThreadingModel(siICENodeSingleThreading);
   st.AssertSucceeded();

   // Add custom types definition
   st = nodeDef.DefineCustomType(L"vdb_prim" ,L"VDB Grid",
      L"openvdb grid type", 155, 21, 10);
   st.AssertSucceeded();

   // Add input ports and groups.
   st = nodeDef.AddPortGroup(kGroup1);
   st.AssertSucceeded();

   st = nodeDef.AddPortGroup(kGroup2, 1, 10);
   st.AssertSucceeded();

   // Add output ports.
   CStringArray customTypes(1);
   customTypes[0] = L"vdb_prim";

   st = nodeDef.AddInputPort(kFilepath, kGroup1, siICENodeDataString,
      siICENodeStructureSingle, siICENodeContextSingleton,
      L"File Path", L"filePath", L"");
   st.AssertSucceeded();

   st = nodeDef.AddInputPort(kVDBGrid, kGroup2,
      customTypes, siICENodeStructureSingle, siICENodeContextSingleton,
      L"VDBGrid", L"vdbGrid",ULONG_MAX,ULONG_MAX,ULONG_MAX);
   st.AssertSucceeded();

   st = nodeDef.AddOutputPort(kSuccess, siICENodeDataBool,
      siICENodeStructureSingle, siICENodeContextSingleton,
      L"Success", L"success");
   st.AssertSucceeded();

   PluginItem nodeItem = reg.RegisterICENode(nodeDef);
   nodeItem.PutCategories(L"OpenVDB");

   return CStatus::OK;
}

SICALLBACK VDB_Node_Write_BeginEvaluate(ICENodeContext& ctxt)
{
   Application().LogMessage(L"[VDB_Node_Write] BeginEvaluate");

   CICEPortState portState(ctxt, kFilepath);
   if (portState.IsDirty(CICEPortState::siAnyDirtyState))
   {
      Application().LogMessage(L"[VDB_Node_Write] port is dirty");
      if (portState.IsDirty(CICEPortState::siDataDirtyState))
      {
         Application().LogMessage(L"[VDB_Node_Write] port is data dirty");
      }
      portState.ClearState();
   }
   return CStatus::OK;
}

SICALLBACK VDB_Node_Write_Evaluate(ICENodeContext& ctxt)
{
   VDB_Node_Write* vdbNode = new VDB_Node_Write();
   vdbNode->Evaluate(ctxt);
   return CStatus::OK;
}
