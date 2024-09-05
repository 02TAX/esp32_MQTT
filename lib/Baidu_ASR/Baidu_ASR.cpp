#include <HTTPClient.h>
#include <base64.h>
#include "Baidu_ASR.h"

#define data_len 16000
#define ADC 39 //ADC引脚定义，一般在与WIFI同时使用的时候要用ADC1

HTTPClient http_client;

uint8_t adc_start_flag = 0;       //开始标志
uint8_t adc_complete_flag = 0;    //完成标志

uint16_t adc_data[data_len];    //16000个数据，8K采样率，即2秒，录音时间为2秒，想要实现更长时间的语音识别，就要改这个数组大小
                                //和下面data_json数组的大小，改大一些。

uint32_t num=0;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR onTimer()//定时器中断函数
{
  // Increment the counter and set the time of ISR
    portENTER_CRITICAL_ISR(&timerMux);
    if(adc_start_flag==1)
    {
        adc_data[num]=analogRead(ADC);
        num++;
        if(num>=data_len)
        {
            adc_complete_flag=1;
            adc_start_flag=0;
            num=0;
            //Serial.println(Complete_flag);
        }
    }
    portEXIT_CRITICAL_ISR(&timerMux);
}

void gain_token(void)   //代码语音识别获取token
{
    int httpCode;
    //注意，要把下面网址中的your_apikey和your_secretkey替换成自己的API Key和Secret Key
    http_client.begin("https://aip.baidubce.com/oauth/2.0/token?grant_type=client_credentials&client_id=bUmjzixadxAujrGCBmDdGw82&client_secret=D59tGpGtmEHqIAdK5kwIdGGWT5QcLuMK");
    httpCode = http_client.GET();
    Serial.println("hello!!Here is Http!");
    if(httpCode > 0) {
        if(httpCode == HTTP_CODE_OK) {
            String payload = http_client.getString();
            Serial.println(payload);
        }
    }
    else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http_client.errorToString(httpCode).c_str());
    }
    http_client.end();
}

String Json_msg_baidu(char data[],hw_timer_t *timer){

    int httpCode;
    String payload;

    Serial.printf("Start recognition\r\n\r\n");
    adc_start_flag=1;
    timerStart(timer);

    //   time1=micros();
    while(!adc_complete_flag)  //等待采集完成
    {
    ets_delay_us(10);
    // Serial.printf("waiting!!\r\n\r\n");
    }
    // time2=micros()-time1;

    timerStop(timer);
    adc_complete_flag=0;        //清标志

    // Serial.printf("采集完成\r\n\r\n");
    memset(data,'\0',strlen(data));   //将数组清空
    strcat(data,"{");
    strcat(data,"\"format\":\"pcm\",");
    strcat(data,"\"rate\":8000,");         //采样率    如果采样率改变了，记得修改该值，只有16000、8000两个固定采样率
    strcat(data,"\"dev_pid\":1537,");      //中文普通话
    strcat(data,"\"channel\":1,");         //单声道
    strcat(data,"\"cuid\":\"123456\",");   //识别码    随便打几个字符，但最好唯一
    strcat(data,"\"token\":\"24.6aa6b0735f89e398523a13a2c6b34cb4.2592000.1728033794.282335-114723353\",");  //token	这里需要修改成自己申请到的token
    strcat(data,"\"len\":32000,");         //数据长度  如果传输的数据长度改变了，记得修改该值，该值是ADC采集的数据字节数，不是base64编码后的长度
    strcat(data,"\"speech\":\"");
    strcat(data,base64::encode((uint8_t *)adc_data,sizeof(adc_data)).c_str());     //base64编码数据
    strcat(data,"\"");
    strcat(data,"}");
    // Serial.println(data);

    http_client.begin("http://vop.baidu.com/server_api");
    http_client.addHeader("Content-Type","application/json");
    httpCode = http_client.POST(data);

    if(httpCode > 0) {
        if(httpCode == HTTP_CODE_OK) {
            payload = http_client.getString();
            Serial.println(payload);
        }
    }
    else {
        Serial.printf("[HTTP] GET... failed, error: %s\n", http_client.errorToString(httpCode).c_str());
    }
    http_client.end();

    Serial.printf("Recognition complete\r\n");

    return payload;
}