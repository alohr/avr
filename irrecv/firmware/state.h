
typedef struct ledstate {
    uint8_t on;
    uint8_t run;
    int8_t led;

    uint8_t toggle;
    unsigned long toggle_time;
} ledstate;
