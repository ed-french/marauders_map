#ifndef BOARD_HAS_PSRAM
#error "Please enable PSRAM !!!"
#endif

#include <Arduino.h>
#include <esp_task_wdt.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "epd_driver.h"
#include "firasans.h"
#include "esp_adc_cal.h"
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
// #include "beach.h"
// #include "cave.h"
#include "cat.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiMulti.h>
#include "credentials.h" // You can put ssid and password declarations here

#define USE_SERIAL Serial

#define BATT_PIN            36
#define SD_MISO             12
#define SD_MOSI             13
#define SD_SCLK             14
#define SD_CS               15

//const char* ssid = "ssid";
//const char* password = "password";

String serverName = "http://192.168.1.125/marauders_map_full";

WiFiMulti wifiMulti;


uint8_t *framebuffer;
int vref = 1100;

void setup()
{
    char buf[128];

    Serial.begin(115200);
    delay(500);
    Serial.println("booted");

    wifiMulti.addAP(ssid, password);
 


    /*
    * SD Card test
    * Only as a test SdCard hardware, use example reference
    * https://github.com/espressif/arduino-esp32/tree/master/libraries/SD/examples
    * * */
    // SPI.begin(SD_SCLK, SD_MISO, SD_MOSI);
    // bool rlst = SD.begin(SD_CS);
    // if (!rlst) {
    //     Serial.println("SD init failed");
    //     snprintf(buf, 128, "➸ No detected SdCard");
    // } else {
    //     snprintf(buf, 128, "➸ Detected SdCard insert:%.2f GB", SD.cardSize() / 1024.0 / 1024.0 / 1024.0);
    // }

    // Correct the ADC reference voltage
    esp_adc_cal_characteristics_t adc_chars;
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
        Serial.printf("eFuse Vref:%u mV", adc_chars.vref);
        vref = adc_chars.vref;
    }

    epd_init();

    //framebuffer = (uint8_t *)ps_calloc(sizeof(uint8_t), EPD_WIDTH * EPD_HEIGHT / 2);
    framebuffer=(uint8_t *)heap_caps_malloc(EPD_WIDTH * EPD_HEIGHT / 2, MALLOC_CAP_SPIRAM);

    if (!framebuffer) {
        Serial.println("alloc memory failed !!!");
        while (1);
    }
    memset(framebuffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);

    // Rect_t area = {
    //     .x = 230,
    //     .y = 20,
    //     .width = logo_width,
    //     .height = logo_height,
    // };
    // Rect_t area = {
    //     .x = 0,
    //     .y = 0,
    //     .width = beach_width,
    //     .height = beach_height,
    // };
    epd_poweron();
    epd_clear();
    // epd_draw_grayscale_image(area, (uint8_t *)cave_data);
    


    // int cursor_x = 200;
    // int cursor_y = 250;

    // char *string1 = "➸ 16 color grayscale  😀 \n";
    // char *string2 = "➸ Use with 4.7\" EPDs 😍 \n";
    // char *string3 = "➸ High-quality font rendering ✎🙋";
    // char *string4 = "➸ ~630ms for full frame draw 🚀\n";

    // epd_poweron();

    // writeln((GFXfont *)&FiraSans, buf, &cursor_x, &cursor_y, NULL);
    // delay(500);
    // cursor_x = 200;
    // cursor_y += 50;
    // writeln((GFXfont *)&FiraSans, string1, &cursor_x, &cursor_y, NULL);
    // delay(500);
    // cursor_x = 200;
    // cursor_y += 50;
    // writeln((GFXfont *)&FiraSans, string2, &cursor_x, &cursor_y, NULL);
    // delay(500);
    // cursor_x = 200;
    // cursor_y += 50;
    // writeln((GFXfont *)&FiraSans, string3, &cursor_x, &cursor_y, NULL);
    // delay(500);
    // cursor_x = 200;
    // cursor_y += 50;
    // writeln((GFXfont *)&FiraSans, string4, &cursor_x, &cursor_y, NULL);
    // delay(500);

    // epd_poweroff();

}

// void reveal_image(uint8_t * image_data)
// {
//     bool revealed_y[540/4];
//     memset(revealed_y,0,sizeof(bool)*540/4);// Set all revealed to false
//     uint16_t left_to_do=540/4;
//     int16_t y=-4;
//     while(left_to_do>0)
//     {
      
//       // Look for an unrevealed random y
//       while (true)
//       {
//         y=y+4;//4*random(540/4);
//         if (!revealed_y[y/4]) break;
//       }
//       revealed_y[y/4]=true; // Mark it done
//       left_to_do--;

//       Rect_t area = {
//           .x = 0,
//           .y = y,
//           .width = 960,
//           .height = 4,
//       };

//     // int cursor_x = 200;
//     // int cursor_y = 500;




//       epd_clear_area(area);
//       epd_clear_area(area);
//       epd_clear_area(area);
//       epd_draw_grayscale_image(area, (uint8_t *)image_data+(960/2*y));
//     //writeln((GFXfont *)&FiraSans, (char *)voltage.c_str(), &cursor_x, &cursor_y, NULL);

//     }
// }

void add_point(uint16_t x, uint16_t y, uint8_t value)
{
    // Find relevant address for the pixel
    uint32_t offset=y*480+(x/2);
    uint8_t * ptr=framebuffer+offset;
    uint8_t screen_byte=*ptr;
    if (x % 2)
    {// greater x is high nibble
        uint8_t old_nibble=(screen_byte & 0xF0)>>4;
        uint8_t new_value=min(old_nibble,value); // Should never overflow the bottom nibble
        if (new_value>15) new_value=15;
        screen_byte=screen_byte & 0x0F; // Zero out the top nibble ready to replace
        screen_byte=screen_byte | (new_value<<4); // Put in the new value      
    } else {
        uint8_t old_nibble=screen_byte & 0x0F;
        uint8_t new_value=min(old_nibble,value);
        if (new_value>15) new_value=15;
        screen_byte=screen_byte & 0xF0;
        screen_byte=screen_byte | new_value;
    }
    framebuffer[offset]=screen_byte;
    //Serial.printf("Wrote %d to %d\n",screen_byte,offset);


}
void draw_cat(uint16_t mid_x,uint16_t mid_y)
{
    uint16_t cat_ptr=0;
    
    for (uint16_t y_offset=0;y_offset<32;y_offset++)//=mid_y-15;y<(mid_y+32);y++)
    {
        for (uint16_t x_offset=0;x_offset<32;x_offset++)//=mid_x-15;x<(mid_x+32);x++)
        {
            uint8_t val=cat_data[y_offset*32+x_offset];
            
            add_point(x_offset+mid_x-15,y_offset+mid_y-15,val);

        }    
    }
}

void loop()
{
    // When reading the battery voltage, POWER_EN must be turned on

    uint8_t * i_data;

    while (true)
    {
    //   reveal_image((uint8_t *)beach_data);
    //   reveal_image((uint8_t *)cave_data);
        memset(framebuffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);
        epd_poweron();
        epd_clear();
        if((wifiMulti.run() == WL_CONNECTED))
        {

            HTTPClient http;

            USE_SERIAL.print("[HTTP] begin...\n");

            // configure server and url
            http.begin(serverName.c_str());
            //http.begin("192.168.1.12", 80, "/test.html");

            USE_SERIAL.print("[HTTP] GET...\n");
            // start connection and send HTTP header
            int httpCode = http.GET();
            if(httpCode > 0)
            {
            // HTTP header has been send and Server response header has been handled
            USE_SERIAL.printf("[HTTP] GET... code: %d\n", httpCode);

            // file found at server
            if(httpCode == HTTP_CODE_OK) {

                // get length of document (is -1 when Server sends no Content-Length header)
                int len = http.getSize()-8;
                USE_SERIAL.printf("Found response of size: %d",len);

                // create buffer for read
                uint8_t buff[2048] = { 0 };

                // get tcp stream
                WiFiClient * stream = http.getStreamPtr();

                uint16_t width;

                stream->readBytes(buff,2); // Read two bytes

                memcpy(&width,buff,2);// Grab two bytes

                


                uint16_t height;
                stream->readBytes(buff,2); // Grab next two bytes

                memcpy(&height,buff,2); // Copy into height



                Serial.printf("Image dimesions sent: %d x %d\n",width,height);

                

                stream->readBytes(buff,4);// Position stuff, ignored for now

                // read all data from server

                uint8_t * screen_ptr=framebuffer;
                uint32_t timeout=millis()+20000; // Max 20 seconds
                while(http.connected() && (len > 0 || len == -1)) {
                    // get available data size
                    if (millis()>timeout)
                    {
                        USE_SERIAL.println("Timing out");
                        break;
                    } 
                    size_t size = stream->available();

                    if(size) {
                        // read up to 128 byte
                        int c = stream->readBytes(screen_ptr, ((size > sizeof(buff)) ? sizeof(buff) : size));
                        screen_ptr+=c;
                        // write it to Serial
                        USE_SERIAL.printf("Written another : %d bytes to screenbuffer, %d left...\n",c,len);

                        if(len > 0) {
                            len -= c;
                        }
                    } 
                    delay(1);
                }

                for (uint16_t z=20;z<500;z+=50)
                {
                    draw_cat(z,z);
                }

                USE_SERIAL.println();
                USE_SERIAL.print("[HTTP] connection closed or file end.\n");
                


                epd_draw_image(epd_full_screen(), framebuffer, BLACK_ON_WHITE);
                epd_draw_image(epd_full_screen(), framebuffer, BLACK_ON_WHITE);
                epd_draw_image(epd_full_screen(), framebuffer, BLACK_ON_WHITE);
                epd_draw_image(epd_full_screen(), framebuffer, BLACK_ON_WHITE);
                epd_draw_image(epd_full_screen(), framebuffer, BLACK_ON_WHITE);
                epd_draw_image(epd_full_screen(), framebuffer, BLACK_ON_WHITE);
                epd_draw_image(epd_full_screen(), framebuffer, BLACK_ON_WHITE);
                epd_draw_image(epd_full_screen(), framebuffer, BLACK_ON_WHITE);
                USE_SERIAL.println("Image drawn");

            }
        } else {
            USE_SERIAL.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
        }

        http.end();
    }
    // uint16_t v = analogRead(BATT_PIN);
    // float battery_voltage = ((float)v / 4095.0) * 2.0 * 3.3 * (vref / 1000.0);
    // String voltage = "➸ Voltage :" + String(battery_voltage) + "V";
    // Serial.println(voltage);

    // There are two ways to close


    // It will turn off the power of the ink screen, but cannot turn off the blue LED light.
    // epd_poweroff();

    //It will turn off the power of the entire
    // POWER_EN control and also turn off the blue LED light

        delay(10000);
    }
    epd_poweroff();
    epd_poweroff_all();
    
}