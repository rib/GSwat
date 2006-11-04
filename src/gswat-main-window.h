#ifndef __GSWAT_MAIN_WINDOW_HEADER
#define __GSWAT_MAIN_WINDOW_HEADER

#include "gswat-session.h"

G_BEGIN_DECLS

typedef GObjectClass            GSwatWindowClass;

/*
 * Type checking and casting macros
 */
#define GSWAT_TYPE_WINDOW            (gswat_window_get_type ())
#define GSWAT_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), GSWAT_TYPE_WINDOW, GSwatWindow))
#define GSWAT_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), GSWAT_TYPE_WINDOW, GSwatWindowClass))
#define GSWAT_IS_WINDOW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), GSWAT_TYPE_WINDOW))
#define GSWAT_IS_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GSWAT_TYPE_WINDOW))
#define GSWAT_WINDOW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GSWAT_TYPE_WINDOW, GSwatWindowClass))


/* Private structure type */
typedef struct _GSwatWindowPrivate	GSwatWindowPrivate;


/*
 * Main object structure
 */
typedef struct _GSwatWindow		GSwatWindow;


struct _GSwatWindow
{   
    GObject base_instance;
    	
	/*< private > */
	GSwatWindowPrivate *priv;
};


/* Public methods */

GType gswat_window_get_type(void);

GSwatWindow *gswat_window_new(GSwatSession *session);


G_END_DECLS


#endif /* __GSWAT_MAIN_WINDOW_HEADER */

