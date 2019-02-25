/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSINamedVectorPropertyTemplate.h"
#include "vtkSIVectorPropertyTemplate.txx"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSIProxy.h"
#include "vtkSMMessage.h"

#include <assert.h>
#include <sstream>
#include <vector>

//----------------------------------------------------------------------------
template <class T, class force_idtype>
vtkSINamedVectorPropertyTemplate<T, force_idtype>::vtkSINamedVectorPropertyTemplate()
{
}

//----------------------------------------------------------------------------
template <class T, class force_idtype>
vtkSINamedVectorPropertyTemplate<T, force_idtype>::~vtkSINamedVectorPropertyTemplate()
{
}

//---------------------------------------------------------------------------
template <class T, class force_idtype>
bool vtkSINamedVectorPropertyTemplate<T, force_idtype>::Pull(vtkSMMessage* message)
{
  if (!this->InformationOnly)
  {
    return this->Superclass::Pull(message);
  }

  if (!this->GetCommand())
  {
    // I would say that we should return false since an InformationOnly property
    // as no meaning if no command is set, but for some legacy reason we just
    // skip the processing if no command is provided.
    return true;
  }

  // Invoke property's method on the root node of the server
  vtkClientServerStream str;
  str << vtkClientServerStream::Invoke << this->GetVTKObject() << this->GetCommand()
      << this->InitialString << vtkClientServerStream::End;

  this->ProcessMessage(str);

  // Get the result
  const vtkClientServerStream& res = this->GetLastResult();

  int numMsgs = res.GetNumberOfMessages();
  if (numMsgs < 1)
  {
    return true;
  }

  int numArgs = res.GetNumberOfArguments(0);
  if (numArgs < 1)
  {
    return true;
  }

  std::vector<T> values;
  AppendValues(values, res, force_idtype());

  // now add 'values' to the message.
  ProxyState_Property* prop = message->AddExtension(ProxyState::property);
  prop->set_name(this->GetXMLName());

  AppendValues(*prop->mutable_value(), values, force_idtype());
  return true;
}

//---------------------------------------------------------------------------
template <class T, class force_idtype>
bool vtkSINamedVectorPropertyTemplate<T, force_idtype>::Push(T* values, int number_of_elements)
{
  if (this->InformationOnly || !this->Command)
  {
    return true;
  }

  vtkClientServerStream stream;
  vtkObjectBase* object = this->GetVTKObject();

  if (this->CleanCommand)
  {
    stream << vtkClientServerStream::Invoke << object << this->CleanCommand << this->InitialString
           << vtkClientServerStream::End;
  }

  if (this->SetNumberCommand)
  {
    stream << vtkClientServerStream::Invoke << object << this->SetNumberCommand
           << this->InitialString << number_of_elements / this->NumberOfElementsPerCommand
           << vtkClientServerStream::End;
  }

  if (!this->Repeatable && number_of_elements > 0)
  {
    stream << vtkClientServerStream::Invoke << object << this->Command << this->InitialString;
    if (this->ArgumentIsArray)
    {
      stream << vtkClientServerStream::InsertArray(values, number_of_elements);
    }
    else
    {
      for (int i = 0; i < number_of_elements; i++)
      {
        stream << values[i];
      }
    }
    stream << vtkClientServerStream::End;
  }
  else if (this->Repeatable)
  {
    int numCommands = number_of_elements / this->NumberOfElementsPerCommand;
    for (int i = 0; i < numCommands; i++)
    {
      stream << vtkClientServerStream::Invoke << object << this->Command << this->InitialString;
      if (this->UseIndex)
      {
        stream << i;
      }
      if (this->ArgumentIsArray)
      {
        stream << vtkClientServerStream::InsertArray(
          &(values[i * this->NumberOfElementsPerCommand]), this->NumberOfElementsPerCommand);
      }
      else
      {
        for (int j = 0; j < this->NumberOfElementsPerCommand; j++)
        {
          stream << values[i * this->NumberOfElementsPerCommand + j];
        }
      }
      stream << vtkClientServerStream::End;
    }
  }
  return this->ProcessMessage(stream);
}

//---------------------------------------------------------------------------
template <class T, class force_idtype>
bool vtkSINamedVectorPropertyTemplate<T, force_idtype>::ReadXMLAttributes(
  vtkSIProxy* proxy, vtkPVXMLElement* element)
{
  this->Repeatable = true;
  this->UseIndex = true;
  if (!this->Superclass::ReadXMLAttributes(proxy, element))
  {
    return false;
  }

  const char* propertyName = element->GetAttribute("property_name");
  if (!propertyName)
  {
    vtkErrorMacro("A named vector property must have a \"property_name\" "
                  "attribute set.");
    return false;
  }

  this->SetInitialString(propertyName);
  return true;
}

//----------------------------------------------------------------------------
template <class T, class force_idtype>
void vtkSINamedVectorPropertyTemplate<T, force_idtype>::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
