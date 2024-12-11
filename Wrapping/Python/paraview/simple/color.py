import paraview
from paraview import servermanager
from paraview.util import proxy as proxy_util

from paraview.modules.vtkRemotingCore import vtkPVSession


# -----------------------------------------------------------------------------
def LoadPalette(paletteName):
    """Load a color palette to override the default foreground and background
    colors used by ParaView views. The current global palette's colors are set
    to the colors in the loaded palette.

    :param paletteName: Name of the palette to load from this list:
        'WarmGrayBackground', 'DarkGrayBackground', 'NeutralGrayBackground',
        'LightGrayBackground', 'WhiteBackground', 'BlackBackground',
        'GradientBackground'.
    :type paletteName: str"""
    name = paletteName

    # Handling backward compatibility
    if paraview.compatibility.GetVersion() <= (5, 10):
        if paletteName == "DefaultBackground" or paletteName == "GrayBackground":
            name = "BlueGrayBackground"
        elif paletteName == "PrintBackground":
            name = "WhiteBackground"
    if paraview.compatibility.GetVersion() <= (5, 11):
        if paletteName == "WarmGrayBackground":
            name = "DarkGrayBackground"
    if paraview.compatibility.GetVersion() <= (5, 12):
        if paletteName == "DefaultBackground":
            name = "BlueGrayBackground"

    pxm = servermanager.ProxyManager()
    palette = pxm.GetProxy("settings", "ColorPalette")
    prototype = pxm.GetPrototypeProxy("palettes", name)

    if palette is None or prototype is None:
        return

    palette.Copy(prototype)
    palette.UpdateVTKObjects()


# -----------------------------------------------------------------------------
def GetColorTransferFunction(arrayname, representation=None, separate=False, **params):
    """Get the color transfer function used to map a data array with the given name to
    colors.

    :param arrayname: Name of the array whose color transfer function is requested.
    :type arrayname: str
    :param representation: Used to modify the array name when using a separate color
        transfer function. Optional, defaults to the active proxy.
    :type representation: Representation proxy.
    :param separate: If `True`, used to recover the separate color transfer function
        even if it is not used currently by the representation. Optional, defaults to
        `False`.
    :type separate: bool
    :return: This may create a new color transfer function if none exists, or return an
        existing one if found.
    :rtype: Color transfer function proxy"""
    if representation:
        if separate or representation.UseSeparateColorMap:
            arrayname = "%s_%s_%s" % (
                "Separate",
                representation.SMProxy.GetGlobalIDAsString(),
                arrayname,
            )
    if not servermanager.ActiveConnection:
        raise RuntimeError("Missing active session")
    session = servermanager.ActiveConnection.Session
    tfmgr = servermanager.vtkSMTransferFunctionManager()
    lut = servermanager._getPyProxy(
        tfmgr.GetColorTransferFunction(arrayname, session.GetSessionProxyManager())
    )
    proxy_util.set(lut, **params)
    return lut


def GetBlockColorTransferFunction(
    selector, arrayname, representation=None, separate=False, **params
):
    """Get the color transfer function used to map a data array with the given name to
    colors in the blocks referred to by a selector expresson.

    :param selector: Selector expression used to select blocks from whom the color
        transfer should be retrieved.
    :type selector: str
    :param arrayname: Name of the array whose color transfer function is requested.
    :type arrayname: str
    :param representation: Used to modify the array name when using a separate color
        transfer function. Optional, defaults to the active proxy.
    :type representation: Representation proxy.
    :param separate: If `True`, used to recover the separate color transfer function
        even if it is not used currently by the representation. Optional, defaults to
        `False`.
    :type separate: bool
    :return: This may create a new color transfer function if none exists, or return an
        existing one if found.
    :rtype: Color transfer function proxy"""
    if representation:
        if representation.GetBlockUseSeparateColorMap(selector):
            arrayname = "%s_%s_%s_%s" % (
                "Separate_",
                representation.SMProxy.GetGlobalIDAsString(),
                selector,
                arrayname,
            )
        elif separate or representation.UseSeparateColorMap:
            arrayname = "%s_%s_%s" % (
                "Separate",
                representation.SMProxy.GetGlobalIDAsString(),
                arrayname,
            )
    if not servermanager.ActiveConnection:
        raise RuntimeError("Missing active session")
    session = servermanager.ActiveConnection.Session
    tfmgr = servermanager.vtkSMTransferFunctionManager()
    lut = servermanager._getPyProxy(
        tfmgr.GetColorTransferFunction(arrayname, session.GetSessionProxyManager())
    )
    proxy_util.set(lut, **params)
    return lut


# -----------------------------------------------------------------------------
def GetOpacityTransferFunction(
    arrayname, representation=None, separate=False, **params
):
    """Get the opacity transfer function used to map a data array with the given name to
    opacities in the blocks referred to by a selector expresson.

    :param arrayname: Name of the array whose opacity transfer function is requested.
    :type arrayname: str
    :param representation: Used to modify the array name when using a separate opacity
        transfer function. Optional, defaults to the active representation.
    :type representation: Representation proxy.
    :param separate: If `True`, used to recover the separate opacity transfer function
        even if it is not used currently by the representation. Optional, defaults to
        `False`.
    :type separate: bool
    :return: This may create a new opacity transfer function if none exists, or return an
        existing one if found.
    :rtype: Opacity transfer function proxy"""
    if representation:
        if separate or representation.UseSeparateColorMap:
            arrayname = "%s%s_%s" % (
                "Separate_",
                representation.SMProxy.GetGlobalIDAsString(),
                arrayname,
            )
    if not servermanager.ActiveConnection:
        raise RuntimeError("Missing active session")
    session = servermanager.ActiveConnection.Session
    tfmgr = servermanager.vtkSMTransferFunctionManager()
    otf = servermanager._getPyProxy(
        tfmgr.GetOpacityTransferFunction(arrayname, session.GetSessionProxyManager())
    )
    proxy_util.set(otf, **params)
    return otf


def GetTransferFunction2D(
    arrayname, array2name=None, representation=None, separate=False, **params
):
    """Get the 2D color transfer function used to map a data array with the given name to
    colors.

    :param arrayname: Name of the first array whose color transfer function is requested.
    :type arrayname: str
    :param array2name: Name of the second array whose color transfer function is requested.
    :type array2name: str
    :param representation: Used to modify the array name when using a separate color
        transfer function. Optional, defaults to the active representation.
    :type representation: Representation proxy.
    :param separate: If `True`, used to recover the separate color transfer function
        even if it is not used currently by the representation. Optional, defaults to
        `False`.
    :type separate: bool
    :return: This may create a new 2D color transfer function if none exists, or return an
        existing one if found.
    :rtype: 2D color transfer function proxy"""
    array2name = "" if array2name is None else ("_%s" % array2name)
    if representation:
        if separate or representation.UseSeparateColorMap:
            arrayname = "%s%s_%s%s" % (
                "Separate_",
                representation.SMProxy.GetGlobalIDAsString(),
                arrayname,
                array2name,
            )
    if not servermanager.ActiveConnection:
        raise RuntimeError("Missing active session")
    session = servermanager.ActiveConnection.Session
    tfmgr = servermanager.vtkSMTransferFunctionManager()
    tf2d = servermanager._getPyProxy(
        tfmgr.GetTransferFunction2D(arrayname, session.GetSessionProxyManager())
    )
    proxy_util.set(tf2d, **params)
    return tf2d


# -----------------------------------------------------------------------------
def ImportPresets(filename, location=vtkPVSession.CLIENT):
    """Import presets from a file. The file can be in the legacy color map XML
    format or in the new JSON format.

    :param filename: Path of the file containing the presets.
    :type filename: str
    :param location: Where the statefile should be save, e.g., pass `vtkPVSession.CLIENT`
        if the statefile is located on the client system (default value), pass in
        `vtkPVSession.SERVERS` if on the server. Optional, defaults to
        `vtkPVSession.CLIENT`.
    :type location: `vtkPVServer.ServerFlags` enum value
    :return: `True` on success, `False` otherwise."""
    presets = servermanager.vtkSMTransferFunctionPresets.GetInstance()
    return presets.ImportPresets(filename, location)


# -----------------------------------------------------------------------------
def ExportTransferFunction(
    colortransferfunction,
    opacitytransferfunction,
    tfname,
    filename,
    location=vtkPVSession.CLIENT,
):
    """Export transfer function to a file. The file will be saved in the new JSON format.

    :param colortransferfunction: The color transfer function to export.
    :type colortransferfunction: Color transfer function proxy.
    :param opacitytransferfunction: The opacity transfer functon to export, if provided.
        can be None.
    :type opacitytransferfunction: Opacity transfer function proxy.
    :param tfname: The name that will be given to the transfer function preset if imported
        back into ParaView.
    :type tfname: str
    :param filename: Path to file where exported transfer function should be saved.
    :type filename: str
    :param location: Where the statefile should be saved, e.g., pass `vtkPVSession.CLIENT`
        if the statefile is located on the client system (default value), pass in
        `vtkPVSession.SERVERS` if on the server. Optional, defaults to `vtkPVSession.CLIENT`.
    :type location: `vtkPVServer.ServerFlags` enum value
    :return: `True` on success, `False` otherwise."""
    return servermanager.vtkSMTransferFunctionProxy.ExportTransferFunction(
        colortransferfunction.SMProxy,
        opacitytransferfunction.SMProxy,
        tfname,
        filename,
        location,
    )


# -----------------------------------------------------------------------------
def CreateLookupTable(**params):
    """Create and return a lookup table.

    :param params: A variadic list of `key=value` pairs giving values of
        specific named properties of the lookup table.
    :return: Lookup table
    :rtype: Lookup table proxy"""
    lt = servermanager.rendering.PVLookupTable()
    controller = servermanager.ParaViewPipelineController()
    controller.InitializeProxy(lt)
    proxy_util.set(lt, **params)
    controller.RegisterColorTransferFunctionProxy(lt)
    return lt


# -----------------------------------------------------------------------------


def CreatePiecewiseFunction(**params):
    """Create and return a piecewise function.

    :param params: A variadic list of `key=value` pairs giving values of
        specific named properties of the opacity function.
    :return: Piecewise opacity function
    :rtype: Opacity functon proxy"""
    pfunc = servermanager.piecewise_functions.PiecewiseFunction()
    controller = servermanager.ParaViewPipelineController()
    controller.InitializeProxy(pfunc)
    proxy_util.set(pfunc, **params)
    controller.RegisterOpacityTransferFunction(pfunc)
    return pfunc


# -----------------------------------------------------------------------------
def AssignLookupTable(arrayInfo, lutName, rangeOveride=[]):
    """Assign a lookup table to an array by lookup table name.

    Example usage::

        track = GetAnimationTrack("Center", 0, sphere) or
        arrayInfo = source.PointData["Temperature"]
        AssignLookupTable(arrayInfo, "Cool to Warm")

    :param arrayInfo: The information object for the array. The array name and its
        range is determined using the info object provided.
    :type arrayInfo: `paraview.modules.vtkRemotingCore.vtkPVArrayInformation`
    :param lutName: The name for the transfer function preset.
    :type lutName: str
    :param rangeOveride: The range to use instead of the range of the
        array determined using the `arrayInfo` parameter. Defaults to `[]`,
        meaning the array info range will be used.
    :type rangeOveride: 2-element tuple or list of floats
    :return: `True` on success, `False` otherwise.
    :rtype: bool"""


# -----------------------------------------------------------------------------
def GetLookupTableNames():
    """Returns a list containing the currently available transfer function
    preset names.

    :return: List of currently availabe transfer function preset names.
    :rtype: List of str"""
    presets = servermanager.vtkSMTransferFunctionPresets.GetInstance()
    return [
        presets.GetPresetName(index) for index in range(presets.GetNumberOfPresets())
    ]


# -----------------------------------------------------------------------------


def LoadLookupTable(fileName):
    """Load transfer function preset from a file. Both JSON (new) and XML (legacy)
    preset formats are supported.

    :param fileName: If the filename ends with a .xml, it's assumed to be a legacy color
        map XML and will be converted to the new format before processing.
    :type fileName: str
    :return: `True` if successful, `False` otherwise.
    :rtype: bool
    """
    presets = servermanager.vtkSMTransferFunctionPresets.GetInstance()
    return presets.ImportPresets(fileName)
