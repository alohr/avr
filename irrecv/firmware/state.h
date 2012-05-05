
typedef struct ledstate {
    struct {
	uint8_t on:1;
	uint8_t run:1;
	uint8_t direction:1;
    } flags;

    int8_t toggle;
    int8_t led;
    unsigned long toggle_time;
    unsigned long delay_time;
} ledstate;
