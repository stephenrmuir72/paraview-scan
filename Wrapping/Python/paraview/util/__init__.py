
# These functions were present in a util.py module.
# Keeping them around for backwards compatibility
def SetOutputWholeExtent(algorithm, extent):
    """
    Convenience method to help set the WHOLE_EXTENT() in RequestInformation.
    Commonly used by programmable filters. The arguments are the algorithm
    and a tuple/list with 6 elements (xmin, xmax, ymin, ymax, zmin, zmax).

    Example use::

        import paraview.util
        # The output will be of dimensions 10, 1, 1
        paraview.util.SetOutputWholeExtent(algorithm, (0, 9, 0, 0, 0, 0)
    """
    if len(extent) != 6:
        raise "Expected a sequence of length 6"
    from vtkmodules.vtkCommonExecutionModel import vtkStreamingDemandDrivenPipeline
    algorithm.GetExecutive().GetOutputInformation(0).Set(vtkStreamingDemandDrivenPipeline.WHOLE_EXTENT(), extent[0], extent[1], extent[2],extent[3], extent[4], extent[5])

def IntegrateCell(dataset, cellId):
    """
    This functions uses vtkCellIntegrator's Integrate method that calculates
    the length/area/volume of a 1D/2D/3D cell. The calculation is exact for
    lines, polylines, triangles, triangle strips, pixels, voxels, convex
    polygons, quads and tetrahedra. All other 3D cells are triangulated
    during volume calculation. In such cases, the result may not be exact.
    """
    from paraview.modules.vtkPVVTKExtensionsFiltersGeneral import vtkCellIntegrator
    return vtkCellIntegrator.Integrate(dataset, cellId)


def ReplaceDollarVariablesWithEnvironment(text):
    """Replaces all substrings of the type `${FOO}` with `FOO` obtained
    from the `os.environ`. If the key is not defined in the environment, this
    raises a `KeyError`.
    """
    import os, re
    r = re.compile(r"\$ENV\{([^}]+)\}")
    def repl(m):
        if m.group(1) in os.environ:
            return os.environ[m.group(1)]
        raise KeyError("'%s' is not defined in the process environment" % m.group(1))
    return re.sub(r, repl, text)

def Glob(fileName, dirPath, rootDir = None):
    """Given a directory `dirPath` composed of files, this function
    checks if files inside this directory match `fileName` pattern using `fnmatch`.
    The list of files or directories matching the pattern are returned in a list.
    `rootDir` is an optional parameter that can be set so `dirPath` is relative to `rootDir`.

    For example, given a directory `dirPath` composed of 3 files 1.vtu, 2.vtu, 3.pdf,
    paraview.util.Glob("*.vtu", dirPath) will return ["1.vtu, "2.vtu"]
    """
    import paraview
    import paraview.simple
    import paraview.servermanager as sm
    import fnmatch

    fullDirPath = dirPath

    if (dirPath[-1] != '/'):
        fullDirPath += '/'

    fileInfoHelperProxy = sm.ProxyManager().NewProxy("misc", "FileInformationHelper")
    fileInfoHelperProxy.GetProperty("DirectoryListing").SetElement(0, True)
    fileInfoHelperProxy.GetProperty("Path").SetElement(0, fullDirPath)
    if rootDir != None:
        fileInfoHelperProxy.GetProperty("WorkingDirectory").SetElement(0, rootDir)
    fileInfoHelperProxy.UpdateVTKObjects()

    localFileInfo = sm.vtkPVFileInformation()
    localFileInfo.Initialize()
    fileInfoHelperProxy.GatherInformation(localFileInfo)
    numFiles = localFileInfo.GetContents().GetNumberOfItems()

    foundFiles = []

    for i in range(numFiles):
        name = localFileInfo.GetContents().GetItemAsObject(i).GetName()
        if fnmatch.fnmatch(name, fileName):
            foundFiles.append(fullDirPath + name)

    foundFiles.sort()

    return foundFiles
