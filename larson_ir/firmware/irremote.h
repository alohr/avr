#define ONKYO_RC_581S

#ifdef ONKYO_RC_581S

#define CHANNEL_1	0x4BC0AB54
#define CHANNEL_2	0x4BC06B94
#define CHANNEL_3	0x4BC0EB14
#define CHANNEL_4	0x4BC01BE4
#define CHANNEL_5	0x4BC09B64
#define CHANNEL_6	0x4BC05BA4
#define CHANNEL_7	0x4BC0DB24
#define CHANNEL_8	0x4BC03BC4
#define CHANNEL_9    	0x4BC0BB44
#define CHANNEL_DOWN 	0x4B20F807
#define CHANNEL_UP   	0x4B207887
#define VOLUME_UP    	0x4BC040BF
#define VOLUME_DOWN  	0x4BC0C03F
#define MODE	     	0x4B2010EF
#define ON_OFF	     	0x4B20D32C
#define SLEEP	     	0x4BC0BA45

#endif

#ifdef RM_130
enum {
    ON_OFF       = 0x0c,
    MUTE         = 0x0d,
    TV_AV        = 0x0b,
    VOLUME_UP    = 0x10,
    VOLUME_DOWN  = 0x11,
    CHANNEL_UP   = 0x20,
    CHANNEL_DOWN = 0x21
};
#endif
