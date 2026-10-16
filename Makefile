SHELL := /usr/bin/bash
.DEFAULT_GOAL := help

PROJECT_NAME := rfsoc_waveform_tx_rx
BUILD_DIR ?= $(CURDIR)/build/vivado
PROJECT_XPR := $(BUILD_DIR)/$(PROJECT_NAME).xpr
DDR_IP_REPO ?= $(CURDIR)/ip_repo/ddr_bram_bridge_1_0
JOBS ?= 8

.PHONY: help check hls stage-ip vivado-project verify bitstream pynq pynq-golden ddr-project rebuild-base

help:
	@echo "Available targets:"
	@echo "  make check           Validate the portable source tree"
	@echo "  make hls             Run HLS C simulation, synthesis, and IP packaging"
	@echo "  make stage-ip        Expand regenerated HLS packages for review"
	@echo "  make vivado-project  Recreate and validate the editable Vivado project"
	@echo "  make verify          Re-run assertions on the recreated block design"
	@echo "  make bitstream       Implement and export BIT/HWH/XSA into output/"
	@echo "  make pynq            Package output/BIT+HWH and Python files into dist/pynq/"
	@echo "  make pynq-golden     Package the checked-in known-good base overlay"
	@echo "  make ddr-project     Apply the optional DDR bridge to the generated BD"
	@echo "  make rebuild-base    Recreate base Vivado project, bitstream, and PYNQ bundle"

check:
	@./scripts/check_project.sh

hls: check
	@./scripts/rebuild_hls.sh

stage-ip:
	@./scripts/stage_generated_ip_repo.sh

vivado-project: check
	@if [[ -f "$(PROJECT_XPR)" ]]; then \
		echo "Vivado project already exists: $(PROJECT_XPR)"; \
		echo "Use a different BUILD_DIR or intentionally remove the old generated build."; \
	else \
		vivado -mode batch -source vivado/create_project.tcl -tclargs "$(BUILD_DIR)"; \
	fi

verify:
	@test -f "$(PROJECT_XPR)" || { echo "Missing project: $(PROJECT_XPR)" >&2; exit 1; }
	@vivado -mode batch -source vivado/verify_project.tcl -tclargs "$(PROJECT_XPR)"

bitstream: vivado-project
	@vivado -mode batch -source vivado/build_bitstream.tcl \
		-tclargs "$(PROJECT_XPR)" "$(JOBS)"

pynq:
	@./scripts/package_pynq.sh output dist/pynq

pynq-golden:
	@./scripts/package_pynq.sh reference/known_good dist/pynq-golden

ddr-project: vivado-project
	@test -f "$(DDR_IP_REPO)/component.xml" || { \
		echo "Missing DDR IP repository: $(DDR_IP_REPO)" >&2; \
		exit 1; \
	}
	@vivado -mode batch -source vivado/apply_ddr_to_bram.tcl \
		-tclargs "$(PROJECT_XPR)" "$(DDR_IP_REPO)"

rebuild-base:
	@$(MAKE) check
	@$(MAKE) vivado-project
	@$(MAKE) bitstream
	@$(MAKE) pynq
