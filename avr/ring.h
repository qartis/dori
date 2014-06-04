struct ring_t {
    uint8_t in;
    uint8_t out;
    uint8_t size;
    volatile uint8_t *buf;
};

static inline uint8_t ring_bytes(volatile struct ring_t *ring)
{
    if (ring->in > ring->out) {
        return (ring->in - ring->out);
    } else if (ring->out > ring->in) {
        return (ring->size - (ring->out - ring->in));
    } else {
        return 0;
    }
}

static inline uint8_t ring_space(volatile struct ring_t *ring)
{
    return ring->size - ring_bytes(ring);
}

static inline uint8_t ring_empty(volatile struct ring_t *ring)
{
    return ring->in == ring->out;
}

static inline uint8_t ring_next(uint8_t index, uint8_t size)
{
    index++;
    index %= size;
    return index;
}

static inline void ring_put(volatile struct ring_t *ring, uint8_t data)
{
    ring->buf[ring->in] = data;
    ring->in = ring_next(ring->in, ring->size);
}

static inline uint8_t ring_get(volatile struct ring_t *ring)
{
    uint8_t data = ring->buf[ring->out];
    ring->out = ring_next(ring->out, ring->size);
    return data;
}

static inline uint8_t ring_get_idx(volatile struct ring_t *ring, uint8_t index)
{
    index += ring->out;
    index %= ring->size;
    return ring->buf[index];
}

static inline void ring_drop(volatile struct ring_t *ring, uint8_t count)
{
    uint8_t index;

    index = ring->out;
    index += count;
    index %= ring->size;

    ring->out = index;
}

static inline struct ring_t ring_init(volatile uint8_t *buf, uint8_t size)
{
    struct ring_t ring;
    ring.in = 0;
    ring.out = 0;
    ring.size = size;
    ring.buf = buf;
    return ring;
}
