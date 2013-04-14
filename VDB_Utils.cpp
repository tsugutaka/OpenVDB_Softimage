// VDB_Utils.cpp

#include "VDB_Utils.h"

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
std::string metadataAsString (const openvdb::MetaMap::ConstMetaIterator& begin, const openvdb::MetaMap::ConstMetaIterator& end, const std::string& indent)
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