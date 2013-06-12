// OpenVDB_Softimage
// VDB_Node_Noise.cpp
// ICE node that applies noise to a level set represented by OpenVDB
// The noise can optionally be masked by another level set

#include <xsi_application.h>
#include <xsi_icenodedef.h>
#include <xsi_factory.h>
#include <xsi_dataarray.h>
#include <xsi_dataarray2D.h>
#include <xsi_icegeometry.h>
#include <xsi_doublearray.h>
#include <xsi_longarray.h>
#include <xsi_iceportstate.h>

//#include <SeVec3d.h>
#include <SeNoise.h>

#include "VDB_Node_Noise.h"
#include "VDB_Primitive.h"

// port values
static const ULONG kGroup1 = 100;
static const ULONG kInVDBGrid = 200;
static const ULONG kOutVDBGrid = 300;

using namespace XSI;

VDB_Node_Noise::VDB_Node_Noise()
{
}

VDB_Node_Noise::~VDB_Node_Noise()
{
}

CStatus VDB_Node_Noise::Evaluate(ICENodeContext& ctxt)
{
   Application().LogMessage(L"[VDB_Node_Noise] Evaluate");

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
            //Application().LogMessage(L"[VDB_Node_Noise] iterator index = " + CValue(it.GetIndex()).GetAsText());

            ULONG inDataSize;
            VDB_Primitive* inVDBPrim;
            inVDBGridPort.GetData(it, (const CDataArrayCustomType::TData**)&inVDBPrim, inDataSize);
            if (!inDataSize)
            {
               Application().LogMessage(L"[VDB_Node_Noise] data size is invalid!", siErrorMsg);
               return CStatus::OK;
            }
            Application().LogMessage(L"[VDB_Node_Noise] previous data size = " + CValue(inDataSize).GetAsText());

            openvdb::GridBase::Ptr grid = inVDBPrim->GetGridPtr();
            openvdb::FloatGrid::Ptr outputGrid;
            outputGrid = openvdb::gridPtrCast<openvdb::FloatGrid>(grid);
            //openvdb::math::Transform::Ptr transform = outputGrid->getTransform();

            for (openvdb::FloatGrid::ValueOnIter iter = outputGrid->beginValueOn(); iter; ++iter)
            {
               if (iter.isVoxelValue())
               {
                  openvdb::Coord coord = iter.getCoord();
                  openvdb::Vec3d vec = outputGrid->indexToWorld(coord);
                  double result;
                  double p[3] = {vec.x(), vec.y(), vec.z()};
                  SeExpr::Noise<3,1>(p, &result);
                  Application().LogMessage(CValue(vec.x()).GetAsText() + "," + CValue(vec.y()).GetAsText() + "," + CValue(vec.z()).GetAsText(), siVerboseMsg);
                  Application().LogMessage(CValue(.5*result+.5).GetAsText(), siVerboseMsg);
                  iter.setValue(*iter + 1.0f * result);
               }
            }

            VDB_Primitive* outVDBPrim = (VDB_Primitive*)output.Resize(it, sizeof(VDB_Primitive));
            ::memcpy(outVDBPrim, inVDBPrim, inDataSize);

            Application().LogMessage(L"[VDB_Node_Noise] memcpy succeeded");
            Application().LogMessage(L"[VDB_Node_Noise] grid type is " + CString(inVDBPrim->GetTypeName()));
         }
         break;
      }
      default:
         break;
   };

   return CStatus::OK;
}

CStatus VDB_Node_Noise::Register(PluginRegistrar& reg)
{
   ICENodeDef nodeDef;
   Factory factory = Application().GetFactory();
   nodeDef = factory.CreateICENodeDef(L"VDB_Node_Noise", L"VDB Noise");

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

   // Add custom type names.
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

SICALLBACK VDB_Node_Noise_BeginEvaluate(ICENodeContext& ctxt)
{
   Application().LogMessage(L"[VDB_Node_Noise] BeginEvaluate");

   CICEPortState portState(ctxt, kInVDBGrid);
   if (portState.IsDirty(CICEPortState::siAnyDirtyState))
   {
      Application().LogMessage(L"[VDB_Node_Noise] port is dirty");
      if (portState.IsDirty(CICEPortState::siDataDirtyState))
      {
         Application().LogMessage(L"[VDB_Node_Noise] port is data dirty");
      }
   }
   return CStatus::OK;
}

SICALLBACK VDB_Node_Noise_Evaluate(ICENodeContext& ctxt)
{
   VDB_Node_Noise* vdbNode = new VDB_Node_Noise();
   vdbNode->Evaluate(ctxt);
   return CStatus::OK;
}
