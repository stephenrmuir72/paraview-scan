module interfaces
!
!   PURPOSE:
!       Provides necessary procedure interfaces for interoperability.
!       Interface blocks contain procedure interfaces furnished with
!       bind(c) attribute to allow the Fortran compiler used to perform
!       consistency checks with respect to corresponding C function
!       prototypes. A Fortran compiler implementing the Fortran 2003
!       standard is required.
!
!   DOCUMENTATION:
!       Further documentation of C functions can be found here:
!       http://www.paraview.org/ParaView/Doc/Nightly/www/cxx-doc/classvtkCPPythonAdaptorAPI.html
!       http://www.paraview.org/ParaView/Doc/Nightly/www/cxx-doc/classvtkCPAdaptorAPI.html#a47f8027450c5fb4845de0ea6ba4d1125
!
  ! Intrinsic module includes named constants, derived types
  ! and module procedures to support interoperability.
  USE iso_c_binding, ONLY: c_int, c_double, c_char, c_null_char, c_loc, c_ptr


  implicit none


! ------------------------------- INTERFACES FOR FEDriver -----------------------------------
  interface
    ! Declared in CPythonAdaptorAPI.h.
    subroutine coprocessorinitializewithpython(pythonFileName, pythonFileNameLength) bind(c)
        import :: c_int, c_char
        character(kind=c_char) :: pythonFileName
        integer(kind=c_int) :: pythonFileNameLength
    end subroutine

    ! Declared in vtkCPAdaptorAPI.h which in turn is
    ! included in vtkCPPythonAdaptorAPI.h.
    subroutine coprocessorfinalize() bind(c)
    end subroutine
  end interface

! --------------------------- INTERFACES FOR FEFortranAdaptor -------------------------------
  interface
    ! Defined in file FECxxAdaptor.cxx.
    subroutine createcpimagedata(nxstart, nxend, nx, ny, nz) bind(c)
        import :: c_int
        integer(kind=c_int) :: nxstart, nxend, nx, ny, nz
    end subroutine

    ! Defined in file FECxxAdaptor.cxx.
    subroutine addfield(scalars, name) bind(c)
        import :: c_ptr, c_char
        type(c_ptr), value :: scalars
        character(kind=c_char) :: name
    end subroutine

    ! Declared in vtkCPAdaptorAPI.h which in turn is
    ! included in vtkCPPythonAdaptorAPI.h.
    subroutine coprocess() bind(c)
    end subroutine

    ! Declared in vtkCPAdaptorAPI.h which in turn is
    ! included in vtkCPPythonAdaptorAPI.h.
    subroutine requestdatadescription(timeStep, time, coprocessThisTimeStep) bind(c)
        import :: c_int, c_double
        integer(kind=c_int) :: timeStep, coprocessThisTimeStep
        real(kind=c_double) :: time
    end subroutine

    ! Declared in vtkCPAdaptorAPI.h which in turn is
    ! included in vtkCPPythonAdaptorAPI.h.
    subroutine needtocreategrid(needGrid) bind(c)
        import :: c_int
        integer(kind=c_int) :: needGrid
    end subroutine
  end interface

end module interfaces