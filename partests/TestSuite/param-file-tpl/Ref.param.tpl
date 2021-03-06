#=============================================================
#    DROPS parameter file for    TestParRef
#    Testroutines for refinement
#=============================================================

Brick {
  BasicRefX   = 4                   # number of basic refines in x direction
  BasicRefY   = 4                   # number of basic refines in y direction
  BasicRefZ   = 4                   # number of basic refines in z direction
  dim         = 1 1 1               # dx, dy, dz
  orig        = 0 0 0               # origin of the brick
}

Refining {
  InitCond    = 0                   # 0 create brick, 1 read serialized Tetra
  Refined     = 0                   # refinement rufe in the case of InitCond==1
  MarkAll     = __REFINE__          # in the case of InitCond==1 number of levels of serialized tetra
  MarkDrop    = 1
  MarkCorner  = 1
  MarkingProc = -1                  # -1 all procs should mark
  Strategy    = 3                   # 0 do refs like specified above, 1 Interpolrefs, 2 Read from file, 3 (RefAll, Unrefall)
}

Coarsening{
 CoarseDrop       = 1
 CoarseAll        = 1
 UnMarkingProc    = 1
}

LoadBalancing{
 RefineStrategy   = 1  # 0 no loadbalancing, 1 adaptive repart, 2 PartKWay
 CoarseStrategy   = 2  # 0 no loadbalancing, 1 adaptive repart, 2 PartKWay
 MiddleMig        = 0  # do migration between refinements and coarsening
}

Misc{
 PrintSize        = 1
 PrintPMG         = 0
 PrintGEO         = 0
 PrintTime        = 1   # display time-information
 CheckAfterRef    = 1   # Check multigrid after refinements
 CheckAfterMig    = 1   # Check multigrid after migrations
 CheckDDD         = 0   # Check for DDD-GCC (Global Consistency Check)
 InitPrefix       = ./geometry/
}
