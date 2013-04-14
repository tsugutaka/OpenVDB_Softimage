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

using namespace XSI;

VDB_Node_MeshToVolume::VDB_Node_MeshToVolume()
{
}

VDB_Node_MeshToVolume::~VDB_Node_MeshToVolume()
{
}

CStatus VDB_Node_MeshToVolume::Evaluate(ICENodeContext& ctxt)
{
   Application().LogMessage(L"Evaluate");

   CDataArrayFloat voxelSize(ctxt, kMeshToVolumeVoxelSize);

   m_transform = openvdb::math::Transform::createLinearTransform(voxelSize[0]);

   CICEGeometry geometry(ctxt, kMeshToVolumeGeometry);
   if (!geometry.IsValid())
   {
      Application().LogMessage(L"[openvdb] Input geometry is invalid!", siErrorMsg);
   }

   CDoubleArray points;
   geometry.GetPointPositions(points);

   // fill pointList for openvdb::tools::MeshToVolume
   std::vector<openvdb::Vec3s> pointList;
   pointList.reserve(geometry.GetPointPositionCount());
   
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
   CDataArrayFloat extWidth(ctxt, kMeshToVolumeExteriorWidth);
   CDataArrayFloat intWidth(ctxt, kMeshToVolumeInteriorWidth);
   converter.convertToLevelSet(pointList, polygonList, extWidth[0], intWidth[0]);

   // The current output port being evaluated...
   ULONG evaluatedPort = ctxt.GetEvaluatedOutputPortID();

   switch (evaluatedPort)
   {
      case kMeshToVolumeVDBGrid:
      {
         CDataArrayCustomType output(ctxt);
         CIndexSet::Iterator it = CIndexSet(ctxt).Begin();
         
         for(; it.HasNext(); it.Next())
			{
            //Application().LogMessage(CValue(sizeof(openvdb::FloatGrid)).GetAsText());
            //openvdb::FloatGrid::Ptr distGrid;
            //vdb_grid* grid = (vdb_grid*)output.Resize(it, sizeof(vdb_grid));
            //grid->m_distGrid = converter.distGridPtr();
            
            openvdb::FloatGrid::Ptr* grid = (openvdb::FloatGrid::Ptr*)output.Resize(it, sizeof(openvdb::FloatGrid));
            grid = &converter.distGridPtr();

            //openvdb::FloatGrid::ConstPtr distGrid;
            //distGrid = (openvdb::FloatGrid::ConstPtr)output.Resize(it, sizeof(openvdb::FloatGrid));
            //distGrid = *(converter.distGridPtr());
            //distGrid = openvdb::gridConstPtrCast<openvdb::FloatGrid>(converter.distGridPtr());
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
   st = nodeDef.DefineCustomType(L"openvdb_grid" ,L"VDB Grid",
      L"openvdb grid type", 155, 21, 10);
   st.AssertSucceeded();

   // Add input ports and groups.
   st = nodeDef.AddPortGroup(kMeshToVolumeGroup1);
   st.AssertSucceeded();

   st = nodeDef.AddInputPort(kMeshToVolumeGeometry, kMeshToVolumeGroup1,
      siICENodeDataGeometry, siICENodeStructureSingle,
      siICENodeContextSingleton, L"Geometry", L"geometry");
   st.AssertSucceeded();

   st = nodeDef.AddInputPort(kMeshToVolumeVoxelSize, kMeshToVolumeGroup1,
      siICENodeDataFloat, siICENodeStructureSingle, siICENodeContextSingleton,
      L"Voxel Size", L"voxelSize", CValue(0.1));
   st.AssertSucceeded();

   st = nodeDef.AddInputPort(kMeshToVolumeExteriorWidth, kMeshToVolumeGroup1,
   siICENodeDataFloat, siICENodeStructureSingle, siICENodeContextSingleton,
   L"Exterior Width", L"extWidth", CValue(2.0));
   st.AssertSucceeded();

   st = nodeDef.AddInputPort(kMeshToVolumeInteriorWidth, kMeshToVolumeGroup1,
   siICENodeDataFloat, siICENodeStructureSingle, siICENodeContextSingleton,
   L"Interior Width", L"intWidth", CValue(2.0));
   st.AssertSucceeded();

   // Add output ports.
   CStringArray customTypes(1);
   customTypes[0] = L"openvdb_grid";

   st = nodeDef.AddOutputPort( kMeshToVolumeVDBGrid, customTypes,
      siICENodeStructureSingle, siICENodeContextAny,
      L"VDB Grid", L"openvdb_grid");
   st.AssertSucceeded();

   PluginItem nodeItem = reg.RegisterICENode(nodeDef);
   nodeItem.PutCategories(L"OpenVDB");

   return CStatus::OK;
}

SICALLBACK VDB_Node_MeshToVolume_Init(ICENodeContext& ctxt)
{
   if (!openvdb::FloatGrid::isRegistered())
   {
      openvdb::initialize();
      Application().LogMessage(L"[openvdb] Initialized!");
   }

   return CStatus::OK;
}

SICALLBACK VDB_Node_MeshToVolume_Evaluate(ICENodeContext& ctxt)
{
   VDB_Node_MeshToVolume* vdbNode = new VDB_Node_MeshToVolume();
   vdbNode->Evaluate(ctxt);
   return CStatus::OK;
}