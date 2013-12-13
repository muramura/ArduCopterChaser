#include <FastSerial.h>
#include "../GCS_MAVLink/include/mavlink/v1.0/ardupilotmega/mavlink.h"

//C_TAKEOFFモード
#define C_TAKEOFF 13

//XBee用
#define STATE_ARMED     0
#define STATE_TAKEOFF   1
#define STATE_CHASER    2

FastSerialPort0(Serial);

//I2C-GPS用
#define I2C_ADDRESS 30
#define I2C_ROM_ADDRESS 43
#define I2C_SPEED 100000L

uint8_t rawADC[6];

//LCD用
#include <LiquidCrystal.h>
LiquidCrystal lcd(8, 7 , 5, 4, 3, 2);
char lin1[18];
char lin2[18];
char lin3[18];
char lin4[18];

//GPS用
struct GPS_DATA{
	union
	{
		uint32_t coord;
		uint8_t data[4];
	}lat;
	union
	{
		uint32_t coord;
		uint8_t data[4];
	}lon;
	union
	{
		uint16_t coord;
		uint8_t data[2];
	}alt;
};

static GPS_DATA gps_data;

void setup()
{
	//XBee用のシリアルポートを開ける
	Serial.begin(57600);

	//GPS取得用のI2Cの初期化
	i2c_init();
	
	gps_data.lat.coord = 0;
	gps_data.lon.coord = 0;
	gps_data.alt.coord = 0;
	
	// LCD表示
	strcpy(lin1, "****************");
	strcpy(lin2, "BEACON");
	strcpy(lin3, "ver0.4");
	strcpy(lin4, "****************");
	lcd_update();
	
	delay(20000);
	//delay(2000);		//デバッグ用
}


void loop(){
	static uint32_t now = 0;
	static uint8_t state = 0;
	
	switch(state){
		case STATE_ARMED:
			//LCD表示
			strcpy(lin1, "Now taking off.");
			strcpy(lin2, "wait 10 seconds...");
			strcpy(lin3, "");
			strcpy(lin4, "");
			lcd_update();
			
			//自動テイクオフコマンド送信
			send_takeoff_cmd();
			
			//10秒待つ（テイクオフするまでのざっくり時間）
			delay(10000);
			
			//状態更新
			state = STATE_CHASER;
			
			break;
			
		case STATE_CHASER:
			gps_data.lat.coord = 1;
			gps_data.lon.coord = 1;
			
			//GPSデータ取得
			get_gps_data();
			
			//機体に送信
			send_chaser_cmd(gps_data.lat.coord,gps_data.lon.coord,700);
			//Serial.println(gps_data.lat.coord);		//デバッグ用
			//Serial.println(gps_data.lon.coord);		//デバッグ用
			
			
			//LCD表示
			strcpy(lin1, "Now chasing!!!");
			sprintf(lin2, "LAT:    %ld",gps_data.lat.coord);
			sprintf(lin3, "LON:   %ld",gps_data.lon.coord);
			strcpy(lin4, "");
			lcd_update();
			
			//ちょっと待たない
			//delay(500);
			
			break;
		
		default:
			//入らないはず、何もしない
			break;
	}
}

static void get_gps_data(){
	//GPSデータ取得部分
	i2c_rep_start(I2C_ADDRESS<<1);//スタートコンディションの発行＋コントロールバイトの送信
	i2c_write(I2C_ROM_ADDRESS);//ROMアドレス送信
	//delay(1500);
	delay(700);
	
	i2c_rep_start((I2C_ADDRESS<<1)|1);//スタートコンディションの再発行+コントロールバイトの送信
	gps_data.lat.data[0] = i2c_readNak();  //ROMデータ受信＋ストップコンディションの発行
	
	i2c_rep_start((I2C_ADDRESS<<1)|1);//スタートコンディションの再発行+コントロールバイトの送信
	gps_data.lat.data[1] = i2c_readNak();  //ROMデータ受信＋ストップコンディションの発行
	
	i2c_rep_start((I2C_ADDRESS<<1)|1);//スタートコンディションの再発行+コントロールバイトの送信
	gps_data.lat.data[2] = i2c_readNak();  //ROMデータ受信＋ストップコンディションの発行
	
	i2c_rep_start((I2C_ADDRESS<<1)|1);//スタートコンディションの再発行+コントロールバイトの送信
	gps_data.lat.data[3] = i2c_readNak();  //ROMデータ受信＋ストップコンディションの発行
	
	i2c_rep_start((I2C_ADDRESS<<1)|1);//スタートコンディションの再発行+コントロールバイトの送信
	gps_data.lon.data[0] = i2c_readNak();  //ROMデータ受信＋ストップコンディションの発行
	
	i2c_rep_start((I2C_ADDRESS<<1)|1);//スタートコンディションの再発行+コントロールバイトの送信
	gps_data.lon.data[1] = i2c_readNak();  //ROMデータ受信＋ストップコンディションの発行
	
	i2c_rep_start((I2C_ADDRESS<<1)|1);//スタートコンディションの再発行+コントロールバイトの送信
	gps_data.lon.data[2] = i2c_readNak();  //ROMデータ受信＋ストップコンディションの発行
	
	i2c_rep_start((I2C_ADDRESS<<1)|1);//スタートコンディションの再発行+コントロールバイトの送信
	gps_data.lon.data[3] = i2c_readNak();  //ROMデータ受信＋ストップコンディションの発行
	
	i2c_rep_start((I2C_ADDRESS<<1)|1);//スタートコンディションの再発行+コントロールバイトの送信
	gps_data.alt.data[0] = i2c_readNak();  //ROMデータ受信＋ストップコンディションの発行
	
	i2c_rep_start((I2C_ADDRESS<<1)|1);//スタートコンディションの再発行+コントロールバイトの送信
	gps_data.alt.data[1] = i2c_readNak();  //ROMデータ受信＋ストップコンディションの発行
}

// 自動テイクオフコマンドを送信する
void send_takeoff_cmd(){
	uint8_t system_id = 20;
	uint8_t component_id = 200;
	uint8_t target_system = 1;
	uint8_t base_mode = MAV_MODE_FLAG_CUSTOM_MODE_ENABLED;
	uint32_t custom_mode = C_TAKEOFF;
	
	mavlink_message_t msg;
	uint8_t buf[MAVLINK_MAX_PACKET_LEN];
	uint16_t len;
	
	mavlink_msg_set_mode_pack(system_id, component_id, &msg, target_system, base_mode, custom_mode);
	
	len = mavlink_msg_to_send_buffer(buf, &msg);
	
	Serial.write(buf, len);
}

void send_chaser_cmd(int32_t lat, int32_t lon, int16_t alt){
	uint8_t system_id = 20;
	uint8_t component_id = 200;
	
	mavlink_message_t msg;
	uint8_t buf[MAVLINK_MAX_PACKET_LEN];
	uint16_t len;
	
	mavlink_msg_chaser_cmd_pack(system_id, component_id, &msg, lat, lon, alt);
	
	len = mavlink_msg_to_send_buffer(buf, &msg);
	
	Serial.write(buf, len);
}



// ***********************************************************************************************************
// I2C general functions
// ************************************************************************************************************

void i2c_init(void) {
	PORTC |= 1<<4;//ポート設定
	PORTC |= 1<<5;//ポート設定
	TWSR = 0;                                    // no prescaler => prescaler = 1
	TWBR = ((F_CPU / I2C_SPEED) - 16) / 2;       // change the I2C clock rate
	TWCR = 1<<TWEN;                              // enable twi module, no interrupt
}

void i2c_rep_start(uint8_t address) {//マスタがスレーブにデータを要求する
  TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN) ; // send REPEAT START condition
  waitTransmissionI2C();                       // wait until transmission completed（いるか？
  TWDR = address;                              // send device address
  TWCR = (1<<TWINT) | (1<<TWEN);              //TWINTクリア　マスタ送信開始
  waitTransmissionI2C();                       // wail until transmission completed
}

void i2c_stop(void) {
  TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);
  //  while(TWCR & (1<<TWSTO));                // <- can produce a blocking state with some WMP clones
}

void i2c_write(uint8_t data ) {
  TWDR = data;                                 // send data to the previously addressed device
  TWCR = (1<<TWINT) | (1<<TWEN);
  waitTransmissionI2C();
}

uint8_t i2c_read(uint8_t ack) {
  TWCR = (1<<TWINT) | (1<<TWEN) | (ack? (1<<TWEA) : 0);
  waitTransmissionI2C();
  uint8_t r = TWDR;
  if (!ack) i2c_stop();
  return r;
}

uint8_t i2c_readAck() {
  return i2c_read(1);
}

uint8_t i2c_readNak(void) {
  return i2c_read(0);
}

void waitTransmissionI2C() {
  uint16_t count = 255;
  while (!(TWCR & (1<<TWINT))) {
    count--;
    if (count==0) {              //we are in a blocking state => we don't insist
      TWCR = 0;                  //and we force a reset on TWINT register
      //neutralizeTime = micros(); //we take a timestamp here to neutralize the value during a short delay
      //i2c_errors_count++;
      break;//この中は、ハングアップした時用の退避コマンドくさい
    }
  }
}


//以下データ処理用　本質的には関係ない
size_t i2c_read_to_buf(uint8_t add, void *buf, size_t size) {
  i2c_rep_start((add<<1) | 1);  // I2C read direction
  size_t bytes_read = 0;
  uint8_t *b = (uint8_t*)buf;
  while (size--) {
    /* acknowledge all but the final byte */
    *b++ = i2c_read(size > 0);
    /* TODO catch I2C errors here and abort */
    bytes_read++;
  }
  return bytes_read;
}

size_t i2c_read_reg_to_buf(uint8_t add, uint8_t reg, void *buf, size_t size) {
  i2c_rep_start(add<<1); // I2C write direction
  i2c_write(reg);        // register selection
  return i2c_read_to_buf(add, buf, size);
}

/* transform a series of bytes from big endian to little
   endian and vice versa. */
void swap_endianness(void *buf, size_t size) {
  /* we swap in-place, so we only have to
  * place _one_ element on a temporary tray
  */
  uint8_t tray;
  uint8_t *from;
  uint8_t *to;
  /* keep swapping until the pointers have assed each other */
  for (from = (uint8_t*)buf, to = &from[size-1]; from < to; from++, to--) {
    tray = *from;
    *from = *to;
    *to = tray;
  }
}

void i2c_getSixRawADC(uint8_t add, uint8_t reg) {
  i2c_read_reg_to_buf(add, reg, &rawADC, 6);
}

void i2c_writeReg(uint8_t add, uint8_t reg, uint8_t val) {
  i2c_rep_start(add<<1); // I2C write direction
  i2c_write(reg);        // register selection
  i2c_write(val);        // value to write in register
  i2c_stop();
}

uint8_t i2c_readReg(uint8_t add, uint8_t reg) {
  uint8_t val;
  i2c_read_reg_to_buf(add, reg, &val, 1);
  return val;
}


void lcd_update()
{
  lcd.begin(20, 4);
  lcd.clear();
  lcd.cursor();
  lcd.blink();
	
  lcd.setCursor(0, 0);
  lcd.print(lin1);
	
  lcd.setCursor(0, 1);
  lcd.print(lin2);
	
  lcd.setCursor(0, 2);
  lcd.print(lin3);
	
  lcd.setCursor(0, 3);
  lcd.print(lin4);
	
}
