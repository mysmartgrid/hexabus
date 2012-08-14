BUILDDIR = build
CMAKECMD = cmake ../
CMAKEOUTPUT = $(BUILDDIR)/Makefile
MFLAGS = -s

all: $(CMAKEOUTPUT)
	@cd $(BUILDDIR) && $(MAKE) $(MFLAGS)

plot: $(CMAKEOUTPUT) all
	@cd $(BUILDDIR) && ./src/ev-simanealing 
	@cd $(BUILDDIR) && Rscript ../tools/plot-all.R

plotref: $(CMAKEOUTPUT) all
	@cd $(BUILDDIR) && ./src/ev-enumerate 
	@cd $(BUILDDIR) && Rscript ../tools/plot-all.R
# No tests for now
#test: all
#	@cd $(BUILDDIR) && $(MAKE) $(MFLAGS) test

$(CMAKEOUTPUT): $(BUILDDIR)
	@cd $(BUILDDIR) && $(CMAKECMD) -DCMAKE_BUILD_TYPE=Debug

release: $(BUILDDIR)
	@cd $(BUILDDIR) && $(CMAKECMD) -DCMAKE_BUILD_TYPE=Release
	@cd $(BUILDDIR) && $(MAKE) $(MFLAGS) package

install: $(BUILDDIR)
	@cd $(BUILDDIR) && $(CMAKECMD) -DCMAKE_BUILD_TYPE=Release
	@cd $(BUILDDIR) && $(MAKE) $(MFLAGS) install

$(BUILDDIR):
	@mkdir $(BUILDDIR)

clean:
	@cd $(BUILDDIR) && $(MAKE) $(MFLAGS) clean

distclean:
	@rm -rf $(BUILDDIR)

bindist:
	(cd ${BUILDDIR}; cpack)

.PHONY: clean distclean
