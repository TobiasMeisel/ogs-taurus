+++
title = "Running OGS With MPI for parallel computing"
date = "2022-05-06T11:20:34+02:00"
author = "Wenqing Wang"
weight = 2

[menu]
  [menu.userguide]
    parent = "basics"
+++

The executable OGS for MPI parallel computing is compiled with special
 build configuration. If you need to compile the source code, please read
 [Build configuration for MPI and PETSc]({{<ref "configure_for_mpi_and_petsc.md">}}).

To conduct DDC enabled parallel computing with OGS, following steps are required:

### 1. Prepare input data
#### 1.1 Partition mesh
For the domain decomposition approach, an application of OGS using METIS as a node wise
 mesh topology partitioner, [`partmesh`]({{<ref
"mesh-partition">}}), is provided to partition meshes by node. An example of how
 to partition meshes is available in a workflow documentation on how to [create a simple parallel model]({{<ref
"create-a-simple-parallel-model">}}). You can type `partmesh --help` for the detailed
command line options.

The partitioned meshes are written in binary files for a fast parallel reading
 in ogs.

#### 1.2 Configure PETSc solver in project file

Setting  PETSc solver is the only change in project file for running parallel ogs with PETSc.
For linear solver, it is done by adding a tag of `petsc` inside `linear_solver` tag,
 e.g:
```
    <linear_solvers>
        <linear_solver>
            <name>linear_solver</name>
            <eigen>
                <solver_type>SparseLU</solver_type>
                <scaling>true</scaling>
            </eigen>
            <petsc>
                <parameters>-ksp_type bcgs
                    -pc_type jacobi
                    -ksp_rtol 1.e-16 -ksp_atol 1.e-12
                    -ksp_max_it 4000
                </parameters>
            </petsc>
        </linear_solver>
    </linear_solvers>
```
If tag of `petsc` is not given in project file, the default setting of PETSc
 linear solver will be taken, which uses the solver type of `cg` and
 the preconditioner type of `jacobi`.

For the simulation of coupled processes with the staggered scheme, a prefix
can be used by tag `prefix` to set PETSc solver for each process, individually.
 For example:
```
    <linear_solvers>
        <linear_solver>
            <name>linear_solver_T</name>
            <petsc>
                <prefix>T</prefix>
                <parameters>-T_ksp_type bcgs
                            -T_pc_type bjacobi
                            -T_ksp_rtol 1e-16
                            -T_ksp_max_it 3000
                </parameters>
            </petsc>
        </linear_solver>
        <linear_solver>
            <name>linear_solver_H</name>
            <petsc>
                <prefix>H</prefix>
                <parameters>-H_ksp_type bcgs
                            -H_pc_type mg
                            -H_ksp_rtol 1e-12
                            -H_ksp_max_it 4000
                </parameters>
            </petsc>
        </linear_solver>
    </linear_solvers>
```
The above example shows that once a prefix is given for PETSc linear solver
 settings, the original prefix of PETSc
 keyword, `-`, can be replaced with a new prefix, `-[given prefix string]_`. In the
above example, `-`, is replaced with `-T_` and `-H_`, respectively.

A introduction and a list of of PETSc ksp solvers and preconditioners can be found by
[this link](https://petsc.org/main/docs/manual/ksp/).

### 2. Launch MPI OGS
For MPI launcher, either *mpiexec* or *mpirun* can be used to run OGS.
 Preferably, *mpiexec* is recommended because it is defined in the MPI standard.
The number of processes to run of *mpiexec* must be identical to the number of
 mesh partitions.
For example, if the meshes of a project, foo.prj, are partitioned into 5 partitions,
 OGS can be launched in MPI as
```
mpiexec -n 5 ogs foo.prj -o [path to the output directory]
```

Running PETSc enabled OGS with one compute thread does not need mesh partitioning.
 However, the MPI launcher mpiexc or mpirun is required, e.g.:
```
mpiexec -n 1 ogs ...
```

Additional PETSc command line options can be given as unlabelled arguments at the end of the ogs run command preceded by two minus-signs
(`... ogs ... -- [PETSc options]`).

With  PETSc command line options, you can
* monitor ksp solver convergence status, e.g.
```
        mpiexec -n 5 ogs foo.prj -o output -- -ksp_converged_reason -ksp_monitor_true_residual
```
* change ksp solver setting, e.g.:
```
        mpiexec -n 5 ogs foo.prj -o output -- -ksp_type gmres -ksp_rtol 1e-16 -ksp_max_it 2000
```
* or use other PETSc command line options.

For Linux clusters or supercomputer, computation job has to be submitted to
 queue and job management system, which may provide a special command to
 launch MPI job. A job script for such
 queue system is required.

The cluster system EVE of UFZ uses SLURM
 (Simple Linux Utility for Resource Management) to manage computing jobs.
Here is an example of job script for the SLURM system on EVE:
```
#!/bin/bash
#SBATCH --job-name=ARESH_HM_3D_20
#SBATCH --chdir=/home/wwang/data_D/project/AREHS/HM_3D
#SBATCH --output=/home/wwang/data_D/project/AREHS/HM_3D/output/log_%x_%j.txt
#SBATCH --error=/home/wwang/data_D/project/AREHS/HM_3D/output/err_%x_%j.txt
#SBATCH --time=0-48:00:00
#SBATCH -n 20
#SBATCH --mem-per-cpu=4G

#SBATCH --mail-user=wenqing.wang@ufz.de
#SBATCH --mail-type=BEGIN,END

###source /etc/profile.d/000-modules.sh
export MODULEPATH="/software/easybuild-broadwell/modules/all/Core:/software/modulefiles"
module load  foss/2020b   petsc-bilke/3.16.5_foss2020b
module load  OpenMPI/4.0.5  HDF5/1.10.7  GMP/6.2.0

APP="/home/wwang/code/ogs6/exe_petsc/bin/ogs"
/bin/echo In directory: `pwd`
/bin/echo Number of CPUs: $SLURM_CPUS_PER_TASK
/bin/echo File name: $1

##load $APP

srun -n "$SLURM_NTASKS"  $APP /home/wwang/data_D/project/AREHS/HM_3D/simHM_glaciation.prj -o /home/wwang/data_D/project/AREHS/HM_3D/output
```
In the job script for EVE, `module load  foss/2020b` must be presented, and
 srun is a sort of MPI job launcher.
 If a job fails with an error message about shared library not found, you can check
 the EVE modules specified in the files in source code directory:
 *scripts/env/eve*, and add the corresponding modules to the load list
in the job script.

Once the the job script is ready, you can
  - submit the job by command, `sbatch [job script name]`,
  - check the job status by command `squeue`,
  - or cancel the job by command `scancel [job ID]`.


For the detailed syntax of job script of SLURM for EVE, please visit https://wiki.ufz.de/eve/
(user login required).

### 3. Check results
There are two output types available, VTK and XMDF.
In the project file, output type can be specified in `type` tag of `output` tag,
for example:
```
    <output>
       <type>VTK</type>
       ...
    </output>
```
or
```
    <output>
       <type>XDMF</type>
       ...
    </output>
```

#### 3.1 VTK output
The results are output in the partitioned vtu files, which are governed by
 a pvtu file. The data in the ghost cells of vtu files are overlapped.
An OGS utility, *RemoveGhostData*, is available to merge the partition vtu files into one vtu file,
meanwhile to eliminate the data overlapping. Here is an example to use that utility:
```
RemoveGhostData -i foo.pvtu -o foo.vtu
```
where the input file name is the name of pvtu file.

#### 3.2 XMDF output
With XMDF, ogs gives two files, one XMDF file and one HDF5 file with file name
 extension of `h5`. You can use ParaView to open the XDMF file by selecting
 `Xdmf3ReaderS` or `Xdmf3ReaderT`. The XMDF output is highly recommended for
  running OGS with a large mesh, especially on supercomputers.

