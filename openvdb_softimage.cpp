// openvdb_softimage
#include <xsi_application.h>
#include <xsi_context.h>
#include <xsi_pluginregistrar.h>
#include <xsi_status.h>
#include <xsi_argument.h>
#include <xsi_factory.h>
#include <xsi_command.h>
#include <xsi_x3dobject.h>
#include <xsi_primitive.h>
#include <xsi_polygonmesh.h>
#include <xsi_geometryaccessor.h>
#include <xsi_doublearray.h>
#include <xsi_longarray.h>
#include <xsi_vector3f.h>
#include <xsi_icenodecontext.h>
#include <xsi_icenodedef.h>
#include <xsi_indexset.h>
#include <xsi_dataarray.h>
#include <xsi_dataarray2D.h>
#include <xsi_iceportstate.h>

#include <iostream>
#include <sstream>
#include <string>
#include <iterator>
#include <vector>
#include <openvdb/openvdb.h>
#include <openvdb/tools/MeshToVolume.h>
#include <openvdb/tools/VolumeToMesh.h>

// defines port, group, and map identifiers for the ice node
enum IDs
{
	ID_IN_filePath = 0,
   ID_IN_isoValue = 1,
   ID_IN_adaptivity = 2,
	ID_G_100 = 100,
	ID_OUT_PointArray = 200,
	ID_OUT_PolygonArray = 201,
	ID_TYPE_CNS = 400,
	ID_STRUCT_CNS,
	ID_CTXT_CNS,
	ID_UNDEF = ULONG_MAX
};

struct VolumeToMeshData
{
   VolumeToMeshData() : filePath(L""), isDirty(true) {}

   XSI::CString filePath;
   bool isDirty;
   openvdb::GridPtrVecPtr grids;
   openvdb::MetaMap::Ptr meta;

};

std::string bkgdValueAsString (const openvdb::GridBase::ConstPtr& grid)
{
    std::ostringstream ostr;
    if (grid) {
        const openvdb::TreeBase& tree = grid->baseTree();
        ostr << "background: ";
        openvdb::Metadata::Ptr background = tree.getBackgroundValue();
        if (background) ostr << background->str();
    }
    return ostr.str();
}

std::string bytesAsString (openvdb::Index64 n)
{
   std::ostringstream ostr;
   ostr << std::setprecision(3);
   if (n >> 30) {
      ostr << (n / double(uint64_t(1) << 30)) << "GB";
   } else if (n >> 20) {
      ostr << (n / double(uint64_t(1) << 20)) << "MB";
   } else if (n >> 10) {
      ostr << (n / double(uint64_t(1) << 10)) << "KB";
   } else {
      ostr << n << "B";
   }
   return ostr.str();
}

std::string sizeAsString (openvdb::Index64 n, const std::string& units)
{
   std::ostringstream ostr;
   ostr << std::setprecision(3);
   if (n < 1000) {
      ostr << n;
   } else if (n < 1000000) {
      ostr << (n / 1.0e3) << "K";
   } else if (n < 1000000000) {
      ostr << (n / 1.0e6) << "M";
   } else {
      ostr << (n / 1.0e9) << "G";
   }
   ostr << units;
   return ostr.str();
}

std::string coordAsString (const openvdb::Coord ijk, const std::string& sep)
{
   std::ostringstream ostr;
   ostr << ijk[0] << sep << ijk[1] << sep << ijk[2];
   return ostr.str();
}

// Return a string representation of the given metadata key, value pairs
std::string metadataAsString (const openvdb::MetaMap::ConstMetaIterator& begin, const openvdb::MetaMap::ConstMetaIterator& end, const std::string& indent = "")
{
   std::ostringstream ostr;
   char sep[2] = { 0, 0 };
   for (openvdb::MetaMap::ConstMetaIterator it = begin; it != end; ++it) {
      ostr << sep << indent << it->first;
      if (it->second) {
         const std::string value = it->second->str();
         if (!value.empty()) ostr << ": " << value;
      }
      sep[0] = '\n';
   }
   return ostr.str();
}

XSI::CStatus getGridsFromFile(VolumeToMeshData* nodeData)
{
   openvdb::io::File file(nodeData->filePath.GetAsciiString());
   try
   {
      file.open();
      nodeData->grids = file.getGrids();
      //meta = file.getMetadata();
      file.close();
   }
   catch (openvdb::Exception& e)
   {
      XSI::Application().LogMessage(XSI::CString(e.what()) + L" : " + nodeData->filePath);
      return XSI::CStatus::Fail;
   }
   return XSI::CStatus::OK;
}

using namespace XSI;
using namespace XSI::MATH;

CStatus RegisterVolumeToMesh(PluginRegistrar& in_reg);

SICALLBACK XSILoadPlugin (PluginRegistrar& in_reg)
{
   in_reg.PutAuthor(L"caron");
   in_reg.PutName(L"openvdb_softimage");
   in_reg.PutVersion(1, 0);
   in_reg.RegisterCommand(L"openvdb_print", L"openvdb_print");
   in_reg.RegisterCommand(L"openvdb_volumeToMesh", L"openvdb_volumeToMesh");
   in_reg.RegisterCommand(L"openvdb_meshToVolume", L"openvdb_meshToVolume");
   
   RegisterVolumeToMesh(in_reg);

   return CStatus::OK;
}

SICALLBACK XSIUnloadPlugin (const PluginRegistrar& in_reg)
{
   CString strPluginName;
   strPluginName = in_reg.GetName();
   Application().LogMessage(strPluginName + L" has been unloaded.", siVerboseMsg);
   return CStatus::OK;
}


CStatus RegisterVolumeToMesh(PluginRegistrar& in_reg)
{
	ICENodeDef nodeDef;
	nodeDef = Application().GetFactory().CreateICENodeDef(L"VolumeToMesh", L"Volume To Mesh");

	CStatus st;
	st = nodeDef.PutColor(110,110,110);
	st.AssertSucceeded();

   st = nodeDef.PutThreadingModel(siICENodeSingleThreading);
	st.AssertSucceeded();

	// Add input ports and groups.
	st = nodeDef.AddPortGroup(ID_G_100);
	st.AssertSucceeded();

	st = nodeDef.AddInputPort(ID_IN_filePath,ID_G_100,siICENodeDataString,siICENodeStructureSingle,siICENodeContextSingleton,L"File Path",L"filePath",L"",CValue(),CValue(),ID_UNDEF,ID_UNDEF,ID_CTXT_CNS);
	st.AssertSucceeded();
   
   st = nodeDef.AddInputPort(ID_IN_isoValue,ID_G_100,siICENodeDataFloat,siICENodeStructureSingle,siICENodeContextSingleton,L"Iso Value",L"isoValue",0.0,CValue(),CValue(),ID_UNDEF,ID_UNDEF,ID_CTXT_CNS);
	st.AssertSucceeded();

   st = nodeDef.AddInputPort(ID_IN_adaptivity,ID_G_100,siICENodeDataFloat,siICENodeStructureSingle,siICENodeContextSingleton,L"Adaptivity",L"adaptivity",0.0,CValue(),CValue(),ID_UNDEF,ID_UNDEF,ID_CTXT_CNS);
	st.AssertSucceeded();

	// Add output ports.
	st = nodeDef.AddOutputPort(ID_OUT_PointArray,siICENodeDataVector3,siICENodeStructureArray,siICENodeContextSingleton,L"Point Array",L"pointList",ID_UNDEF,ID_UNDEF,ID_CTXT_CNS);
	st.AssertSucceeded();

	st = nodeDef.AddOutputPort(ID_OUT_PolygonArray,siICENodeDataLong,siICENodeStructureArray,siICENodeContextSingleton,L"Polygon Array",L"polygonPoolList",ID_UNDEF,ID_UNDEF,ID_CTXT_CNS);
	st.AssertSucceeded();

	PluginItem nodeItem = in_reg.RegisterICENode(nodeDef);
	nodeItem.PutCategories(L"OpenVDB");

	return CStatus::OK;
}

SICALLBACK openvdb_print_Init (CRef& ref)
{
   Context ctxt(ref);
   Command oCmd;
   oCmd = ctxt.GetSource();
   oCmd.PutDescription(L"print info about an input openvdb file(.vdb)");
   oCmd.EnableReturnValue(true);

   ArgumentArray oArgs;
   oArgs = oCmd.GetArguments();
   oArgs.Add(L"file", CString());
   oArgs.Add(L"metadata", false);
   return CStatus::OK;
}

SICALLBACK openvdb_print_Execute (CRef& ref)
{
   Context ctxt(ref);
   CValueArray args = ctxt.GetAttribute(L"Arguments");
   CString filename = args[0];
   bool printMetadata = args[1];

   if (filename.IsEmpty())
   {
      Application().LogMessage(L"No file name provided!", siErrorMsg);
      ctxt.PutAttribute(L"ReturnValue", false);
      return CStatus::Fail;
   }

   openvdb::initialize();

   openvdb::GridPtrVecPtr grids;
   openvdb::MetaMap::Ptr meta;

   openvdb::io::File file(filename.GetAsciiString());
   try
   {
      file.open();
      grids = file.getGrids();
      meta = file.getMetadata();
      file.close();
   }
   catch (openvdb::Exception& e)
   {
      Application().LogMessage(CString(e.what()) + L" : " + filename);
   }
   if (!grids)
   {
      Application().LogMessage(L"No grids in %s", siErrorMsg);
      ctxt.PutAttribute(L"ReturnValue", false);
      return CStatus::Fail;
   }

   if (printMetadata)
   {
      CString metadataStr(metadataAsString(meta->beginMeta(), meta->endMeta(), "").c_str());
      Application().LogMessage(metadataStr);
   }

   for (openvdb::GridPtrVec::const_iterator it = grids->begin(); it != grids->end(); ++it)
   {
      const openvdb::GridBase::ConstPtr grid = *it;
      if (!grid) continue;

      // grid name and data type
      CString gridName(grid->getName().c_str());
      CString gridType(grid->valueType().c_str());
      
      // grid dimensions
      openvdb::CoordBBox bbox = grid->evalActiveVoxelBoundingBox();
      CString dimensions(coordAsString(bbox.extents(), "x").c_str());
      
      // grid active voxel count
      CString voxelCount(sizeAsString(grid->activeVoxelCount(), " Voxels").c_str());

      // grid size in bytes
      CString gridSizeInBytes(bytesAsString(grid->memUsage()).c_str());

      Application().LogMessage(gridName + "\t" + gridType + "\t" + dimensions + "\t" + voxelCount + "\t" + gridSizeInBytes);
      if (printMetadata)
      {
         CString bkgValue(bkgdValueAsString(grid).c_str());

         // background value
         Application().LogMessage(bkgValue);

         // custom metadata
         CString metadataStr(metadataAsString(grid->beginMeta(), grid->endMeta(), "").c_str());
         Application().LogMessage(metadataStr);
      }
   }
   
   ctxt.PutAttribute(L"ReturnValue", true);

   // Return CStatus::Fail if you want to raise a script error
   return CStatus::OK;
}

SICALLBACK openvdb_volumeToMesh_Init (CRef& ref)
{
   Context ctxt(ref);
   Command oCmd;
   oCmd = ctxt.GetSource();
   oCmd.PutDescription(L"mesh a scalar grid from an input openvdb file(.vdb)");
   oCmd.EnableReturnValue(true);

   ArgumentArray oArgs;
   oArgs = oCmd.GetArguments();
   oArgs.Add(L"file", CString());
   oArgs.Add(L"gridName", CString());
   return CStatus::OK;
}

SICALLBACK openvdb_volumeToMesh_Execute (CRef& ref)
{
   Context ctxt(ref);
   CValueArray args = ctxt.GetAttribute(L"Arguments");
   CString filename = args[0];

   if (filename.IsEmpty())
   {
      Application().LogMessage(L"No file name provided!", siErrorMsg);
      ctxt.PutAttribute(L"ReturnValue", false);
      return CStatus::Fail;
   }

   return CStatus::OK;
}

SICALLBACK openvdb_meshToVolume_Init (CRef& ref)
{
   Context ctxt(ref);
   Command oCmd;
   oCmd = ctxt.GetSource();
   oCmd.PutDescription(L"mesh a scalar grid from an input openvdb file(.vdb)");
   oCmd.EnableReturnValue(true);

   ArgumentArray oArgs;
   oArgs = oCmd.GetArguments();
   oArgs.Add(L"file", CString());
   oArgs.AddObjectArgument(L"inputMesh");
   oArgs.Add(L"voxelSize", 0.1);
   oArgs.Add(L"extWidth", 2.0);
   oArgs.Add(L"intWidth", 2.0);
   return CStatus::OK;
}

SICALLBACK openvdb_meshToVolume_Execute (CRef& ref)
{
   Context ctxt(ref);
   CValueArray args = ctxt.GetAttribute(L"Arguments");
   CString filename = args[0];
   CValue inputMesh = args[1];
   double voxelSize = args[2];
   float extWidth = args[3];
   float intWidth = args[4];
   
   // handle arguments
   if (filename.IsEmpty())
   {
      Application().LogMessage(L"No output file name provided!", siErrorMsg);
      ctxt.PutAttribute(L"ReturnValue", false);
      return CStatus::Fail;
   }

   if (inputMesh.IsEmpty())
   {
      Application().LogMessage(L"No input mesh provided!", siErrorMsg);
      ctxt.PutAttribute(L"ReturnValue", false);
      return CStatus::Fail;
   }

   openvdb::initialize();
   openvdb::math::Transform::Ptr transform = openvdb::math::Transform::createLinearTransform(voxelSize);

   X3DObject object = CRef(inputMesh);
   PolygonMesh polymesh = object.GetActivePrimitive().GetGeometry();
   CGeometryAccessor geomAccessor = polymesh.GetGeometryAccessor();

   CDoubleArray positionArray;
   geomAccessor.GetVertexPositions(positionArray);
   //Application().LogMessage(L"positionArray = "+CValue(positionArray.GetCount()).GetAsText());
   
   // fill pointList for openvdb::tools::MeshToVolume
   //openvdb::math::Transform::Ptr transform = grid->transformPtr();
   std::vector<openvdb::Vec3s> pointList;
   pointList.reserve(positionArray.GetCount()/3);
   for (LONG i=0; i<positionArray.GetCount(); i+=3)
   {
      openvdb::Vec3s pnt(positionArray[i], positionArray[i+1], positionArray[i+2]);
      pointList.push_back(transform->worldToIndex(pnt));
   }
   //Application().LogMessage(L"pointList size = "+CValue(pointList.size()).GetAsText());
   CLongArray triangleIdxArray;
   geomAccessor.GetTriangleVertexIndices(triangleIdxArray);

   // fill polygonList for openvdb::tools::MeshToVolume
   // triangles only for now
   std::vector<openvdb::Vec4I> polygonList;
   polygonList.reserve(triangleIdxArray.GetCount()/3);
   for (LONG i=0; i<triangleIdxArray.GetCount(); i+=3)
   {
      // look into PolygonPool for meshes with quads and triangles.
      openvdb::Vec4I tri(triangleIdxArray[i], triangleIdxArray[i+1], triangleIdxArray[i+2], openvdb::util::INVALID_IDX);
      polygonList.push_back(tri);
   }

   //Application().LogMessage(L"polygonList size = "+CValue(polygonList.size()).GetAsText());
   
   openvdb::tools::MeshToVolume<openvdb::FloatGrid> converter(transform);
   converter.convertToLevelSet(pointList, polygonList, extWidth, intWidth);
   //openvdb::FloatGrid::Ptr grid = converter.distGridPtr();
   CString voxelCount(sizeAsString(converter.distGridPtr()->activeVoxelCount(), " Voxels").c_str());

   Application().LogMessage(voxelCount);

   openvdb::io::File file(filename.GetAsciiString());

   openvdb::GridPtrVec grids;
   grids.push_back(converter.distGridPtr());
   
   file.write(grids);
   file.close();

   return CStatus::OK;
}
SICALLBACK VolumeToMesh_Init (ICENodeContext& ctxt)
{
   if (!openvdb::FloatGrid::isRegistered())
      Application().LogMessage(L"openvdb : initialized!");
      openvdb::initialize();

   return CStatus::OK;
}

SICALLBACK VolumeToMesh_BeginEvaluate (ICENodeContext& ctxt)
{
   CDataArrayString filePathData(ctxt, ID_IN_filePath);
   Application().LogMessage(L"BeginEvaluate");
   
   CValue userData = ctxt.GetUserData();
   VolumeToMeshData* nodeData;
   if (userData.IsEmpty())
   {
      nodeData = new VolumeToMeshData;
      nodeData->filePath = CString(filePathData[0]);
      
      if (getGridsFromFile(nodeData)==CStatus::OK)
      {
         ctxt.PutUserData((CValue::siPtrType)nodeData);
      }
      else
      {
         delete nodeData;
         return CStatus::OK;
      }   
   }
   else
   {
      nodeData = (VolumeToMeshData*)(CValue::siPtrType)userData;
      CICEPortState filePathState(ctxt, ID_IN_filePath);
      if (filePathState.IsDirty(CICEPortState::siDataDirtyState))
      {
         Application().LogMessage(L"data dirty");
         nodeData->filePath = CString(filePathData[0]);
         nodeData->isDirty = true;
      }
      else
      {
         Application().LogMessage(L"data not dirty");
         nodeData->isDirty = false;
      }
      filePathState.ClearState();
      ctxt.PutUserData((CValue::siPtrType)nodeData);
   }
   return CStatus::OK;
}

SICALLBACK VolumeToMesh_Evaluate (ICENodeContext& ctxt)
{
   Application().LogMessage(L"Evaluate");

   CValue userData = ctxt.GetUserData();
   VolumeToMeshData* nodeData;
   if (userData.IsEmpty())
   {
      return CStatus::OK;
   }   
   else
   {
      nodeData = (VolumeToMeshData*)(CValue::siPtrType)userData;
      if (nodeData->isDirty==false)
      {
         Application().LogMessage(L"skipping evaluate");
         return CStatus::OK;
      }
   }

   CDataArrayFloat iso(ctxt, ID_IN_isoValue);
   CDataArrayFloat adaptivity(ctxt, ID_IN_adaptivity);
   
   // Setup level set mesher
   openvdb::tools::VolumeToMesh mesher(iso[0], adaptivity[0]);

   openvdb::GridPtrVec::const_iterator it = nodeData->grids->begin();
   for (; it != nodeData->grids->end(); ++it)
   {
      const openvdb::GridBase::ConstPtr grid = *it;
      if (!grid) continue;
      CString gridName(grid->getName().c_str());
      CString gridType(grid->valueType().c_str());
      Application().LogMessage(nodeData->filePath + L" : " + gridName + L" : " + gridType );

      // first level set only for now
      const openvdb::GridClass gridClass = grid->getGridClass();
      if (gridClass == openvdb::GRID_LEVEL_SET)
      {
         openvdb::FloatGrid::ConstPtr levelSetGrid = openvdb::gridConstPtrCast<openvdb::FloatGrid>(*it);
         mesher(*(levelSetGrid.get()));
         break;
      }
   }

   // The current output port being evaluated...
   ULONG out_portID = ctxt.GetEvaluatedOutputPortID();
   
   switch(out_portID)
   {
      case ID_OUT_PointArray:
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
      case ID_OUT_PolygonArray:
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


//SICALLBACK VolumeToMesh_BeginEvaluate (ICENodeContext& in_ctxt)
//{
//   VolumeToMeshData
//	// Example to demonstrate how to allocate a thread safe user data buffer.
//	CValue userData = in_ctxt.GetUserData();
//
//	pPerThreadData = new std::vector<CSampleData>;
//	in_ctxt.PutUserData( (CValue::siPtrType)pPerThreadData );
//
//
//	return CStatus::OK;
//}
//
//SICALLBACK MyCustomICENode_EndEvaluate( ICENodeContext& in_ctxt )
//{
//	// Release memory allocated in BeginEvaluate
//	CValue userData = in_ctxt.GetUserData( );
//	if ( userData.IsEmpty() )
//	{
//		return CStatus::OK;
//	}
//
//	std::vector<CSampleData>* pPerThreadData = (std::vector<CSampleData>*)(CValue::siPtrType)in_ctxt.GetUserData( );
//	delete pPerThreadData;
//
//	// Clear user data"] ); 
//	in_ctxt.PutUserData( CValue() );
//	
//	return CStatus::OK;
//}
