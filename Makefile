SUBDIRS = sas ssim

.PHONY: all
.PHONY: $(SUBDIRS) subdirs
.PHONY: sIDE

all: subdirs sIDE
	mkdir -p build
	cp sIDE/sIDE sas/sas.exe ssim/ssim.exe build/

subdirs: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@

sIDE:
	cd sIDE && \
	pwd && \
	qmake sIDE.pro;
	$(MAKE) -C sIDE
