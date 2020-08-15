#ifndef __LMC_APP_H
#define __LMC_APP_H

#include <gtk/gtk.h>
#include "lmc-math.h"

G_BEGIN_DECLS

#define LMC_APP_TYPE (lmc_app_get_type())

G_DECLARE_FINAL_TYPE (LmcApp, lmc_app, LMC, APP, GtkApplication)

LmcApp  *lmc_app_new  (void);

G_END_DECLS
