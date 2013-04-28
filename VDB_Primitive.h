// OpenVDB_Softimage
// VDB_Utils.h
// class to hold VDB data and passed through the ICE graph

#ifndef VDB_PRIMITIVE_H
#define VDB_PRIMITIVE_H

#include <xsi_string.h>

#include <openvdb/openvdb.h>

class VDB_Primitive
{
public:
   VDB_Primitive();
   ~VDB_Primitive();

   void SetGrid(const openvdb::GridBase& grid);
   const XSI::CString GetTypeName();
   
   openvdb::GridBase::ConstPtr GetConstGridPtr() const;

private:
   openvdb::GridBase::Ptr  m_grid;
};

#endif
