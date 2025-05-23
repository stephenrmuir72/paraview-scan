if(PARAVIEW_CONTRACT_FILE_URLS)
  include(ParaViewContractTesting)

  list(REMOVE_DUPLICATES PARAVIEW_CONTRACT_FILE_URLS)
  foreach(contract_url IN LISTS PARAVIEW_CONTRACT_FILE_URLS)
    paraview_contract_test(
        TEST_FILE_URL ${contract_url}
        EXTRA_BUILD_ARGUMENTS ${extra_build_args})
  endforeach()
endif()
