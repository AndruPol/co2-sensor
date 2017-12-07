/*
 *  DHT21/DHT22 driver
 */

#include "dht.h"

#include "ch.h"
#include "hal.h"

#define DHT_BIT_TIMEOUT_US 	(80 * 4) /* one bit timeout */
#define DHT_START_PULSE_MS 	1
#define DHT_PKT_TIMEOUT_MS 	10
#define DHT_PKT_SIZE 		5

#define ICUDRIVER			ICUD4

#define DHT_CHANNEL_GPIO	GPIOB 		/* channel 1 GPIO */
#define DHT_CHANNEL_PIN		GPIOB_DHT	/* channel 1 PIN */
#define DHT_CHANNEL			ICU_CHANNEL_1

#define DHT_PERIOD			80	// 80 uS
#define DHT_BYTE_START		60	// 60 uS with 2 byte
#define DHT_BIT_START		50	// 50 uS
#define DHT_BIT0			26	// 26-28 uS
#define DHT_BIT1			70	// 70 uS

#define ERROR_DIV 2
#define PERIOD_OK(x, l, h) \
	((x)->low >= ((l) - (l) / ERROR_DIV) && \
	(x)->low < ((l) + (l) / ERROR_DIV) && \
	((x)->period - (x)->low) >= ((h) - (h) / ERROR_DIV) && \
	((x)->period - (x)->low) < ((h) + (h) / ERROR_DIV))
/*-----------------------------------------------------------------------------*/

#define DHT_PRIO	(NORMALPRIO+2)

static thread_t *DHTThread_p;
static THD_WORKING_AREA(waDHTThread, 192);

typedef struct _icu_capture_t icu_capture_t;
struct _icu_capture_t {
	uint16_t period;
	uint16_t low;
};

typedef struct _dht_read_t dht_read_t;
struct _dht_read_t {
	dht_error_t error; 			/* out */
	uint8_t data[DHT_PKT_SIZE]; /* out */
};

static binary_semaphore_t icusem, cb_sem;
static volatile icu_capture_t icu_data;
static dht_read_t rd;

static void icuwidthcb(ICUDriver *icup) {
  icu_data.low = icuGetWidthX(icup);
}

static void icuperiodcb(ICUDriver *icup) {
  icu_data.period = icuGetPeriodX(icup);
  chSysLockFromISR();
  chBSemSignalI(&cb_sem);
  chSysUnlockFromISR();
}

static void icuoverflowcb(ICUDriver *icup) {
  (void)icup;
  icu_data.period = 0;
  chSysLockFromISR();
  chBSemSignalI(&cb_sem);
  chSysUnlockFromISR();
}

static ICUConfig icucfg = {
  ICU_INPUT_ACTIVE_LOW,
  1000000,                                    /* 1mHz ICU clock frequency.   */
  icuwidthcb,
  icuperiodcb,
  icuoverflowcb,
  DHT_CHANNEL,
  0
};

/*
 * DHT read thread.
 */
static THD_FUNCTION(DHTThread, arg) {
  (void)arg;
  chRegSetThreadName("DHTThd");

  chBSemObjectInit(&cb_sem, TRUE);
  while (TRUE) {
	/* wait for read request */
	dht_read_t *req;
	thread_t *tp;
	tp = chMsgWait();
	req = (dht_read_t *) chMsgGet(tp);
	chMsgRelease(tp, (msg_t) req);

	// set DHT pin low on 2ms
	palSetPadMode(DHT_CHANNEL_GPIO, DHT_CHANNEL_PIN, PAL_MODE_OUTPUT_OPENDRAIN);
	palClearPad(DHT_CHANNEL_GPIO, DHT_CHANNEL_PIN);

	chThdSleepMilliseconds(DHT_START_PULSE_MS);

	icuStart(&ICUDRIVER, &icucfg);
	palSetPadMode(DHT_CHANNEL_GPIO, DHT_CHANNEL_PIN, PAL_MODE_INPUT);
	icuStartCapture(&ICUDRIVER);
	icuEnableNotifications(&ICUDRIVER);

	// IRQ timeout or receive timeout
	if(chBSemWaitTimeout(&cb_sem, US2ST(DHT_BIT_TIMEOUT_US)) == MSG_TIMEOUT) {
		req->error = DHT_IRQ_TIMEOUT;
		goto reply;
	}

	if(!icu_data.period) {
		req->error = DHT_TIMEOUT;
		goto reply;
	}

	/* start sequence received */
	if(!PERIOD_OK(&icu_data, DHT_PERIOD, DHT_PERIOD)) {
		req->error = DHT_DECODE_ERROR;
		goto reply;
	}

	for (uint8_t i=0; i < DHT_PKT_SIZE; i++) {
		uint8_t mask = 0x80;
		uint8_t byte = 0;
		while(mask) {
			if(chBSemWaitTimeout(&cb_sem, US2ST(DHT_BIT_TIMEOUT_US)) == MSG_TIMEOUT) {
				req->error = DHT_IRQ_TIMEOUT;
				goto reply;
			}
			// with start second byte 1st low = 63..67uS
			uint8_t low = DHT_BIT_START;
			if (i > 0 && mask == 0x80)
				low = DHT_BYTE_START;
			/* next bit received */
			if(PERIOD_OK(&icu_data, low, DHT_BIT1)) {
				byte |= mask; /* 1 */
			} else if(!PERIOD_OK(&icu_data, low, DHT_BIT0)) {
				req->error = DHT_DECODE_ERROR;
				goto reply;
			}
			mask >>= 1;
		}
		req->data[i] = byte;
	}

	req->error = DHT_OK;

reply:
	icuDisableNotifications(&ICUDRIVER);
	icuStopCapture(&ICUDRIVER);
	icuStop(&ICUDRIVER);
	chBSemSignal(&icusem);
  } //while
}

dht_error_t dht_read(int16_t *temperature, uint16_t *humidity) {
	dht_read_t *rd_p = &rd;

	chBSemWait(&icusem); /* to be sure */

	chMsgSend(DHTThread_p, (msg_t) rd_p);

	/* wait for reply */
	if(chBSemWaitTimeout(&icusem, MS2ST(DHT_PKT_TIMEOUT_MS)) == MSG_TIMEOUT) {
		chBSemReset(&icusem, FALSE);
		return DHT_RCV_TIMEOUT;
	}
	chBSemReset(&icusem, FALSE);

	if(rd.error != DHT_OK) {
		return rd.error;
	}

	/* compute checksum */
	uint8_t checksum = 0;
	for(uint8_t i = 0; i < DHT_PKT_SIZE-1; i++)
		checksum += rd.data[i];
	if((checksum & 0xff) != rd.data[DHT_PKT_SIZE-1]) {
		return DHT_CHECKSUM_ERROR;
	}

	if (rd.data[1] == 0 && rd.data[3] == 0)
	{	// DHT11
		*humidity = ((uint16_t) rd.data[0]);
		*temperature = ((uint16_t)rd.data[2]);
	}
	else
	{	// DHT21/22
		/* read 16 bit humidity value */
		*humidity = ((uint16_t) rd.data[0] << 8) | rd.data[1];

		/* read 16 bit temperature value */
		int val = ((uint16_t) rd.data[2] << 8) | rd.data[3];

		*temperature = val & 0x8000 ? -(val & ~0x8000) : val;
	}
	return DHT_OK;
}

void dht_init(void){
//	AFIO->MAPR |= AFIO_MAPR_SWJ_CFG_JTAGDISABLE;

	chBSemObjectInit(&icusem, FALSE);
	DHTThread_p = chThdCreateStatic(waDHTThread, sizeof(waDHTThread), DHT_PRIO, DHTThread, NULL);
}
