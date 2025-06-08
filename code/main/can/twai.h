#ifndef TWAI_H
#define TWAI_H

typedef enum {
	TWAI_OK,
	TWAI_ERROR,
	TWAI_ALLOC_ERROR,
	TWAI_INVALID_STATE,
	TWAI_QUEUE_FULL
} TwaiReturn_t;

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

typedef void (*CBTwaiMessageReceived)(TwaiBus_t thisBus, TwaiBus_t outBus, twai_message_t* msg);

typedef struct {
	const TwaiBus_t				thisBus;
	const TwaiBus_t				outBus;
	const twai_general_config_t twaiConfig;
	SemaphoreHandle_t			receiveTaskMutex;
	SemaphoreHandle_t			alertTaskMutex;
	SemaphoreHandle_t			busOffMutex;
	twai_handle_t				twaiHandle;
	CBTwaiMessageReceived		onMessageReceived;
} TwaiBusData_t;

class CTwai {
public:
	static TwaiReturn_t			init();
	static TwaiReturn_t			deinit();

	static TwaiReturn_t			start(TwaiBaud_t baud);
	static TwaiReturn_t			stop();

	static void					setBusCallback(TwaiBus_t bus, CBTwaiMessageReceived cb) { canBus[bus].onMessageReceived = cb; }

	static TwaiReturn_t			send(TwaiBus_t bus, twai_message_t* msg);
	static inline TwaiBaud_t	getBaud() { return baudRate; }
	static const char*			baudString(TwaiBaud_t baud);
	static inline TwaiState_t	getCurrentState() { return currentState; }

private:
	static void					alertTask(void* arg);
	static void					receiveTask(void* arg);
	static twai_timing_config_t	getBaudConfig(TwaiBaud_t baud);

	static TwaiBusData_t		canBus[TWAI_BUS_COUNT];

	static TwaiBaud_t			baudRate;
	static TwaiState_t			currentState;
};

extern CTwai Twai;

#endif