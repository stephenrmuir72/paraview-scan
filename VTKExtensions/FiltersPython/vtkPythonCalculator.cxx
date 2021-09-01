/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPythonCalculator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPython.h" // has to be first!

#include "vtkPythonCalculator.h"

#include "vtkCellData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataArray.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkGraph.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMolecule.h"
#include "vtkObjectFactory.h"
#include "vtkPVStringFormatter.h"
#include "vtkPointData.h"
#include "vtkPythonInterpreter.h"
#include "vtkPythonUtil.h"
#include "vtkSmartPyObject.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTable.h"

#include <algorithm>
#include <map>
#include <string>

namespace
{
bool CheckAndFlushPythonErrors()
{
  if (PyErr_Occurred())
  {
    PyErr_Print();
    PyErr_Clear();
    return true;
  }
  return false;
}
}

vtkStandardNewMacro(vtkPythonCalculator);

//----------------------------------------------------------------------------
vtkPythonCalculator::vtkPythonCalculator()
{
  this->Expression = nullptr;
  this->ArrayName = nullptr;
  this->SetArrayName("result");
  this->SetExecuteMethod(vtkPythonCalculator::ExecuteScript, this);
  this->ArrayAssociation = vtkDataObject::FIELD_ASSOCIATION_POINTS;
}

//----------------------------------------------------------------------------
vtkPythonCalculator::~vtkPythonCalculator()
{
  this->SetExpression(nullptr);
  this->SetArrayName(nullptr);
}

//----------------------------------------------------------------------------
int vtkPythonCalculator::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = nullptr;
  if (inputVector[0]->GetNumberOfInformationObjects() > 0)
  {
    inInfo = inputVector[0]->GetInformationObject(0);
  }
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  double dataTime = 0;
  bool dataTimeValid = false;
  std::vector<double> timeSteps;
  int timeIndex = 0;

  // get the input and output
  if (inInfo)
  {
    if (inInfo->Has(vtkDataObject::DATA_TIME_STEP()))
    {
      dataTimeValid = true;
      dataTime = inInfo->Get(vtkDataObject::DATA_TIME_STEP());
    }

    if (inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
    {
      int numberOfTimeSteps = inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
      double* tempTimeSteps = inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());

      timeSteps.insert(timeSteps.begin(), tempTimeSteps, tempTimeSteps + numberOfTimeSteps);
    }

    timeIndex = 0;
    if (dataTimeValid && !timeSteps.empty())
    {
      for (int i = 0; i < timeSteps.size(); ++i)
      {
        if (timeSteps[i] == dataTime)
        {
          timeIndex = i;
        }
      }
    }

    vtkDataObject* objInput = inInfo->Get(vtkDataObject::DATA_OBJECT());
    if (vtkDataSet::SafeDownCast(objInput))
    {
      vtkDataSet* dsInput = vtkDataSet::SafeDownCast(objInput);
      vtkDataSet* dsOutput = vtkDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
      // First, copy the input to the output as a starting point
      if (dsInput && dsOutput && dsInput->GetDataObjectType() == dsOutput->GetDataObjectType())
      {
        if (this->CopyArrays)
        {
          dsOutput->ShallowCopy(dsInput);
        }
        else
        {
          dsOutput->CopyStructure(dsInput);
        }
      }
    }
    if (vtkGraph::SafeDownCast(objInput))
    {
      vtkGraph* graphInput = vtkGraph::SafeDownCast(objInput);
      vtkGraph* graphOutput = vtkGraph::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
      // First, copy the input to the output as a starting point
      if (graphInput && graphOutput &&
        graphInput->GetDataObjectType() == graphOutput->GetDataObjectType())
      {
        if (this->CopyArrays)
        {
          graphOutput->ShallowCopy(graphInput);
        }
        else
        {
          graphOutput->CopyStructure(graphInput);
        }
      }
    }
    if (vtkMolecule::SafeDownCast(objInput))
    {
      vtkMolecule* molInput = vtkMolecule::SafeDownCast(objInput);
      vtkMolecule* molOutput =
        vtkMolecule::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
      // First, copy the input to the output as a starting point
      if (molInput && molOutput && molInput->GetDataObjectType() == molOutput->GetDataObjectType())
      {
        if (this->CopyArrays)
        {
          molOutput->ShallowCopy(molInput);
        }
        else
        {
          molOutput->CopyStructure(molInput);
        }
      }
    }
    if (vtkTable::SafeDownCast(objInput))
    {
      vtkTable* tableInput = vtkTable::SafeDownCast(objInput);
      vtkTable* tableOutput = vtkTable::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
      // First, copy the input to the output as a starting point
      if (tableInput && tableOutput &&
        tableInput->GetDataObjectType() == tableOutput->GetDataObjectType())
      {
        if (this->CopyArrays)
        {
          tableOutput->ShallowCopy(tableInput);
        }
      }
    }
    if (vtkCompositeDataSet::SafeDownCast(objInput))
    {
      vtkCompositeDataSet* cdsInput = vtkCompositeDataSet::SafeDownCast(objInput);
      vtkCompositeDataSet* cdsOutput =
        vtkCompositeDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
      // First, copy the input to the output as a starting point
      if (cdsInput && cdsOutput && cdsInput->GetDataObjectType() == cdsOutput->GetDataObjectType())
      {
        cdsOutput->CopyStructure(cdsInput);
        vtkCompositeDataIterator* iter = cdsInput->NewIterator();
        for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
        {
          vtkDataObject* iblock = iter->GetCurrentDataObject();
          vtkDataObject* oblock = iblock->NewInstance();
          if (iblock)
          {
            if (this->CopyArrays)
            {
              oblock->ShallowCopy(iblock);
            }
            else
            {
              vtkDataSet* iblockDS = vtkDataSet::SafeDownCast(iblock);
              vtkDataSet* oblockDS = vtkDataSet::SafeDownCast(oblock);
              if (iblockDS && oblockDS)
              {
                oblockDS->CopyStructure(iblockDS);
              }
            }
          }
          cdsOutput->SetDataSet(iter, oblock);
          oblock->Delete();
        }
        iter->Delete();
      }
    }
  }

  vtkDebugMacro(<< "Executing programmable filter");

  // define annotate scope
  PV_STRING_FORMATTER_NAMED_SCOPE(
    "CALCULATOR", fmt::arg("timevalue", dataTime), fmt::arg("timeindex", timeIndex));

  this->SetExpression(vtkPVStringFormatter::Format(this->Expression).c_str());

  // Now invoke the procedure, if specified.
  if (this->ExecuteMethod != nullptr)
  {
    (*this->ExecuteMethod)(this->ExecuteMethodArg);
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkPythonCalculator::RequestDataObject(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Output type is same as input
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    return 0;
  }
  vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  if (input)
  {
    // for each output
    for (int i = 0; i < this->GetNumberOfOutputPorts(); ++i)
    {
      vtkInformation* info = outputVector->GetInformationObject(i);
      vtkDataObject* output = info->Get(vtkDataObject::DATA_OBJECT());

      if (!output || !output->IsA(input->GetClassName()))
      {
        vtkDataObject* newOutput = input->NewInstance();
        info->Set(vtkDataObject::DATA_OBJECT(), newOutput);
        newOutput->Delete();
        this->GetOutputPortInformation(0)->Set(
          vtkDataObject::DATA_EXTENT_TYPE(), newOutput->GetExtentType());
      }
    }
    return 1;
  }
  return 0;
}

//----------------------------------------------------------------------------
void vtkPythonCalculator::ExecuteScript(void* arg)
{
  vtkPythonCalculator* self = static_cast<vtkPythonCalculator*>(arg);
  if (self)
  {
    self->Exec(self->GetExpression());
  }
}

//----------------------------------------------------------------------------
void vtkPythonCalculator::Exec(const char* expression)
{
  // Do not execute if expression is nullptr or empty.
  if (!expression || expression[0] == '\0')
  {
    return;
  }

  // Replace tabs with two spaces
  std::string orgscript;
  size_t len = strlen(expression);
  for (size_t i = 0; i < len; i++)
  {
    if (expression[i] == '\t')
    {
      orgscript += "  ";
    }
    else
    {
      orgscript.push_back(expression[i]);
    }
  }

  // ensure Python is initialized (safe to call many times)
  vtkPythonInterpreter::Initialize();

  vtkPythonScopeGilEnsurer gilEnsurer;
  vtkSmartPyObject modCalculator(PyImport_ImportModule("paraview.detail.calculator"));
  CheckAndFlushPythonErrors();
  if (!modCalculator)
  {
    vtkErrorMacro("Failed to import `paraview.detail.calculator` module.");
    return;
  }

  vtkSmartPyObject self(vtkPythonUtil::GetObjectFromPointer(this));
  vtkSmartPyObject fname(PyString_FromString("execute"));
  vtkSmartPyObject pyexpression(PyString_FromString(orgscript.c_str()));

  // call `paraview.detail.calculator.execute(self)`
  vtkSmartPyObject retVal(PyObject_CallMethodObjArgs(
    modCalculator, fname.GetPointer(), self.GetPointer(), pyexpression.GetPointer(), nullptr));

  CheckAndFlushPythonErrors();

  // at some point we may want to check retval
  (void)retVal;
}

//----------------------------------------------------------------------------
int vtkPythonCalculator::FillOutputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  // now add our info
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
  return 1;
}

//----------------------------------------------------------------------------
int vtkPythonCalculator::FillInputPortInformation(int port, vtkInformation* info)
{
  if (!this->Superclass::FillInputPortInformation(port, info))
  {
    return 0;
  }
  if (port == 0)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
    info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }
  return 1;
}

//----------------------------------------------------------------------------
void vtkPythonCalculator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
