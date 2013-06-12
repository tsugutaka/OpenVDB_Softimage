// OpenVDB_Softimage
// VDB_Node_Turbulence.cpp
// ICE node that applies a mutli-frequency noise function,
// FBM (fractal browning motion) to a level set represented by OpenVDB.
// Absolute value of each noise term is taken. This gives billowy appearance.
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

#include "VDB_Node_Turbulence.h"
#include "VDB_Primitive.h"

// port values
static const ULONG kGroup1 = 100;
static const ULONG kInVDBGrid = 200;
static const ULONG kOctaves = 201;
static const ULONG kLacunarity = 202;
static const ULONG kGain = 203;
static const ULONG kOutVDBGrid = 300;

using namespace XSI;

VDB_Node_Turbulence::VDB_Node_Turbulence()
{
}

VDB_Node_Turbulence::~VDB_Node_Turbulence()
{
}

CStatus VDB_Node_Turbulence::Evaluate(ICENodeContext& ctxt)
{
   Application().LogMessage(L"[VDB_Node_Turbulence] Evaluate");

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
            //Application().LogMessage(L"[VDB_Node_Turbulence] iterator index = " + CValue(it.GetIndex()).GetAsText());

            ULONG inDataSize;
            VDB_Primitive* inVDBPrim;
            inVDBGridPort.GetData(it, (const CDataArrayCustomType::TData**)&inVDBPrim, inDataSize);
            if (!inDataSize)
            {
               Application().LogMessage(L"[VDB_Node_Turbulence] data size is invalid!", siErrorMsg);
               return CStatus::OK;
            }
            Application().LogMessage(L"[VDB_Node_Turbulence] previous data size = " + CValue(inDataSize).GetAsText());

            openvdb::GridBase::Ptr grid = inVDBPrim->GetGridPtr();
            openvdb::FloatGrid::Ptr outputGrid;
            outputGrid = openvdb::gridPtrCast<openvdb::FloatGrid>(grid);
            //openvdb::math::Transform::Ptr transform = outputGrid->getTransform();

            CDataArrayLong octaves(ctxt, kOctaves);
            CDataArrayFloat lacunarity(ctxt, kLacunarity);
            CDataArrayFloat gain(ctxt, kGain);

            for (openvdb::FloatGrid::ValueOnIter iter = outputGrid->beginValueOn(); iter; ++iter)
            {
               if (iter.isVoxelValue())
               {
                  openvdb::Coord coord = iter.getCoord();
                  openvdb::Vec3d vec = outputGrid->indexToWorld(coord);
                  double result;
                  double p[3] = {vec.x(), vec.y(), vec.z()};
                  SeExpr::FBM<3,1,true>(p, &result, double(octaves[0]), double(lacunarity[0]), double(gain[0]));
                  Application().LogMessage(CValue(vec.x()).GetAsText() + "," + CValue(vec.y()).GetAsText() + "," + CValue(vec.z()).GetAsText(), siVerboseMsg);
                  Application().LogMessage(CValue(.5*result+.5).GetAsText(), siVerboseMsg);
                  iter.setValue(*iter + 1.0f * result);
               }
            }

            VDB_Primitive* outVDBPrim = (VDB_Primitive*)output.Resize(it, sizeof(VDB_Primitive));
            ::memcpy(outVDBPrim, inVDBPrim, inDataSize);

            Application().LogMessage(L"[VDB_Node_Turbulence] memcpy succeeded");
            Application().LogMessage(L"[VDB_Node_Turbulence] grid type is " + CString(inVDBPrim->GetTypeName()));
         }
         break;
      }
      default:
         break;
   };

   return CStatus::OK;
}

CStatus VDB_Node_Turbulence::Register(PluginRegistrar& reg)
{
   ICENodeDef nodeDef;
   Factory factory = Application().GetFactory();
   nodeDef = factory.CreateICENodeDef(L"VDB_Node_Turbulence", L"VDB Turbulence");

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

   st = nodeDef.AddInputPort(kOctaves, kGroup1, siICENodeDataLong,
      siICENodeStructureSingle, siICENodeContextSingleton,
      L"Octaves", L"octaves", CValue(6));
   st.AssertSucceeded();

   st = nodeDef.AddInputPort(kLacunarity, kGroup1, siICENodeDataFloat,
      siICENodeStructureSingle, siICENodeContextSingleton,
      L"Lacunarity", L"lacunarity", CValue(2.0));
   st.AssertSucceeded();

   st = nodeDef.AddInputPort(kGain, kGroup1, siICENodeDataFloat,
      siICENodeStructureSingle, siICENodeContextSingleton,
      L"Gain", L"gain", CValue(0.5));
   st.AssertSucceeded();

   st = nodeDef.AddOutputPort(kOutVDBGrid, customTypes,
      siICENodeStructureSingle, siICENodeContextSingleton,
      L"Out", L"outVDBGrid");
   st.AssertSucceeded();

   PluginItem nodeItem = reg.RegisterICENode(nodeDef);
   nodeItem.PutCategories(L"OpenVDB");

   return CStatus::OK;
}

SICALLBACK VDB_Node_Turbulence_BeginEvaluate(ICENodeContext& ctxt)
{
   Application().LogMessage(L"[VDB_Node_Turbulence] BeginEvaluate");

   CICEPortState portState(ctxt, kInVDBGrid);
   if (portState.IsDirty(CICEPortState::siAnyDirtyState))
   {
      Application().LogMessage(L"[VDB_Node_Turbulence] port is dirty");
      if (portState.IsDirty(CICEPortState::siDataDirtyState))
      {
         Application().LogMessage(L"[VDB_Node_Turbulence] port is data dirty");
      }
   }
   return CStatus::OK;
}

SICALLBACK VDB_Node_Turbulence_Evaluate(ICENodeContext& ctxt)
{
   VDB_Node_Turbulence* vdbNode = new VDB_Node_Turbulence();
   vdbNode->Evaluate(ctxt);
   return CStatus::OK;
}
