#ifndef GSWAT_DEBUGGABLE_H
#define GSWAT_DEBUGGABLE_H

#include <glib-object.h>

G_BEGIN_DECLS

#define GSWAT_TYPE_DEBUGGABLE           (gswat_debuggable_get_type ())
#define GSWAT_DEBUGGABLE(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSWAT_TYPE_DEBUGGABLE, GSwatDebuggable))
#define GSWAT_IS_DEBUGGABLE(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSWAT_TYPE_DEBUGGABLE))
#define GSWAT_DEBUGGABLE_GET_IFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GSWAT_TYPE_DEBUGGABLE, GSwatDebuggableIface))

typedef struct _GSwatDebuggableIface GSwatDebuggableIface;
typedef struct _GSwatDebuggable GSwatDebuggable; /* dummy typedef */

struct _GSwatDebuggableIface
{
	GTypeInterface g_iface;

	/* signals: */
    //void (* signal_member)(GSwatDebuggable *self);

    /* VTable: */
    void (*target_connect)(GSwatDebuggable* object);
    void (*target_disconnect)(GSwatDebuggable* object);
    void (*request_line_breakpoint)(GSwatDebuggable* object,
                                    gchar *uri,
                                    guint line);
    void (*request_function_breakpoint)(GSwatDebuggable* object,
                                        gchar *symbol);
    gchar *(*get_source_uri)(GSwatDebuggable* object);
    gint (*get_source_line)(GSwatDebuggable* object);
    void (*cont)(GSwatDebuggable* object);
    void (*finish)(GSwatDebuggable* object);
    void (*next)(GSwatDebuggable* object);
    void (*step_into)(GSwatDebuggable* object);
    void (*interrupt)(GSwatDebuggable* object);
    void (*restart)(GSwatDebuggable* object);
    guint (*get_state)(GSwatDebuggable* object);
    guint (*get_state_stamp)(GSwatDebuggable* object);
    GList *(*get_stack)(GSwatDebuggable* object);
    GList *(*get_breakpoints)(GSwatDebuggable* object);
    GList *(*get_locals_list)(GSwatDebuggable* object);
    guint (*get_frame)(GSwatDebuggable* object);
    void (*set_frame)(GSwatDebuggable* object, guint frame);
};

typedef enum {
    GSWAT_DEBUGGABLE_DISCONNECTED,
	GSWAT_DEBUGGABLE_RUNNING,
    GSWAT_DEBUGGABLE_INTERRUPTED
}GSwatDebuggableState;

typedef struct {
    gchar *name;
    gchar *value;
}GSwatDebuggableFrameArgument;

typedef struct {
    int level;
    unsigned long address;
    gchar *function;
    GList *arguments;
}GSwatDebuggableFrame;

typedef struct {
    gchar   *source_uri;
    gint    line;
}GSwatDebuggableBreakpoint;

GType gswat_debuggable_get_type(void);

/* Interface functions */
void gswat_debuggable_target_connect(GSwatDebuggable* object);
guint gswat_debuggable_get_state(GSwatDebuggable* object);
void gswat_debuggable_request_line_breakpoint(GSwatDebuggable* object,
                                              gchar *uri,
                                              guint line);
void gswat_debuggable_request_function_breakpoint(GSwatDebuggable* object,
                                                  gchar *symbol);
gchar *gswat_debuggable_get_source_uri(GSwatDebuggable* object);
gint gswat_debuggable_get_source_line(GSwatDebuggable* object);
void gswat_debuggable_continue(GSwatDebuggable* object);
void gswat_debuggable_finish(GSwatDebuggable* object);
void gswat_debuggable_next(GSwatDebuggable* object);
void gswat_debuggable_step_into(GSwatDebuggable* object);
void gswat_debuggable_interrupt(GSwatDebuggable* object);
void gswat_debuggable_restart(GSwatDebuggable* object);
guint gswat_debuggable_get_state_stamp(GSwatDebuggable* object);
GList *gswat_debuggable_get_stack(GSwatDebuggable* object);
void gswat_debuggable_free_stack(GList *stack);
GList *gswat_debuggable_get_breakpoints(GSwatDebuggable* object);
void gswat_debuggable_free_breakpoints(GList *breakpoints);
GList *gswat_debuggable_get_locals_list(GSwatDebuggable* object);
guint gswat_debuggable_get_frame(GSwatDebuggable* object);
void gswat_debuggable_set_frame(GSwatDebuggable* object, guint frame);

G_END_DECLS

#endif /* GSWAT_DEBUGGABLE_H */

