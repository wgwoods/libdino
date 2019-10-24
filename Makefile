# I "borrowed" this from libvarlink

BUILDDIR=builddir
GITBUILD=gitbuild

all: $(BUILDDIR)
	ninja -C $(BUILDDIR)

config: $(BUILDDIR)

$(BUILDDIR):
	meson $(BUILDDIR)

gitbuild:
	git archive --format=tar --prefix=$@/ HEAD | tar -xf -
	make -C $@

check: $(BUILDDIR)
	meson test -C $(BUILDDIR)

valgrind-check: $(BUILDDIR)
	meson test -C $(BUILDDIR) --wrap=valgrind

clean:
	rm -rf $(BUILDDIR)/ $(GITBUILD)/

install: $(BUILDDIR)
	ninja -C $(BUILDDIR) install

fake-install: $(BUILDDIR)
	meson -C $(BUILDDIR) -v -n install

install-deps:
	sudo dnf install rpm-devel openssl-devel xz-devel libzstd-devel

.PHONY: all config check valgrind-check clean install fake-install gitbuild
