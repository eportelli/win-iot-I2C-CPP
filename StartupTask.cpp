// Copyright (c) Microsoft. All rights reserved.

#include "pch.h"
#include "StartupTask.h"

using namespace BlinkyHeadlessCpp;

using namespace Platform;
using namespace Windows::ApplicationModel::Background;
using namespace Windows::Foundation;
using namespace Windows::Devices::Gpio;
using namespace Windows::System::Threading;
using namespace concurrency;

StartupTask::StartupTask()
{
}

void StartupTask::Run(IBackgroundTaskInstance^ taskInstance)
{
	Deferral = taskInstance->GetDeferral();
	InitGpio();
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

void StartupTask::pinValueChangedEventHandler(GpioPin^ gpioPin, GpioPinValueChangedEventArgs^ eventArgs)
{
	// No update of GUI componnents
	// Toggle function - Eatch push buttun change led light
	if (eventArgs->Edge == GpioPinEdge::FallingEdge)
	{
		//pinInputValue = Inpin->Read();

		pinInputValue = (pinInputValue == GpioPinValue::High) ? GpioPinValue::Low : GpioPinValue::High;
		pin->Write(pinInputValue);
	}
}

