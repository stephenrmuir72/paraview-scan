/*=========================================================================

   Program: ParaView
   Module:    pqProxy.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "pqProxy.h"

#include "pqServer.h"
#include "vtkSMProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMTrace.h"
#include "vtkSmartPointer.h"

#include <QPointer>
#include <vtksys/RegularExpression.hxx>

//-----------------------------------------------------------------------------
class pqProxy::pqInternals
{
public:
  QPointer<pqServer> Server;
  vtkSmartPointer<vtkSMProxy> Proxy;
  pqProxy::ModifiedState Modified{ pqProxy::UNMODIFIED };
  QString RegistrationGroup;
  QString RegistrationName;
};

//-----------------------------------------------------------------------------
pqProxy::pqProxy(const QString& group, const QString& name, vtkSMProxy* aproxy, pqServer* aserver,
  QObject* parentObject /*=nullptr*/)
  : Superclass(parentObject)
  , Internals(new pqProxy::pqInternals())
{
  auto& internals = (*this->Internals);
  internals.Proxy = aproxy;
  internals.Server = aserver;
  internals.RegistrationGroup = group;
  internals.RegistrationName = name;
}

//-----------------------------------------------------------------------------
pqProxy::~pqProxy() = default;

//-----------------------------------------------------------------------------
pqServer* pqProxy::server() const
{
  const auto& internals = (*this->Internals);
  return internals.Server;
}

//-----------------------------------------------------------------------------
void pqProxy::setRegistrationName(const QString& name)
{
  auto& internals = (*this->Internals);
  internals.RegistrationName = name;
  Q_EMIT this->nameChanged(this);
}

//-----------------------------------------------------------------------------
void pqProxy::rename(const QString& newname)
{
  auto& internals = (*this->Internals);
  if (newname != internals.RegistrationName)
  {
    SM_SCOPED_TRACE(RenameProxy).arg("proxy", internals.Proxy);
    auto* pxm = this->proxyManager();
    pxm->RegisterProxy(
      qUtf8Printable(internals.RegistrationGroup), qUtf8Printable(newname), internals.Proxy);
    pxm->UnRegisterProxy(qUtf8Printable(internals.RegistrationGroup),
      qUtf8Printable(internals.RegistrationName), internals.Proxy);
    // this will cause pqProxy::setRegistrationName(..) to be called.
    Q_ASSERT(internals.RegistrationName == newname);
  }
}

//-----------------------------------------------------------------------------
const QString& pqProxy::registrationName() const
{
  const auto& internals = (*this->Internals);
  return internals.RegistrationName;
}

//-----------------------------------------------------------------------------
const QString& pqProxy::registrationGroup() const
{
  const auto& internals = (*this->Internals);
  return internals.RegistrationGroup;
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqProxy::proxy() const
{
  const auto& internals = (*this->Internals);
  return internals.Proxy;
}

//-----------------------------------------------------------------------------
vtkPVXMLElement* pqProxy::xmlHints() const
{
  const auto& internals = (*this->Internals);
  return internals.Proxy->GetHints();
}

//-----------------------------------------------------------------------------
pqProxy::ModifiedState pqProxy::modifiedState() const
{
  const auto& internals = (*this->Internals);
  return internals.Modified;
}

//-----------------------------------------------------------------------------
void pqProxy::setModifiedState(pqProxy::ModifiedState modified)
{
  auto& internals = (*this->Internals);
  if (modified != internals.Modified)
  {
    internals.Modified = modified;
    Q_EMIT this->modifiedStateChanged(this);
  }
}

//-----------------------------------------------------------------------------
vtkSMSessionProxyManager* pqProxy::proxyManager() const
{
  const auto& internals = (*this->Internals);
  return internals.Server->proxyManager();
}

//-----------------------------------------------------------------------------
QString pqProxy::rstToHtml(const QString& rstStr)
{
  return QString::fromStdString(pqProxy::rstToHtml(rstStr.toUtf8().data()));
}

//-----------------------------------------------------------------------------
std::string pqProxy::rstToHtml(const char* rstStr)
{
  std::string htmlStr = rstStr;
  {
    // bold
    vtksys::RegularExpression re("[*][*]([^*]+)[*][*]");
    while (re.find(htmlStr))
    {
      const char* s = htmlStr.c_str();
      std::string bold(s + re.start(1), re.end(1) - re.start(1));
      htmlStr.replace(re.start(0), re.end(0) - re.start(0), std::string("<b>") + bold + "</b>");
    }
  }
  {
    // italic
    vtksys::RegularExpression re("[*]([^*]+)[*]");
    while (re.find(htmlStr))
    {
      const char* s = htmlStr.c_str();
      std::string it(s + re.start(1), re.end(1) - re.start(1));
      htmlStr.replace(re.start(0), re.end(0) - re.start(0), std::string("<i>") + it + "</i>");
    }
  }
  {
    // begin bullet list
    size_t start = 0;
    std::string src("\n\n- ");
    while ((start = htmlStr.find(src, start)) != std::string::npos)
    {
      htmlStr.replace(start, src.size(), "\n<ul><li>");
    }
  }
  {
    // li for bullet list
    size_t start = 0;
    std::string src("\n- ");
    while ((start = htmlStr.find(src, start)) != std::string::npos)
    {
      htmlStr.replace(start, src.size(), "\n<li>");
    }
  }
  {
    // end bullet list
    vtksys::RegularExpression re("<li>(.*)\n\n([^-])");
    while (re.find(htmlStr))
    {
      const char* s = htmlStr.c_str();
      std::string listItem(s + re.start(1), re.end(1) - re.start(1));
      std::string afterList(s + re.start(2), re.end(2) - re.start(2));
      htmlStr.replace(re.start(0), re.end(0) - re.start(0),
        std::string("<li>").append(listItem).append("</ul>").append(afterList));
    }
  }
  {
    // paragraph
    size_t start = 0;
    std::string src("\n\n");
    while ((start = htmlStr.find(src, start)) != std::string::npos)
    {
      htmlStr.replace(start, src.size(), "\n<p>\n");
    }
  }
  return htmlStr;
}
