
#ifndef __GSWAT_VIEW_H__
#define __GSWAT_VIEW_H__

#include <gtk/gtk.h>

#include "gedit-view.h"
#include "gedit-document.h"
#include "gswat-debugger.h"

G_BEGIN_DECLS

/*
 * Type checking and casting macros
 */
#define GSWAT_TYPE_VIEW            (gswat_view_get_type ())
#define GSWAT_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), GSWAT_TYPE_VIEW, GSwatView))
#define GSWAT_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), GSWAT_TYPE_VIEW, GSwatViewClass))
#define GSWAT_IS_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), GSWAT_TYPE_VIEW))
#define GSWAT_IS_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GSWAT_TYPE_VIEW))
#define GSWAT_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GSWAT_TYPE_VIEW, GSwatViewClass))

/* Private structure type */
typedef struct _GSwatViewPrivate	GSwatViewPrivate;

/*
 * Main object structure
 */
typedef struct _GSwatView		GSwatView;
 
struct _GSwatView
{
	GeditView view;
	
	/*< private > */
	GSwatViewPrivate *priv;
};

/*
 * Class definition
 */
typedef struct _GSwatViewClass		GSwatViewClass;
 
struct _GSwatViewClass
{
	GeditViewClass parent_class;
	
};

/*
 * Public methods
 */
GtkType	    gswat_view_get_type     	(void) G_GNUC_CONST;

GtkWidget	*gswat_view_new(GeditDocument *doc, GSwatDebugger *debugger);

void        gswat_view_add_breakpoint(GSwatView *view, gint line);
void        gswat_view_remove_breakpoint(GSwatView *view, gint line);


G_END_DECLS

#endif /* __GSWAT_VIEW_H__ */

