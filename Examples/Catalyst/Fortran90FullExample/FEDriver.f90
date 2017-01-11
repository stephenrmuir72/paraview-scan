! A Fortran Catalyst example. Note that coproc.py
! must be in the directory where the example
! is run from.
! Thanks to Lucas Pettey for helping create the example.

PROGRAM main
  USE tcp, ONLY: testcoprocessor
  USE interfaces	! Procedure interfaces for interoperability.


  implicit none
  include 'mpif.h'


  integer, parameter :: nx=100, ny=100, nz=100, ntime=10    ! Maximal node numbers and time steps.
  integer :: i, j, k, &                                     ! Loop variables in x, y and z direction, respectively.
             time, nxstart, nxend, numtasks, rank, ierr, &
             status                                         ! Flag for array allocation.
  real :: max
  complex(kind=8), allocatable :: psi01(:,:,:)              ! Deferred-shape array of rank 3.

  call mpi_init(ierr)
  call mpi_comm_size(MPI_COMM_WORLD, numtasks, ierr)
  call mpi_comm_rank(MPI_COMM_WORLD, rank, ierr)

  call coprocessorinitializewithpython("coproc.py"//c_null_char,9)      ! Originally ("coproc.py",9).

  ! Partition in the x-direction only.
  nxstart = rank*nx/numtasks+1
  nxend = (rank+1)*nx/numtasks

  if(numtasks /= rank+1) then
     nxend=nxend+1
  endif

  allocate(psi01(nxend-nxstart+1,ny,nz), STAT=status)

  ! Set initial values.
  max=sqrt(real(nx)**2+real(ny)**2+real(nz)**2)
  do k=1,nz
     do j=1,ny
        do i=1,nxend-nxstart+1
           psi01(i,j,k)=CMPLX(max-sqrt(real(i-50)**2+real(j-50)**2+real(k-50)**2), real(1+j+k),8)/max*100.0     ! Kind parameter 8 inserted subsequently.
        enddo
     enddo
  enddo

  do time=1,ntime
     do k=1,nz
        do j=1,ny
           do i=1,nxend-nxstart+1
              psi01(i,j,k) = CMPLX(real(0.30),0.0)+psi01(i,j,k)
           end do
        end do
     end do

     call testcoprocessor(nxstart,nxend,nx,ny,nz,time,dble(time),psi01)
  enddo

  deallocate(psi01, STAT=status)

  call coprocessorfinalize()
  call mpi_finalize(ierr)

end program main
