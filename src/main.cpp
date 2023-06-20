#include <mbed.h>
#include "ULN2003/ULN2003.h"
#include "Sht31/Sht31.h"
#include "MLX90614/MLX90614.h"
//#include "ov7670/ov7670.h"  

#include "MQTTmbed.h"
#include "MQTTClient.h"
#include "MQTTNetwork.h"


WiFiInterface *wifi;
#define MQTTCLIENT_QOS2 1
int arrivedcount = 0;
MQTT::Client<MQTTNetwork, Countdown> *client;

ULN2003 motor(D3, A3, D6, A2);
Sht31 sensor(I2C_SDA, I2C_SCL);

I2C i2c(PB_9, PB_8);   //sda,scl
MLX90614 mlx90614(&i2c);
volatile float child_temp;

AnalogIn my_micro(A0);
float decibel;
 
int steps=4096;
int speed=500;

Thread thread;
Thread thread2;
Thread thread3;
Thread thread4;
Thread thread5;
Thread thread6;
Thread thread7;
int sleep_t=1000;

//DigitalIn s1(A3);
//DigitalIn s2(A5);


//DigitalIn s3(D7);
//DigitalIn s4(D11);

volatile float child_temp_new;
volatile float average_dec;
volatile float decc;
volatile float atmosphere_temp;
volatile float humidity;
volatile bool motor_moved;


void messageArrived(MQTT::MessageData& md){
    //motor_moved = false;
    MQTT::Message &message = md.message;
    printf("Message arrived: qos %d, retained %d, dup %d, packetid %d\r\n", message.qos, message.retained, message.dup, message.id);
    printf("Payload %.*s\r\n", message.payloadlen, (char*)message.payload);
    //printf("Payloadddddd %s\r\n", message.payload);
    ++arrivedcount;
    string payl =  (char*)message.payload;
    if (payl.find("ON") == std::string::npos){
    }
    if (payl.find("OFF") < std::string::npos) {
        motor_moved = false;
        printf("Motor = %d\r\n", motor_moved);
    }
    if (payl.find("ON") < std::string::npos) {
        motor_moved = true;
        //printf(motor_moved );
        //printf( "\r\n");
        printf("Motor = %d\r\n", motor_moved);
    }
    }

void mqtt_demo(NetworkInterface *net)
{
    float version = 0.6;
    char* topic = "noternata/groups/Baby_bed/json";
 
    MQTTNetwork network(net);
    client = new MQTT::Client<MQTTNetwork, Countdown>(network);
 
    char* hostname = "52.54.110.50";
    int port = 1883;
 
    printf("Connecting to %s:%d\r\n", hostname, port);

    int rc = network.connect(hostname, port);
 
    if (rc != 0)
        printf("rc from TCP connect is %d\r\n", rc);
    printf("Connected socket\n\r");
 
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.MQTTVersion = 3;
    data.clientID.cstring = "2a617592b917";
    data.username.cstring = "noternata";
    data.password.cstring = "aio_QLLK3792OAP6cPEgAdn56mXS148N";
    if ((rc = client->connect(data)) != 0)
        printf("rc from MQTT connect is %d\r\n", rc);
 
    if ((rc = client->subscribe(topic, MQTT::QOS2, messageArrived)) != 0)
        printf("rc from MQTT subscribe is %d\r\n", rc);

    while (true)
       client->yield(100);

    
 
    printf("Version %.2f: finish %d msgs\r\n", version, arrivedcount);
    return;
}

const char *sec2str(nsapi_security_t sec)
{
    switch (sec) {
        case NSAPI_SECURITY_NONE:
            return "None";
        case NSAPI_SECURITY_WEP:
            return "WEP";
        case NSAPI_SECURITY_WPA:
            return "WPA";
        case NSAPI_SECURITY_WPA2:
            return "WPA2";
        case NSAPI_SECURITY_WPA_WPA2:
            return "WPA/WPA2";
        case NSAPI_SECURITY_UNKNOWN:
        default:
            return "Unknown";
    }
}

void print_warning(char *warning = "Alert!!! Damage temperature!\n\r"){
  printf("%s", warning);
}
void alert(){
   sleep_t=200;
}
void normal(){
   sleep_t = 1000;
}

void control_temp(){
  while(1) {
    //get temp, humidity
    float t = sensor.readTemperature();
    int hum = sensor.readHumidity();
    //print temp, humidity
    printf("T: %f, H: %f\r\n", sensor.readTemperature(), sensor.readHumidity());

    if (t<18 || t>27){
      alert();
      print_warning();
    }
    else{
      normal();
    }

    thread_sleep_for(sleep_t);
  }
}

void control_temp_child(){
  while(1) {
    child_temp=mlx90614.read_temp(1);
    //send_message(child_temp);
    //printf("Temperature : %4.2f Celcius\r\n", child_temp);
    //return child_temp;
    //thread_sleep_for(1000);
  }
}

void control_micro(){
  while(1) {
    printf("Decibels read = %f \r\n", (my_micro.read()));
    thread_sleep_for(1000);
  }
}

int steps2=2048;
int speed2=300;

void move_motor(){
  while (1){
    if (motor_moved ==1){
      printf("Start moving\n");
      motor.moveForward(600, 500);
      thread_sleep_for(500);
      motor.moveReverse(600, 500);
    }
    if (motor_moved == 0){
        motor.relax();;
    }
  }

  
    //motor.moveReverse(steps2, speed2);
    //thread_sleep_for(100);
    //motor.moveForward(steps2, speed2);
    //thread_sleep_for(300);
    //printf("ADC read = %f\n\r", (my_micro.read()*100));
    //thread_sleep_for(10);
    
    //move motor
    ////////////////motor.moveForward(steps, speed);
    //thread_sleep_for(1000);
    //motor.moveReverse(steps, speed);
    //thread_sleep_for(4000);

  
}

void send_child_temp(){
    while(1) {
      child_temp_new =mlx90614.read_temp(1);
      float version = 0.6;
      char* topic = "noternata/feeds/baby-bed.temp";
      MQTT::Message message;
      char buf[100];
      // sprintf(buf, "{\"feeds\":{\"temp\": %f}}\r\n", 36.6f);
      sprintf(buf, "%f\r\n", child_temp_new);
      message.qos = MQTT::QOS0;
      message.retained = false;
      message.dup = false;
      message.payload = (void*)buf;
      message.payloadlen = strlen(buf)+1;
      int rc = client->publish(topic, message);
      thread_sleep_for(8050);
      //printf("rc from MQTT connect is %d\r\n", rc);
    }
}
void send_decibels(){
    average_dec =0;
    decc =average_dec;
    while(1) {
      
      //float decibels_new =my_micro.read();
      //float version = 0.6;
      //char* topic = "noternata/feeds/baby-bed.decibel";
      
      average_dec +=my_micro.read()*100;
      thread_sleep_for(500);
      average_dec +=my_micro.read()*100;
      thread_sleep_for(500);
      average_dec +=my_micro.read()*100;
      thread_sleep_for(500);
      average_dec +=my_micro.read()*100;
      thread_sleep_for(500);
      average_dec +=my_micro.read()*100;
      thread_sleep_for(500);
      average_dec +=my_micro.read()*100;
      thread_sleep_for(500);
      average_dec +=my_micro.read()*100;
      thread_sleep_for(500);
      average_dec +=my_micro.read()*100;
      thread_sleep_for(500);
      average_dec +=my_micro.read()*100;
      thread_sleep_for(500);
      average_dec +=my_micro.read()*100;
      thread_sleep_for(500);
      average_dec +=my_micro.read()*100;
      thread_sleep_for(500);
      average_dec +=my_micro.read()*100;
      thread_sleep_for(500);
      average_dec +=my_micro.read()*100;
      thread_sleep_for(500);
      average_dec +=my_micro.read()*100;
      thread_sleep_for(500);
      average_dec +=my_micro.read()*100;
      thread_sleep_for(500);
      average_dec +=my_micro.read()*100;
      average_dec /= 16;
      decc= average_dec;
      //MQTT::Message message;
      //char buf[100];
      //sprintf(buf, "%f\r\n", average_dec);
      //message.qos = MQTT::QOS0;
      //message.retained = false;
      //message.dup = false;
      //message.payload = (void*)buf;
      //message.payloadlen = strlen(buf)+1;
      //int rc = client->publish(topic, message);
      //thread_sleep_for(500);
      //return;
    }
}
void send_atmosphere_temp(){
    while(1) {
      atmosphere_temp = sensor.readTemperature();
      float version = 0.6;
      char* topic = "noternata/feeds/baby-bed.atmosphere-temp";
      MQTT::Message message;
      char buf[100];
      sprintf(buf, "%f\r\n", atmosphere_temp);
      message.qos = MQTT::QOS0;
      message.retained = false;
      message.dup = false;
      message.payload = (void*)buf;
      message.payloadlen = strlen(buf)+1;
      int rc = client->publish(topic, message);
      thread_sleep_for(8050);
    }
}

void send_humidity(){
    while(1) {
      humidity = sensor.readHumidity();
      float version = 0.6;
      char* topic = "noternata/feeds/baby-bed.humidity";
      MQTT::Message message;
      char buf[100];
      // sprintf(buf, "%f\r\n", humidity);
      // message.qos = MQTT::QOS0;
      // message.retained = false;
      // message.dup = false;
      // message.payload = (void*)buf;
      // message.payloadlen = strlen(buf)+1;
      // int rc = client->publish(topic, message);
      // thread_sleep_for(8050);
    }
}
void control_pokazateley(){
  while(1) {
        //humidity = sensor.readHumidity();
        humidity = sensor.readHumidity();
        float version = 0.6;
        MQTT::Message message;
        char buf[100];

        char* topic = "noternata/feeds/baby-bed.info";
        char* topic_h = "noternata/feeds/baby-bed.humidity";
        // влажность 
        sprintf(buf, "%f\r\n", humidity);
        message.qos = MQTT::QOS0;
        message.retained = false;
        message.dup = false;
        message.payload = (void*)buf;
        message.payloadlen = strlen(buf)+1;
        int rc = client->publish(topic_h, message);
        thread_sleep_for(8000);

        if (humidity> 60 ){
          thread_sleep_for(1000);
          sprintf(buf, "Too high Humidity for child: %f'\r\n", humidity);
          message.qos = MQTT::QOS0;
          message.retained = false;
          message.dup = false;
          message.payload = (void*)buf;
          message.payloadlen = strlen(buf)+1;
          rc = client->publish(topic, message);
          thread_sleep_for(8000);
        }
        else if (humidity< 33 ){
          thread_sleep_for(1000);
          sprintf(buf, "Too small Humidity for child: %f'\r\n", humidity);
          message.qos = MQTT::QOS0;
          message.retained = false;
          message.dup = false;
          message.payload = (void*)buf;
          message.payloadlen = strlen(buf)+1;
          rc = client->publish(topic, message);
          thread_sleep_for(8000);
        }

        //decibels

        //thread_sleep_for(3000);
        atmosphere_temp = sensor.readTemperature();
        char* topic_at = "noternata/feeds/baby-bed.atmosphere-temp";
        sprintf(buf, "%f\r\n", atmosphere_temp);
        message.qos = MQTT::QOS0;
        message.retained = false;
        message.dup = false;
        message.payload = (void*)buf;
        message.payloadlen = strlen(buf)+1;
        rc = client->publish(topic_at, message);
        thread_sleep_for(8000);
        
        if (atmosphere_temp > 30 ){
          thread_sleep_for(1000);
          sprintf(buf, "Too high Atmosphere temp for child: %f'\r\n", humidity);
          message.qos = MQTT::QOS0;
          message.retained = false;
          message.dup = false;
          message.payload = (void*)buf;
          message.payloadlen = strlen(buf)+1;
          rc = client->publish(topic, message);
          thread_sleep_for(8000);
        }
        else if (atmosphere_temp < 23 )        {
          thread_sleep_for(1000);
          sprintf(buf, "Too small Atmosphere temp for child: %f'\r\n", humidity);
          message.qos = MQTT::QOS0;
          message.retained = false;
          message.dup = false;
          message.payload = (void*)buf;
          message.payloadlen = strlen(buf)+1;
          rc = client->publish(topic, message);
          thread_sleep_for(8000);
        }
         thread_sleep_for(8000);

         // child temperathure
        child_temp_new =mlx90614.read_temp(1);
        char* topic_ch = "noternata/feeds/baby-bed.temp";
        sprintf(buf, "%f\r\n", child_temp_new);
        message.qos = MQTT::QOS0;
        message.retained = false;
        message.dup = false;
        message.payload = (void*)buf;
        message.payloadlen = strlen(buf)+1;
        rc = client->publish(topic_ch, message);
        printf("Temp1 code %d\n\r",rc);
        thread_sleep_for(8000);
        if (child_temp_new > 36 ){
          thread_sleep_for(1000);
          sprintf(buf, "Too high temp of child: %f'\r\n", child_temp_new);
          message.qos = MQTT::QOS0;
          message.retained = false;
          message.dup = false;
          message.payload = (void*)buf;
          message.payloadlen = strlen(buf)+1;
          rc = client->publish(topic, message);
          thread_sleep_for(8000);
        }
        else if (child_temp_new < 30 )        {
          thread_sleep_for(1000);
          printf("I'm here!\n\r");
          sprintf(buf, "Too small temp of child: %f'\r\n", child_temp_new);
          message.qos = MQTT::QOS0;
          message.retained = false;
          message.dup = false;
          message.payload = (void*)buf;
          message.payloadlen = strlen(buf)+1;
          rc = client->publish(topic, message);
          printf("Temp code %d\n\r",rc);
          thread_sleep_for(8000);
        }

        decc= decc;
        printf("I'm here!\n\r");
        char* topic_d = "noternata/feeds/baby-bed.decibel";
        sprintf(buf, "%f\r\n", decc);
        message.qos = MQTT::QOS0;
        message.retained = false;
        message.dup = false;
        message.payload = (void*)buf;
        message.payloadlen = strlen(buf)+1;
        rc = client->publish(topic_d, message);
        printf("decibel code %d\n\r",rc);
        thread_sleep_for(8000);
        if (decc> 6 ){
          thread_sleep_for(1000);
          sprintf(buf, "Too high volume for child: %f'\r\n", decc);
          message.qos = MQTT::QOS0;
          message.retained = false;
          message.dup = false;
          message.payload = (void*)buf;
          message.payloadlen = strlen(buf)+1;
          
          rc = client->publish(topic, message);
          thread_sleep_for(8000);
        }


        thread_sleep_for(8000);
        //char buf2[500];

        //char* topic_all = "noternata/groups/Baby_bed/json" ;
        //sprintf(buf2, "{\"feeds\":{\"temp\":%d, \"atmosphere-temp\":%d, \"decibel\":%d}}", 67, 6, 7);//child_temp_new, atmosphere_temp, average_dec);
        //message.qos = MQTT::QOS0;
        //message.retained = false;
        //message.dup = false;
        //message.payload = (void*)buf2;
        //message.payloadlen = strlen(buf2)+1;
        //int rc = client->publish(topic_all, message);
        //printf("All code %d\n\r",rc);
        //thread_sleep_for(3000);
        //thread_sleep_for(8000);
        
  }

  
 //child_temp_new;
 //average_dec;
 //atmosphere_temp;
 //humidity;
}

int main() {
 //thread4.start(move_motor);

    printf("Start WiFi connecting\n");
    wifi = WiFiInterface::get_default_instance();
    if (!wifi) {
        printf("ERROR: No WiFiInterface found.\n");
        return -1;
    }

    //int count = scan_demo(wifi);
    //if (count == 0) {
    //     printf("No WIFI APs found - can't continue further.\n");
    //     return -1;
    // }

    printf("\nConnecting to %s...\n", MBED_CONF_APP_WIFI_SSID);
    int ret = wifi->connect(MBED_CONF_APP_WIFI_SSID, MBED_CONF_APP_WIFI_PASSWORD, NSAPI_SECURITY_WPA_WPA2);
    if (ret != 0) {
        printf("\nConnection error: %d\n", ret);
        return -1;
    }

    printf("Success\n\n");
    printf("MAC: %s\n", wifi->get_mac_address());
    SocketAddress a;
    wifi->get_ip_address(&a);
    printf("IP: %s\n", a.get_ip_address());
    wifi->get_netmask(&a);
    printf("Netmask: %s\n", a.get_ip_address());
    wifi->get_gateway(&a);
    printf("Gateway: %s\n", a.get_ip_address());
    //wifi->disconnect();
    printf("\nDone\n");

  thread5.start([&](){
   mqtt_demo(wifi);
  });

  thread_sleep_for(2000);
  //thread2.start(send_child_temp);
  //thread_sleep_for(1000);
  thread3.start(send_decibels);
  thread4.start(move_motor);
  //thread_sleep_for(1000);
  //thread.start(send_atmosphere_temp);
  //thread_sleep_for(1000);
  //thread6.start(send_humidity);
  thread_sleep_for(1000);
  thread7.start(control_pokazateley);
  thread_sleep_for(1000);
  
  while(1) {
    //send_child_temp(child_temp);
    //thread_sleep_for(6000);
    //printf("ADC read = %f\n\r", (my_micro.read()*100));
    //thread_sleep_for(10);
    
    //move motor
    ////////////////motor.moveForward(steps, speed);
    //thread_sleep_for(1000);
    //motor.moveReverse(steps, speed);
    //thread_sleep_for(4000);
    //motor.moveForward(steps2, speed2);
    //thread_sleep_for(1000);
    //motor.moveReverse(steps2, speed2);
    //thread_sleep_for(1000);


   
  }
}