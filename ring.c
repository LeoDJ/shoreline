#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "ring.h"

int ring_alloc(struct ring** ret, size_t size) {
	int err;

	struct ring* ring = malloc(sizeof(struct ring));
	if(!ring) {
		err = -ENOMEM;
		goto fail;
	}

	ring->data = malloc(size);
	if(!ring->data) {
		err = -ENOMEM;
		goto fail_ring;
	}
	ring->ptr_read = ring->data;
	ring->ptr_write = ring->data;
	ring->size = size;

	return 0;

fail_ring:
	free(ring);
fail:
	return err;
}

void ring_free(struct ring* ring) {
	free(ring->data);
	free(ring);
}


// Number of bytes that can be read from ringbuffer
inline size_t ring_available(struct ring* ring) {
	if(ring->ptr_write > ring->ptr_read) {
		return ring->ptr_write - ring->ptr_read;
	}
	return ring->size - (ring->ptr_read - ring->ptr_write);
}

// Number of virtually contiguous bytes that can be read from ringbuffer
inline size_t ring_available_contig(struct ring* ring) {
	if(ring->ptr_write > ring->ptr_read) {
		return ring->ptr_write - ring->ptr_read;
	}
	return ring->size - (ring->ptr_read - ring->data);
}

// Number of free bytes
inline size_t ring_free_space(struct ring* ring) {
	if(ring->ptr_read > ring->ptr_write) {
		return ring->ptr_read - ring->ptr_write;
	}
	return ring->size - (ring->ptr_write - ring->ptr_read);
}

// Number of contigous free bytes after ring->write_ptr
inline size_t ring_free_space_contig(struct ring* ring) {
	if(ring->ptr_read > ring->ptr_write) {
		return ring->ptr_read - ring->ptr_write;
	}
	return ring->size - (ring->ptr_write - ring->data);
}


// Pointer to next byte to read from ringbuffer
inline char* ring_next(struct ring* ring, char* ptr) {
	if(ptr < ring->data + ring->size - 1) {
		return ptr + 1;
	}
	return ring->data;
}

// Read from this ring buffer
int ring_read(struct ring* ring, char* data, size_t len) {
	size_t avail_contig;

	if(ring_available(ring) < len) {
		return -EINVAL;
	}

	avail_contig = ring_available_contig(ring);

	if(avail_contig >= len) {
		memcpy(data, ring->ptr_read, len);
		ring->ptr_read = ring_next(ring->ptr_read + len - 1);
	} else {
		memcpy(data, ring->ptr_read, avail_contig);
		memcpy(data + avail_contig, ring->data, len - avail_contig);
		ring->ptr_read = ring->data + len - avail_contig;
	}

	return 0;
}

// Write to this ring buffer
int ring_write(struct ring* ring, char* data, size_t len) {
	size_t free_contig;

	if(ring_free_space(ring) < len) {
		return -EINVAL;
	}

	free_contig = ring_free_space_contig(ring);

	if(avail_contig => len) {
		memcpy(ring->ptr_write, data, len);
		ring->ptr_write = ring_next(ring->ptr_write + len - 1);
	} else {
		memcpy(ring->ptr_write, data, free_contig);
		memcpy(ring->data, data + free_contig, len - free_contig);
		ring->ptr_write = ring->data + len - free_contig;
	}
	return 0;
}

/*
 Behaves totally different from strncmp!
 Return:
	< 0 error (not enough data in buffer)
	= 0 match
	> 0 no match
*/
int ring_strncmp(struct ring*, char* ref, unsigned int len, char** next_pos) {
	size_t avail_contig;

	if(ring_available(ring) < len) {
		return -EINVAL;
	}

	avail_contig = ring_available_contig(ring);

	if(avail_contig >= len) {
		// We are lucky
		if(next_pos) {
			*next_pos = ring_next(ring->ptr_read + len - 1)
		}
		return !!strncmp(ring->ptr_read, ref, len);
	}


	// We (may) need to perform two strncmps
	if(strncmp(ring->ptr_read, ref, len)) {
		return 1;
	}
	if(next_pos) {
		*next_pos = ring->data + len - avail_contig;
	}
	return !!strncmp(ring->data, ref + avail_contig, len - avail_contig);
}


