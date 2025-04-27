#ifndef TWAI_H
#define TWAI_H

#define TWAI_OK					0
#define TWAI_ERROR              1
#define TWAI_ALLOC_ERROR        2
#define TWAI_INVALID_STATE      3
#define TWAI_QUEUE_FULL         4

typedef enum {
	TWAI_IN_BUS,
	TWAI_OUT_BUS,

	TWAI_BUS_COUNT
} TwaiBus_t;

typedef enum {
	TWAI_DEINIT,
	TWAI_INIT,
	TWAI_STOPPING,
	TWAI_RUNNING
} TwaiState_t;

typedef enum {
	TWAI_BAUD_25,
	TWAI_BAUD_50,
	TWAI_BAUD_100,
	TWAI_BAUD_125,
	TWAI_BAUD_250,
	TWAI_BAUD_500,
	TWAI_BAUD_800,
	TWAI_BAUD_1MB
} TwaiBaud_t;

typedef struct {
	TwaiBus_t			thisBus;
	TwaiBus_t			outBus;
	SemaphoreHandle_t	receiveTaskMutex;
	SemaphoreHandle_t	alertTaskMutex;
	SemaphoreHandle_t	busOffMutex;
	twai_handle_t		twaiHandle;
} TwaiBusData_t;

class CTwai {
public:
	static int					init();
	static int					deinit();

	static int					start(TwaiBaud_t baud);
	static int					stop();

	static int					send(TwaiBus_t bus, twai_message_t* msg);
	static inline TwaiBaud_t	getBaud() { return baudRate; }
	static const char*			baudString(TwaiBaud_t baud);
	static inline TwaiState_t	getCurrentState() { return currentState; }

private:
	static void					alertTask(void* arg);
	static void					receiveTask(void* arg);
	static twai_timing_config_t	getBaudConfig(TwaiBaud_t baud);

	static TwaiBusData_t		canStruct[TWAI_BUS_COUNT];

	static TwaiBaud_t			baudRate;
	static TwaiState_t			currentState;
};

extern CTwai Twai;

#endif