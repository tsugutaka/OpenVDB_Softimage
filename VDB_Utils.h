// OpenVDB_Softimage
// VDB_Utils.h
// utilties use to help logging VDB related data

#ifndef VDB_UTILS_H
#define VDB_UTILS_H

#include <iostream>
#include <sstream>
#include <string>
#include <iterator>
#include <vector>

#include <openvdb/openvdb.h>

std::string bkgdValueAsString (const openvdb::GridBase::ConstPtr& grid);

std::string bytesAsString (openvdb::Index64 n);

std::string sizeAsString (openvdb::Index64 n, const std::string& units);

std::string coordAsString (const openvdb::Coord ijk, const std::string& sep);

// Return a string representation of the given metadata key, value pairs
std::string metadataAsString (const openvdb::MetaMap::ConstMetaIterator& begin, const openvdb::MetaMap::ConstMetaIterator& end, const std::string& indent = "");

#endif
