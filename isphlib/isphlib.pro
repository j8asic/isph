TEMPLATE = lib
CONFIG += staticlib
CONFIG -= qt
TARGET = isph

QMAKE_CC = clang
QMAKE_CXX = clang++

LIBS = -stdlib=libc++ -lc++abi -lpthread -lOpenCL
QMAKE_CXXFLAGS = -stdlib=libc++ -std=c++11
QMAKE_CXXFLAGS_RELEASE = -O3
QMAKE_CXXFLAGS_DEBUG = -O0 -g

INCLUDEPATH += /usr/include/c++/v1 ./extern

HEADERS += \ 
    extern/clpp/clpp.h \
    extern/clpp/clppContext.h \
    extern/clpp/clppCount.h \
    extern/clpp/clppProgram.h \
    extern/clpp/clppScan.h \
    extern/clpp/clppScan_Default.h \
    extern/clpp/clppScan_Default_CLKernel.h \
    extern/clpp/clppScan_GPU.h \
    extern/clpp/clppScan_GPU_CLKernel.h \
    extern/clpp/clppSort.h \
    extern/clpp/clppSort_BitonicSort.h \
    extern/clpp/clppSort_BitonicSort_CLKernel.h \
    extern/clpp/clppSort_BitonicSortGPU.h \
    extern/clpp/clppSort_BitonicSortGPU_CLKernel.h \
    extern/clpp/clppSort_RadixSort.h \
    extern/clpp/clppSort_RadixSort_CLKernel.h \
    extern/clpp/clppSort_RadixSortGPU.h \
    extern/clpp/clppSort_RadixSortGPU_CLKernel.h \
    extern/pugixml/pugiconfig.hpp \
    extern/pugixml/pugixml.hpp \
    extern/tinythread/tinythread.h \
    bodyforcewriter.h \
    cldevice.h \
    clglobalbuffer.h \
    clkernelargument.h \
    cllink.h \
    cllocalbuffer.h \
    clplatform.h \
    clprogram.h \
    clprogramconstant.h \
    clsubprogram.h \
    clsystem.h \
    clvariable.h \
    csvwriter.h \
    geometry.h \
    isph.h \
    isphsimulation.h \
    loader.h \
    log.h \
    particle.h \
    probemanager.h \
    simulation.h \
    stdwriter.h \
    timer.h \
    utils.h \
    vec.h \
    version.h \
    vtkwriter.h \
    wcsphsimulation.h \
    writer.h \
    xmlloader.h

SOURCES += \ 
    extern/clpp/clpp.cpp \
    extern/clpp/clppContext.cpp \
    extern/clpp/clppCount.cpp \
    extern/clpp/clppProgram.cpp \
    extern/clpp/clppScan_Default.cpp \
    extern/clpp/clppScan_GPU.cpp \
    extern/clpp/clppSort.cpp \
    extern/clpp/clppSort_BitonicSort.cpp \
    extern/clpp/clppSort_BitonicSortGPU.cpp \
    extern/clpp/clppSort_RadixSort.cpp \
    extern/clpp/clppSort_RadixSortGPU.cpp \
    extern/pugixml/pugixml.cpp \
    extern/tinythread/tinythread.cpp \
    bodyforcewriter.cpp \
    cldevice.cpp \
    clglobalbuffer.cpp \
    clkernelargument.cpp \
    cllink.cpp \
    cllocalbuffer.cpp \
    clplatform.cpp \
    clprogram.cpp \
    clprogramconstant.cpp \
    clsubprogram.cpp \
    clsystem.cpp \
    clvariable.cpp \
    csvwriter.cpp \
    geometry.cpp \
    isphsimulation.cpp \
    log.cpp \
    particle.cpp \
    probemanager.cpp \
    simulation.cpp \
    stdwriter.cpp \
    timer.cpp \
    utils.cpp \
    vec.cpp \
    vtkwriter.cpp \
    wcsphsimulation.cpp \
    writer.cpp \
    xmlloader.cpp

OTHER_FILES += \
    general/max_vel.cl \
    general/obj_pos.cl \
    general/obj_pos_vel.cl \
    general/obj_vel.cl \
    general/types.cl \
    integrators/wcsph_corrector.cl \
    integrators/wcsph_euler.cl \
    integrators/wcsph_predictor.cl \
    integrators/wcsph_rkstep41.cl \
    integrators/wcsph_rkstep42.cl \
    integrators/wcsph_rkstep43.cl \
    integrators/wcsph_rkstep44.cl \
    isph/bicgstab_update_conjugate_0.cl \
    isph/bicgstab_update_conjugate_1.cl \
    isph/bicgstab_update_result.cl \
    isph/build_rhs.cl \
    isph/calc_volumes.cl \
    isph/cg_update_conjugate.cl \
    isph/cg_update_result.cl \
    isph/correct.cl \
    isph/div_vel.cl \
    isph/dot.cl \
    isph/dummy_scalar_copy.cl \
    isph/dummy_vector_copy.cl \
    isph/fix_pressure.cl \
    isph/shifting.cl \
    isph/shifting_update.cl \
    isph/spmv_product.cl \
    isph/temp_positions.cl \
    isph/temp_velocities.cl \
    kernels/correction.cl \
    kernels/cubic.cl \
    kernels/delta_p.cl \
    kernels/gauss.cl \
    kernels/gaussmod.cl \
    kernels/quadratic.cl \
    kernels/quintic.cl \
    kernels/wendland.cl \
    scene/grid_cellids.cl \
    scene/grid_cellstart.cl \
    scene/grid_clear.cl \
    scene/grid_utils.cl \
    scene/out_of_bounds.cl \
    wcsph/acceleration.cl \
    wcsph/accelerations_colagrossi.cl \
    wcsph/cfl.cl \
    wcsph/continuity.cl \
    wcsph/continuity_colagrossi.cl \
    wcsph/init.cl \
    wcsph/MLS_post.cl \
    wcsph/MLS_pre.cl \
    wcsph/read_probes_scalar.cl \
    wcsph/read_probes_vector.cl \
    wcsph/set_masses.cl \
    wcsph/shepard_filter.cl \
    wcsph/tait_eos.cl \
    wcsph/tait_eos_inv.cl
