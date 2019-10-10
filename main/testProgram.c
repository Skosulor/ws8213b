#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ws813b.h"
#include "colors.h"
#include "freertos/queue.h"

#define N_CONF     5
#define N_LEDS     120
#define N_SECTIONS 1
#define LED_PIN    4

void queueTestTask(void *param);

void app_main(){


  mode_config conf[N_CONF];
  // TODO add to README

  initModeConfigs(conf, N_CONF, N_LEDS, N_SECTIONS);
  initRmt(conf, LED_PIN);

  conf[0].walk.on                 = FALSE;
  conf[0].mirror.on               = FALSE;
  conf[0].fade.on                 = FALSE;
  conf[0].fadeWalk.on             = FALSE; //TODO change the func
  conf[0].cycleConfig.on          = FALSE;
  conf[0].music.mode1             = FALSE;
  conf[0].music.mode2             = FALSE;
  conf[0].mirror.sharedReflection = TRUE;
  conf[0].walk.dir                = FALSE;
  conf[0].simGrav.on              = TRUE;

  // TODO handle fade in cycleConfig
  conf[0].music.rate           = 1000;
  conf[0].mirror.mirrors       = 2;
  conf[0].smooth               = 10;
  conf[0].fadeWalk.rate        = 10; //TODO change the func
  conf[0].walk.rate            = 50;
  conf[0].cycleConfig.rate     = 0.5;
  conf[0].fade.iteration       = 63;
  conf[0].fade.rate            = 40;
  conf[0].fade.dir             = 1;
  conf[0].simGrav.rate         = 500;
  conf[0].simGrav.ledsPerMeter = 60;
  conf[0].simGrav.gravity      = 1.00;

  repeatModeZero(conf);

  conf[1].walk.on        = TRUE;
  conf[1].fade.on        = TRUE;
  conf[1].walk.dir       = TRUE;
  conf[1].fadeWalk.on    = FALSE;
  conf[1].music.mode1    = FALSE;
  conf[1].music.mode2    = FALSE;
  conf[1].mirror.mirrors = 2;

  /* conf[2].walk.on     = FALSE; */
  /* conf[2].fade.on     = TRUE; */
  /* conf[2].fadeWalk.on = FALSE; */
  /* conf[2].music.mode1 = FALSE; */
  /* conf[2].music.mode2 = FALSE; */

  /* conf[3].walk.on     = FALSE; */
  /* conf[3].fade.on     = TRUE; */
  /* conf[3].fadeWalk.on = FALSE; */
  /* conf[3].music.mode1 = FALSE; */
  /* conf[3].music.mode2 = FALSE; */

  /* conf[4].walk.on        = TRUE; */
  /* conf[4].fade.on        = TRUE; */
  /* conf[4].fadeWalk.on    = FALSE; */
  /* conf[4].music.mode1  = FALSE; */
  /* conf[4].music.mode2  = FALSE; */


  // TODO add to README // TODO rename and ability to choose specific config
  /* initColors(conf, testColors); */
  initColors(conf, &no_color);

  // TODO add to README, MIGHT BE REDUDANT???
  setSectionColors(conf[0]);

  TaskHandle_t xHandle = NULL;
  portBASE_TYPE x;

  x = xTaskCreate(ledEngineTask, "engine", 7000, (void *)conf,  2, xHandle);
  /* x = xTaskCreate(queueTestTask, "queueTEst", 5000, NULL,  3, xHandle); */
  printf("%d\n", x);
  vTaskDelay(1000);
  vTaskDelete(NULL);

}


void queueTestTask(void *param){
  mode_config *config;
    vTaskDelay(10000);
    config              = requestConfig();
    config[1].walk.on   = TRUE;
    config[2].walk.on   = TRUE;
    config[1].walk.rate = 5;
    config[2].walk.rate = 10;
    sendAck();
    vTaskDelete(NULL);
}
