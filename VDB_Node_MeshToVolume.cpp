// OpenVDB_Softimage
// VDB_Node_MeshToVolume.cpp
// Mesh to Volume custom ICE node

#include <xsi_application.h>
#include <xsi_icenodedef.h>
#include <xsi_factory.h>
#include <xsi_dataarray.h>
#include <xsi_dataarray2D.h>
#include <xsi_icegeometry.h>
#include <xsi_doublearray.h>
#include <xsi_longarray.h>

#include <openvdb/tools/MeshToVolume.h>

#include "VDB_Node_MeshToVolume.h"
#include "VDB_Primitive.h"

// port values
static const ULONG kGroup1 = 100;
static const ULONG kGeometry = 0;
static const ULONG kVoxelSize = 1;
static const ULONG kExteriorWidth = 2;
static const ULONG kInteriorWidth = 3;
static const ULONG kGridName = 4;
static const ULONG kVDBGrid = 200;

using namespace XSI;

VDB_Node_MeshToVolume::VDB_Node_MeshToVolume()
   : m_isDirty(true)
{
}

VDB_Node_MeshToVolume::~VDB_Node_MeshToVolume()
{
}

CStatus VDB_Node_MeshToVolume::Evaluate(ICENodeContext& ctxt)
{
   Application().LogMessage(L"[VDB_Node_MeshToVolume] Evaluate");

   CDataArrayFloat voxelSize(ctxt, kVoxelSize);

   m_transform = openvdb::math::Transform::createLinearTransform(voxelSize[0]);

   CICEGeometry geometry(ctxt, kGeometry);
   if (!geometry.IsValid())
   {
      Application().LogMessage(L"[VDB_Node_MeshToVolume] Input geometry is invalid!", siErrorMsg);
      return CStatus::OK;
   }
   if (geometry.GetGeometryType() != CICEGeometry::siMeshSurfaceType)
   {
      Application().LogMessage(L"[VDB_Node_MeshToVolume] Input geometry must be polymesh at this time!", siErrorMsg);
      return CStatus::OK;
   }

   ULONG pntCount = geometry.GetPointPositionCount();
   // don't process geometry with no points
   if (pntCount==0) return CStatus::OK;
   CDoubleArray points;
   geometry.GetPointPositions(points);

   // fill pointList for openvdb::tools::MeshToVolume
   std::vector<openvdb::Vec3s> pointList;
   pointList.reserve(pntCount);
   
   for (LONG i=0; i<points.GetCount(); i+=3)
   {
      openvdb::Vec3s pnt(points[i], points[i+1], points[i+2]);
      pointList.push_back(m_transform->worldToIndex(pnt));
   }

   CLongArray triangles;
   geometry.GetTrianglePointIndices(triangles);
   
   // fill polygonList for openvdb::tools::MeshToVolume
   std::vector<openvdb::Vec4I> polygonList;
   polygonList.reserve(geometry.GetTriangleCount());
   
   for (LONG i=0; i<triangles.GetCount(); i+=3)
   {
      // look into PolygonPool for meshes with quads and triangles
      openvdb::Vec4I tri(triangles[i], triangles[i+1], triangles[i+2], openvdb::util::INVALID_IDX);
      polygonList.push_back(tri);
   }

   openvdb::tools::MeshToVolume<openvdb::FloatGrid> converter(m_transform);
   CDataArrayFloat extWidth(ctxt, kExteriorWidth);
   CDataArrayFloat intWidth(ctxt, kInteriorWidth);
   converter.convertToLevelSet(pointList, polygonList, extWidth[0], intWidth[0]);
   openvdb::GridBase::Ptr outputGrid = converter.distGridPtr();
   CDataArrayString gridName(ctxt, kGridName);
   if (!gridName[0].IsEmpty()) outputGrid->setName(gridName[0].GetAsciiString());

   // The current output port being evaluated...
   ULONG evaluatedPort = ctxt.GetEvaluatedOutputPortID();

   switch (evaluatedPort)
   {
      case kVDBGrid:
      {
         CDataArrayCustomType output(ctxt);
         CIndexSet::Iterator it = CIndexSet(ctxt).Begin();
        
         for(; it.HasNext(); it.Next())
         {
            VDB_Primitive* vdbPrim = (VDB_Primitive*)output.Resize(it, sizeof(VDB_Primitive));
            vdbPrim->SetGrid(*outputGrid);
         }
         break;
      }
      default:
         break;
   };

   return CStatus::OK;
}

CStatus VDB_Node_MeshToVolume::Register(PluginRegistrar& reg)
{
   ICENodeDef nodeDef;
   Factory factory = Application().GetFactory();
   nodeDef = factory.CreateICENodeDef(L"VDB_Node_MeshToVolume", L"Mesh To Volume");

   CStatus st;
   st = nodeDef.PutColor(110, 110, 110);
   st.AssertSucceeded();

   st = nodeDef.PutThreadingModel(siICENodeSingleThreading);
   st.AssertSucceeded();

   // Add custom types definition
   // 178, 26, 13 - old color
   st = nodeDef.DefineCustomType(L"vdb_prim" ,L"VDB Grid",
      L"openvdb grid type", 155, 21, 10);
   st.AssertSucceeded();

   // Add input ports and groups.
   st = nodeDef.AddPortGroup(kGroup1);
   st.AssertSucceeded();

   st = nodeDef.AddInputPort(kGeometry, kGroup1, siICENodeDataGeometry,
      siICENodeStructureSingle, siICENodeContextSingleton,
      L"Geometry", L"geometry");
   st.AssertSucceeded();

   st = nodeDef.AddInputPort(kVoxelSize, kGroup1, siICENodeDataFloat,
      siICENodeStructureSingle, siICENodeContextSingleton,
      L"Voxel Size", L"voxelSize", CValue(0.1));
   st.AssertSucceeded();

   st = nodeDef.AddInputPort(kExteriorWidth, kGroup1, siICENodeDataFloat,
      siICENodeStructureSingle, siICENodeContextSingleton,
      L"Exterior Width", L"extWidth", CValue(2.0));
   st.AssertSucceeded();

   st = nodeDef.AddInputPort(kInteriorWidth, kGroup1, siICENodeDataFloat,
      siICENodeStructureSingle, siICENodeContextSingleton,
      L"Interior Width", L"intWidth", CValue(2.0));
   st.AssertSucceeded();

   st = nodeDef.AddInputPort(kGridName, kGroup1, siICENodeDataString,
      siICENodeStructureSingle, siICENodeContextSingleton,
      L"Grid Name", L"gridName", L"");
   st.AssertSucceeded();

   // Add output ports.
   CStringArray customTypes(1);
   customTypes[0] = L"vdb_prim";

   st = nodeDef.AddOutputPort( kVDBGrid, customTypes, siICENodeStructureSingle,
      siICENodeContextSingleton, L"VDB Grid", L"outVDBGrid");
   st.AssertSucceeded();

   PluginItem nodeItem = reg.RegisterICENode(nodeDef);
   nodeItem.PutCategories(L"OpenVDB");

   return CStatus::OK;
}

SICALLBACK VDB_Node_MeshToVolume_Evaluate(ICENodeContext& ctxt)
{
   VDB_Node_MeshToVolume* vdbNode = new VDB_Node_MeshToVolume();
   vdbNode->Evaluate(ctxt);
   return CStatus::OK;
}