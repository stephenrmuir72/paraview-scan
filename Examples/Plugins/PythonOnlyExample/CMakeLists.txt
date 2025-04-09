if (PARAVIEW_USE_PYTHON)
  paraview_add_plugin(
    PreserveInput
    REQUIRED_ON_SERVER
    VERSION "1.0"
    PYTHON_MODULES preserve_input.py
  )
endif()
