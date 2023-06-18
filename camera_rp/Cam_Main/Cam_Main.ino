#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <ArduCAM.h>
#include <ov2640_regs.h>

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
int k = 60;

void setup(){
  Serial.begin(9600);
  delay(2000);
	Serial.println(F("Less Go"));
	
  //SD初期化編
  //pinMode(PIN_SD, OUTPUT);
  //digitalWrite(PIN_SD, HIGH);
  SPI.setRX(4);
  SPI.setTX(3);
  SPI.setSCK(2);
  SPI.begin();
	if (!SD.begin(PIN_SD)) {
		Serial.println(F("SD Card ❌"));
		//while (1);
	}else{
  	Serial.println("SD Card 🏆 +" + String(k--) + " ポイント");
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

	// Reset the CPLD
	myCAM.write_reg(0x07, 0x80);
	delay(100);
	myCAM.write_reg(0x07, 0x00);
	delay(100);

  while (1) {
		// Check if the ArduCAM SPI bus is OK
		myCAM.write_reg(ARDUCHIP_TEST1, 0x55);
		uint8_t temp = myCAM.read_reg(ARDUCHIP_TEST1);
		if (temp != 0x55){
			Serial.println(F("Camera ❌"));
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
	myCAM.write_reg(ARDUCHIP_FRAMES, 0);

	myCAM.CS_HIGH();		// これしておかないとSDにかけない．

  Serial.println("Camera 🏁 +" + String(k--) + " ポイント");

}

void loop(){
  if(k > 0)CAM2_TakePic();
  delay(1000);
}

//CanSatForHighSchool より引用
void CAM2_TakePic() {
	myCAM.CS_HIGH();
	int total_time = 0;

	//SD_Write(F("TakePic"));

	myCAM.CS_LOW();
	myCAM.flush_fifo();
	myCAM.clear_fifo_flag();
	myCAM.OV2640_set_JPEG_size(OV2640_160x120);		// FIXME: ここも要検討

	myCAM.start_capture();
	//Serial.println(F("CAMERA: Start Capture"));

	total_time = millis();

	while ( !myCAM.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK));

	//Serial.println(F("CAMERA: Capture Done."));
	total_time = millis() - total_time;
	//Serial.print(total_time);
	//Serial.println(F(" ms elapsed."));

	total_time = millis();

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
		Serial.println("CAMERA: Failed to take pic ❌");
		return 0;
	}
	if (length == 0 ) {		// 0 kb
		//Serial.println(F("CAMERA: Size is 0."));
		Serial.println("CAMERA: Failed to take pic ❌");
		return 0;
	}

	myCAM.CS_LOW();
	myCAM.set_fifo_burst();  // Set fifo burst mode

	int i = 0;
	bool is_header = false;
	File outFile;
	uint8_t temp = 0, temp_last = 0;
	while ( length-- ) {
		temp_last = temp;
		temp =  SPI.transfer(0x00);
		// Read JPEG data from FIFO
		if ( (temp == 0xD9) && (temp_last == 0xFF) ) {	// If find the end ,break while,
			buf[i++] = temp;  // save the last  0XD9
			// Write the remain bytes in the buffer
			myCAM.CS_HIGH();
			outFile.write(buf, i);
			// Close the file
			outFile.close();
			//Serial.println(F("CAMERA: Save OK"));
			is_header = false;
			myCAM.CS_LOW();
			myCAM.set_fifo_burst();
			i = 0;
		}
		if (is_header == true) {
			// Write image data to buffer if not full
			if (i < 256) {
				buf[i++] = temp;
			} else {
				// Write 256 bytes image data to file
				myCAM.CS_HIGH();
				outFile.write(buf, 256);
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
			//SD_Write("picname:"String(filename));
			Serial.println(String(filename) + " 🏁 +" + String(k--) + " ポイント");
			//Serial.println(SD_GetDirName() + String(filename));

			// k = k + 1;
			// itoa(k, str, 10);
			// strcat(str, ".jpg");
			// Open the new file
			// outFile = SD.open(str, O_WRITE | O_CREAT | O_TRUNC);
			// outFile = SD.open(SD_GetDirName() + String(filename), FILE_WRITE);
			outFile = SD.open(String(filename), O_WRITE | O_CREAT | O_TRUNC);
			// outFile = SD.open(String(filename), O_WRITE | O_CREAT | O_TRUNC);
			if (! outFile)
			{
				Serial.println(F("SD: File open failed"));
				// while (1);
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
	strcpy(filename, "000.JPG");
	for (uint16_t i = 0; i < 1000; i++) {
		filename[0] = '0' + i/100;
		filename[1] = '0' + (i/10)%10;
		filename[2] = '0' + i%10;
		if (!SD.exists(String(filename)) ) {
			break;
		}
	}
	// FIXME: 上限枚数に達したときの挙動
}