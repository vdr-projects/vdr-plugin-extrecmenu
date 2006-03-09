#include "extrecmenu.h"

cPluginExtrecmenu::cPluginExtrecmenu(void)
{
  // Initialize any member variables here.
  // DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
  // VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!
}

cPluginExtrecmenu::~cPluginExtrecmenu()
{
  // Clean up after yourself!
}

const char *cPluginExtrecmenu::CommandLineHelp(void)
{
  // Return a string that describes all known command line options.
  return NULL;
}

bool cPluginExtrecmenu::ProcessArgs(int argc, char *argv[])
{
  // Implement command line argument processing here if applicable.
  return true;
}

bool cPluginExtrecmenu::Initialize(void)
{
 RegisterI18n(Phrases);
 return true;
}

bool cPluginExtrecmenu::Start(void)
{
  // Start any background activities the plugin shall perform.
  return true;
}

void cPluginExtrecmenu::Stop(void)
{
  // Stop any background activities the plugin shall perform.
}

void cPluginExtrecmenu::Housekeeping(void)
{
  // Perform any cleanup or other regular tasks.
}

cOsdObject *cPluginExtrecmenu::MainMenuAction(void)
{
  // Perform the action when selected from the main VDR menu.
  return new myMenuRecordings();
}

cMenuSetupPage *cPluginExtrecmenu::SetupMenu(void)
{
  // Return a setup menu in case the plugin supports one.
  return NULL;
}

bool cPluginExtrecmenu::SetupParse(const char *Name, const char *Value)
{
  // Parse your own setup parameters and store their values.
  return false;
}

bool cPluginExtrecmenu::Service(const char *Id, void *Data)
{
  // Handle custom service requests from other plugins
  return false;
}

const char **cPluginExtrecmenu::SVDRPHelpPages(void)
{
  // Return help text for SVDRP commands this plugin implements
  return NULL;
}

cString cPluginExtrecmenu::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode)
{
  // Process SVDRP commands this plugin implements
  return NULL;
}

VDRPLUGINCREATOR(cPluginExtrecmenu); // Don't touch this!
