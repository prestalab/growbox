#include <EEPROM.h>
#include <LiquidCrystal.h>
#include <ABounce.h>
#include <DS1302.h>
#include <DHT.h>
#include <PID_v1.h>

//---------------Пины---------------
//Вентилятор
#define FAN_PIN 10
//Нагреватель
#define HEATER_PIN 11
//Увлажнитель
#define HUMIDIFIER_PIN 12
//Освещение
#define LIGHT_PIN A1
//Блок питания
#define POWER_PIN A2

//---------------Настройки---------------
//Интервал включения вентилятора, час
#define FAN_PERIOD 0
//Время работы вентиялтора, мин
#define FAN_DURATION 1
//Поддерживаемая температура
#define HEATER_TEMPERATURE 2
//Поддерживаемая влажность
#define HUMIDIFIER_VALUE 3
//Время включения освещения, час
#define LIGHT_POWER_ON 4
//Время выключения освещения, час
#define LIGHT_POWER_OFF 5
//Предустановленные настройки
#define WORK_MODE 6

#define FAN_ON 7
#define HUMIDIFIER_ON 8
#define HEATER_ON 9
#define LIGHT_ON 10

//ПИД, окна и коээфициенты
#define DHT_WINDOW 5000
#define HUMIDITY_KP 6
#define HUMIDITY_KI -0.1
#define HUMIDITY_KD 0.1
#define TEMPERATURE_KP 6
#define TEMPERATURE_KI -0.1
#define TEMPERATURE_KD 0.1

//Интервал обновления значений датчика
#define UPDATE_INTERVAL 1000

//Переменные
int setFanPeriod, setFanDuration, setLightPowerOn, setLightPowerOff;
byte setFanOn, setLightOn, setHeaterOn, setHumidifierOn, setWorkMode;
//Статусы выходов
byte fanState = 0, heaterState = 0, humidifierState = 0, lightState = 0, powerState = 0, menuState = 0;
double setHeaterTemperature, setHumidifierValue;
//Вход и выход ПИД регулятора
double inHumidity, outHumidity, inTemperature, outTemperature;
// храним время последнего обновления
long previousMillis = 0;

unsigned long windowStartTime;

byte gradus[8] = {
  0b00110,
  0b01001,
  0b01001,
  0b00110,
  0b00000,
  0b00000,
  0b00000,
  0b00000
};

//Часы
DS1302 rtc(3, 1, 0);
//Экран
LiquidCrystal lcd(8, 9, 4, 5, 6, 7 );
//Кнопки
ABounce bouncer = ABounce(A0, 200);
//Температура и влажность
DHT dht(2, DHT22);
//Управление влажностью
PID humidityPID(&inHumidity, &outHumidity, &setHumidifierValue, HUMIDITY_KP, HUMIDITY_KI, HUMIDITY_KD, DIRECT);
//Управление температурой
PID temperaturePID(&inTemperature, &outTemperature, &setHeaterTemperature, TEMPERATURE_KP, TEMPERATURE_KI, TEMPERATURE_KD, DIRECT);

void setup()
{
	//Начальные настройки
	/*EEPROM.write(FAN_PERIOD, 3);
	EEPROM.write(FAN_DURATION, 30);
	EEPROM.write(HEATER_TEMPERATURE, 27.00);
	EEPROM.write(HUMIDIFIER_VALUE, 90);
	EEPROM.write(LIGHT_POWER_ON, 6);
	EEPROM.write(LIGHT_POWER_OFF, 18);
	EEPROM.write(LIGHT_ON, 1);
	EEPROM.write(FAN_ON, 1);
	EEPROM.write(HEATER_ON, 1);
	EEPROM.write(HUMIDIFIER_ON, 0);
	EEPROM.write(WORK_MODE, 0);*/

	/*
	rtc.setDOW(SUNDAY);
	rtc.setTime(9, 45, 0);
	rtc.setDate(8, 11, 2014);*/
	

	//Загружаем настройки
	setFanPeriod = EEPROM.read(FAN_PERIOD);
	setFanDuration = EEPROM.read(FAN_DURATION);
	setHeaterTemperature = EEPROM.read(HEATER_TEMPERATURE);
	setHumidifierValue = EEPROM.read(HUMIDIFIER_VALUE);
	setLightPowerOn = EEPROM.read(LIGHT_POWER_ON);
	setLightPowerOff = EEPROM.read(LIGHT_POWER_OFF);
	setFanOn = EEPROM.read(FAN_ON);
	setHeaterOn = EEPROM.read(HEATER_ON);
	setHumidifierOn = EEPROM.read(HUMIDIFIER_ON);
	setLightOn = EEPROM.read(LIGHT_ON);
	setWorkMode = EEPROM.read(WORK_MODE);

	//Включаем выходы
	pinMode(FAN_PIN, OUTPUT);
	pinMode(HEATER_PIN, OUTPUT);
	pinMode(HUMIDIFIER_PIN, OUTPUT);
	pinMode(LIGHT_PIN, OUTPUT);
	pinMode(POWER_PIN, OUTPUT);

	digitalWrite(FAN_PIN, HIGH);
	digitalWrite(HEATER_PIN, HIGH);
	digitalWrite(HUMIDIFIER_PIN, HIGH);
	digitalWrite(LIGHT_PIN, HIGH);
	digitalWrite(POWER_PIN, HIGH);

	//Включаем экран
	lcd.createChar(1, gradus);
	lcd.begin(16, 2);
	
	//Включаем часы
	rtc.halt(false);
	rtc.writeProtect(false);
	//Включаем датчик влажности и температуры
	dht.begin();

	//ПИД
	humidityPID.SetOutputLimits(0, DHT_WINDOW);
	humidityPID.SetMode(AUTOMATIC);
	temperaturePID.SetOutputLimits(0, DHT_WINDOW);
	temperaturePID.SetMode(AUTOMATIC);
	windowStartTime = millis();

	//Serial.begin(9600);
}

void loop()
{

	if (!menuState)
	{
		unsigned long currentMillis = millis();

		//проверяем не прошел ли нужный интервал, если прошел то
		if (currentMillis - previousMillis > UPDATE_INTERVAL)
		{
			// сохраняем время последнего переключения
			previousMillis = currentMillis; 


			//Значения температуры и влажности от датчиков
			inHumidity = dht.readHumidity();
			inTemperature = dht.readTemperature();

			//Выводим на экран
			lcd.setCursor(0, 0);
			lcd.print("T=");
			lcd.print(inTemperature, 1);
			lcd.write(1);
			lcd.print("C H=");
			lcd.print(inHumidity, 1);
			lcd.print("%");
			lcd.setCursor(0, 1);
			lcd.print(rtc.getTimeStr());
			lcd.setCursor(8, 1);
			lcd.print(rtc.getDateStr(FORMAT_SHORT));
		
			//Управление влажностью и температурой
			if (millis() - windowStartTime>DHT_WINDOW)
			{
				windowStartTime += DHT_WINDOW;
			}

			if (setHumidifierOn)
			{
				humidityPID.Compute();
				if (outHumidity*1000 < millis() - windowStartTime)
				{
					humidifierChange(0);
				}
				else
				{
					humidifierChange(1);
				}
			}
			if (setHeaterOn)
			{
				temperaturePID.Compute();
				if (outTemperature*1000 < millis() - windowStartTime)
				{
					heaterChange(0);
				}
				else
				{
					heaterChange(1);
				}
			}

			//Включение освещения по расписанию
			Time t = rtc.getTime();
			if (setLightOn)
			{
				if (setLightPowerOff == t.hour && t.min == 0)
				{
					lightChange(1);
				}
				else if (setLightPowerOn == t.hour && t.min == 0)
				{
					lightChange(0);
				}
			}

			//Включение вентилятора по расписанию
			if (setFanOn)
			{
				if (setFanPeriod % t.hour == 0 && t.min == 0)
				{
					fanChange(1);
				}
				else if (setFanPeriod % t.hour == 0 && t.min == setFanDuration)
				{
					fanChange(0);
				}
			}
		}
	}

	//Ручное управление
	if (bouncer.update())
	{
		switch (bouncer.read())
		{
			case BUTTON_SELECT:
				menuState ^= 1;
				lcd.noBlink();
				if (menuState)
					displayMenu();
				break;
			case BUTTON_UP:
				if (!menuState)
				{
					fanChange(fanState ^ 1);
				}
				break;
			case BUTTON_DOWN:
				if (!menuState)
				{
					lightChange(lightState ^ 1);
				}
				break;
			case BUTTON_LEFT:

				break;
			case BUTTON_RIGHT:

				break;
		}
	}
}

//Включение - выключение увлажнителя
void humidifierChange(byte newState)
{
	if (newState)
	{
		powerChange(1);
		digitalWrite(HUMIDIFIER_PIN, LOW);
		humidifierState = 1;
	}
	else
	{
		digitalWrite(HUMIDIFIER_PIN, HIGH);
		powerChange(0);
		humidifierState = 0;
	}
}

//Включение - выключение нагревателя
void heaterChange(byte newState)
{
	if (newState)
	{
		powerChange(1);
		digitalWrite(HEATER_PIN, LOW);
		heaterState = 1;
	}
	else
	{
		digitalWrite(HEATER_PIN, HIGH);
		powerChange(0);
		heaterState = 0;
	}
}

//Включение - выключение света
void lightChange(byte newState)
{
	if (newState)
	{
		powerChange(1);
		digitalWrite(LIGHT_PIN, LOW);
		lightState = 1;
	}
	else
	{
		digitalWrite(LIGHT_PIN, HIGH);
		powerChange(0);
		lightState = 0;
	}
}

//Включение - выключение вентилятора
void fanChange(byte newState)
{
	if (newState)
	{
		powerChange(1);
		digitalWrite(FAN_PIN, LOW);
		fanState = 1;
	}
	else
	{
		digitalWrite(FAN_PIN, HIGH);
		powerChange(0);
		fanState = 0;
	}
}

//Включение - выключение блока питания. Выключение, если все приборы отключены
void powerChange(byte newState)
{
	if (newState)
	{
		digitalWrite(POWER_PIN, LOW);
		powerState = 1;
	}
	else
	{
		if (!(fanState || lightState || heaterState || humidifierState))
		{
			digitalWrite(POWER_PIN, HIGH);
			powerState = 0;
		}
	}
}

byte menuId = 0, editMenu = 0, pos = 0;

void displayMenu()
{
	byte tMinute, tHour, tDay, tMonth, tYear;
	Time t = rtc.getTime();
	tMinute = t.min;
	tHour = t.hour;
	tDay = t.date;
	tMonth = t.mon;
	tYear = t.year;
	updateMenu();
	while (menuState)
	{
		if (bouncer.update())
		{
			switch (bouncer.read())
			{
				case BUTTON_SELECT:
					if (editMenu)
					{
						switch (menuId)
						{
							case 0:
								setMode();
								break;
							case 1:
								EEPROM.write(HEATER_TEMPERATURE, setHeaterTemperature);
								EEPROM.write(HEATER_ON, setHeaterOn);
							case 2:
								EEPROM.write(HUMIDIFIER_VALUE, setHumidifierValue);
								EEPROM.write(HUMIDIFIER_ON, setHumidifierOn);
							case 3:
								EEPROM.write(LIGHT_ON, setLightOn);
								EEPROM.write(LIGHT_POWER_ON, setLightPowerOn);
								EEPROM.write(LIGHT_POWER_OFF, setLightPowerOff);
							case 4:
								EEPROM.write(FAN_ON, setFanOn);
								EEPROM.write(FAN_PERIOD, setFanPeriod);
								EEPROM.write(FAN_DURATION, setFanDuration);
							case 5:
								rtc.setTime(tHour, tMinute, 0);
								rtc.setDate(tDay, tMonth, tYear);
						}
					}
					menuState = 0;
					editMenu = 0;
					pos = 0;
					lcd.noBlink();
					lcd.clear();
					break;
				case BUTTON_UP:
					if (editMenu)
					{
						switch (menuId)
						{
							case 0:
								setWorkMode++;
								if (setWorkMode == 4)
									setWorkMode = 0;
								updateMenu();
								break;
							case 1:
								if (pos==0)
									setHeaterOn ^= 1;
								else
								{
									setHeaterTemperature++;
								}
								updateMenu();
								break;
							case 2:
								if (pos==0)
									setHumidifierOn ^= 1;
								else
								{
									setHumidifierValue++;
								}
								updateMenu();
								break;
							case 3:
								if (pos==0)
									setLightOn ^= 1;
								else if (pos == 4)
								{
									setLightPowerOn++;
								}
								else
								{
									setLightPowerOff++;
								}
								updateMenu();
								break;
							case 4:
								if (pos==0)
									setFanOn ^= 1;
								else if (pos == 4)
								{
									setFanPeriod++;
								}
								else
								{
									setFanDuration++;
								}
								updateMenu();
								break;
							case 5:
								switch (pos)
								{
									case 0:
										tHour++;
										lcd.setCursor(pos, 1);
										lcd.print(tHour);
										break;
									case 3:
										tMinute++;
										lcd.setCursor(pos, 1);
										lcd.print(tMinute);
										break;
									case 6:
										tDay++;
										lcd.setCursor(pos, 1);
										lcd.print(tDay);
										break;
									case 9:
										tMonth++;
										lcd.setCursor(pos, 1);
										lcd.print(tMonth);
										break;
									case 12:
										tYear++;
										lcd.setCursor(pos, 1);
										lcd.print(tYear);
										break;
								}
								break;
						}
						lcd.setCursor(pos, 1);
					}
					else
					{
						menuId--;
						updateMenu();
					}
					break;
				case BUTTON_DOWN:
					if (editMenu)
					{
						switch (menuId)
						{
							case 0:
								setWorkMode--;
								if (setWorkMode == 0xFF)
									setWorkMode = 3;
								updateMenu();
								break;
							case 1:
								if (pos==0)
									setHeaterOn ^= 1;
								else
								{
									setHeaterTemperature--;
								}
								updateMenu();
								break;
							case 2:
								if (pos==0)
									setHumidifierOn ^= 1;
								else
								{
									setHumidifierValue--;
								}
								updateMenu();
								break;
							case 3:
								if (pos==0)
									setLightOn ^= 1;
								else if (pos == 4)
								{
									setLightPowerOn--;
								}
								else
								{
									setLightPowerOff--;
								}
								updateMenu();
								break;
							case 4:
								if (pos==0)
									setFanOn ^= 1;
								else if (pos == 4)
								{
									setFanPeriod--;
								}
								else
								{
									setFanDuration--;
								}
								updateMenu();
								break;
							case 5:
								switch (pos)
								{
									case 0:
										tHour--;
										lcd.setCursor(pos, 1);
										lcd.print(tHour);
										break;
									case 3:
										tMinute--;
										lcd.setCursor(pos, 1);
										lcd.print(tMinute);
										break;
									case 6:
										tDay--;
										lcd.setCursor(pos, 1);
										lcd.print(tDay);
										break;
									case 9:
										tMonth--;
										lcd.setCursor(pos, 1);
										lcd.print(tMonth);
										break;
									case 12:
										tYear--;
										lcd.setCursor(pos, 1);
										lcd.print(tYear);
										break;
								}
								break;
						}
						lcd.setCursor(pos, 1);
					}
					else
					{
						menuId++;
						updateMenu();
					}
					break;
				case BUTTON_LEFT:
					if (editMenu)
					{
						switch (menuId)
						{
							case 0:
								lcd.setCursor(1, 1);
								break;
							case 1:
							case 2:
								if (pos == 0)
									pos = 4;
								else if (pos == 4)
									pos = 0;
								lcd.setCursor(pos, 1);
								break;
							case 3:
							case 4:
								if (pos == 0)
									pos = 7;
								else if (pos == 7)
									pos = 4;
								else if (pos == 4)
									pos = 0;
								lcd.setCursor(pos, 1);
								break;
							case 5:
								pos -=3;
								if (pos == 0xFD)
									pos = 12;
								lcd.setCursor(pos, 1);
								break;
						}
					}
					break;
				case BUTTON_RIGHT:
					if (editMenu)
					{
						switch (menuId)
						{
							case 0:
								lcd.setCursor(1, 1);
								break;
							case 1:
							case 2:
								if (pos == 0)
									pos = 4;
								else if (pos == 4)
									pos = 0;
								lcd.setCursor(pos, 1);
								break;
							case 3:
							case 4:
								if (pos == 0)
									pos = 4;
								else if (pos == 4)
									pos = 7;
								else if (pos == 7)
									pos = 0;
								lcd.setCursor(pos, 1);
								break;
							case 5:
								pos +=3;
								if (pos == 15)
									pos = 0;
								lcd.setCursor(pos, 1);
								break;
						}
					}
					else
					{
						editMenu = 1;
						lcd.setCursor(pos, 1);
						lcd.blink();
					}
					break;
			}
		}
	}
}

void setMode()
{
	setFanPeriod = 3;
	setFanDuration = 15;
	setHeaterTemperature = 26;
	setHumidifierValue = 90;
	setLightPowerOn = 6;
	setLightPowerOff = 18;
	setFanOn = 1;
	setHeaterOn = 1;
	setHumidifierOn = 1;
	setLightOn = 1;
	switch (setWorkMode)
	{
		case 0:
			setFanOn = 0;
			setHumidifierOn = 0;
			setLightOn = 0;
			break;
		case 1:
			setFanOn = 0;
			setHumidifierOn = 1;
			setLightOn = 0;
			setHeaterTemperature = 27;
			break;
		case 2:
			setFanOn = 1;
			setHumidifierOn = 1;
			setLightOn = 1;
			setHeaterTemperature = 25;
			setHumidifierValue = 95;
			break;
		case 3:
			setFanOn = 1;
			setHumidifierOn = 1;
			setLightOn = 1;
			setHeaterTemperature = 24;
			break;
	}

	EEPROM.write(FAN_PERIOD, setFanPeriod);
	EEPROM.write(FAN_DURATION, setFanDuration);
	EEPROM.write(HEATER_TEMPERATURE, setHeaterTemperature);
	EEPROM.write(HUMIDIFIER_VALUE, setHumidifierValue);
	EEPROM.write(LIGHT_POWER_ON, setLightPowerOn);
	EEPROM.write(LIGHT_POWER_OFF, setLightPowerOff);
	EEPROM.write(LIGHT_ON, setLightOn);
	EEPROM.write(FAN_ON, setFanOn);
	EEPROM.write(HEATER_ON, setHeaterOn);
	EEPROM.write(HUMIDIFIER_ON, setHumidifierOn);
	EEPROM.write(WORK_MODE, setWorkMode);
}

void updateMenu()
{
	if (menuId == 6)
		menuId = 0;
	else if (menuId == 0xFF)
		menuId = 5;
	lcd.clear();
	lcd.setCursor(0, 0);
	switch (menuId)
	{
		case 0:
			lcd.print("Settings");
			lcd.setCursor(0, 1);
			switch (setWorkMode)
			{
				case 0:
					lcd.print("Substrat");
					break;
				case 1:
					lcd.print("Cover");
					break;
				case 2:
					lcd.print("Primordia");
					break;
				case 3:
					lcd.print("Fruiting");
					break;
			}
			break;
		case 1:
			lcd.print("Temperature");
			lcd.setCursor(0, 1);
			if (setHeaterOn)
				lcd.print("ON");
			else
				lcd.print("OFF");
			lcd.setCursor(4, 1);
			lcd.print(setHeaterTemperature, 0);
			break;
		case 2:
			lcd.print("Humidity");
			lcd.setCursor(0, 1);
			if (setHumidifierOn)
				lcd.print("ON");
			else
				lcd.print("OFF");
			lcd.setCursor(4, 1);
			lcd.print(setHumidifierValue, 0);
			break;
		case 3:
			lcd.print("Light");
			lcd.setCursor(0, 1);
			if (setLightOn)
				lcd.print("ON");
			else
				lcd.print("OFF");
			lcd.setCursor(4, 1);
			lcd.print(setLightPowerOn);
			lcd.setCursor(7, 1);
			lcd.print(setLightPowerOff);
			break;
		case 4:
			lcd.print("Fan");
			lcd.setCursor(0, 1);
			if (setFanOn)
				lcd.print("ON");
			else
				lcd.print("OFF");
			lcd.setCursor(4, 1);
			lcd.print(setFanPeriod);
			lcd.setCursor(7, 1);
			lcd.print(setFanDuration);
			break;
		case 5:
			Time t = rtc.getTime();
			lcd.print("Clock");
			lcd.setCursor(0, 1);
			lcd.print(t.hour);
			lcd.setCursor(2, 1);
			lcd.print(":");
			lcd.print(t.min);
			lcd.setCursor(6, 1);
			lcd.print(t.date);
			lcd.setCursor(8, 1);
			lcd.print("/");
			lcd.print(t.mon);
			lcd.setCursor(11, 1);
			lcd.print("/");
			lcd.print(t.year);
			break;
	}
}