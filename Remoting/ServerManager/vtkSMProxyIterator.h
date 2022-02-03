/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxyIterator.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMProxyIterator
 * @brief   iterates over all registered proxies (and groups)
 *
 * vtkSMProxyIterator iterates over all proxies registered with the
 * proxy manager. It can also iterate over groups.
 * @sa
 * vtkSMProxy vtkSMProxyManager
 */

#ifndef vtkSMProxyIterator_h
#define vtkSMProxyIterator_h

#include "vtkObject.h"
#include "vtkRemotingServerManagerModule.h" //needed for exports

struct vtkSMProxyIteratorInternals;

class vtkSMProxy;
class vtkSMSessionProxyManager;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMProxyIterator : public vtkObject
{
public:
  static vtkSMProxyIterator* New();
  vtkTypeMacro(vtkSMProxyIterator, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Override the set session so the SessionProxyManager could be cache for
   */
  void SetSessionProxyManager(vtkSMSessionProxyManager*);

  /**
   * Go to the beginning of the collection.
   */
  void Begin();

  /**
   * Go to the beginning of one group, and set mode to iterate over one group.
   */
  void Begin(const char* groupName);

  /**
   * Is the iterator pointing past the last element?
   */
  int IsAtEnd();

  /**
   * Move to the next property.
   */
  void Next();

  /**
   * Get the group at the current iterator location.
   */
  const char* GetGroup();

  /**
   * Get the key (proxy name) at the current iterator location.
   */
  const char* GetKey();

  /**
   * Get the proxy at the current iterator location.
   */
  vtkSMProxy* GetProxy();

  //@{
  /**
   * The traversal mode for the iterator. If the traversal mode is
   * set to GROUPS, each Next() will move to the next group, in
   * ONE_GROUP mode, all proxies in one group are visited and finally
   * in ALL mode, all proxies are visited.
   */
  vtkSetMacro(Mode, int);
  vtkGetMacro(Mode, int);
  void SetModeToGroupsOnly() { this->SetMode(GROUPS_ONLY); }
  void SetModeToOneGroup() { this->SetMode(ONE_GROUP); }
  void SetModeToAll() { this->SetMode(ALL); }
  //@}

  enum TraversalMode
  {
    GROUPS_ONLY = 0,
    ONE_GROUP = 1,
    ALL = 2
  };

protected:
  vtkSMProxyIterator();
  ~vtkSMProxyIterator() override;

  int Mode;

private:
  vtkSMProxyIteratorInternals* Internals;

  vtkSMProxyIterator(const vtkSMProxyIterator&) = delete;
  void operator=(const vtkSMProxyIterator&) = delete;
};

#endif
