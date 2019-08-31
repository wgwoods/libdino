# I "borrowed" this from libvarlink

BUILDDIR=builddir

all: $(BUILDDIR)
	ninja -C $(BUILDDIR)

config: $(BUILDDIR)

$(BUILDDIR):
	meson $@

check: $(BUILDDIR)
	meson test -C $(BUILDDIR) --wrap=valgrind

clean:
	rm -rf $(BUILDDIR)/

install: $(BUILDDIR)
	ninja -C $(BUILDDIR) install

fake-install: $(BUILDDIR)
	ninja -C $(BUILDDIR) -n install

.PHONY: all config check clean install fake-install
