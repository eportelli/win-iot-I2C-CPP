// Copyright (c) Microsoft. All rights reserved.

#include "pch.h"
#include "StartupTask.h"

using namespace BlinkyHeadlessCpp;

using namespace Platform;
using namespace Windows::ApplicationModel::Background;
using namespace Windows::Foundation;
using namespace Windows::Devices::Gpio;
using namespace Windows::Devices::I2c;
using namespace Windows::Devices::Enumeration;
using namespace Windows::System::Threading;
using namespace concurrency;

StartupTask::StartupTask()
{
}

void StartupTask::Run(IBackgroundTaskInstance^ taskInstance)
{
	Deferral = taskInstance->GetDeferral();
	InitGpio();
	InitI2C();
	TimerElapsedHandler ^handler = ref new TimerElapsedHandler(
		[this](ThreadPoolTimer ^timer)
	{
		pinInputValue = Inpin->Read();
		pinValue = (pinInputValue == GpioPinValue::High) ? GpioPinValue::Low : GpioPinValue::High;
		pin->Write(pinValue);
	});

	TimeSpan interval;
	interval.Duration = 500 * 1000 * 10;
	//Timer = ThreadPoolTimer::CreatePeriodicTimer(handler, interval);
	
}

void StartupTask::InitGpio()
{
	pin = GpioController::GetDefault()->OpenPin(LED_PIN);
	pinValue = GpioPinValue::High;
	pin->Write(pinValue);
	pin->SetDriveMode(GpioPinDriveMode::Output);
	Inpin = GpioController::GetDefault()->OpenPin(IN_PIN);
	Inpin->SetDriveMode(GpioPinDriveMode::Input);
	TimeSpan interval;
	interval.Duration = 50 * 1000 * 10;
	Inpin->DebounceTimeout = interval;
	Inpin->ValueChanged += ref new Windows::Foundation::TypedEventHandler<GpioPin ^, GpioPinValueChangedEventArgs ^>(this, &StartupTask::pinValueChangedEventHandler);

}


task<void> StartupTask::InitI2C()
{
	
	String^ i2cDeviceSelector = I2cDevice::GetDeviceSelector("I2C1");
//	auto i2cDeviceControllers = DeviceInformation::FindAllAsync(i2cDeviceSelector);

	return create_task(DeviceInformation::FindAllAsync(i2cDeviceSelector))
		.then([this](DeviceInformationCollection^ devices) {

		auto BMP280Settings = ref new I2cConnectionSettings(0x29); //0x29 for ICS34725
		
		return I2cDevice::FromIdAsync(devices->GetAt(0)->Id, BMP280Settings);
	}).then([this](I2cDevice^ i2cDevice) {
		BMP280Sensor = i2cDevice;
		I2CStartTimer();

	});
	//ConnectionSettings
	//DeviceId
	//FromIdAsync
	//WriteRead
}

void StartupTask::I2CStartTimer()
{


	TimerElapsedHandler ^i2chandler = ref new TimerElapsedHandler(
		[this](ThreadPoolTimer ^i2ctimer)
	{
		auto command = ref new Array<uint8>(1);
		auto pressure = ref new Array<uint8>(3);
		auto temperature = ref new Array<uint8>(3);
		auto status = ref new Array<uint8>(1);
		auto enablereg = ref new Array<uint8>(2);
		auto initialized = ref new Array<uint8>(1);
		auto clearlight = ref new Array<uint8_t>(2);
		auto IntegrationTime = ref new Array<uint8_t>(2);
		auto GainSet = ref new Array<uint8_t>(2);
		auto InterruptSettings = ref new Array<uint8_t>(2);

		if (initLight == 0)
		{
			//Set Integration time
			IntegrationTime[0] = 0x80 | 0x0F;
			IntegrationTime[1] = 0x00 & 0xFF;
			BMP280Sensor->Write(IntegrationTime);

			//Set Gain
			GainSet[0] = 0x80 | 0x01;
			GainSet[1] = 0xFF & 0xFF;
			BMP280Sensor->Write(GainSet);



			//TCS34725 enable
			command[0] = 0x00;
			enablereg[0] = 0x00;
			enablereg[1] = 0x01 & 0xFF;
			BMP280Sensor->Write(enablereg);
			Sleep(3);
			enablereg[1] = (0x01 | 0x02) & 0xFF;
			BMP280Sensor->Write(enablereg);

			//Clear Interrupts
			command[0] = 0x80 | 0x66;
			BMP280Sensor->Write(command);

			//Set Max, Min Interrupt settings
			InterruptSettings[0] = 0x80 | 0x04;
			InterruptSettings[1] = 0x00 & 0xFF;
			BMP280Sensor->Write(InterruptSettings);

			InterruptSettings[0] = 0x80 | 0x05;
			InterruptSettings[1] = 0x00 & 0xFF;
			BMP280Sensor->Write(InterruptSettings);

			InterruptSettings[0] = 0x80 | 0x06;
			InterruptSettings[1] = 0xFF & 0xFF;
			BMP280Sensor->Write(InterruptSettings);

			InterruptSettings[0] = 0x80 | 0x07;
			InterruptSettings[1] = 0xFF & 0xFF;
			BMP280Sensor->Write(InterruptSettings);

			command[0] = 0x80 | 0x12; //request ID
			BMP280Sensor->WriteRead(command, initialized);
			//BMP280Sensor->Read(initialized);
			uint8_t _init = initialized[0];
			if (initialized[0] == 0x44)
			{
				initLight = 1;
			}
		}


			command[0] = 0x80 | 0x14;
		//BMP280Sensor->WriteRead(command, clearlight);
		BMP280Sensor->WriteRead(command, clearlight);
//		BMP280Sensor->Read(clearlight);

		uint16_t clight = clearlight[1];
		clight<<= 8;
		clight |= clearlight[0];

//		clight = clearlight[1];
//		clight <<= 8;
		
	});

	TimeSpan i2cinterval;
	i2cinterval.Duration = 5000 * 1000 * 10;
	Timer = ThreadPoolTimer::CreatePeriodicTimer(i2chandler, i2cinterval);
}

void StartupTask::write8(uint8 reg, uint8 value)
{


}

void StartupTask::pinValueChangedEventHandler(GpioPin^ gpioPin, GpioPinValueChangedEventArgs^ eventArgs)
{
	// No update of GUI componnents
	if (eventArgs->Edge == GpioPinEdge::FallingEdge)
	{
		//pinInputValue = Inpin->Read();

		pinInputValue = (pinInputValue == GpioPinValue::High) ? GpioPinValue::Low : GpioPinValue::High;
		pin->Write(pinInputValue);
	}
}

