/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMProxyDefinitionsIterator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkSMProxyDefinitionsIterator_h
#define vtkSMProxyDefinitionsIterator_h

#include "vtkServerManagerCoreModule.h" // for exports
#include <vtkSmartPointer.h>            // for vtkSmartPointer

#include <map>    // for std::map::iterator
#include <string> // for std::string

class vtkPVXMLElement;

class VTKSERVERMANAGERCORE_EXPORT vtkSMProxyDefinitionsIterator
{
public:
  using MapType = std::map<std::string, std::map<std::string, vtkSmartPointer<vtkPVXMLElement>>>;
  using Level0InternalIteratorType = MapType::iterator;
  using Level1InternalIteratorType = MapType::mapped_type::iterator;

  /**
   * Construct a new iterator. The range of the level0 iterator will be
   * constrtained by \p level0IteratorCurrent and \p level0IteratorEnd while
   * its value will be set equal to level1Iterator.
   */
  vtkSMProxyDefinitionsIterator(Level0InternalIteratorType level0IteratorCurrent,
    Level0InternalIteratorType level0IteratorEnd, Level1InternalIteratorType level1Iterator);

  bool operator==(const vtkSMProxyDefinitionsIterator& other) const;
  bool operator!=(const vtkSMProxyDefinitionsIterator& other) const;

  vtkSMProxyDefinitionsIterator operator->();
  vtkSMProxyDefinitionsIterator operator*();

  vtkSMProxyDefinitionsIterator& operator++();
  vtkSMProxyDefinitionsIterator operator++(int);

  std::string GetGroup() const;
  std::string GetName() const;

  vtkSmartPointer<vtkPVXMLElement> GetDefinition() const;

private:
  Level0InternalIteratorType InternalLevel0IteratorCurrent;
  Level0InternalIteratorType InternalLevel0IteratorEnd;
  Level1InternalIteratorType InternalLevel1Iterator;
};
#endif
