/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxySelectionModel.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMProxySelectionModel
 * @brief   selects proxies.
 *
 * vtkSMProxySelectionModel is used to select proxies. vtkSMProxyManager uses
 * two instances of vtkSMProxySelectionModel for keeping track of the
 * selected/active sources/filters and the active view.
 * @sa
 * vtkSMProxyManager
 */

#ifndef vtkSMProxySelectionModel_h
#define vtkSMProxySelectionModel_h

#include "vtkObject.h"
#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSmartPointer.h"                // needed for vtkSmartPointer.

class vtkSMProxySelectionModelInternal;
class vtkCollection;
class vtkSMProxy;

#include <list> // needed for vtkset::list.

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMProxySelectionModel : public vtkObject
{
public:
  static vtkSMProxySelectionModel* New();
  vtkTypeMacro(vtkSMProxySelectionModel, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Type for selection.
   */
  typedef std::list<vtkSmartPointer<vtkSMProxy>> SelectionType;

  // vtkSMProxy selection flags
  enum ProxySelectionFlag
  {
    NO_UPDATE = 0,
    CLEAR = 1,
    SELECT = 2,
    DESELECT = 4,
    CLEAR_AND_SELECT = CLEAR | SELECT
  };

  /**
   * Returns the proxy that is current, nullptr if there is no current.
   */
  vtkSMProxy* GetCurrentProxy();

  /**
   * Sets the current proxy. \c command is used to control how the current
   * selection is affected.
   * \li NO_UPDATE: change the current without affecting the selected set of
   * proxies.
   * \li CLEAR: clear current selection
   * \li SELECT: also select the proxy being set as current
   * \li DESELECT: deselect the proxy being set as current.
   */
  void SetCurrentProxy(vtkSMProxy* proxy, int command);

  /**
   * Returns true if the proxy is selected.
   */
  bool IsSelected(vtkSMProxy* proxy);

  // Returns the number of selected proxies.
  unsigned int GetNumberOfSelectedProxies();

  /**
   * Returns the selected proxy at the given index.
   */
  vtkSMProxy* GetSelectedProxy(unsigned int index);

  /**
   * Returns the selection set.
   */
  const SelectionType& GetSelection() { return this->Selection; }

  /**
   * Update the selected set of proxies. \c command affects how the selection is
   * updated.
   * \li NO_UPDATE: don't affect the selected set of proxies.
   * \li CLEAR: clear selection
   * \li SELECT: add the proxies to selection
   * \li DESELECT: deselect the proxies
   * \li CLEAR_AND_SELECT: clear selection and then add the specified proxies as
   * the selection.
   */

  void Select(const SelectionType& proxies, int command);

  void Select(vtkSMProxy* proxy, int command);

  /**
   * Wrapper friendly methods to doing what Select() can do.
   */
  void Clear() { this->Select(nullptr, CLEAR); }
  void Select(vtkSMProxy* proxy) { this->Select(proxy, SELECT); }
  void Deselect(vtkSMProxy* proxy) { this->Select(proxy, DESELECT); }
  void ClearAndSelect(vtkSMProxy* proxy) { this->Select(proxy, CLEAR_AND_SELECT); }

  /**
   * Utility method to get the data bounds for the currently selected items.
   * This only makes sense for selections comprising of source-proxies or
   * output-port proxies.
   * Returns true is the bounds are valid.
   */
  bool GetSelectionDataBounds(double bounds[6]);

protected:
  vtkSMProxySelectionModel();
  ~vtkSMProxySelectionModel() override;

  void InvokeCurrentChanged(vtkSMProxy* proxy);
  void InvokeSelectionChanged();

  vtkSmartPointer<vtkSMProxy> Current;
  SelectionType Selection;

private:
  vtkSMProxySelectionModel(const vtkSMProxySelectionModel&) = delete;
  void operator=(const vtkSMProxySelectionModel&) = delete;

  class vtkInternal;
  friend class vtkInternal;
  vtkInternal* Internal;
};

#endif
