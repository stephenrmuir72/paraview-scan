/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPacket.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPacket.h"

int TestPacket(int /*argc*/, char* /*argv*/[])
{
  vtkPacket packet0;
  packet0.Print(std::cout, 4);

  vtkPacket packet1(vtkNJson{ { "data", 12 }, { "value", 13.0 } });
  packet1.Print();

  vtkPacket packet2(vtkNJson({ { "data", 12 }, { "value", 13.0 } }));
  packet2.Print();

  vtkPacket packet3({ { "data", 12 }, { "value", 13.0 } });
  packet3.Print();

  vtkPacket packet4(vtkNJson({ "data", 12 }));
  packet4.Print();

  vtkPacket packet5(vtkNJson{ { "data", 12 } });
  packet5.Print();

  return EXIT_SUCCESS;
}
