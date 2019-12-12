#include <driver/adc.h>
#include <WiFi.h>
#include <Adafruit_BME280.h>
#include <WiFiUdp.h>
#include <HardwareSerial.h>

#define ledPin 4
#define SDA 5
#define SCL 4

RTC_DATA_ATTR int bootCount = 0; //RTC fast memory
WiFiUDP udp;
Adafruit_BME280 bme; // I2C
const char* ssid = "BOSON2.4";
const char* passwd = "kartoshka";
const byte influxdb_host[] = {138, 201, 158, 136}; // the IP address of your InfluxDB host
const int influxdb_port = 8089; // UDP port of your InfluxDB host
float temperature, pressure, humidity;

void read_sensors(void);
void getWeather(void);

void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}

void setup() {
  char buf[255];

  pinMode(ledPin, OUTPUT);

  Serial.begin(115200);
  delay(100);
  
  //ADC1 config
  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(ADC1_CHANNEL_6,ADC_ATTEN_DB_11);
  int val = adc1_get_raw(ADC1_CHANNEL_6);
  int adcr = analogRead(A6); //Использовать в аргументах ADC1_CHANNEL_0 нельзя!!!
  Serial.print("Read ADC pin [" + String(ADC1_CHANNEL_6_GPIO_NUM) + "]: " + String(val) + " [" + String(adcr)+"]");

  bootCount++;

  Serial.println("********************");
  snprintf(buf, 254, "Wakeup count=%d", bootCount);
  Serial.println(buf);
  Serial.println("********************");

  print_wakeup_reason();

  // Connecting to WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.print(ssid);

  WiFi.begin(ssid, passwd);
  for (int i = 0; i < 20 && (WiFi.status() != WL_CONNECTED); i++) {
  	delay(1500);
  	Serial.print(".");
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println();
    Serial.println("WiFi is NOT connected. Spleeping 10 min.");
      esp_sleep_enable_timer_wakeup(600e6);
    delay(2000);
    Serial.flush();

    esp_deep_sleep_start();
    
  }
  
  digitalWrite(ledPin, HIGH);
  Serial.println();
  Serial.println("WiFi connected.");

  // Print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
  
  Serial.println(WiFi.localIP());

  if (!bme.begin(0x76)) {
  	Serial.println("Could not find a valid BME280 sensor, check wiring!");
  	while (1) {
  		digitalWrite(ledPin, ! digitalRead(ledPin));
  		delay(300);
  	}
  }
  
  delay(3000);
  read_sensors();
  delay(1000);


  esp_sleep_enable_timer_wakeup(60e6);
  Serial.println("Going to sleep now");
  delay(2000);
  Serial.flush();

  esp_deep_sleep_start();
}

void loop() {
  // put your main code here, to run repeatedly:

}

void getWeather() {
	temperature = bme.readTemperature();
	pressure = bme.readPressure();
	humidity = bme.readHumidity();
}

void read_sensors() {

  String udp_data;

  getWeather();

  Serial.println();
  Serial.print("Temperature = ");
  Serial.println(temperature);

  Serial.print("Pressure = ");
  Serial.println(pressure);

  Serial.print("Humidity = ");
  Serial.println(humidity);

  udp_data = String("BME280,device_id=003 temperature=" + String(temperature));
  udp_data += String(",humidity=" + String(humidity));
  udp_data += String(",pressure=" + String(pressure));
  Serial.println("Sending UDP packet...");
  Serial.println(udp_data);
  udp.beginPacket(influxdb_host, influxdb_port);
  udp.print(udp_data);
  udp.endPacket();
}
