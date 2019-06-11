.SUFFIXES: .cpp .o

INC = -Iextern

.cpp.o:
	g++ $(INC) -g -c -O2 $<

main: \
	cldevice.o \
	clglobalbuffer.o \
	clkernelargument.o \
	cllink.o \
	cllocalbuffer.o \
	clplatform.o \
	clprogram.o \
	clprogramconstant.o \
	clsubprogram.o \
	clsystem.o \
	clvariable.o \
	csvwriter.o \
	geometry.o \
	isphsimulation.o \
	log.o \
	particle.o \
	probemanager.o \
	simulation.o \
	stdwriter.o \
	timer.o \
	utils.o \
	vec.o \
	vtkwriter.o \
	wcsphsimulation.o \
	writer.o \
	xmlloader.o \
	extern/clpp/clpp.o \
	extern/clpp/clppContext.o \
	extern/clpp/clppCount.o \
	extern/clpp/clppProgram.o \
	extern/clpp/clppScan_Default.o \
	extern/clpp/clppScan_GPU.o \
	extern/clpp/clppSort.o \
	extern/clpp/clppSort_BitonicSort.o \
	extern/clpp/clppSort_BitonicSortGPU.o \
	extern/clpp/clppSort_RadixSort.o \
	extern/clpp/clppSort_RadixSortGPU.o \
	extern/tinythread/tinythread.o \
	extern/pugixml/pugixml.o
	ar rcs ../lib/libisph.a \
	cldevice.o \
	clglobalbuffer.o \
	clkernelargument.o \
	cllink.o \
	cllocalbuffer.o \
	clplatform.o \
	clprogram.o \
	clprogramconstant.o \
	clsubprogram.o \
	clsystem.o \
	clvariable.o \
	csvwriter.o \
	geometry.o \
	isphsimulation.o \
	log.o \
	particle.o \
	probemanager.o \
	simulation.o \
	stdwriter.o \
	timer.o \
	utils.o \
	vec.o \
	vtkwriter.o \
	wcsphsimulation.o \
	writer.o \
	xmlloader.o \
	clpp.o \
	clppContext.o \
	clppCount.o \
	clppProgram.o \
	clppScan_Default.o \
	clppScan_GPU.o \
	clppSort.o \
	clppSort_BitonicSort.o \
	clppSort_BitonicSortGPU.o \
	clppSort_RadixSort.o \
	clppSort_RadixSortGPU.o \
	tinythread.o \
	pugixml.o
	tar -cvf ../lib/isph.cta --exclude .svn wcsph isph general kernels scene integrators
	ln ../lib/isph.cta ../bin/isph.cta

clean:
	rm -f *.o *~ ../bin/isph.cta ../lib/isph.cta ../lib/libisph.a
