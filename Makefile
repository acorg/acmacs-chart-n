# -*- Makefile -*-
# Eugene Skepner 2017
# ----------------------------------------------------------------------

MAKEFLAGS = -w

# ----------------------------------------------------------------------

TARGETS = \
    $(ACMACS_CHART_LIB) \
    $(DIST)/chart-info \
    $(DIST)/chart-names \
    $(DIST)/chart-plot-spec \
    $(DIST)/chart-layout \
    $(DIST)/chart-convert \
    $(DIST)/chart-modify-projection \
    $(DIST)/chart-modify-plot-spec \
    $(DIST)/chart-titer-merging-report \
    $(DIST)/chart-titer-merging-from-layers \
    $(DIST)/chart-stress \
    $(DIST)/chart-transformation \
    $(DIST)/chart-relax-test \
    $(DIST)/chart-relax \
    $(DIST)/chart-relax-existing \
    $(DIST)/chart-relax-disconnected \
    $(DIST)/chart-error-lines \
    $(DIST)/chart-common \
    $(DIST)/chart-procrustes \
    $(DIST)/chart-degradation-resolver \
    $(DIST)/chart-html \
    $(DIST)/chart-serum-circles \
    $(DIST)/chart-update \
    $(DIST)/chart-remove-antigens-sera \
    $(DIST)/chart-remove-antigens-by-full-name \
    $(DIST)/chart-keep-antigens-sera \
    $(DIST)/chart-keep-antigens-titrated-against-sera \
    $(DIST)/chart-grid-test \
    $(DIST)/test-titer-iterator \
    $(DIST)/test-chart-modify \
    $(DIST)/test-chart-create-from-scratch \
    $(DIST)/test-chart-from-text \
    $(DIST)/chart-serum-titers-check \
    $(DIST)/chart-column-bases \
    $(DIST)/chart-list-antigens-without-titers

SOURCES = chart.cc titers.cc column-bases.cc bounding-ball.cc stress.cc optimize.cc randomizer.cc \
    rjson-import.cc \
    factory-import.cc ace-import.cc acd1-import.cc lispmds-import.cc lispmds-token.cc \
    factory-export.cc ace-export.cc lispmds-export.cc lispmds-encode.cc chart-modify.cc \
    common.cc procrustes.cc grid-test.cc serum-line.cc

ALGLIB = alglib-3.13.0
ALGLIB_SOURCES = optimization.cpp ap.cpp alglibinternal.cpp linalg.cpp alglibmisc.cpp \
    dataanalysis.cpp statistics.cpp specialfunctions.cpp solvers.cpp
ALGLIB_CXXFLAGS = -DAE_COMPILE_MINLBFGS -DAE_COMPILE_PCA -DAE_COMPILE_MINCG -g -MMD -O3 -mfpmath=sse -mtune=intel -fPIC -std=c++11 -Icc -Wall
ifeq ($(CXX_TYPE),gcc)
ALGLIB_CXXFLAGS += -Wno-maybe-uninitialized
endif

# ----------------------------------------------------------------------

include $(ACMACSD_ROOT)/share/makefiles/Makefile.g++
include $(ACMACSD_ROOT)/share/makefiles/Makefile.dist-build.vars

ACMACS_CHART_LIB_MAJOR = 2
ACMACS_CHART_LIB_MINOR = 0
ACMACS_CHART_LIB = $(DIST)/$(call shared_lib_name,libacmacschart,$(ACMACS_CHART_LIB_MAJOR),$(ACMACS_CHART_LIB_MINOR))

CXXFLAGS += -Icc $(PKG_INCLUDES)
# ifneq ($(wildcard $(AD_INCLUDE)/acmacs-base/pch.$(shell uname).pch),)
#   CXXFLAGS += -include $(AD_INCLUDE)/acmacs-base/pch.$(shell uname).hh -verify-pch
# endif
# CXX := gtime $(CXX)

LDLIBS = \
	$(AD_LIB)/$(call shared_lib_name,libacmacsbase,1,0) \
	$(AD_LIB)/$(call shared_lib_name,liblocationdb,1,0) \
	$$(pkg-config --libs liblzma) -lbz2 $(CXX_LIB)

PKG_INCLUDES = $(shell pkg-config --cflags liblzma)

# ----------------------------------------------------------------------

all: check-acmacsd-root $(TARGETS)

install: check-acmacsd-root install-headers $(TARGETS)
	$(call install_lib,$(ACMACS_CHART_LIB))
	ln -sf $(abspath dist)/chart-* $(AD_BIN)
	ln -sf $(abspath bin)/chart-* $(AD_BIN)

test: install
	test/test

# ----------------------------------------------------------------------

-include $(BUILD)/*.d
include $(ACMACSD_ROOT)/share/makefiles/Makefile.dist-build.rules
include $(ACMACSD_ROOT)/share/makefiles/Makefile.rtags

# ----------------------------------------------------------------------

$(ACMACS_CHART_LIB): $(patsubst %.cc,$(BUILD)/%.o,$(SOURCES)) $(patsubst %.cpp,$(BUILD)/%.o,$(ALGLIB_SOURCES)) | $(DIST) $(LOCATION_DB_LIB)
	@printf "%-16s %s\n" "SHARED" $@
	@$(call make_shared,libacmacschart,$(ACMACS_CHART_LIB_MAJOR),$(ACMACS_CHART_LIB_MINOR)) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(DIST)/%: $(BUILD)/%.o | $(ACMACS_CHART_LIB)
	@printf "%-16s %s\n" "LINK" $@
	@$(CXX) $(LDFLAGS) -o $@ $^ $(ACMACS_CHART_LIB) $(LDLIBS) $(AD_RPATH)

$(BUILD)/%.o: cc/$(ALGLIB)/%.cpp | $(BUILD) install-headers
	@echo $(CXX_NAME) $(ALGLIB_CXXFLAGS) $<
	@$(CXX) $(ALGLIB_CXXFLAGS) -c -o $@ $(abspath $<)

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
