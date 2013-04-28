// OpenVDB_Softimage
// VDB_Node_VolumeToMesh.cpp
// Volume to Mesh custom ICE node

#include <xsi_application.h>
#include <xsi_icenodedef.h>
#include <xsi_factory.h>
#include <xsi_iceportstate.h>

#include <openvdb/tools/VolumeToMesh.h>

#include "VDB_Node_VolumeToMesh.h"
#include "VDB_Primitive.h"

// port values
static const ULONG kGroup1 = 100;
static const ULONG kFilePath = 0;
static const ULONG kIsoValue = 1;
static const ULONG kAdaptivity = 2;
static const ULONG kVDBGrid = 3;
static const ULONG kPointArray = 200;
static const ULONG kPolygonArray = 201;
static const ULONG kTypeCns = 400;

using namespace XSI;
using namespace XSI::MATH;

VDB_Node_VolumeToMesh::VDB_Node_VolumeToMesh()
{
}

VDB_Node_VolumeToMesh::~VDB_Node_VolumeToMesh()
{
}

//CStatus VDB_Node_VolumeToMesh::BeginEvaluate(ICENodeContext& ctxt)
//{
//   CDataArrayCustomType inVDBGridPort(ctxt, kVDBGrid);
//
//   ULONG inDataSize;
//   VDB_Primitive* inVDBPrim;
//   inVDBGridPort.GetData(0, (const CDataArrayCustomType::TData**)&inVDBPrim, inDataSize);
//
//   // if the vdb grid port has no data, then exit
//   if (!inDataSize) return CStatus::OK;
//
//   CDataArrayFloat iso(ctxt, kIsoValue);
//   CDataArrayFloat adaptivity(ctxt, kAdaptivity);
//   
//   // Setup level set mesher
//   openvdb::tools::VolumeToMesh mesher(iso[0], adaptivity[0]);
//
//   const openvdb::GridBase::ConstPtr grid = inVDBPrim->GetConstGridPtr();
//   
//   // if the grid is invalid?
//   if (!grid) return CStatus::OK;
//
//   // log some info about the grid
//   CString gridName(grid->getName().c_str());
//   CString gridType(grid->valueType().c_str());
//   Application().LogMessage(L"[VDB_Node_VolumeToMesh] " + gridName + L" : " + gridType );
//
//   // level set types only
//   const openvdb::GridClass gridClass = grid->getGridClass();
//   if (gridClass == openvdb::GRID_LEVEL_SET)
//   {
//      openvdb::FloatGrid::ConstPtr levelSetGrid;
//      levelSetGrid = openvdb::gridConstPtrCast<openvdb::FloatGrid>(grid);
//      mesher(*(levelSetGrid.get()));
//   }
//
//   m_isValid = true;
//   return CStatus::OK;
//}

CStatus VDB_Node_VolumeToMesh::Evaluate(ICENodeContext& ctxt)
{
   Application().LogMessage(L"[VDB_Node_VolumeToMesh] Evaluate");

   CDataArrayCustomType inVDBGridPort(ctxt, kVDBGrid);
   
   ULONG inDataSize;
   VDB_Primitive* inVDBPrim;
   inVDBGridPort.GetData(0, (const CDataArrayCustomType::TData**)&inVDBPrim, inDataSize);
   
   // if the vdb grid port has no data, then exit
   if (!inDataSize) return CStatus::OK;

   const openvdb::GridBase::ConstPtr grid = inVDBPrim->GetConstGridPtr();
   
   // if the grid is invalid?
   if (!grid) return CStatus::OK;

   // log some info about the grid
   CString gridName(grid->getName().c_str());
   CString gridType(grid->valueType().c_str());
   Application().LogMessage(L"[VDB_Node_VolumeToMesh] " + gridName + L" : " + gridType );

   CDataArrayFloat iso(ctxt, kIsoValue);
   CDataArrayFloat adaptivity(ctxt, kAdaptivity);
   
   // Setup level set mesher
   openvdb::tools::VolumeToMesh mesher(iso[0], adaptivity[0]);

   // level set classes only
   const openvdb::GridClass gridClass = grid->getGridClass();
   if (gridClass == openvdb::GRID_LEVEL_SET)
   {
      openvdb::FloatGrid::ConstPtr levelSetGrid;
      levelSetGrid = openvdb::gridConstPtrCast<openvdb::FloatGrid>(grid);
      mesher(*(levelSetGrid.get()));
   }
   else
   {
      return CStatus::OK;
   }

   // The current output port being evaluated...
   ULONG evaluatedPort = ctxt.GetEvaluatedOutputPortID();
   
   switch (evaluatedPort)
   {
      case kPointArray:
      {
         CDataArray2DVector3f output(ctxt);
         ULONG listSize = mesher.pointListSize();
         CDataArray2DVector3f::Accessor iter;
         iter = output.Resize(0, (ULONG)listSize);
         CIndexSet::Iterator index = CIndexSet(ctxt).Begin();
         const openvdb::tools::PointList& points = mesher.pointList();

         for (size_t i=0; i<listSize; ++i, index.Next())
         {
            openvdb::math::Vec3s pnt = points[i];
            iter[index] = CVector3f(pnt.x(), pnt.y(), pnt.z());
         }
         break;
      }
      case kPolygonArray:
      {
         using openvdb::tools::PolygonPoolList;
         using openvdb::tools::PolygonPool;

         const PolygonPoolList& polyList = mesher.polygonPoolList();

         ULONG arraySize = 0;
         for (size_t n=0; n<mesher.polygonPoolListSize(); ++n)
         {
            const PolygonPool& polygons = polyList[n];
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
            const PolygonPool& polygons = polyList[i];
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

   // Add custom types definition
   st = nodeDef.DefineCustomType(L"vdb_prim" ,L"VDB Grid",
      L"openvdb grid type", 155, 21, 10);
   st.AssertSucceeded();

   // Add input ports and groups.
   st = nodeDef.AddPortGroup(kGroup1);
   st.AssertSucceeded();

   //st = nodeDef.AddInputPort(kFilePath, kGroup1,
   //   siICENodeDataString, siICENodeStructureSingle, siICENodeContextSingleton,
   //   L"File Path", L"filePath", L"");
   //st.AssertSucceeded();

   CStringArray customTypes(1);
   customTypes[0] = L"vdb_prim";
   
   // stupid default arguments wont work have to add ULONG_MAX
   st = nodeDef.AddInputPort(kVDBGrid, kGroup1, customTypes,
      siICENodeStructureSingle, siICENodeContextSingleton,
      L"VDB Grid", L"inVDBGrid",ULONG_MAX,ULONG_MAX,ULONG_MAX);
   st.AssertSucceeded();
   
   st = nodeDef.AddInputPort(kIsoValue, kGroup1, siICENodeDataFloat,
      siICENodeStructureSingle, siICENodeContextSingleton,
      L"Iso Value", L"isoValue", 0.0);
   st.AssertSucceeded();

   st = nodeDef.AddInputPort(kAdaptivity, kGroup1, siICENodeDataFloat,
      siICENodeStructureSingle, siICENodeContextSingleton,
      L"Adaptivity", L"adaptivity", 0.0);
   st.AssertSucceeded();

   // Add output ports.
   st = nodeDef.AddOutputPort(kPointArray, siICENodeDataVector3,
      siICENodeStructureArray, siICENodeContextSingleton,
      L"Point Array", L"pointList");
   st.AssertSucceeded();

   st = nodeDef.AddOutputPort(kPolygonArray, siICENodeDataLong,
      siICENodeStructureArray, siICENodeContextSingleton,
      L"Polygon Array", L"polygonPoolList");
   st.AssertSucceeded();

   PluginItem nodeItem = reg.RegisterICENode(nodeDef);
   nodeItem.PutCategories(L"OpenVDB");

   return CStatus::OK;
}

//SICALLBACK VDB_Node_VolumeToMesh_Init(ICENodeContext& ctxt)
//{
   //Application().LogMessage(L"Init");
   //if (!openvdb::FloatGrid::isRegistered())
   //{
   //   openvdb::initialize();
   //   Application().LogMessage(L"[openvdb] Initialized!");
   //}

//   return CStatus::OK;
//}

//SICALLBACK VDB_Node_VolumeToMesh_BeginEvaluate(ICENodeContext& ctxt)
//{
//   //CDataArrayString filePathData(ctxt, kFilePath);
//   Application().LogMessage(L"[VDB_Node_VolumeToMesh] BeginEvaluate");
//
//   CValue userData = ctxt.GetUserData();
//   VDB_Node_VolumeToMesh* vdbNode;
//   if (userData.IsEmpty())
//   {
//      vdbNode = new VDB_Node_VolumeToMesh;
//   }
//   else
//   {
//      vdbNode = (VDB_Node_VolumeToMesh*)(CValue::siPtrType)userData;
//   }
//   vdbNode->BeginEvaluate(ctxt);
//   ctxt.PutUserData((CValue::siPtrType)vdbNode);
//   return CStatus::OK;
//}

SICALLBACK VDB_Node_VolumeToMesh_Evaluate(ICENodeContext& ctxt)
{
   //CValue userData = ctxt.GetUserData();
   //VDB_Node_VolumeToMesh* vdbNode;
   //vdbNode = (VDB_Node_VolumeToMesh*)(CValue::siPtrType)userData;
   VDB_Node_VolumeToMesh* vdbNode = new VDB_Node_VolumeToMesh();
   vdbNode->Evaluate(ctxt);
   
   return CStatus::OK;
}