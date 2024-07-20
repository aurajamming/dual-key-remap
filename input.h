#ifndef INPUT_H
#define INPUT_H

//#define DEBUG(cond, x) do { if ((cond & 1 || cond & 0) && g_debug) { x;} } while (0)
#define DEBUG(cond, x)

// A semi random value used to identify inputs generated
// by Dual Key Remap. Ideally high to minimize chances of a collision
// with a real pointer used by another application.
// Note: This approach is what AHK used, we should a different key id
// from them to avoid collisions.
#define INJECTED_KEY_ID (0xFFC3CED7 & 0xFFFFFF00)

#define INPUT_BUFFER_SIZE 16 // power of 2
#define INPUT_BUFFER_MASK (INPUT_BUFFER_SIZE-1)
#define RESET       "\033[0m"
#define RED         "\033[31m"
#define GREEN       "\033[32m"
#define YELLOW      "\033[33m"
#define BLUE        "\033[34m"
#define MAGENTA     "\033[35m"
#define CYAN        "\033[36m"
#define WHITE       "\033[37m"

union __declspec(align(8)) rte_ring_hts_headtail {
    volatile LONG64 raw;
    struct {
        volatile uint32_t head;
        volatile uint32_t tail;
    } pos;
};

struct InputBuffer {
    INPUT inputs[INPUT_BUFFER_SIZE + INPUT_BUFFER_SIZE-2];
    volatile union rte_ring_hts_headtail prod;
    volatile union rte_ring_hts_headtail cons;
};

static inline void input_buffer_init(struct InputBuffer * input_buffer) {
    //ZeroMemory(input_buffer->inputs, sizeof(input_buffer->inputs));
    input_buffer->prod.raw = 0;
    input_buffer->cons.raw = 0;
}

static inline uint32_t input_buffer_move_prod_head(struct InputBuffer * input_buffer, uint32_t * old_head) {
    union rte_ring_hts_headtail new, old;
    do {
        do {
            old = input_buffer->prod;
        } while (old.pos.head != old.pos.tail);
        if ((INPUT_BUFFER_SIZE - 1 + input_buffer->cons.pos.tail - old.pos.head) & INPUT_BUFFER_MASK == 0)
            return 0;
        new.pos.tail = old.pos.tail;
        new.pos.head = old.pos.head + 1;
    } while (InterlockedCompareExchange64(&input_buffer->prod.raw, new.raw, old.raw) != old.raw);
    *old_head = old.pos.head;
    return 1;
}

static inline uint32_t input_buffer_move_cons_head(struct InputBuffer * input_buffer, int num, uint32_t * old_head) {
    union rte_ring_hts_headtail new, old;
    uint32_t n, ncont;
    do {
        do {
            old = input_buffer->cons;
        } while (old.pos.head != old.pos.tail);
        n = (input_buffer->prod.pos.tail - old.pos.head) & INPUT_BUFFER_MASK;
        if (num < 0) {
            ncont = (INPUT_BUFFER_SIZE - old.pos.head) & INPUT_BUFFER_MASK;
            if (n > ncont && ncont > 0) {
                if (num < -1) {
                    CopyMemory(&input_buffer->inputs[INPUT_BUFFER_SIZE],
                               &input_buffer->inputs[0], (n-ncont)*sizeof(INPUT));
                } else {
                    n = ncont;
                }
            }
        } else if (n > num) {
            n = num;
        }
        if (n == 0) return 0;
        new.pos.tail = old.pos.tail;
        new.pos.head = old.pos.head + n;
    } while (InterlockedCompareExchange64(&input_buffer->cons.raw, new.raw, old.raw) != old.raw);
    *old_head = old.pos.head;
    return n;
}

static inline void input_buffer_update_tail(volatile union rte_ring_hts_headtail * ht, uint32_t old_tail, uint32_t n) {
    ht->pos.tail = old_tail + n;
}

static inline uint32_t input_buffer_count(struct InputBuffer * input_buffer) {
    return (input_buffer->prod.pos.tail - input_buffer->cons.pos.tail) & INPUT_BUFFER_MASK;
}

static inline uint32_t input_buffer_free_count(struct InputBuffer * input_buffer) {
    return INPUT_BUFFER_SIZE - 1 - input_buffer_count(input_buffer);
}

static inline int input_buffer_full(struct InputBuffer * input_buffer) {
    return input_buffer_free_count(input_buffer) == 0;
}

static inline uint32_t input_buffer_empty(struct InputBuffer * input_buffer) {
    return input_buffer->prod.pos.tail == input_buffer->cons.pos.tail;
}

enum Direction {
    UP,
    DOWN,
};

void debug_print(const char* color, const char* format, ...);
void send_input(int scan_code, int virt_code, enum Direction direction, int remap_id, struct InputBuffer *input_buffer);
void rehook();

#endif
