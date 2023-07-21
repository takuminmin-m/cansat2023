#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <ArduCAM.h>
#include <ov2640_regs.h>
#include <Adafruit_NeoPixel.h>

static mutex_t usb_mutex;
void SerialUsb(uint8_t* buffer,uint32_t lenght);
int SerialUSBAvailable(void);
int SerialUsbRead(void);
#define BMPIMAGEOFFSET 66
uint8_t bmp_header[BMPIMAGEOFFSET] =
{
  0x42, 0x4D, 0x36, 0x58, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x42, 0x00, 0x00, 0x00, 0x28, 0x00,
  0x00, 0x00, 0x40, 0x01, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x00, 0x01, 0x00, 0x10, 0x00, 0x03, 0x00,
  0x00, 0x00, 0x00, 0x58, 0x02, 0x00, 0xC4, 0x0E, 0x00, 0x00, 0xC4, 0x0E, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0x00, 0x00, 0xE0, 0x07, 0x00, 0x00, 0x1F, 0x00,
  0x00, 0x00
};

// set pin 10 as the slave select for the digital pot:
const uint8_t CS = 1;
const uint8_t PIN_SD = 26;
bool is_header = false;
int mode = 0;
uint8_t start_capture = 0;
ArduCAM myCAM( OV2640, CS );
uint8_t read_fifo_burst(ArduCAM myCAM);
int k = 3000;

#define NUMPIXELS 1
#define NEO_PWR 11
#define NEOPIX 12

Adafruit_NeoPixel pixels(NUMPIXELS, NEOPIX, NEO_GRB + NEO_KHZ800);

void setup(){
  Serial.begin(9600);
  delay(2000);
	Serial.println(F(""));
	Serial.println(F(">> Less Go <<"));

  pixels.begin();
  pinMode(NEO_PWR, OUTPUT);
  digitalWrite(NEO_PWR, HIGH);

  bool sdQualified = false;
  bool cameraQualified = false;

  //SD初期化編
  //pinMode(PIN_SD, OUTPUT);
  //digitalWrite(PIN_SD, HIGH);

  //模試と味噌どっちがどっちだっけ()
  SPI.setRX(4);
  SPI.setTX(3);

  SPI.setSCK(2);
  SPI.begin();
	if (!SD.begin(PIN_SD)) {
		Serial.println(F("SD Card ❌"));
		//while (1);
	}else{
  	Serial.println("SD Card 🏆");// +" + String(k--) + " ポイント");
    sdQualified = true;
  }

  //カメラ初期化編
	pinMode(CS, OUTPUT);
	digitalWrite(CS, HIGH);
	// put your setup code here, to run once:
	//myCAM.InitCAM();
  myCAM.Arducam_init();

  Wire.setSDA(6);
  Wire.setSCL(7);
  Wire.begin();

	myCAM.write_reg(0x01, 0x01);

	// Reset the CPLD
	myCAM.write_reg(0x07, 0x80);
	delay(100);
	myCAM.write_reg(0x07, 0x00);
	delay(100);

  cameraQualified = true;
  while (1) {
		// Check if the ArduCAM SPI bus is OK
		myCAM.write_reg(ARDUCHIP_TEST1, 0x55);
		uint8_t temp = myCAM.read_reg(ARDUCHIP_TEST1);
		if (temp != 0x55){
      cameraQualified = false;
			delay(1000);break;// continue;
		} else {
      break;
		}
	}

  while (1) {
		// Check if the camera module type is OV2640
		uint8_t vid, pid;
		myCAM.wrSensorReg8_8(0xff, 0x01);
		myCAM.rdSensorReg8_8(OV2640_CHIPID_HIGH, &vid);
		myCAM.rdSensorReg8_8(OV2640_CHIPID_LOW, &pid);
		if ((vid != 0x26 ) && (( pid != 0x41 ) || ( pid != 0x42 ))) {
			Serial.println(F("CAMERA: ACK CMD Can't find OV2640 module!"));
      cameraQualified = false;
      break;
		}
		else {
			//Serial.println(F("CAMERA: ACK CMD OV2640 detected."));
      break;
		}
	}

	// Change to JPEG capture mode and initialize the OV5640 module
	myCAM.set_format(JPEG);
	myCAM.InitCAM();
	myCAM.clear_fifo_flag();
	myCAM.write_reg(ARDUCHIP_FRAMES, 1);

	myCAM.CS_HIGH();		// これしておかないとSDにかけない．

  if(!cameraQualified){
		Serial.println(F("Camera ❌"));
    LED(255, 0, 255);
  }else if(!sdQualified){
    Serial.println("Camera 🏆");// +" + String(k--) + " ポイント");
    LED(255, 255, 0);
  }else{
    Serial.println("Camera 🏁");// +" + String(k--) + " ポイント");
    LED(0, 255, 32);
  }

	delay(3000);

}

void loop(){
  if(k > 0)CAM2_TakePic();
  //delay(1000);
}

//CanSatForHighSchool より引用
void CAM2_TakePic() {
  LED(0, 255, 0);
	myCAM.CS_HIGH();
	int total_time = 0;

	//SD_Write(F("TakePic"));

	myCAM.CS_LOW();
	myCAM.flush_fifo();
	myCAM.clear_fifo_flag();
	myCAM.OV2640_set_JPEG_size(7);		// FIXME: ここも要検討

  /*
  トリエル値
  #define OV2640_160x120 		0	//160x120
  #define OV2640_176x144 		1	//176x144
  #define OV2640_320x240 		2	//320x240
  #define OV2640_352x288 		3	//352x288
  #define OV2640_640x480		4	//640x480
  #define OV2640_800x600 		5	//800x600
  #define OV2640_1024x768		6	//1024x768
  #define OV2640_1280x1024	7	//1280x1024
  #define OV2640_1600x1200	8	//1600x1200
  */

	myCAM.start_capture();
	//Serial.println(F("CAMERA: Start Capture"));

	total_time = millis();

	while ( !myCAM.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK));

	//Serial.println(F("CAMERA: Capture Done."));
	total_time = millis() - total_time;
	//Serial.print(total_time);
	Serial.println("Camera 💍 +" + String(total_time) + " ポイント");

	total_time = millis();

  LED(0, 0, 255);

	CAM2_save_to_sd_(myCAM);

	total_time = millis() - total_time;
	//Serial.print(total_time);
	//Serial.println(F(" ms elapsed."));

	// Clear the capture done flag
	myCAM.clear_fifo_flag();
}


uint8_t CAM2_save_to_sd_(ArduCAM myCAM) {
	// static int k = 0;
	// char str[16];

	byte buf[256];
	char filename[16];

	uint32_t length = myCAM.read_fifo_length();
	myCAM.CS_HIGH();
	//Serial.print(F("CAMERA: The fifo length is :"));
	//Serial.println(length, DEC);

	if (length >= MAX_FIFO_SIZE) {		// 8M
		//Serial.println("CAMERA: Over size.");
		Serial.println("Camera ❌");
    LED(255, 64, 0);
		return 0;
	}
	if (length == 0 ) {		// 0 kb
		//Serial.println(F("CAMERA: Size is 0."));
		Serial.println("Camera ❌");
		return 0;
    LED(64, 0, 255);
	}

	myCAM.CS_LOW();
	myCAM.set_fifo_burst();  // Set fifo burst mode

	int i = 0;
	bool is_header = false;
	File outFile;
	uint8_t temp = 0, temp_last = 0;

  // セーニャ(中の人=小池)「一度に撮った写真の最初の 1 枚のみノイズが入るみたいです。
  // セーニャ(中の人=小池)「同時に 2 枚撮影し、1 枚目をザラキーマして 2 枚目だけを保存するよう
  // セーニャ(中の人=小池)「プログラムを組んでありますわ。
  int n = 0;

	while ( length-- ) {
		temp_last = temp;
		temp =  SPI.transfer(0x00);
		// Read JPEG data from FIFO
		if ( (temp == 0xD9) && (temp_last == 0xFF) ) {	// If find the end ,break while,
			buf[i++] = temp;  // save the last  0XD9
			// Write the remain bytes in the buffer
			myCAM.CS_HIGH();
			if(n > 0)outFile.write(buf, i);
			// Close the file
			if(n > 0)outFile.close();
			//Serial.println(F("CAMERA: Save OK"));
			is_header = false;
			myCAM.CS_LOW();
			myCAM.set_fifo_burst();
			i = 0;
      n++;
		}
		if (is_header == true) {
			// Write image data to buffer if not full
			if (i < 256) {
				buf[i++] = temp;
			} else {
				// Write 256 bytes image data to file
				myCAM.CS_HIGH();
				if(n > 0)outFile.write(buf, 256);
				i = 0;
				buf[i++] = temp;
				myCAM.CS_LOW();
				myCAM.set_fifo_burst();
			}
		}
		else if ((temp == 0xD8) & (temp_last == 0xFF)) {
			//Serial.println("CAMERA: HEADER FOUND!!!");
			is_header = true;

			myCAM.CS_HIGH();
			CAM2_get_filename_(filename);

      // ベロニカ「外で投げられた❌はここでしっかり受け止めるわよ！
      // セーニャ(中の人=小池)「お姉さま、それより今ひざが痛いのですが回復できますか...？
      // ベロニカ「何言ってるの！？ホイミ要因はセーニャのほうでしょうよ！！第一、かいふく魔力が 1.5x10^4 もあるのに...
      if(filename[0] == 'X'){
		  		// while (1);
	  			Serial.println(String(filename) + " ❌");
          LED(255, 0, 0);
      }else{
			  //SD_Write("picname:"String(filename));
	  		//Serial.println(SD_GetDirName() + String(filename));
	  		// k = k + 1;
	  		// itoa(k, str, 10);
	  		// strcat(str, ".jpg");
	  		// Open the new file
	  		// outFile = SD.open(str, O_WRITE | O_CREAT | O_TRUNC);
	  		// outFile = SD.open(SD_GetDirName() + String(filename), FILE_WRITE);
	  		if(n > 0){
          outFile = SD.open(String(filename), O_WRITE | O_CREAT | O_TRUNC);
	    		// outFile = SD.open(String(filename), O_WRITE | O_CREAT | O_TRUNC);
		    	if (! outFile){
	  		  	Serial.println(String(filename) + " ❌");
		  		  // while (1);
            LED(255, 64, 128);
	  		  }else{
  			    Serial.println(String(filename) + " 🏁 (" + String(--k) + " Left)");
            LED(255, 255, 255);
          }
        }
      }
			myCAM.CS_LOW();
			myCAM.set_fifo_burst();
			buf[i++] = temp_last;
			buf[i++] = temp;

		}
	}

	myCAM.CS_HIGH();
	return 1;
}


void CAM2_get_filename_(char filename[16]) {
	myCAM.CS_HIGH();
	strcpy(filename, "0000.jpg");
  
  // セーニャ(中の人=小池)「ここはしっかり 2 分探索でピオリムしていきますわ！
  int a = 0; int b = 10000;
  while(true){
    int i = (b - a) / 2 + a;
		filename[0] = '0' + i/1000;
		filename[1] = '0' + (i/100)%100;
		filename[2] = '0' + (i/10)%10;
		filename[3] = '0' + i%10;
    if(a == b)break;
		if (!SD.exists(String(filename)) ) {
			b = i;
    }else{
      a = i + 1;
    }
	}

  // ベロニカ「ちょっとセーニャ、なんかコード内にこんな伝言があったんだけど...

	// FIXME: 上限枚数に達したときの挙動

  // セーニャ(中の人=小池)「お姉さまっ！それもしっかり直しておきましたわ！
  if(b == 10000){
    Serial.println("何がエンターテイメントじゃ！！");
    filename[0] = 'X';
    //filename[0]に X が返されたら外で撮影失敗ルートに移行
  }

}

void LED(int red, int green, int blue){
  pixels.setPixelColor(0, pixels.Color(red, green, blue));
  pixels.show();
}