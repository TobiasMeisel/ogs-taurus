if (NOT OGS_USE_MPI)
    OgsTest(PROJECTFILE StokesFlow/ParallelPlate.prj RUNTIME 2)
    OgsTest(PROJECTFILE StokesFlow/ParallelPlateWithBodyForce.prj RUNTIME 2)
endif()
