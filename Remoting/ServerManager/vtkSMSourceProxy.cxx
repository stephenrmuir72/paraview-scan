/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSourceProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMSourceProxy.h"

#include "vtkClientSession.h"
#include "vtkCommand.h"
#include "vtkDataSetAttributes.h"
#include "vtkObjectFactory.h"
#include "vtkPVDataInformation.h"
#include "vtkPVLogger.h"
#include "vtkPVXMLElement.h"
#include "vtkRemoteObjectProvider.h"
#include "vtkRemotingCoreUtilities.h"
#include "vtkSMDocumentation.h"
#include "vtkSMInputProperty.h"
#include "vtkSMOutputPort.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSmartPointer.h"

#include <cassert>
#include <sstream>
#include <string>
#include <vector>

#define OUTPUT_PORTNAME_PREFIX "Output-"
#define MAX_NUMBER_OF_PORTS 10

//---------------------------------------------------------------------------
class vtkSelectionForwarderCommand : public vtkCommand
{
public:
  vtkTypeMacro(vtkSelectionForwarderCommand, vtkCommand);

  static vtkSelectionForwarderCommand* New() { return new vtkSelectionForwarderCommand; }

  vtkSelectionForwarderCommand()
    : PortIndex(0)
    , Proxy(nullptr)
  {
  }

  void Execute(vtkObject*, unsigned long, void*) override
  {
    if (this->Proxy)
    {
      this->Proxy->InvokeEvent(vtkCommand::SelectionChangedEvent, &this->PortIndex);
    }
  }

  int PortIndex;
  vtkSMSourceProxy* Proxy;
};

//---------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMSourceProxy);

// This struct will keep all information associated with the output port.
struct vtkSMSourceProxyOutputPort
{
  vtkSmartPointer<vtkSMOutputPort> Port;
  vtkSmartPointer<vtkSMDocumentation> Documentation;
  std::string Name;
};

struct vtkSMSourceProxyInternals
{
  const std::thread::id OwnerTID{ std::this_thread::get_id() };
  typedef std::vector<vtkSMSourceProxyOutputPort> VectorOfPorts;
  VectorOfPorts OutputPorts;
  std::vector<vtkSmartPointer<vtkSMSourceProxy>> SelectionProxies;
  std::vector<unsigned long> SelectionObservers;

  vtkTypeUInt32 UpdatePipelineTag{ 0 };
  rxcpp::subjects::behavior<std::pair<vtkTypeUInt32, bool>> UpdatePipelineSubject{ std::make_pair(
    0, false) };

  // Resizes output ports and ensures that Name for each port is initialized to
  // the default.
  void ResizeOutputPorts(unsigned int newsize)
  {
    this->OutputPorts.resize(newsize);

    VectorOfPorts::iterator it = this->OutputPorts.begin();
    for (unsigned int idx = 0; it != this->OutputPorts.end(); it++, idx++)
    {
      if (it->Name.empty())
      {
        std::ostringstream nameStream;
        nameStream << OUTPUT_PORTNAME_PREFIX << idx;
        it->Name = nameStream.str();
      }
    }
  }

  void EnsureOutputPortsSize(unsigned int size)
  {
    if (this->OutputPorts.size() < size)
    {
      this->ResizeOutputPorts(size);
    }
  }

  ~vtkSMSourceProxyInternals() { this->UpdatePipelineSubject.get_subscriber().on_completed(); }
};

//---------------------------------------------------------------------------
vtkSMSourceProxy::vtkSMSourceProxy()
{
  this->PInternals = new vtkSMSourceProxyInternals;
  this->DisableSelectionProxies = false;
  this->SelectionProxiesCreated = false;
  this->NumberOfAlgorithmOutputPorts = 0;
  this->NumberOfAlgorithmRequiredInputPorts = 0;
  this->ProcessSupport = vtkSMSourceProxy::BOTH;
  this->MPIRequired = false;
}

//---------------------------------------------------------------------------
vtkSMSourceProxy::~vtkSMSourceProxy()
{
  delete this->PInternals;
}

//---------------------------------------------------------------------------
unsigned int vtkSMSourceProxy::GetNumberOfOutputPorts() const
{
  return static_cast<int>(this->PInternals->OutputPorts.size());
}

//---------------------------------------------------------------------------
vtkSMOutputPort* vtkSMSourceProxy::GetOutputPort(unsigned int idx)
{
  return idx == VTK_UNSIGNED_INT_MAX ? nullptr
                                     : this->PInternals->OutputPorts[idx].Port.GetPointer();
}

//---------------------------------------------------------------------------
vtkSMOutputPort* vtkSMSourceProxy::GetOutputPort(const char* portname)
{
  return this->GetOutputPort(this->GetOutputPortIndex(portname));
}

//---------------------------------------------------------------------------
unsigned int vtkSMSourceProxy::GetOutputPortIndex(const char* portname)
{
  // Since there are not really going to be hundreds of output ports, the is not
  // going to be any noticeable difference in accessing output ports by index or
  // by name.
  vtkSMSourceProxyInternals::VectorOfPorts::iterator it = this->PInternals->OutputPorts.begin();
  for (unsigned int idx = 0; it != this->PInternals->OutputPorts.end(); it++, idx++)
  {
    if (it->Name == portname)
    {
      return idx;
    }
  }

  return VTK_UNSIGNED_INT_MAX;
}

//---------------------------------------------------------------------------
const char* vtkSMSourceProxy::GetOutputPortName(unsigned int index)
{
  if (index >= this->PInternals->OutputPorts.size())
  {
    return nullptr;
  }

  return this->PInternals->OutputPorts[index].Name.c_str();
}

//---------------------------------------------------------------------------
vtkSMDocumentation* vtkSMSourceProxy::GetOutputPortDocumentation(unsigned int index)
{
  if (index >= this->PInternals->OutputPorts.size())
  {
    return nullptr;
  }

  return this->PInternals->OutputPorts[index].Documentation;
}

//---------------------------------------------------------------------------
vtkSMDocumentation* vtkSMSourceProxy::GetOutputPortDocumentation(const char* portname)
{
  return this->GetOutputPortDocumentation(this->GetOutputPortIndex(portname));
}

//---------------------------------------------------------------------------
int vtkSMSourceProxy::ReadXMLAttributes(vtkSMSessionProxyManager* pm, vtkPVXMLElement* element)
{
  int numPorts = 0;
  if (element->GetScalarAttribute("number_of_output_ports", &numPorts))
  {
    this->NumberOfAlgorithmOutputPorts = numPorts;
  }

  if (const char* mp = element->GetAttribute("multiprocess_support"))
  {
    // For ParaView we don't mark any sources as multiple_processes only
    // since for things like tracing or Catalyst script generation we
    // want to make them always available. Other tools that are built on
    // top of ParaView though may want to be able to set multiple_processes
    // only.
    if (strcmp(mp, "multiple_processes") == 0)
    {
      this->ProcessSupport = vtkSMSourceProxy::MULTIPLE_PROCESSES;
    }
    else if (strcmp(mp, "single_process") == 0)
    {
      this->ProcessSupport = vtkSMSourceProxy::SINGLE_PROCESS;
    }
    else
    {
      this->ProcessSupport = vtkSMSourceProxy::BOTH;
    }
  }

  if (const char* mpi = element->GetAttribute("mpi_required"))
  {
    if (strcmp(mpi, "true") == 0 || strcmp(mpi, "1") == 0)
    {
      this->MPIRequired = true;
    }
    else
    {
      this->MPIRequired = false;
    }
  }

  int port_count = 0;
  unsigned int numElems = element->GetNumberOfNestedElements();
  for (unsigned int cc = 0; cc < numElems; cc++)
  {
    vtkPVXMLElement* child = element->GetNestedElement(cc);
    if (child && child->GetName() && strcmp(child->GetName(), "OutputPort") == 0)
    {
      // Load output port configuration.
      int index = 0;
      if (!child->GetScalarAttribute("index", &index))
      {
        index = port_count;
      }
      const char* portname = child->GetAttribute("name");
      if (!portname)
      {
        vtkErrorMacro("Missing OutputPort attribute 'name'.");
        return 0;
      }
      port_count++;
      this->PInternals->EnsureOutputPortsSize(index + 1);
      this->PInternals->OutputPorts[index].Name = portname;

      // Load output port documentation.
      for (unsigned int kk = 0; kk < child->GetNumberOfNestedElements(); ++kk)
      {
        vtkPVXMLElement* subElem = child->GetNestedElement(kk);
        if (strcmp(subElem->GetName(), "Documentation") == 0)
        {
          this->Documentation->SetDocumentationElement(subElem);
          vtkSMDocumentation* doc = vtkSMDocumentation::New();
          doc->SetDocumentationElement(subElem);
          this->PInternals->OutputPorts[index].Documentation = doc;
          doc->Delete();
        }
      }
    }
  }

  this->CreateOutputPorts();
  return this->Superclass::ReadXMLAttributes(pm, element);
}

//---------------------------------------------------------------------------
// Call Update() on all sources with given time
rxcpp::observable<bool> vtkSMSourceProxy::UpdatePipeline(double time)
{
  auto& internals = (*this->PInternals);
  vtkRemotingCoreUtilities::EnsureThread(internals.OwnerTID);

  const vtkTypeUInt32 tag = ++internals.UpdatePipelineTag;
  auto* session = this->GetSession();

  auto subscriber = internals.UpdatePipelineSubject.get_subscriber();
  auto packet = vtkRemoteObjectProvider::UpdatePipeline(this->GetGlobalID(), time);
  auto eventual = session->SendRequest(this->GetLocation(), std::move(packet));
  eventual.GetObservable()
    .observe_on(session->GetCoordination())
    .subscribe([this, subscriber, tag](const vtkPacket& reply) {
      vtkLogF(INFO, "handle UpdatePipeline response");

      const auto& json = reply.GetJSON();
      const bool status = json.at("status").get<bool>();
      this->GetSessionProxyManager()->ProcessPiggybackedMessages(json);
      subscriber.on_next(std::make_pair(tag, status));
    });

  auto coordination = session->GetCoordination();
  return internals.UpdatePipelineSubject.get_observable()
    .filter([tag](const std::pair<vtkTypeUInt32, bool>& pair) { return tag == pair.first; })
    .take(1)
    .map([](const std::pair<vtkTypeUInt32, bool>& pair) { return pair.second; });
}

//---------------------------------------------------------------------------
unsigned int vtkSMSourceProxy::GetNumberOfAlgorithmRequiredInputPorts()
{
  // FIXME:
  if (this->NumberOfAlgorithmRequiredInputPorts != VTK_UNSIGNED_INT_MAX)
  {
    // avoid unnecessary information gathers.
    return this->NumberOfAlgorithmRequiredInputPorts;
  }

  return 0;
}

//---------------------------------------------------------------------------
void vtkSMSourceProxy::CreateOutputPorts()
{
  // We simply set/replace the Port pointers in the
  // this->PInternals->OutputPorts. This ensures that the port names,
  // port documentation is preserved.

  // Create one output port proxy for each output of the filter
  int numOutputs = this->GetNumberOfAlgorithmOutputPorts();

  // Ensure that output ports size matches the number of output ports provided
  // by the algorithm.
  this->PInternals->ResizeOutputPorts(numOutputs);
  for (int j = 0; j < numOutputs; j++)
  {
    vtkSMOutputPort* opPort = vtkSMOutputPort::New();
    opPort->ProxyManager = this->ProxyManager;
    opPort->SetPortIndex(j);
    opPort->SetSourceProxy(this);
    this->PInternals->OutputPorts[j].Port = opPort;
    opPort->Delete();
  }
}

//----------------------------------------------------------------------------
void vtkSMSourceProxy::SetOutputPort(
  unsigned int index, const char* name, vtkSMOutputPort* port, vtkSMDocumentation* doc)
{
  this->PInternals->EnsureOutputPortsSize(index + 1);
  this->PInternals->OutputPorts[index].Name = name;
  this->PInternals->OutputPorts[index].Port = port;
  this->PInternals->OutputPorts[index].Documentation = doc;
  if (port && port->GetSourceProxy() == nullptr)
  {
    port->SetSourceProxy(this);
  }
}

//----------------------------------------------------------------------------
void vtkSMSourceProxy::RemoveAllOutputPorts()
{
  this->PInternals->OutputPorts.clear();
}

//----------------------------------------------------------------------------
void vtkSMSourceProxy::SetExtractSelectionProxy(unsigned int index, vtkSMSourceProxy* proxy)
{
  if (this->PInternals->SelectionProxies.size() <= index + 1)
  {
    this->PInternals->SelectionProxies.resize(index + 1);
    this->PInternals->SelectionObservers.resize(index + 1);
  }

  this->PInternals->SelectionProxies[index] = proxy;

  // Set up observer on the "Selection" property's ModifiedEvent and invoke
  // a SelectionChangedEvent from this proxy.
  vtkNew<vtkSelectionForwarderCommand> selectionCommand;
  selectionCommand->PortIndex = index;
  selectionCommand->Proxy = this;
  this->PInternals->SelectionObservers[index] =
    proxy->AddObserver(vtkCommand::ModifiedEvent, selectionCommand);
}

//----------------------------------------------------------------------------
void vtkSMSourceProxy::RemoveAllExtractSelectionProxies()
{
  for (size_t i = 0; i < this->PInternals->SelectionObservers.size(); ++i)
  {
    this->PInternals->SelectionProxies[i]->RemoveObserver(this->PInternals->SelectionObservers[i]);
  }
  this->PInternals->SelectionProxies.clear();
  this->PInternals->SelectionObservers.clear();
}

//----------------------------------------------------------------------------
vtkPVDataInformation* vtkSMSourceProxy::GetDataInformation(unsigned int idx)
{
  if (idx >= this->GetNumberOfOutputPorts())
  {
    return nullptr;
  }

  return this->GetOutputPort(idx)->GetDataInformation();
}

//----------------------------------------------------------------------------
vtkPVDataInformation* vtkSMSourceProxy::GetRankDataInformation(unsigned int idx, int rank)
{
  if (idx >= this->GetNumberOfOutputPorts())
  {
    return nullptr;
  }

  return this->GetOutputPort(idx)->GetRankDataInformation(rank);
}

//----------------------------------------------------------------------------
vtkPVDataInformation* vtkSMSourceProxy::GetSubsetDataInformation(
  unsigned int idx, const char* selector, const char* assemblyName)
{
  if (idx >= this->GetNumberOfOutputPorts())
  {
    return nullptr;
  }

  return this->GetOutputPort(idx)->GetSubsetDataInformation(selector, assemblyName);
}

//----------------------------------------------------------------------------
vtkPVDataInformation* vtkSMSourceProxy::GetSubsetDataInformation(
  unsigned int idx, unsigned int compositeIndex)
{
  if (idx >= this->GetNumberOfOutputPorts())
  {
    return nullptr;
  }

  return this->GetOutputPort(idx)->GetSubsetDataInformation(compositeIndex);
}

//----------------------------------------------------------------------------
void vtkSMSourceProxy::PipelineUpdated()
{
  // This is called when the remote VTK object has re-executed.
  vtkLogF(INFO, "Proxy (%s, %s): %u has been updated!", this->GetXMLGroup(), this->GetXMLName(),
    this->GetGlobalID());
  for (const auto& item : this->PInternals->OutputPorts)
  {
    item.Port->PipelineUpdated();
  }

  // Notify the world that this algorithm has re-executed.
  this->InvokeEvent(vtkCommand::UpdateDataEvent, nullptr);

  this->UpdateConsumerDomains();
}

//---------------------------------------------------------------------------
void vtkSMSourceProxy::CreateSelectionProxies()
{
  if (this->DisableSelectionProxies || this->SelectionProxiesCreated)
  {
    return;
  }

  this->SelectionProxiesCreated = true;

#if 0 // FIXME: ASYNC
  int numOutputs = this->GetNumberOfAlgorithmOutputPorts();

  // Setup selection proxies
  if (numOutputs > MAX_NUMBER_OF_PORTS)
  {
    vtkErrorMacro("vtkSMSourceProxy was not designed to handle more than "
      << MAX_NUMBER_OF_PORTS
      << " output ports. "
         "In general, that's not a good practice. Try  reducing the number of "
         "output ports. Aborting for debugging purposes.");
    abort();
  }
  this->PInternals->SelectionProxies.resize(numOutputs);
  this->PInternals->SelectionObservers.resize(numOutputs);

  vtkClientServerStream stream;
  vtkSMSessionProxyManager* pxm = this->GetSessionProxyManager();
  for (int j = 0; j < numOutputs; j++)
  {
    vtkSmartPointer<vtkSMSourceProxy> esProxy;
    // Otherwise create it properly
    esProxy.TakeReference(
      vtkSMSourceProxy::SafeDownCast(pxm->NewProxy("filters", "PVExtractSelection")));
    if (esProxy)
    {
      esProxy->DisableSelectionProxies = true;
      esProxy->UpdateVTKObjects();

      std::ostringstream sstream;
      sstream << this->GetLogNameOrDefault() << "(ExtractSelection:" << j << ")";
      esProxy->SetLogName(sstream.str().c_str());

      this->SetExtractSelectionProxy(j, esProxy);

      // We don't use input property since that leads to reference loop cycles
      // and I don't feel like doing the garbage collection thing right now.
      stream << vtkClientServerStream::Invoke << SIPROXY(this) << "SetupSelectionProxy" << j
             << SIPROXY(esProxy) << vtkClientServerStream::End;
    }
  }
#endif
}

//---------------------------------------------------------------------------
void vtkSMSourceProxy::SetSelectionInput(
  unsigned int portIndex, vtkSMSourceProxy* input, unsigned int outputport)
{
  this->CreateSelectionProxies();

  if (this->PInternals->SelectionProxies.size() <= portIndex)
  {
    return;
  }
  vtkSMSourceProxy* esProxy = this->PInternals->SelectionProxies[portIndex];
  if (esProxy)
  {
    vtkSMInputProperty* pp = vtkSMInputProperty::SafeDownCast(esProxy->GetProperty("Selection"));
    pp->RemoveAllProxies();
    pp->AddInputConnection(input, outputport);
    esProxy->UpdateVTKObjects();
    this->InvokeEvent(vtkCommand::SelectionChangedEvent, &portIndex);
  }
}

//---------------------------------------------------------------------------
vtkSMSourceProxy* vtkSMSourceProxy::GetSelectionInput(unsigned int portIndex)
{
  if (this->PInternals->SelectionProxies.size() > portIndex)
  {
    vtkSMSourceProxy* esProxy = this->PInternals->SelectionProxies[portIndex];
    if (esProxy)
    {
      vtkSMInputProperty* pp = vtkSMInputProperty::SafeDownCast(esProxy->GetProperty("Selection"));
      if (pp->GetNumberOfProxies() == 1)
      {
        return vtkSMSourceProxy::SafeDownCast(pp->GetProxy(0));
      }
    }
  }
  return nullptr;
}

//---------------------------------------------------------------------------
unsigned int vtkSMSourceProxy::GetSelectionInputPort(unsigned int portIndex)
{
  if (this->PInternals->SelectionProxies.size() > portIndex)
  {
    vtkSMSourceProxy* esProxy = this->PInternals->SelectionProxies[portIndex];
    if (esProxy)
    {
      vtkSMInputProperty* pp = vtkSMInputProperty::SafeDownCast(esProxy->GetProperty("Selection"));
      if (pp->GetNumberOfProxies() == 1)
      {
        return pp->GetOutputPortForConnection(portIndex);
      }
    }
  }
  return 0;
}

//---------------------------------------------------------------------------
void vtkSMSourceProxy::CleanSelectionInputs(unsigned int portIndex)
{
  if (this->PInternals->SelectionProxies.size() <= portIndex)
  {
    return;
  }
  vtkSMSourceProxy* esProxy = this->PInternals->SelectionProxies[portIndex];
  if (esProxy)
  {
    esProxy->RemoveObserver(this->PInternals->SelectionObservers[portIndex]);
    vtkSMInputProperty* pp = vtkSMInputProperty::SafeDownCast(esProxy->GetProperty("Selection"));
    pp->RemoveAllProxies();
    esProxy->UpdateVTKObjects();
    this->InvokeEvent(vtkCommand::SelectionChangedEvent, &portIndex);
  }
}

//---------------------------------------------------------------------------
vtkSMSourceProxy* vtkSMSourceProxy::GetSelectionOutput(unsigned int portIndex)
{
  if (this->PInternals->SelectionProxies.size() > portIndex)
  {
    return this->PInternals->SelectionProxies[portIndex];
  }

  return nullptr;
}

//---------------------------------------------------------------------------
void vtkSMSourceProxy::SetLogNameInternal(
  const char* name, bool propagate_to_subproxies, bool propagate_to_proxylistdomains)
{
  this->Superclass::SetLogNameInternal(
    name, propagate_to_subproxies, propagate_to_proxylistdomains);
  auto& internals = *this->PInternals;
  for (size_t port = 0, max = internals.SelectionProxies.size(); port < max; ++port)
  {
    if (auto esProxy = internals.SelectionProxies[port])
    {
      std::ostringstream stream;
      stream << name << "(ExtractSelection:" << port << ")";
      esProxy->SetLogName(stream.str().c_str());
    }
  }
}

//---------------------------------------------------------------------------
void vtkSMSourceProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ProcessSupport: " << this->ProcessSupport << endl;
}
