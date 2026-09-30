#include "WIN_WinWrapper.h"
#include <string>
#include <chrono>
#include <thread>
#include "WIN_Window.h"
#include "G3D_Include.h"
#include "ERR_ErrorEngine.h"
#include "AST_Core.h"

int CALLBACK WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR     lpCmdLine,
	int       nCmdShow)
{
	try
	{
		ERR::errorTracker.PrintToLog("window init successful");

		// temporary clock stuff
		int inputPeriod_ms = 25;
		long long lastTime_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
		long long nextTime_ms = 0;

		// window
		WIN::Window mainWin;
		std::string allReceivedKeys = "";
		mainWin.SetTitle(L"test 2");

		// G3D testing
		G3D::RenderEngine renderEngine(mainWin.GetHandle(), mainWin.width, mainWin.height);
		// add a wire frame object
		std::vector<UTL::vector3f> vertixXYZ =
		{
			{0.0f, 0.0f, 0.0f},
			{0.0f, 0.5f, 0.0f},
			{0.0f, 0.5f, 0.5f}
		};
		std::vector<unsigned short> indexData =
		{
			0, 1, 2
		};
		G3D::Obj_WireFrame wireFrameObject{ vertixXYZ ,indexData, renderEngine };
		UTL::vector3f objPosition = { 5, 0, 0 };
		UTL::vector3f objAttitude = { 0, 0, 0 };
		UTL::vector3f objScale = { 1, 1, 1 };
		wireFrameObject.UpdateBodyAndGlobalFrame(objAttitude, objPosition, objScale);
		G3D::Shader_WireFrame wireFrameShader{ renderEngine };

		UTL::vector3f cameraPosition{ 0.0f, 0.0f, 0.0f };
		UTL::vector3f cameraAttitude{ 0.0f, 0.0f, 0.0f };

		// AST testing
		AST::G1 cube;
		int returnCode = -1;
		returnCode = cube.OpenCopy("cube.G1");

		std::vector<UTL::vector3f> cubeVertices;
		UTL::InterleaveVectors(cube.vertexX, cube.vertexY, cube.vertexZ, cubeVertices);
		std::vector<UTL::vector3f> cubeIndices;
		G3D::Obj_WireFrame cubeObject{ cubeVertices, cube.indices, renderEngine };
		cubeObject.UpdateBodyAndGlobalFrame(objAttitude, objPosition, objScale);

		// main loop
		while (true)
		{
			
			nextTime_ms = lastTime_ms + inputPeriod_ms;
			while (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count() < nextTime_ms) {}
			lastTime_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
			
			// process all messages pending, but not block new messages
			if (const auto ecode = WIN::Window::ProcessMessageQue())
			{
				// if return optional has value, means we're quitting so return exit code
				//return *ecode;
				return 0;
			}

			bool GDown = mainWin.kbd.CheckRaw(KBDRAW::SemiColon)->Down;
			bool ColonDown = mainWin.kbd.CheckASCII(KBDASCII::VerticalBar)->Down;
			if (GDown || ColonDown)
			{
				mainWin.SetTitle(L"good");
			}
			else
			{
				mainWin.SetTitle(L"nah");
			}

			// camera keyboard controls
			cameraPosition = { 0, 0, 0 };
			if (mainWin.kbd.CheckASCII(KBDASCII::w)->Down)
			{
				cameraPosition.r1c1 += 0.01;
			}
			if (mainWin.kbd.CheckASCII(KBDASCII::s)->Down)
			{
				cameraPosition.r1c1 -= 0.01;
			}
			if (mainWin.kbd.CheckASCII(KBDASCII::a)->Down)
			{
				cameraPosition.r2c1 -= 0.01;
			}
			if (mainWin.kbd.CheckASCII(KBDASCII::d)->Down)
			{
				cameraPosition.r2c1 += 0.01;
			}
			if (mainWin.kbd.CheckASCII(KBDASCII::r)->Down)
			{
				cameraPosition.r3c1 -= 0.01;
			}
			if (mainWin.kbd.CheckASCII(KBDASCII::f)->Down)
			{
				cameraPosition.r3c1 += 0.01;
			}
			if (mainWin.kbd.CheckASCII(KBDASCII::W)->Down)
			{
				cameraAttitude.r2c1 -= 0.01;
			}
			if (mainWin.kbd.CheckASCII(KBDASCII::S)->Down)
			{
				cameraAttitude.r2c1 += 0.01;
			}
			if (mainWin.kbd.CheckASCII(KBDASCII::A)->Down)
			{
				cameraAttitude.r1c1 -= 0.01;
			}
			if (mainWin.kbd.CheckASCII(KBDASCII::D)->Down)
			{
				cameraAttitude.r1c1 += 0.01;
			}
			if (mainWin.kbd.CheckASCII(KBDASCII::R)->Down)
			{
				cameraAttitude.r3c1 -= 0.01;
			}
			if (mainWin.kbd.CheckASCII(KBDASCII::F)->Down)
			{
				cameraAttitude.r3c1 += 0.01;
			}

			cameraPosition = UTL::RotateVectorByInverseEuler(cameraAttitude, cameraPosition);
			
			// G3D testing
			renderEngine.camera.attitude = cameraAttitude;
			renderEngine.camera.position = UTL::Add(cameraPosition, renderEngine.camera.position);
			renderEngine.camera.nearPlane = 0.1f;
			renderEngine.camera.farPlane = 1000.0f;
			renderEngine.camera.fov = UTL::pi / 3;
			wireFrameShader.Draw(renderEngine);
			cubeObject.Draw(renderEngine);
			renderEngine.PresentFrame();

		};

		return 0;

	}
	catch (const std::exception& e)
	{
		// otherwise unhandled exception from the rest of the program, constitutes a critical error for the program
		MessageBoxA(NULL, e.what(), "Error", MB_ICONERROR);
		return 1;
	}
	
	return 1;
}