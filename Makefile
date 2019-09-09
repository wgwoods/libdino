# I "borrowed" this from libvarlink

BUILDDIR=builddir

all: $(BUILDDIR)
	ninja -C $(BUILDDIR)

config: $(BUILDDIR)

$(BUILDDIR):
	meson $(BUILDDIR)

check: $(BUILDDIR)
	meson test -C $(BUILDDIR)

valgrind-check: $(BUILDDIR)
	meson test -C $(BUILDDIR) --wrap=valgrind

clean:
	rm -rf $(BUILDDIR)/

install: $(BUILDDIR)
	ninja -C $(BUILDDIR) install

fake-install: $(BUILDDIR)
	meson -C $(BUILDDIR) -v -n install

.PHONY: all config check valgrind-check clean install fake-install
