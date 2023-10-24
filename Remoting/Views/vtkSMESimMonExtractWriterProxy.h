// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkSMESimMonExtractWriterProxy
 * @brief
 *
 * @sa
 */

#ifndef vtkSMESimMonExtractWriterProxy_h
#define vtkSMESimMonExtractWriterProxy_h

#include "vtkRemotingViewsModule.h"  // needed for exports
#include "vtkSMExtractWriterProxy.h" // base class
#include "vtkSMViewProxy.h"          // for member variable

class vtkSMViewProxy;

class VTKREMOTINGVIEWS_EXPORT vtkSMESimMonExtractWriterProxy : public vtkSMExtractWriterProxy
{
public:
  static vtkSMESimMonExtractWriterProxy* New();
  vtkTypeMacro(vtkSMESimMonExtractWriterProxy, vtkSMExtractWriterProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set the view proxy to export.
   */
  vtkSetObjectMacro(View, vtkSMViewProxy);
  vtkGetObjectMacro(View, vtkSMViewProxy);
  ///@}

  ///@{
  /**
   * Implementation for vtkSMExtractWriterProxy API.
   */
  bool Write(vtkSMExtractsController* extractor) override;
  bool CanExtract(vtkSMProxy* proxy) override;
  bool IsExtracting(vtkSMProxy* proxy) override;
  void SetInput(vtkSMProxy* proxy) override;
  vtkSMProxy* GetInput() override;
  ///@}
protected:
  vtkSMESimMonExtractWriterProxy();
  ~vtkSMESimMonExtractWriterProxy() override;

private:
  vtkSMESimMonExtractWriterProxy(const vtkSMESimMonExtractWriterProxy&) = delete;
  void operator=(const vtkSMESimMonExtractWriterProxy&) = delete;

  vtkSMViewProxy* View = nullptr;
};

#endif
