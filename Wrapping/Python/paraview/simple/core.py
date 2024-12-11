import paraview
import paraview._backwardscompatibilityhelper
from paraview.util import proxy as proxy_util

from paraview import servermanager


# ==============================================================================
# Set of Internal functions
# ==============================================================================


def _create_func(key, module, skipRegistration=False):
    """Internal function."""

    from paraview.simple.session import active_objects

    def CreateObject(*input, **params):
        """
        This function creates a new proxy. For pipeline objects that accept inputs,
        all non-keyword arguments are assumed to be inputs. All keyword arguments are
        assumed to be property-value pairs and are passed to the new proxy.

        """

        registrationName = None
        for nameParam in ["registrationName", "guiName"]:
            if nameParam in params:
                registrationName = params[nameParam]
                del params[nameParam]

        # Create a controller instance.
        controller = servermanager.ParaViewPipelineController()

        from paraview.catalyst.detail import IsInsituInput, CreateProducer

        if IsInsituInput(registrationName):
            # This is a catalyst input, replace with a trivial producer
            px = CreateProducer(registrationName)
        else:
            # Instantiate the actual object from the given module.
            px = paraview._backwardscompatibilityhelper.GetProxy(
                module, key, no_update=True
            )

        # preinitialize the proxy.
        controller.PreInitializeProxy(px)

        # Make sure non-keyword arguments are valid
        for inp in input:
            if inp != None and not isinstance(inp, servermanager.Proxy):
                if px.GetProperty("Input") != None:
                    raise RuntimeError("Expecting a proxy as input.")
                else:
                    raise RuntimeError(
                        "This function does not accept non-keyword arguments."
                    )

        # Assign inputs
        inputName = servermanager.vtkSMCoreUtilities.GetInputPropertyName(px.SMProxy, 0)

        if px.GetProperty(inputName) != None:
            if len(input) > 0:
                px.SetPropertyWithName(inputName, input)
            else:
                # If no input is specified, try the active pipeline object
                if (
                    px.GetProperty(inputName).GetRepeatable()
                    and active_objects.get_selected_sources()
                ):
                    px.SetPropertyWithName(
                        inputName, active_objects.get_selected_sources()
                    )
                elif active_objects.source:
                    px.SetPropertyWithName(inputName, active_objects.source)
        else:
            if len(input) > 0:
                raise RuntimeError("This function does not expect an input.")

        # Pass all the named arguments as property,value pairs
        proxy_util.set(px, **params)

        # post initialize
        controller.PostInitializeProxy(px)

        if isinstance(px, servermanager.MultiplexerSourceProxy):
            px.UpdateDynamicProperties()

        if not skipRegistration:
            # Register the proxy with the proxy manager (assuming we are only using
            # these functions for pipeline proxies or animation proxies.
            if isinstance(px, servermanager.SourceProxy):
                controller.RegisterPipelineProxy(px, registrationName)
            elif px.GetXMLGroup() == "animation":
                controller.RegisterAnimationProxy(px)
        return px

    # add special tag to detect these functions in _remove_functions
    CreateObject.__paraview_create_object_tag = True
    CreateObject.__paraview_create_object_key = key
    CreateObject.__paraview_create_object_module = module
    return CreateObject


def _func_name_valid(name):
    "Internal function."
    valid = True
    for c in name:
        if c == "(" or c == ")":
            valid = False
            break
    return valid


def _remove_functions(g):
    to_remove = [
        item[0]
        for item in g.items()
        if hasattr(item[1], "__paraview_create_object_tag")
    ]
    for key in to_remove:
        del g[key]
        # paraview.print_info("remove %s", key)


def _add_functions(g):
    print("calling my _add_functions")
    if not servermanager.ActiveConnection:
        return []

    added_entries = set()
    activeModule = servermanager.ActiveConnection.ProxiesNS

    # add deprecated proxies first, so we create the new function and documentation.
    modules = paraview._backwardscompatibilityhelper.get_deprecated_proxies(
        activeModule
    )
    for proxyModule in modules:
        for deprecatedProxyPair in modules[proxyModule]:
            oldProxyLabel = deprecatedProxyPair[0]
            newProxyLabel = deprecatedProxyPair[1]
            if not newProxyLabel in g and _func_name_valid(newProxyLabel):
                f = _create_func(deprecatedProxyPair, proxyModule)
                f.__doc__ = "{} is a deprecated proxy. It will be automatically replaced by {}".format(
                    oldProxyLabel, newProxyLabel
                )
                f.__name__ = oldProxyLabel
                g[oldProxyLabel] = f
                added_entries.add(oldProxyLabel)

    for m in _get_proxymodules_to_import(servermanager.ActiveConnection):
        # Skip registering proxies in certain modules (currently only writers)
        skipRegistration = m is activeModule.writers
        for key in dir(m):
            if key not in g and _func_name_valid(key):
                # print "add %s function" % key
                f = _create_func(key, m, skipRegistration)
                f.__name__ = key
                f.__doc__ = _create_doc(m.getDocumentation(key), f.__doc__)
                g[key] = f
                added_entries.add(key)

    return list(added_entries)


def _listProperties(create_func):
    """Internal function that, given a proxy creation function, e.g.,
    paraview.simple.Sphere, returns the list of named properties that can
    be set during construction."""
    key = create_func.__paraview_create_object_key
    module = create_func.__paraview_create_object_module
    prototype_func = _create_func(key, module, skipRegistration=True)

    # Find the prototype by XML label name
    xml_definition = module._findProxy(name=key)
    xml_group = xml_definition["group"]
    xml_name = xml_definition["key"]
    prototype = sm.ProxyManager().GetPrototypeProxy(xml_group, xml_name)

    # Iterate over properties and add them to the list
    property_iter = prototype.NewPropertyIterator()
    property_iter.UnRegister(None)
    property_names = []
    while not property_iter.IsAtEnd():
        label = property_iter.GetPropertyLabel()
        if label is None:
            label = property_iter.GetKey()
        property_names.append(paraview.make_name_valid(label))
        property_iter.Next()

    return property_names


def _get_function_arguments(function):
    """The a list of named arguments for a function.
    Used for autocomplete in PythonShell in ParaView GUI and in pvpython"""
    import inspect

    if not inspect.isfunction(function) and not inspect.ismethod(function):
        raise TypeError(f"input argument: {function} is not a function")
    result = []
    if hasattr(function, "__paraview_create_object_tag") and getattr(
        function, "__paraview_create_object_tag"
    ):
        result = ListProperties(function)
    else:
        for _, parameter in inspect.signature(function).parameters.items():
            # skip *args and **kwargs
            if (
                parameter.kind != inspect.Parameter.VAR_POSITIONAL
                and parameter.kind != inspect.Parameter.VAR_KEYWORD
            ):
                result.append(parameter.name)
    return result


# -----------------------------------------------------------------------------


def _create_doc(new, old):
    "Internal function."
    import string

    res = new + "\n"
    ts = []
    strpd = old.split("\n")
    for s in strpd:
        ts.append(s.lstrip())
    res += " ".join(ts)
    res += "\n"
    return res


# -----------------------------------------------------------------------------


def _func_name_valid(name):
    "Internal function."
    valid = True
    for c in name:
        if c == "(" or c == ")":
            valid = False
            break
    return valid


# -----------------------------------------------------------------------------
def _get_proxymodules_to_import(connection):
    """
    used in _add_functions, _get_generated_proxies, and _remove_functions to get
    modules to import proxies from.
    """
    if connection and connection.ProxiesNS:
        modules = connection.ProxiesNS
        return [modules.filters, modules.sources, modules.writers, modules.animation]
    else:
        return []


# -----------------------------------------------------------------------------


def _get_generated_proxies():
    proxies = []
    for m in _get_proxymodules_to_import(servermanager.ActiveConnection):
        for key in dir(m):
            proxies.append(key)
    return proxies


# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------


def _find_writer(filename):
    "Internal function."
    extension = None
    parts = filename.split(".")
    if len(parts) > 1:
        extension = parts[-1]
    else:
        raise RuntimeError("Filename has no extension, please specify a write")

    if extension == "png":
        return "vtkPNGWriter"
    elif extension == "bmp":
        return "vtkBMPWriter"
    elif extension == "ppm":
        return "vtkPNMWriter"
    elif extension == "tif" or extension == "tiff":
        return "vtkTIFFWriter"
    elif extension == "jpg" or extension == "jpeg":
        return "vtkJPEGWriter"
    else:
        raise RuntimeError("Cannot infer filetype from extension:", extension)


# -----------------------------------------------------------------------------

# ==============================================================================
# Set of Internal classes
# ==============================================================================

# -----------------------------------------------------------------------------


class _funcs_internals:
    "Internal class."
    first_render = True


# -----------------------------------------------------------------------------


def ListProperties(proxyOrCreateFunction):
    """Given a proxy or a proxy creation function, e.g. paraview.simple.Sphere,
    returns the list of properties for the proxy that would be created.

    :param proxyOrCreateFunction: Proxy or proxy creation function whose property
        names are desired.
    :type proxyOrCreateFunction: Proxy or proxy creation function
    :return: List of property names
    :rtype: List of str"""

    try:
        return _listProperties(proxyOrCreateFunction)
    except:
        pass

    try:
        return proxyOrCreateFunction.ListProperties()
    except:
        pass

    return None


def _DisableFirstRenderCameraReset():
    """Normally a ResetCamera is called automatically when Render is called for
    the first time after importing the `paraview.simple` module. Calling this
    function disables this first render camera reset."""
    _funcs_internals.first_render = False
