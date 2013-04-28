// OpenVDB_Softimage
// VDB_Node_TestCustomData.cpp
// ICE node to test the custom data type

#include <xsi_application.h>
#include <xsi_icenodedef.h>
#include <xsi_factory.h>
#include <xsi_dataarray.h>
#include <xsi_dataarray2D.h>
#include <xsi_icegeometry.h>
#include <xsi_doublearray.h>
#include <xsi_longarray.h>
#include <xsi_iceportstate.h>

#include "VDB_Node_TestCustomData.h"
#include "VDB_Primitive.h"

// port values
static const ULONG kGroup1 = 100;
static const ULONG kInVDBGrid = 200;
static const ULONG kOutVDBGrid = 300;

using namespace XSI;

VDB_Node_TestCustomData::VDB_Node_TestCustomData()
{
}

VDB_Node_TestCustomData::~VDB_Node_TestCustomData()
{
}

CStatus VDB_Node_TestCustomData::Evaluate(ICENodeContext& ctxt)
{
   Application().LogMessage(L"[VDB_Node_TestCustomData] Evaluate");

   CDataArrayCustomType inVDBGridPort(ctxt, kInVDBGrid);

   // The current output port being evaluated...
   ULONG evaluatedPort = ctxt.GetEvaluatedOutputPortID();

   switch (evaluatedPort)
   {
      case kOutVDBGrid:
      {
         CDataArrayCustomType output(ctxt);
         CIndexSet indexSet(ctxt);

         for(CIndexSet::Iterator it = indexSet.Begin(); it.HasNext(); it.Next())
         {
            //Application().LogMessage(L"[VDB_Node_TestCustomData] iterator index = " + CValue(it.GetIndex()).GetAsText());

            ULONG inDataSize;
            VDB_Primitive* inVDBPrim;
            inVDBGridPort.GetData(it, (const CDataArrayCustomType::TData**)&inVDBPrim, inDataSize);
            if (!inDataSize)
            {
               Application().LogMessage(L"[VDB_Node_TestCustomData] data size is invalid!", siErrorMsg);
               return CStatus::OK;
            }
            Application().LogMessage(L"[VDB_Node_TestCustomData] previous data size = " + CValue(inDataSize).GetAsText());

            VDB_Primitive* outVDBPrim = (VDB_Primitive*)output.Resize(it, sizeof(VDB_Primitive));
            ::memcpy(outVDBPrim, inVDBPrim, inDataSize);

            Application().LogMessage(L"[VDB_Node_TestCustomData] memcpy succeeded");
            Application().LogMessage(L"[VDB_Node_TestCustomData] grid type is " + CString(inVDBPrim->GetTypeName()));
         }
         break;
      }
      default:
         break;
   };

   return CStatus::OK;
}

CStatus VDB_Node_TestCustomData::Register(PluginRegistrar& reg)
{
   ICENodeDef nodeDef;
   Factory factory = Application().GetFactory();
   nodeDef = factory.CreateICENodeDef(L"VDB_Node_TestCustomData", L"Test Custom Data");

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

   // Add output ports.
   CStringArray customTypes(1);
   customTypes[0] = L"vdb_prim";

   st = nodeDef.AddInputPort(kInVDBGrid, kGroup1,
      customTypes, siICENodeStructureSingle, siICENodeContextSingleton,
      L"In", L"inVDBGrid",ULONG_MAX,ULONG_MAX,ULONG_MAX);
   st.AssertSucceeded();

   st = nodeDef.AddOutputPort(kOutVDBGrid, customTypes,
      siICENodeStructureSingle, siICENodeContextSingleton,
      L"Out", L"outVDBGrid");
   st.AssertSucceeded();

   PluginItem nodeItem = reg.RegisterICENode(nodeDef);
   nodeItem.PutCategories(L"OpenVDB");

   return CStatus::OK;
}

SICALLBACK VDB_Node_TestCustomData_BeginEvaluate(ICENodeContext& ctxt)
{
   Application().LogMessage(L"[VDB_Node_TestCustomData] BeginEvaluate");

   CICEPortState portState(ctxt, kInVDBGrid);
   if (portState.IsDirty(CICEPortState::siAnyDirtyState))
   {
      Application().LogMessage(L"[VDB_Node_TestCustomData] port is dirty");
      if (portState.IsDirty(CICEPortState::siDataDirtyState))
      {
         Application().LogMessage(L"[VDB_Node_TestCustomData] port is data dirty");
      }
   }
   return CStatus::OK;
}

SICALLBACK VDB_Node_TestCustomData_Evaluate(ICENodeContext& ctxt)
{
   VDB_Node_TestCustomData* vdbNode = new VDB_Node_TestCustomData();
   vdbNode->Evaluate(ctxt);
   return CStatus::OK;
}
