vtk_module_add_serdes_wasm_package(
  MODULE ParaView::WebAssembly
  OUTPUT_NAME pvWebAssembly
  EXTRA_LINK_OPTIONS
    # # Uncomment the next line when quick linking is desired for faster dev cycles.
    # "-O1" 
)
