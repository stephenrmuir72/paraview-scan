import paraview
from paraview import servermanager
from paraview.simple import core

# ==============================================================================
# Internal helpers
# ==============================================================================


def _switchToActiveConnectionCallback(caller, event):
    """Callback called when the active session/connection changes in the
    ServerManager. We update the Python state to reflect the change."""
    if servermanager:
        session = servermanager.vtkSMProxyManager.GetProxyManager().GetActiveSession()
        connection = servermanager.GetConnectionFromSession(session)
        SetActiveConnection(connection)


def _initializeSession(connection):
    """Internal method used to initialize a session. Users don't need to
    call this directly. Whenever a new session is created this method is called
    by API in this module."""
    if not connection:
        raise RuntimeError("'connection' cannot be empty.")
    controller = servermanager.ParaViewPipelineController()
    controller.InitializeSession(connection.Session)


class _active_session_observer:
    def __init__(self):
        pxm = servermanager.vtkSMProxyManager.GetProxyManager()
        self.ObserverTag = pxm.AddObserver(
            pxm.ActiveSessionChanged, _switchToActiveConnectionCallback
        )

    def __del__(self):
        if servermanager:
            if servermanager.vtkSMProxyManager:
                servermanager.vtkSMProxyManager.GetProxyManager().RemoveObserver(
                    self.ObserverTag
                )


# -----------------------------------------------------------------------------


class _active_objects(object):
    """This class manages the active objects (source and view). The active
    objects are shared between Python and the user interface. This class
    is for internal use. Use the :ref:`SetActiveSource`,
    :ref:`GetActiveSource`, :ref:`SetActiveView`, and :ref:`GetActiveView`
    methods for setting and getting active objects."""

    def __get_selection_model(self, name, session=None):
        "Internal method."
        if session and session != servermanager.ActiveConnection.Session:
            raise RuntimeError(
                "Try to set an active object with invalid active connection."
            )
        pxm = servermanager.ProxyManager(session)
        model = pxm.GetSelectionModel(name)
        if not model:
            model = servermanager.vtkSMProxySelectionModel()
            pxm.RegisterSelectionModel(name, model)
        return model

    def set_view(self, view):
        "Sets the active view."
        active_view_model = self.__get_selection_model("ActiveView")
        if view:
            active_view_model = self.__get_selection_model(
                "ActiveView", view.GetSession()
            )
            active_view_model.SetCurrentProxy(
                view.SMProxy, active_view_model.CLEAR_AND_SELECT
            )
        else:
            active_view_model = self.__get_selection_model("ActiveView")
            active_view_model.SetCurrentProxy(None, active_view_model.CLEAR_AND_SELECT)

    def get_view(self):
        "Returns the active view."
        return servermanager._getPyProxy(
            self.__get_selection_model("ActiveView").GetCurrentProxy()
        )

    def set_source(self, source):
        "Sets the active source."
        active_sources_model = self.__get_selection_model("ActiveSources")
        if source:
            # 3 == CLEAR_AND_SELECT
            active_sources_model = self.__get_selection_model(
                "ActiveSources", source.GetSession()
            )
            active_sources_model.SetCurrentProxy(
                source.SMProxy, active_sources_model.CLEAR_AND_SELECT
            )
        else:
            active_sources_model = self.__get_selection_model("ActiveSources")
            active_sources_model.SetCurrentProxy(
                None, active_sources_model.CLEAR_AND_SELECT
            )

    def __convert_proxy(self, px):
        "Internal method."
        if not px:
            return None
        elif px.IsA("vtkSMOutputPort"):
            return servermanager.OutputPort(
                servermanager._getPyProxy(px.GetSourceProxy()), px.GetPortIndex()
            )
        else:
            return servermanager._getPyProxy(px)

    def get_source(self):
        "Returns the active source."
        return self.__convert_proxy(
            self.__get_selection_model("ActiveSources").GetCurrentProxy()
        )

    def get_selected_sources(self):
        "Returns the set of sources selected in the pipeline browser."
        model = self.__get_selection_model("ActiveSources")
        proxies = []
        for i in range(model.GetNumberOfSelectedProxies()):
            proxies.append(self.__convert_proxy(model.GetSelectedProxy(i)))
        return proxies

    view = property(get_view, set_view)
    source = property(get_source, set_source)


# ==============================================================================
# Active Source / View / Camera / AnimationScene
# ==============================================================================


def GetActiveView():
    """Returns the active view.

    :return: Active view.
    :rtype: View proxy."""
    return active_objects.view


# -----------------------------------------------------------------------------


def SetActiveView(view):
    """Sets the active view.

    :param view: The view to make active.
    :type view: View proxy."""
    active_objects.view = view


# -----------------------------------------------------------------------------


def GetActiveSource():
    """Returns the active source.

    :return: Active pipeline source.
    :rtype: Source proxy"""
    return active_objects.source


# -----------------------------------------------------------------------------


def SetActiveSource(source):
    """Sets the active source.

    :param source: The source
    :type source: Source proxy"""
    active_objects.source = source


# -----------------------------------------------------------------------------


def GetActiveCamera():
    """Returns the active camera for the active view.

    :return: The active camera
    :rtype: `vtkCamera`"""
    return GetActiveView().GetActiveCamera()


# ==============================================================================
# Client/Server Connection methods
# ==============================================================================


def Disconnect(ns=None, force=True):
    """Disconnect from the currently connected server and free the active
    session. Does not shut down the client application where the call is executed.

    :param ns: Namespace in which ParaView functions were created. Optional, defaults
        to the namespace returned by `globals()`
    :type ns: Dict or `None`
    :param force: Force disconnection in a simultaneous connection. Optional, defaults
        to forcing disconnection.
    :type force: bool
    """
    if not ns:
        ns = globals()

    supports_simutaneous_connections = (
        servermanager.vtkProcessModule.GetProcessModule().GetMultipleSessionsSupport()
    )
    if not force and supports_simutaneous_connections:
        # This is an internal Disconnect request that doesn't need to happen in
        # multi-server setup. Ignore it.
        return
    if servermanager.ActiveConnection:
        core._remove_functions(ns)
        servermanager.Disconnect()
        import gc

        gc.collect()


# -----------------------------------------------------------------------------


def Connect(ds_host=None, ds_port=11111, rs_host=None, rs_port=11111, timeout=60):
    """Creates a connection to a server.

    :param ds_host: Data server hostname. Needed to set up a client/data server/render server connection.
    :type ds_host: string
    :param ds_port: Data server port to listen on.
    :type ds_port: int
    :param rs_host: Render server hostname. Needed to set up a client/data server/render server connection.
    :type rs_host: string
    :param rs_port: Render server port to listen on.
    :type rs_port: int
    :param timeout: Time in seconds at which the connection is abandoned if not made.
    :type timeout: int

    Example usage connecting to a host named "amber"::

        Connect("amber") # Connect to a single server at default port
        Connect("amber", 12345) # Connect to a single server at port 12345
        Connect("amber", 11111, "vis_cluster", 11111) # connect to data server, render server pair
        Connect("amber", timeout=30) # Connect to a single server at default port with a 30s timeout instead of default 60s
        Connect("amber", timeout=-1) # Connect to a single server at default port with no timeout instead of default 60s
        Connect("amber", timeout=0)  # Connect to a single server at default port without retrying instead of retrying for
        the default 60s

    """
    Disconnect(globals(), False)
    connection = servermanager.Connect(ds_host, ds_port, rs_host, rs_port, timeout)
    if not (connection is None):
        _initializeSession(connection)
    return connection


# -----------------------------------------------------------------------------


def ReverseConnect(port=11111):
    """Create a reverse connection to a server.  First, disconnects from any servers,
    then listens on port and waits for an incoming connection from the server.

    :param port: The port to listen on for incoming connections.
    :type port: int
    :return: Connection object
    :rtype:
    """
    Disconnect(globals(), False)
    connection = servermanager.ReverseConnect(port)
    _initializeSession(connection)
    return connection


# -----------------------------------------------------------------------------


def ResetSession():
    """Reset the session to its initial state. All pipeline readers, sources,
    filters extractors, and representations are removed."""
    connection = servermanager.ResetSession()
    _initializeSession(connection)
    return connection


# ==============================================================================
# Multi-servers
# ==============================================================================


def SetActiveConnection(connection=None, ns=None):
    """Set the active connection. If the process was run without multi-server
    enabled and this method is called with a non-None argument while an
    ActiveConnection is present, it will raise a RuntimeError.

    :param connection: If provided, changes the current connection.
    :type connection: Connection object.
    :param ns: Namespace in which functions from the old Connection are removed
        and functions in the new Connection are added.
    :raises RuntimeError: If called when ParaView is not running in multi-server
        mode, a `RuntimeError` will be raised.
    """
    if not ns:
        ns = globals()
    if servermanager.ActiveConnection != connection:
        core._remove_functions(ns)
        servermanager.SetActiveConnection(connection)
        core._add_functions(ns)


# -----------------------------------------------------------------------------
# Initialize module
# -----------------------------------------------------------------------------

if not paraview.options.satelite:
    active_session_observer = _active_session_observer()
    active_objects = _active_objects()
