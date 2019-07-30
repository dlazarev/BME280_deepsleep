#include <WiFi.h>
//#include <Wire.h>
#include <SSD1306.h>
#include <Adafruit_BME280.h>
#include <WiFiUDP.h>


#define ledPin 16
//#define ledPin 2
#define SDA 5
#define SCL 4

SSD1306Wire  display(0x3c, SDA, SCL);
RTC_DATA_ATTR int bootCount = 0; //RTC fast memory
WiFiUDP udp;
Adafruit_BME280 bme; // I2C
const char* ssid = "BOSON2.4";
const char* passwd = "kartoshka";
const byte influxdb_host[] = {138, 201, 158, 136}; // the IP address of your InfluxDB host
const int influxdb_port = 8089; // UDP port of your InfluxDB host
float temperature, pressure, humidity;

void display_init() {
  display.init();
  display.flipScreenVertically(); // does what is says
  display.setFont(ArialMT_Plain_16); // does what is says
  display.clear();
  display.drawString(0, 0, "Hello");
  display.display(); // display whatever is in the buffer  
}

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
  delay(1000);
  //display_init();

  bootCount++;

  //display.clear();
  snprintf(buf, 254, "Wakeup count=%d", bootCount);
  Serial.println(buf);
  //display.drawString(0, 0, buf);
  //display.display();

  print_wakeup_reason();

  digitalWrite(ledPin, HIGH);

  // Connecting to WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.print(ssid);

  WiFi.begin(ssid, passwd);
  while (WiFi.status() != WL_CONNECTED) {
  	delay(500);
  	Serial.print(".");
  }
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


  esp_sleep_enable_timer_wakeup(300e6);
  Serial.println("Going to sleep now");
  delay(2000);
  Serial.flush();
  //display.ssd1306_command(SSD1306_DISPLAYOFF);

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

  udp_data = String("BME280,device_id=001 temperature=" + String(temperature));
  udp_data += String(",humidity=" + String(humidity));
  udp_data += String(",pressure=" + String(pressure));
  Serial.println("Sending UDP packet...");
  Serial.println(udp_data);
  udp.beginPacket(influxdb_host, influxdb_port);
  udp.print(udp_data);
  udp.endPacket();
}
