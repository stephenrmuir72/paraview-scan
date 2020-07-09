enable_language(Fortran)
include(FortranCInterface)

FortranCInterface_HEADER(PagosaAdaptorAPIMangling.h SYMBOLS
  setcoprocessorgeometry setcoprocessorfield
  setmarkergeometry
  setmarkerscalarfield setmarkervectorfield setmarkertensorfield)

vtk_module_add_module(ParaView::AdaptorsPagosa
  HEADERS PagosaAdaptor.h
  SOURCES PagosaAdaptor.cxx
)
vtk_module_client_server_exclude()
