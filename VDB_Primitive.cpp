// OpenVDB_Softimage
// VDB_Primitive.cpp
// class to hold VDB data and passed through the ICE graph

#include <xsi_application.h>

#include "VDB_Primitive.h"

VDB_Primitive::VDB_Primitive()
{
}

VDB_Primitive::~VDB_Primitive()
{
}

void VDB_Primitive::SetGrid(const openvdb::GridBase& grid)
{
   if (m_grid.get() == &grid)
   {
      XSI::Application().LogMessage(L"[VDB_Primitive] grids are equal?");
      return;
   }
   // shallow copy grid, according to openvdb_houdini
   m_grid = grid.copyGrid();
}

const XSI::CString VDB_Primitive::GetTypeName()
{
   if (!m_grid) return XSI::CString();
   return XSI::CString(m_grid->valueType().c_str());
}

openvdb::GridBase::ConstPtr VDB_Primitive::GetConstGridPtr() const
{
   return m_grid;
}

openvdb::GridBase::Ptr VDB_Primitive::GetGridPtr()
{
   return m_grid;
}
