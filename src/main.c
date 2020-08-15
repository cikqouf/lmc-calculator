#include <gtk/gtk.h>
#include "Lmc-application.h"

int
main (int argc,char *argv[])
{
	setlocale (LC_ALL,"");
	bindtextdomain (GETTEXT_PACKAGE,LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE,"UTF-8");
	textdomain (GETTEXT_PACKAGE);

	return g_application_run (G_APPLICATION(lmc_application_new()),argc,argv);
}
