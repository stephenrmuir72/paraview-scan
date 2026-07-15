set(python_plugins
  PythonSources.py)

if(PARAVIEW_USE_PYTHON)
  paraview_add_plugin(
    PythonAlgorithm
    REQUIRED_ON_SERVER
    VERSION "1.0"
    PYTHON_MODULES ${python_plugins}
  )
endif()
