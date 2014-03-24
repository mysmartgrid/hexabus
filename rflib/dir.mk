DIRS += $(OBJDIR)/rflib

rflib_SRC := $(wildcard rflib/*.cpp)

rflib_OBJ := $(patsubst %.cpp,$(OBJDIR)/%.o,$(filter %.cpp,$(rflib_SRC)))

DEP_SRC += $(rflib_SRC)

$(OBJDIR)/librflib.a: $(rflib_OBJ)
	@echo "[AR]	" $@
	$V$(AR) -rcs $@ $^
