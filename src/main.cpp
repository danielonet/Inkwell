/*
 * Copyright 2026 Daniel Dreichlinger
 * Distributed under the terms of the MIT License.
 */

#include <Application.h>

#include "MainWindow.h"

static const char* kAppSignature = "application/x-vnd.Inkwell";

class InkwellApp : public BApplication {
public:
	InkwellApp()
		:
		BApplication(kAppSignature)
	{
	}

	virtual void ReadyToRun()
	{
		(new MainWindow())->Show();
	}
};

int
main()
{
	InkwellApp app;
	app.Run();
	return 0;
}
