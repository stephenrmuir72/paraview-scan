/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxySelectionModel.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMProxySelectionModel.h"

#include "vtkBoundingBox.h"
#include "vtkCollection.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPVDataInformation.h"
#include "vtkSMOutputPort.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyLocator.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMTrace.h"
#include "vtkSmartPointer.h"

#include <vtkNew.h>

#include <algorithm>

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMProxySelectionModel);

//-----------------------------------------------------------------------------
class vtkSMProxySelectionModel::vtkInternal
{
public:
  vtkSMProxySelectionModel* Owner;
  bool Initialized;

  vtkInternal(vtkSMProxySelectionModel* owner)
  {
    this->Owner = owner;
    this->Initialized = false;
  }
};

//-----------------------------------------------------------------------------
vtkSMProxySelectionModel::vtkSMProxySelectionModel()
{
  this->Internal = new vtkSMProxySelectionModel::vtkInternal(this);
}

//-----------------------------------------------------------------------------
vtkSMProxySelectionModel::~vtkSMProxySelectionModel()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
vtkSMProxy* vtkSMProxySelectionModel::GetCurrentProxy()
{
  return this->Current;
}

//-----------------------------------------------------------------------------
void vtkSMProxySelectionModel::SetCurrentProxy(vtkSMProxy* proxy, int command)
{
  if (this->Current != proxy)
  {
    SM_SCOPED_TRACE(SetCurrentProxy)
      .arg("selmodel", this)
      .arg("proxy", proxy)
      .arg("command", command);
    this->Current = proxy;
    this->Select(proxy, command);
    this->InvokeCurrentChanged(proxy);
  }
}

//-----------------------------------------------------------------------------
bool vtkSMProxySelectionModel::IsSelected(vtkSMProxy* proxy)
{
  return std::find(this->Selection.begin(), this->Selection.end(), proxy) != this->Selection.end();
}

//-----------------------------------------------------------------------------
unsigned int vtkSMProxySelectionModel::GetNumberOfSelectedProxies()
{
  return static_cast<unsigned int>(this->Selection.size());
}

//-----------------------------------------------------------------------------
vtkSMProxy* vtkSMProxySelectionModel::GetSelectedProxy(unsigned int idx)
{
  if (idx < this->GetNumberOfSelectedProxies())
  {
    SelectionType::iterator iter = this->Selection.begin();
    for (unsigned int cc = 0; cc < idx; ++cc, ++iter)
    {
    }
    return vtkSMProxy::SafeDownCast(iter->GetPointer());
  }

  return nullptr;
}

//-----------------------------------------------------------------------------
void vtkSMProxySelectionModel::Select(vtkSMProxy* proxy, int command)
{
  SelectionType selection;
  if (proxy)
  {
    selection.push_back(proxy);
  }
  this->Select(selection, command);
}

//-----------------------------------------------------------------------------
void vtkSMProxySelectionModel::Select(
  const vtkSMProxySelectionModel::SelectionType& proxies, int command)
{
  if (command == vtkSMProxySelectionModel::NO_UPDATE)
  {
    return;
  }

  SelectionType new_selection;

  if (command & vtkSMProxySelectionModel::CLEAR)
  {
    // everything from old-selection needs to be removed.
  }
  else
  {
    // start with existing selection.
    new_selection = this->Selection;
  }

  for (SelectionType::const_iterator iter = proxies.begin(); iter != proxies.end(); ++iter)
  {
    vtkSMProxy* proxy = iter->GetPointer();
    if (proxy && (command & vtkSMProxySelectionModel::SELECT) != 0)
    {
      if (std::find(new_selection.begin(), new_selection.end(), proxy) == new_selection.end())
      {
        new_selection.push_back(proxy);
      }
    }
    if (proxy && (command & vtkSMProxySelectionModel::DESELECT) != 0)
    {
      new_selection.remove(proxy);
    }
  }

  bool changed = (this->Selection != new_selection);
  if (changed)
  {
    this->Selection = new_selection;
    this->InvokeSelectionChanged();
  }
}

//-----------------------------------------------------------------------------
void vtkSMProxySelectionModel::InvokeCurrentChanged(vtkSMProxy* proxy)
{
  this->InvokeEvent(vtkCommand::CurrentChangedEvent, proxy);
}

//-----------------------------------------------------------------------------
void vtkSMProxySelectionModel::InvokeSelectionChanged()
{
  this->InvokeEvent(vtkCommand::SelectionChangedEvent);
}

//-----------------------------------------------------------------------------
bool vtkSMProxySelectionModel::GetSelectionDataBounds(double bounds[6])
{
  vtkBoundingBox bbox;
  for (SelectionType::iterator iter = this->Selection.begin(); iter != this->Selection.end();
       ++iter)
  {
    vtkSMProxy* proxy = iter->GetPointer();
    vtkSMSourceProxy* source = vtkSMSourceProxy::SafeDownCast(proxy);
    vtkSMOutputPort* opPort = vtkSMOutputPort::SafeDownCast(proxy);
    if (source)
    {
      for (unsigned int kk = 0; kk < source->GetNumberOfOutputPorts(); kk++)
      {
        bbox.AddBounds(source->GetDataInformation(kk)->GetBounds());
      }
    }
    else if (opPort)
    {
      bbox.AddBounds(opPort->GetDataInformation()->GetBounds());
    }
  }
  if (bbox.IsValid())
  {
    bbox.GetBounds(bounds);
    return true;
  }

  return false;
}

//-----------------------------------------------------------------------------
void vtkSMProxySelectionModel::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Current Proxy: " << (this->Current ? this->Current->GetGlobalID() : 0) << endl;
  os << indent << "Selected Proxies: ";
  for (SelectionType::iterator iter = this->Selection.begin(); iter != this->Selection.end();
       iter++)
  {
    os << iter->GetPointer()->GetGlobalID() << " ";
  }
  os << endl;
}
