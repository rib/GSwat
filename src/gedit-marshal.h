
#ifndef __gedit_marshal_MARSHAL_H__
#define __gedit_marshal_MARSHAL_H__

#include	<glib-object.h>

G_BEGIN_DECLS

/* BOOLEAN:OBJECT (gedit-marshal.list:1) */
extern void gedit_marshal_BOOLEAN__OBJECT (GClosure     *closure,
                                           GValue       *return_value,
                                           guint         n_param_values,
                                           const GValue *param_values,
                                           gpointer      invocation_hint,
                                           gpointer      marshal_data);

/* BOOLEAN:NONE (gedit-marshal.list:2) */
extern void gedit_marshal_BOOLEAN__VOID (GClosure     *closure,
                                         GValue       *return_value,
                                         guint         n_param_values,
                                         const GValue *param_values,
                                         gpointer      invocation_hint,
                                         gpointer      marshal_data);
#define gedit_marshal_BOOLEAN__NONE	gedit_marshal_BOOLEAN__VOID

/* VOID:VOID (gedit-marshal.list:3) */
#define gedit_marshal_VOID__VOID	g_cclosure_marshal_VOID__VOID

/* VOID:BOOLEAN (gedit-marshal.list:4) */
#define gedit_marshal_VOID__BOOLEAN	g_cclosure_marshal_VOID__BOOLEAN

/* VOID:OBJECT (gedit-marshal.list:5) */
#define gedit_marshal_VOID__OBJECT	g_cclosure_marshal_VOID__OBJECT

/* VOID:POINTER (gedit-marshal.list:6) */
#define gedit_marshal_VOID__POINTER	g_cclosure_marshal_VOID__POINTER

/* VOID:UINT64,UINT64 (gedit-marshal.list:7) */
extern void gedit_marshal_VOID__UINT64_UINT64 (GClosure     *closure,
                                               GValue       *return_value,
                                               guint         n_param_values,
                                               const GValue *param_values,
                                               gpointer      invocation_hint,
                                               gpointer      marshal_data);

/* VOID:BOOLEAN,POINTER (gedit-marshal.list:8) */
extern void gedit_marshal_VOID__BOOLEAN_POINTER (GClosure     *closure,
                                                 GValue       *return_value,
                                                 guint         n_param_values,
                                                 const GValue *param_values,
                                                 gpointer      invocation_hint,
                                                 gpointer      marshal_data);

/* VOID:BOXED,BOXED (gedit-marshal.list:9) */
extern void gedit_marshal_VOID__BOXED_BOXED (GClosure     *closure,
                                             GValue       *return_value,
                                             guint         n_param_values,
                                             const GValue *param_values,
                                             gpointer      invocation_hint,
                                             gpointer      marshal_data);

G_END_DECLS

#endif /* __gedit_marshal_MARSHAL_H__ */

