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

#define SECTAB_OFFSET sizeof(Dino_Dhdr)

ssize_t read_sectab(int fd, Dino *dino) {
    /* clear any old data in the sectab */
    clear_sectab(&dino->sectab);

    /* allocate memory for sectab */
    realloc_sectab(&dino->sectab, dino->dhdr.section_count);
    if (!dino->sectab.shdr || !dino->sectab.sec) {
        return -ENOMEM;
    }

    /* read sectab data */
    Dino_SectabSize shdrsize = dino->dhdr.section_count * sizeof(Dino_Shdr);
    ssize_t nread = pread_retry(fd, dino->sectab.shdr, shdrsize, SECTAB_OFFSET);
    if (nread < shdrsize) {
        return -EIO;
    }

    /* If we have 64-bit stuff, grab those values */
    Dino_Size64 *sec64val = NULL;
    Dino_SectabSize sec64size = 0;
    if (dino->dhdr.encoding & DINO_ENCODING_SEC64) {
        sec64size = dino->dhdr.sectab_size - shdrsize;
        if (sec64size % 8 != 0) {
            return -EINVAL;
        }
        sec64val = malloc(sec64size);
        if (sec64val == NULL) {
            return -ENOMEM;
        }
        sec64size = sec64size >> 3; /* this is now the count of items in sec64val */
        ssize_t tread = pread_retry(fd, sec64val, sec64size, SECTAB_OFFSET+shdrsize);
        if (tread < sec64size) {
            free(sec64val);
            return -EIO;
        }
        nread += tread;
    }

    /* FIXME: byteswap if needed! */

    Dino_Size64 sec_offset = SECTAB_OFFSET + dino->dhdr.sectab_size + dino->dhdr.namtab_size;
    for (int i=0; i < dino->dhdr.section_count; i++) {
        //TODO: dino_sectab_append(&sechdrs[i]);
        Dino_Sec *s = &dino->sectab.sec[i];
        s->data = EMPTY_DATA_LIST;
        s->index = i;
        s->dino = dino;
        s->shdr = &dino->sectab.shdr[i];
        s->size = s->shdr->size;
        s->count = s->shdr->count;
        s->offset = sec_offset;
        /* Fix 64-bit values, if any */
        if (sec64val) {
            Dino_Size sec64idx = -1;
            if (DINO_SIZE_IS_64(s->size)) {
                sec64idx = DINO_SIZE64_IDX(s->size);
                s->size = (sec64idx < sec64size) ? sec64val[sec64idx] : DINO_SIZE64_INVALID;
            }
            if (DINO_SIZE_IS_64(s->count)) {
                sec64idx = DINO_SIZE64_IDX(s->count);
                s->count = (sec64idx < sec64size) ? sec64val[sec64idx] : DINO_SIZE64_INVALID;
            }
        }
        /* Now that we have the correct size we can update sec_offset */
        sec_offset += s->size;
    }
    dino->sectab.count = dino->dhdr.section_count;

    /* All finished! Clean up and return the number of bytes read. */
    free(sec64val);
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
    /* TODO: instantiate Dino_Data? */
    return dino;
}

Dino_Dhdr *get_dhdr(Dino *dino) {
    return &(dino->dhdr);
}
