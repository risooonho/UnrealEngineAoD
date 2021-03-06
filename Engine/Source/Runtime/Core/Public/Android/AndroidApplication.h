// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GenericApplication.h"
#include "AndroidWindow.h"

namespace FAndroidAppEntry
{
	void PlatformInit();

	// if the native window handle has changed then the new handle is required.
	void ReInitWindow(void* NewNativeWindowHandle = nullptr);

	void DestroyWindow();
	void ReleaseEGL();
}

struct FPlatformOpenGLContext;
namespace FAndroidEGL
{
	// Back door into more intimate Android OpenGL variables (a.k.a. a hack)
	FPlatformOpenGLContext*	GetRenderingContext();
	FPlatformOpenGLContext*	CreateContext();
	void					MakeCurrent(FPlatformOpenGLContext*);
	void					ReleaseContext(FPlatformOpenGLContext*);
	void					SwapBuffers(FPlatformOpenGLContext*);
	void					SetFlipsEnabled(bool Enabled);
	void					BindDisplayToContext(FPlatformOpenGLContext*);
}

//disable warnings from overriding the deprecated forcefeedback.  
//calls to the deprecated function will still generate warnings.
PRAGMA_DISABLE_DEPRECATION_WARNINGS

class FAndroidApplication : public GenericApplication
{
public:

	static FAndroidApplication* CreateAndroidApplication();

	// Returns the java environment
	static void InitializeJavaEnv(JavaVM* VM, jint Version, jobject GlobalThis);
	static jobject GetGameActivityThis();
	static JNIEnv* GetJavaEnv(bool bRequireGlobalThis = true);
	static jclass FindJavaClass(const char* name);
	static void DetachJavaEnv();
	static bool CheckJavaException();
	
	static FAndroidApplication* Get() { return _application; }

public:	
	
	virtual ~FAndroidApplication() {}

	void SetMessageHandler( const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler );

	virtual void PollGameDeviceState( const float TimeDelta ) override;

	virtual FPlatformRect GetWorkArea( const FPlatformRect& CurrentWindow ) const override;

	virtual IInputInterface* GetInputInterface() override;

	virtual TSharedRef< FGenericWindow > MakeWindow() override;

	virtual void AddExternalInputDevice(TSharedPtr<class IInputDevice> InputDevice);

	void InitializeWindow( const TSharedRef< FGenericWindow >& InWindow, const TSharedRef< FGenericWindowDefinition >& InDefinition, const TSharedPtr< FGenericWindow >& InParent, const bool bShowImmediately );

	static void OnWindowSizeChanged();

	virtual bool IsGamepadAttached() const override;

private:

	FAndroidApplication();


private:

	TSharedPtr< class FAndroidInputInterface > InputInterface;
	bool bHasLoadedInputPlugins;

	TArray< TSharedRef< FAndroidWindow > > Windows;

	static bool bWindowSizeChanged;

	static FAndroidApplication* _application;
};

PRAGMA_ENABLE_DEPRECATION_WARNINGS
