#=============================================================
#    DROPS parameter file for    TestMGSerPar
#    Testroutines for parallel multigrid serialization
#=============================================================

Refining {    
  BasicRefX  = 4          # number of basic refines in x direction
  BasicRefY  = 4          # number of basic refines in y direction
  BasicRefZ  = 4          # number of basic refines in z direction  
  orig       = -3.6e-3 0 -3.6e-3  # origin of the brick
  MarkAll    = __REFINE__  # in the case of InitCond==1 number of levels of serialized tetra
  MarkDrop   = 0
  MarkCorner = 0
  MarkingProc=-1          # -1 all procs should mark
}
Misc {
  Serialization = 1             # Not used yet: Number of planned serialization steps.
  Overwrite     = 1             # Not used yet: Overwrite existing output-files 
  SerDir        = ./geometry/   # Output directory
}
