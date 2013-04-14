// OpenVDB_Softimage
// VDB_Node_VolumeToMesh.cpp
// Volume to Mesh custom ICE node

#include <xsi_application.h>
#include <xsi_icenodedef.h>
#include <xsi_factory.h>


#include <openvdb/tools/VolumeToMesh.h>

#include "VDB_Node_VolumeToMesh.h"

using namespace XSI;
using namespace XSI::MATH;

VDB_Node_VolumeToMesh::VDB_Node_VolumeToMesh()
{
   m_filePath = CString();
}

VDB_Node_VolumeToMesh::~VDB_Node_VolumeToMesh()
{
}

const CStatus VDB_Node_VolumeToMesh::LoadGrids()
{
   openvdb::io::File file(m_filePath.GetAsciiString());
   try
   {
      file.open();
      m_grids = file.getGrids();
      m_meta = file.getMetadata();
      file.close();
   }
   catch (openvdb::Exception& e)
   {
      XSI::Application().LogMessage(CString(e.what()) + L" : " + m_filePath);
      return CStatus::Fail;
   }
   return CStatus::OK;
}

const CStatus VDB_Node_VolumeToMesh::SetFilePath(CString filePath)
{
   m_filePath = filePath;
   return CStatus::OK;
}

CStatus VDB_Node_VolumeToMesh::Evaluate(ICENodeContext& ctxt)
{
   Application().LogMessage(L"Evaluate");

   CDataArrayFloat iso(ctxt, kVolumeToMeshIsoValue);
   CDataArrayFloat adaptivity(ctxt, kVolumeToMeshAdaptivity);
   
   // Setup level set mesher
   openvdb::tools::VolumeToMesh mesher(iso[0], adaptivity[0]);

   openvdb::GridPtrVec::const_iterator it = m_grids->begin();
   for (; it != m_grids->end(); ++it)
   {
      const openvdb::GridBase::ConstPtr grid = *it;
      if (!grid) continue;
      CString gridName(grid->getName().c_str());
      CString gridType(grid->valueType().c_str());
      Application().LogMessage(m_filePath + L" : " + gridName + L" : " + gridType );

      // first level set only for now
      const openvdb::GridClass gridClass = grid->getGridClass();
      if (gridClass == openvdb::GRID_LEVEL_SET)
      {
         openvdb::FloatGrid::ConstPtr levelSetGrid;
         levelSetGrid = openvdb::gridConstPtrCast<openvdb::FloatGrid>(*it);
         mesher(*(levelSetGrid.get()));
         break;
      }
   }

   // The current output port being evaluated...
   ULONG evaluatedPort = ctxt.GetEvaluatedOutputPortID();
   
   switch (evaluatedPort)
   {
      case kVolumeToMeshPointArray:
      {
         CDataArray2DVector3f output(ctxt);
         ULONG listSize = mesher.pointListSize();
         CDataArray2DVector3f::Accessor iter = output.Resize(0, listSize);
         CIndexSet::Iterator index = CIndexSet(ctxt).Begin();
         const openvdb::tools::PointList& points = mesher.pointList();

         for (size_t i=0; i<listSize; ++i, index.Next())
         {
            openvdb::math::Vec3s pnt = points[i];
            iter[index] = CVector3f(pnt.x(), pnt.y(), pnt.z());
         }
         break;
      }
      case kVolumeToMeshPolygonArray:
      {
         using openvdb::tools::PolygonPoolList;
         using openvdb::tools::PolygonPool;

         const PolygonPoolList& poolList = mesher.polygonPoolList();

         ULONG arraySize = 0;
         for (size_t n=0; n<mesher.polygonPoolListSize(); ++n)
         {
            const PolygonPool& polygons = poolList[n];
            // accumulate all quads and triangles
            // keep in mind we need a -1 at the end of each polygon 
            arraySize += polygons.numQuads() * 5;
            arraySize += polygons.numTriangles() * 4;
         }

         CDataArray2DLong output(ctxt);
         CDataArray2DLong::Accessor iter = output.Resize(0, arraySize);
         CIndexSet::Iterator index = CIndexSet(ctxt).Begin();
         
         size_t i=0, q=0, t=0;
         for (; i<mesher.polygonPoolListSize(); ++i, q=0, t=0)
         {
            const PolygonPool& polygons= poolList[i];
            for (; q<polygons.numQuads(); ++q)
            {
               const openvdb::Vec4I& quad = polygons.quad(q);
               iter[index] = quad.w(); index.Next();
               iter[index] = quad.z(); index.Next();
               iter[index] = quad.y(); index.Next();
               iter[index] = quad.x(); index.Next();
               // end of quad
               iter[index] = -1; index.Next();
            }
            for (; t<polygons.numTriangles(); ++t)
            {
               const openvdb::Vec3I& triangle = polygons.triangle(t);
               iter[index] = triangle.z(); index.Next();
               iter[index] = triangle.y(); index.Next();
               iter[index] = triangle.x(); index.Next();
               // end of triangle
               iter[index] = -1; index.Next();
            }
         }
         break;
      }
      default:
         break;
	};
	
	return CStatus::OK;
}

CStatus VDB_Node_VolumeToMesh::Register(PluginRegistrar& reg)
{
	ICENodeDef nodeDef;
   Factory factory = Application().GetFactory();
	nodeDef = factory.CreateICENodeDef(L"VDB_Node_VolumeToMesh", L"Volume To Mesh");

	CStatus st;
	st = nodeDef.PutColor(110, 110, 110);
	st.AssertSucceeded();

   st = nodeDef.PutThreadingModel(siICENodeSingleThreading);
	st.AssertSucceeded();

	// Add input ports and groups.
	st = nodeDef.AddPortGroup(kVolumeToMeshGroup1);
	st.AssertSucceeded();

	st = nodeDef.AddInputPort(kVolumeToMeshFilePath, kVolumeToMeshGroup1,
      siICENodeDataString, siICENodeStructureSingle, siICENodeContextSingleton,
      L"File Path", L"filePath", L"");
	st.AssertSucceeded();
   
   st = nodeDef.AddInputPort(kVolumeToMeshIsoValue, kVolumeToMeshGroup1,
      siICENodeDataFloat, siICENodeStructureSingle, siICENodeContextSingleton,
      L"Iso Value", L"isoValue", 0.0);
	st.AssertSucceeded();

   st = nodeDef.AddInputPort(kVolumeToMeshAdaptivity, kVolumeToMeshGroup1,
      siICENodeDataFloat, siICENodeStructureSingle, siICENodeContextSingleton,
      L"Adaptivity", L"adaptivity", 0.0);
	st.AssertSucceeded();

	// Add output ports.
	st = nodeDef.AddOutputPort(kVolumeToMeshPointArray, siICENodeDataVector3,
      siICENodeStructureArray, siICENodeContextSingleton,
      L"Point Array", L"pointList");
	st.AssertSucceeded();

	st = nodeDef.AddOutputPort(kVolumeToMeshPolygonArray, siICENodeDataLong,
      siICENodeStructureArray, siICENodeContextSingleton,
      L"Polygon Array", L"polygonPoolList");
	st.AssertSucceeded();

	PluginItem nodeItem = reg.RegisterICENode(nodeDef);
	nodeItem.PutCategories(L"OpenVDB");

	return CStatus::OK;
}

SICALLBACK VDB_Node_VolumeToMesh_Init(ICENodeContext& ctxt)
{
   Application().LogMessage(L"Init");
   if (!openvdb::FloatGrid::isRegistered())
   {
      openvdb::initialize();
      Application().LogMessage(L"[openvdb] Initialized!");
   }

   return CStatus::OK;
}

SICALLBACK VDB_Node_VolumeToMesh_BeginEvaluate(ICENodeContext& ctxt)
{
   CDataArrayString filePathData(ctxt, kVolumeToMeshFilePath);
   Application().LogMessage(L"BeginEvaluate");
   
   CValue userData = ctxt.GetUserData();
   VDB_Node_VolumeToMesh* vdbNode;
   if (userData.IsEmpty())
   {
      vdbNode = new VDB_Node_VolumeToMesh;
   }
   else
   {
      vdbNode = (VDB_Node_VolumeToMesh*)(CValue::siPtrType)userData;
   }
   
   vdbNode->SetFilePath(filePathData[0]);
   vdbNode->LoadGrids();
   ctxt.PutUserData((CValue::siPtrType)vdbNode);

   return CStatus::OK;
}

SICALLBACK VDB_Node_VolumeToMesh_Evaluate(ICENodeContext& ctxt)
{
   
   CValue userData = ctxt.GetUserData();
   VDB_Node_VolumeToMesh* vdbNode;
   if (userData.IsEmpty())
   {
      return CStatus::OK;
   }   
   else
   {
      vdbNode = (VDB_Node_VolumeToMesh*)(CValue::siPtrType)userData;
      vdbNode->Evaluate(ctxt);
   }
   
   return CStatus::OK;
}