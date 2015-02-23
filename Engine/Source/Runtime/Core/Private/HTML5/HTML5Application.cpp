// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "CorePrivatePCH.h"
#include "HTML5Application.h"
#include "HTML5Cursor.h"
#include "HTML5InputInterface.h"

#include "SDL_opengl.h"

DEFINE_LOG_CATEGORY_STATIC(LogHTML5Application, Log, All);

#if PLATFORM_HTML5_BROWSER
#include "emscripten.h"
#include "html5.h"

EM_BOOL mouse_callback(int eventType, const EmscriptenMouseEvent *e, void *userData)
{
	return 1;
}

EM_BOOL pointerlockchange_callback(int eventType, const EmscriptenPointerlockChangeEvent *Event, void *userData)
{
	UE_LOG(LogHTML5Application, Verbose, TEXT("PointerLockChangedEvent: Active:%d"), Event->isActive );

	static uint Prev = 0;
	// Generate a fake WindowsEnter event when the pointerlock goes from inactive to active. 
	if (Event->isActive && Prev == 0)
	{
		SDL_Event event;
		SDL_zero(event);
		event.type = SDL_WINDOWEVENT;
		event.window.event = SDL_WINDOWEVENT_ENTER;
		SDL_PushEvent(&event);
	}
	Prev = Event->isActive; 

	return 1; 
}

#endif 

static const uint32 MaxWarmUpTicks = 10; 

FHTML5Application* FHTML5Application::CreateHTML5Application()
{
	return new FHTML5Application();
}

FHTML5Application::FHTML5Application()
	: GenericApplication( MakeShareable( new FHTML5Cursor() ) )
	, ApplicationWindow(FHTML5Window::Make())
	, InputInterface( FHTML5InputInterface::Create(MessageHandler, Cursor ) )
	, WarmUpTicks(-1)
{

#if PLATFORM_HTML5_BROWSER
    // full screen will only be requested after the first click after the window gains focus. 
    // the problem is that because of security/UX reasons browsers don't allow pointer lock in main loop
    // but only through callbacks generated by browser. 

 	// work around emscripten bug where deffered browser requests are not called if there are no callbacks.
	emscripten_set_mousedown_callback("#canvas",0,1,mouse_callback);
	emscripten_set_pointerlockchange_callback(0,0,true,pointerlockchange_callback);
#endif 

}


void FHTML5Application::SetMessageHandler( const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler )
{
	GenericApplication::SetMessageHandler(InMessageHandler);
	InputInterface->SetMessageHandler( MessageHandler );
}

void FHTML5Application::PollGameDeviceState( const float TimeDelta )
{
	SDL_Event Event;
	while (SDL_PollEvent(&Event)) 
	{
		// Tick Input Interface. 
		switch (Event.type)
		{
				case SDL_WINDOWEVENT: 
				{
					SDL_WindowEvent windowEvent = Event.window;


					// ignore resized client Height/Width
#if PLATFORM_HTML5_BROWSER
					int fs;
					emscripten_get_canvas_size(&WindowWidth, &WindowHeight, &fs);
					UE_LOG(LogHTML5Application, Verbose, TEXT("emscripten_get_canvas_size: Width:%d, Height:%d, Fullscreen:%d"), WindowWidth, WindowHeight, fs);
#endif 

#if PLATFORM_HTML5_WIN32 
					WindowWidth = windowEvent.data1; 
					WindowHeight = windowEvent.data2; 
#endif 

					switch (windowEvent.event)
					{
					case SDL_WINDOWEVENT_SIZE_CHANGED:
						{
							UE_LOG(LogHTML5Application, Verbose, TEXT("WindowSizeChanged: Width:%d, Height:%d"), WindowWidth, WindowHeight);
							MessageHandler->OnSizeChanged(ApplicationWindow,WindowWidth,WindowHeight, false);
							MessageHandler->OnResizingWindow(ApplicationWindow);

							FDisplayMetrics DisplayMetrics;
							FDisplayMetrics::GetDisplayMetrics(DisplayMetrics);
							BroadcastDisplayMetricsChanged(DisplayMetrics);
						}
						break;
					case SDL_WINDOWEVENT_RESIZED:
						{
							UE_LOG(LogHTML5Application, Verbose, TEXT("WindowResized: Width:%d, Height:%d"), WindowWidth, WindowHeight);
							MessageHandler->OnResizingWindow(ApplicationWindow);

							FDisplayMetrics DisplayMetrics;
							FDisplayMetrics::GetDisplayMetrics(DisplayMetrics);
							BroadcastDisplayMetricsChanged(DisplayMetrics);
						}
						break;
					case SDL_WINDOWEVENT_ENTER:
						{
							UE_LOG(LogHTML5Application, Verbose, TEXT("WindowEnter"));
							MessageHandler->OnCursorSet();
							MessageHandler->OnWindowActivationChanged(ApplicationWindow, EWindowActivation::Activate); 
							WarmUpTicks = 0; 
						}
						break;
					case SDL_WINDOWEVENT_LEAVE:
						{
							UE_LOG(LogHTML5Application, Verbose, TEXT("WindowLeave"));
							MessageHandler->OnWindowActivationChanged(ApplicationWindow, EWindowActivation::Deactivate);
						}
						break;
					case SDL_WINDOWEVENT_FOCUS_GAINED:
						{
							UE_LOG(LogHTML5Application, Verbose, TEXT("WindowFocusGained"));
							MessageHandler->OnWindowActivationChanged(ApplicationWindow, EWindowActivation::Activate);
									WarmUpTicks = 0;
						}
						break;
					case SDL_WINDOWEVENT_FOCUS_LOST:
						{
							UE_LOG(LogHTML5Application, Verbose, TEXT("WindowFocusLost"));
							MessageHandler->OnWindowActivationChanged(ApplicationWindow, EWindowActivation::Deactivate);
						}
						break;
					 default:
						break;
					}
				}
			default:
			{
				InputInterface->Tick( TimeDelta,Event, ApplicationWindow);
			}
		}
	}
	InputInterface->SendControllerEvents();


	if ( WarmUpTicks >= 0)
		WarmUpTicks ++; 


	if ( WarmUpTicks == MaxWarmUpTicks  )
	{
        // browsers don't allow locking and hiding to work independently. use warmup ticks after the application has settled
        // on its mouse lock/visibility status.  This is necessary even in cases where the game doesn't want to locking because 
        // the lock status oscillates for few ticks before settling down. This causes a Browser UI pop even when we don't intend to lock.
        // see http://www.w3.org/TR/pointerlock more for information. 
#if PLATFORM_HTML5_WIN32
		SDL_Window* WindowHandle= SDL_GL_GetCurrentWindow();
		if (((FHTML5Cursor*)Cursor.Get())->LockStatus && !((FHTML5Cursor*)Cursor.Get())->CursorStatus)
		{
			SDL_SetWindowGrab(WindowHandle, SDL_TRUE);
			SDL_ShowCursor(SDL_DISABLE);
		    SDL_SetRelativeMouseMode(SDL_TRUE);
		}
		else
		{
			SDL_SetRelativeMouseMode(SDL_FALSE);
			SDL_ShowCursor(SDL_ENABLE);
			SDL_SetWindowGrab(WindowHandle, SDL_FALSE);
		}
#endif 

#if PLATFORM_HTML5_BROWSER
		if (((FHTML5Cursor*)Cursor.Get())->LockStatus && !((FHTML5Cursor*)Cursor.Get())->CursorStatus)
		{
			UE_LOG(LogHTML5Application, Verbose, TEXT("Request pointer lock"));
			emscripten_request_pointerlock ( "#canvas" , true);
		}
		else
		{
			UE_LOG(LogHTML5Application, Verbose, TEXT("Exit pointer lock"));
			emscripten_exit_pointerlock(); 
		}
#endif 

		WarmUpTicks = -1; 
	}
}

FPlatformRect FHTML5Application::GetWorkArea( const FPlatformRect& CurrentWindow ) const
{
	return  FHTML5Window::GetScreenRect();
}

void FDisplayMetrics::GetDisplayMetrics(FDisplayMetrics& OutDisplayMetrics)
{
	OutDisplayMetrics.PrimaryDisplayWorkAreaRect = FHTML5Window::GetScreenRect();
	OutDisplayMetrics.VirtualDisplayRect    =	OutDisplayMetrics.PrimaryDisplayWorkAreaRect;
	OutDisplayMetrics.PrimaryDisplayWidth   =	OutDisplayMetrics.PrimaryDisplayWorkAreaRect.Right;
	OutDisplayMetrics.PrimaryDisplayHeight  =	OutDisplayMetrics.PrimaryDisplayWorkAreaRect.Bottom; 
	UE_LOG(LogHTML5Application, Verbose, TEXT("GetDisplayMetrics Width:%d, Height:%d"), OutDisplayMetrics.PrimaryDisplayWorkAreaRect.Right, OutDisplayMetrics.PrimaryDisplayWorkAreaRect.Bottom);
}

TSharedRef< FGenericWindow > FHTML5Application::MakeWindow()
{
	return ApplicationWindow;
}
