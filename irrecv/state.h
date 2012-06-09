
typedef struct ledstate {
    struct {
	int on:1;
	int run:1;
	int direction:1;
    } flags;

    int8_t toggle;
    int8_t led;
    unsigned long toggle_time;
    unsigned long delay_time;
} ledstate;
