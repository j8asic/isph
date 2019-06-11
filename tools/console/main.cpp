
// standard includes
#include <iostream>
#include <stdlib.h>
//#include <vld.h>
using namespace std;

// include isph
#include <isph.h>
using namespace isph;

// program globals
Simulation* sim = NULL;

void RecieveLogMessage(const Log::Message&);

int main(int argc, char *argv[])
{
	//VLDSetReportOptions(VLD_OPT_REPORT_TO_FILE, L"e:/isph/bin/LEAKS.txt");

	// initial setup

	CLSystem::Instance()->SetProfiling(false);
   //Log::SetOutput("console_log.txt");
   Log::SetLevel(Log::Info);
   Log::SetUserReceiver(&RecieveLogMessage);
	Log::Send(Log::Info, "ISPH console simulator, ISPH v" + std::string(ISPH_VERSION_STRING));
	Log::Send(Log::Info, "Copyright (C) 2012. ISPH Team");
	cout << endl;

	// parse application calling arguments

	if(argc < 2)
	{
		Log::Send(Log::Error, "You have to specify input file.");
		return 0;
	}

	bool profileTimestep = false;

	for (int i=0; i<argc; i++)
	{
		string arg = argv[i];
		if(arg == "-p")
			profileTimestep = true;
		// TODO
	}

	// setup simulation

	XmlLoader loader;
	loader.SetInput(argv[argc-1]);
	sim = loader.Read();

	if(!sim)
	{
		Log::Send(Log::Error, "Error while loading input file.");
		return 0;
	}

	if(sim->Initialize())
	{
		Log::Send(Log::Info, "Particle count: " + Utils::IntegerString(sim->ParticleCount()) + ", memory used: " + Utils::IntegerString(sim->UsedMemorySize() / 1024 / 1024) + " mb");

		if(profileTimestep)
		{
			sim->Exporters().clear();
			Log::Send(Log::Info, "Profiling...");
         sim->Advance(1.0e-7);
         sim->Finish();
			Timer t;

         while (t.Time() < 5000)
         {
            sim->Advance(1e-7);
            sim->Finish();
         }

         Log::Send(Log::Info, "One time step takes around: " + Utils::DoubleString(t.Time() / (sim->TimeStepCount() - 1)) + " ms");
		}
		else
		{
			sim->Run();
		}
	}

	delete sim;
	return 0;
}

void RecieveLogMessage(const Log::Message& m)
{
	switch(m.type)
	{
	case Log::Info:
		cout << "> " << m.text << endl; break;
	case Log::Warning:
		cout << "? " << m.text << endl; break;
	case Log::Error:
		cout << "! " << m.text << endl; break;
	case Log::User:
		cout << "> " << m.text << endl; break;
   case Log::DebugInfo:
      cout << "  " << m.text << endl; break;
	default: break;
	}
}
