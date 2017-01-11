module tcp
  USE interfaces    ! Procedure interfaces for interoperability with C code.


  implicit none


  contains
    subroutine testcoprocessor(nxstart, nxend, nx, ny, nz, step, time, psi01)
      implicit none

      integer, intent(in) :: nxstart, nxend, nx, ny, nz, step
      real(kind=8), intent(in) :: time
      complex(kind=8), dimension(:,:,:), intent (in), target :: psi01       ! Target attribute added subsequently.
      integer :: flag

      ! Check the current pipelines to see if any of them
      ! needs to execute for the given time and time step.
      ! After this procedure call, flag is set either to
      !   0: no coprcessing necessary, or
      !   1: coprcessing needs to be performed.
      call requestdatadescription(step, time, flag)

      ! If flag == 1, set above, coprocessing needs to be performed.
      if (flag /= 0) then
          ! Set flag to 1 if it does not have copy of grid.
          call needtocreategrid(flag)

          if (flag /= 0) then
            call createcpimagedata(nxstart, nxend, nx, nz, nz)
          end if

          call addfield(c_loc(psi01), "psi01"//c_null_char)     ! Modification by passing address of psi01.
          call coprocess()
      end if

      return
    end subroutine
end module tcp
