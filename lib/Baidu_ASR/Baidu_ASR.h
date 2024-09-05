#ifndef BAIDU_ASR_H
#define BAIDU_ASR_H

#include"stdio.h"

void IRAM_ATTR onTimer();
void gain_token(void);
String Json_msg_baidu(char data[],hw_timer_t *timer);

#endif // BAIDU_ASR_H