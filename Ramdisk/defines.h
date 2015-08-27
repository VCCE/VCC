
//Speed throttling
#define FRAMEINTERVAL 120	//Number of frames to sum the framecounter over
#define TARGETFRAMERATE 60	//Number of throttled Frames per second to render

// Audio handling
#define BITRATE (LINESPERFIELD*TARGETFRAMERATE)
#define BLOCK_SIZE  LINESPERFIELD*2
#define BLOCK_COUNT 6

//CPU 
#define _894KHZ	57
#define JIFFIESPERLINE  (_894KHZ*4)
#define LINESPERFIELD 262

//Misc
#define MAX_LOADSTRING 100
#define QUERY 255
#define INDEXTIME ((LINESPERFIELD * TARGETFRAMERATE)/5)


//Common CPU defs
#define IRQ		1
#define FIRQ	2
#define NMI		3


