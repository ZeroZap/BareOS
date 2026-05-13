#ifndef _XY_MQTT_H_
#define _XY_MQTT_H_


union xy_mqtt_fix_header{
    unsigned char byte;
    struct {
        unsigned char retain : 1;
        unsigned char qos : 2;
        unsigned char dup : 1;
        unsigned char type : 4;
    }connect;
    struct {
        unsigned char bit0 : 1;
        unsigned char bit1 : 1;
        unsigned char bit2 : 1;
        unsigned char bit3 : 1;
        unsigned char type : 4;
    }generic;
    // remaining length no need to define
};


struct xy_mqtt_connect_packet {

    /* Fix Header */
    union xy_mqtt_fix_header fheader;
    uint32_t packet_length;

    unsigned char vheader_offset; // 1,2,3,4

    /* Variable Header : Proto name length: Fix in 2byte*/

    /* Variable Header  : Proto name MQTT*/
    unsigned char protol_name[4]; // usual name MQTT

    /* protocol level - 4: 3.1.1, */
    unsigned char protol_level;

    union flag{
        unsigned char data;
        struct {
            unsigned char reserved : 1;
            unsigned char clean_session : 1 ;
            unsigned char will : 1;
            unsigned char will_qos : 2;
            unsigned char will_retain : 1;
        }bits;
    };
};

struct xy_mqtt{
    uint8_t status;
    void *packet;
};
#endif