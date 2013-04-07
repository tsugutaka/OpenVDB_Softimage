// openvdb_softimage
#include <xsi_application.h>
#include <xsi_context.h>
#include <xsi_pluginregistrar.h>
#include <xsi_status.h>
#include <xsi_argument.h>
#include <xsi_command.h>
#include <xsi_x3dobject.h>
#include <xsi_primitive.h>
#include <xsi_polygonmesh.h>
#include <xsi_geometryaccessor.h>
#include <xsi_doublearray.h>

#include <iostream>
#include <sstream>
#include <string>
#include <iterator>
#include <vector>
#include <openvdb/openvdb.h>
#include <openvdb/tools/MeshToVolume.h>

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

using namespace XSI; 

SICALLBACK XSILoadPlugin (PluginRegistrar& in_reg)
{
   in_reg.PutAuthor(L"caron");
   in_reg.PutName(L"openvdb_softimage");
   in_reg.PutVersion(1, 0);
   in_reg.RegisterCommand(L"openvdb_print", L"openvdb_print");
   in_reg.RegisterCommand(L"openvdb_volumeToMesh", L"openvdb_volumeToMesh");
   in_reg.RegisterCommand(L"openvdb_meshToVolume", L"openvdb_meshToVolume");
   //RegistrationInsertionPoint - do not remove this line

   return CStatus::OK;
}

SICALLBACK XSIUnloadPlugin (const PluginRegistrar& in_reg)
{
   CString strPluginName;
   strPluginName = in_reg.GetName();
   Application().LogMessage(strPluginName + L" has been unloaded.", siVerboseMsg);
   return CStatus::OK;
}

SICALLBACK openvdb_print_Init (CRef& in_ctxt)
{
   Context ctxt(in_ctxt);
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

SICALLBACK openvdb_print_Execute (CRef& in_ctxt)
{
   Context ctxt(in_ctxt);
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

SICALLBACK openvdb_volumeToMesh_Init (CRef& in_ctxt)
{
   Context ctxt(in_ctxt);
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

SICALLBACK openvdb_volumeToMesh_Execute (CRef& in_ctxt)
{
   Context ctxt(in_ctxt);
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

SICALLBACK openvdb_meshToVolume_Init (CRef& in_ctxt)
{
   Context ctxt(in_ctxt);
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

SICALLBACK openvdb_meshToVolume_Execute (CRef& in_ctxt)
{
   Context ctxt(in_ctxt);
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
