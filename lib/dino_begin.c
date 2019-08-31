#include "common.h"
#include "system.h"
#include "libdino.h"

ssize_t read_dhdr(int fd, Dino *dino) {
    ssize_t nread = pread_retry(fd, &(dino->dhdr), sizeof(Dino_Dhdr), 0);
    if (nread < sizeof(Dino_Dhdr)) {
        return -EIO; /* FIXME: what's a good error code here */
    }
    /* TODO: byteswap if needed */
    return nread;
}

ssize_t read_sectab(int fd, Dino *dino) {
    Dino_Shdr *sechdrs = calloc(dino->dhdr.section_count, sizeof(Dino_Shdr));
    if (sechdrs == NULL) {
        return -ENOMEM;
    }
    ssize_t nread = pread_retry(fd, sechdrs, sizeof(Dino_Shdr) * dino->dhdr.section_count, sizeof(Dino_Dhdr));
    if (nread < sizeof(sechdrs)) {
        free(sechdrs);
        return -EIO;
    }
    /* FIXME: byteswap if needed */
    /* FIXME: check 64-bitness */
    /* if (nread < dino->dhdr.sectab_size) { .. */
    for (int i=0; i < dino->dhdr.section_count; i++) {
        //TODO: dino_sectab_append(&sechdrs[i]);
        dino->sectab.sec[i].index = i;
        dino->sectab.sec[i].shdr.s32 = &sechdrs[i];
    }
    dino->sectab.cnt = dino->dhdr.section_count;
    return nread;
}

ssize_t read_namtab(int fd, Dino *dino) {
    char *namtabdata = calloc(1, dino->dhdr.namtab_size);
    ssize_t nread = pread_retry(fd, namtabdata, dino->dhdr.namtab_size,
                                sizeof(Dino_Dhdr)+dino->dhdr.sectab_size);
    if (nread < dino->dhdr.namtab_size) {
        free(namtabdata);
        return -EIO;
    }
    dino->namtab.size = nread;
    dino->namtab.data = namtabdata;
    return nread;
}

Dino *read_dino(int fd) {
    ssize_t nr=0, t=0;
    Dino *dino = new_dino();
    dino->fd = fd;
    if ((nr = read_dhdr(fd, dino)) < 0) {
        return NULL;
    }
    t += nr;
    if ((nr = read_sectab(fd, dino)) < 0) {
        return NULL;
    }
    t += nr;
    if ((nr = read_namtab(fd, dino)) < 0) {
        return NULL;
    }
    t += nr;
    /* TODO: instantiate Dino_Data */
    return dino;
}

Dino_Dhdr *get_dhdr(Dino *dino) {
    return &(dino->dhdr);
}
