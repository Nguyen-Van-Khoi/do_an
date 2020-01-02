//Thêm các thư viện cần thiết
#include "DHT.h"
//Cấu hình chân cảm biến là chân 2
#define DHTPIN 2 
//Chọn loại cảm biến là DHT11
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

//Thêm thư viện màn hình 16x2
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
//Khởi tạo màn hình 16 cột và 2 hàng
LiquidCrystal_I2C lcd(0x3F, 16, 2);
//Bảng mã cho kí tự độ C
byte degree[8] = 
{
  0B01110,
  0B01010,
  0B01110,
  0B00000,
  0B00000,
  0B00000,
  0B00000,
  0B00000
};

//Định nghĩa các hàm tắt bật thiết bị
#define quat_bat digitalWrite(12,HIGH)
#define quat_tat digitalWrite(12,LOW)
#define bom_bat  digitalWrite(13,HIGH)
#define bom_tat  digitalWrite(13,LOW)
#define den_bat  digitalWrite(15,HIGH)
#define den_tat  digitalWrite(15,LOW)

//Thêm thư viện kết nối wifi
#include <ESP8266WiFi.h>
//Thêm thư viện kết nối Firebase
#include <FirebaseArduino.h>
//Link Firebase
#define FIREBASE_HOST "smart-home-nhatpb.firebaseio.com" //Thay bằng địa chỉ firebase của bạn
#define FIREBASE_AUTH ""   //Không dùng xác thực nên không đổi
//Thêm tên wifi và mật khẩu
#define WIFI_SSID "ABC"   //Thay wifi và mật khẩu
#define WIFI_PASSWORD "123456798"


//Cấu hình các chân cảm biến bụi
int measurePin = A0;
int ledPower = 16;

//Các biến lưu giá trị điện áp và hàm lượng bụi
float voMeasured = 0;
float calcVoltage = 0;
float dustDensity = 0;

//Các biến lưu giá trị nhiệt độ độ ẩm và bụi
float temp = 0;
float hump = 0;
float dust = 0;

//Các biến lưu giá trị trạng thái bơm quạt và đèn
String tt_bom = "0";
String tt_quat = "0";
String tt_den = "0";

//Hàm đọc giá trị cảm biến
void read_sensor(void);
//Hàm hiển thị và điều khiển thiết bị
void display_data(void);
//Hàm lấy dữ liệu từ Firebase
void get_data(void);
//Hàm gửi dữ liệu lên Firebase
void send_data(void);

//Hàm khởi tạo
void setup() 
{
  //Khởi tạo màn hình
  lcd.begin();
  lcd.backlight();
  lcd.createChar(1, degree);
  lcd.clear();
  //In ra Smart home
  lcd.print("   Smart Home   ");
  Serial.begin(9600);
  Serial.println("Smart home control");

  //Khởi tạo cảm biến
  dht.begin();
  //Cấu hình các chân vào ra
  pinMode(ledPower,OUTPUT);
  pinMode(12,OUTPUT);
  pinMode(13,OUTPUT);
  pinMode(15,OUTPUT);

  //Kết nối wifi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("connecting");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("connected: ");
  Serial.println(WiFi.localIP());
  //Hàm kết nối Firebase
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
}

void loop() 
{
 read_sensor();
 delay(1000);
 send_data();
 delay(1000);
 get_data();
 delay(1000);
 display_data();
 delay(1000);
 Serial.print("Temp: "); 
 Serial.print(temp);
 Serial.print("  Hump: "); 
 Serial.print(hump);
 Serial.print("  Dust: "); 
 Serial.print(dust);
 Serial.println("ug/m3");
 Serial.print("TT_quat:"); 
 Serial.print(tt_quat);
 //Serial.print("TT_bom_abc"); 
// Serial.print(Firebase.getString("tt_bom"));
 
 Serial.print("    TT_bom:"); 
 Serial.print(tt_bom);
 Serial.print("        TT_den:"); 
 Serial.println(tt_den);
}

//Hàm đọc cảm biến 
void read_sensor(void)
{
  temp = dht.readTemperature();
  hump = dht.readHumidity();
  if (isnan(temp) || isnan(hump)) 
  {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }
  //Đọc cảm biến bụi
  digitalWrite(ledPower,LOW); // Bật IR LED
  delayMicroseconds(280);  //Delay 0.28ms
  voMeasured = analogRead(measurePin); // Đọc giá trị ADC V0
  delayMicroseconds(40); //Delay 0.04ms
  digitalWrite(ledPower,HIGH); // Tắt LED
  delayMicroseconds(9680); //Delay 9.68ms
 
  // Tính điện áp từ giá trị ADC
  calcVoltage = voMeasured * (5.0 / 1024); //Điệp áp Vcc của cảm biến (5.0 hoặc 3.3)
 
  // Linear Equation http://www.howmuchsnow.com/arduino/airquality/
  // Chris Nafis (c) 2012
  dust = 100*(0.17 * calcVoltage - 0.1);
}

void display_data(void)
{
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("T:");
  lcd.print((int)temp);
  lcd.write(1);
  lcd.print("C   "); 
  lcd.setCursor(8,0);
  lcd.print("H:");
  lcd.print((int)hump);
  lcd.print("%   "); 
  lcd.setCursor(0,1);
  lcd.print("D:");
  lcd.print(dust);
  lcd.print("ug/m3     ");  
  lcd.setCursor(13,1); 
  if(tt_bom  == "0"){lcd.print("T");bom_tat; }else{lcd.print("B");bom_bat;}
  if(tt_quat == "0"){lcd.print("T");quat_tat;}else{lcd.print("B");quat_bat;}
  if(tt_den  == "0"){lcd.print("T");den_tat; }else{lcd.print("B");den_bat;}
}

void get_data(void)
{
 tt_bom  = Firebase.getString("tt_bom"); 
 tt_quat = Firebase.getString("tt_quat"); 
 tt_den  = Firebase.getString("tt_den"); 
}

void send_data(void)
{
  Firebase.setFloat("temp",temp);
  Firebase.setFloat("hump",hump);
  Firebase.setFloat("dust",dust);
}
