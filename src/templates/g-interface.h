#ifndef MY_DOABLE_H
#define MY_DOABLE_H

#include <glib-object.h>

G_BEGIN_DECLS

#define MY_TYPE_DOABLE           (my_doable_get_type ())
#define MY_DOABLE(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), MY_TYPE_DOABLE, MyDoable))
#define MY_IS_DOABLE(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MY_TYPE_DOABLE))
#define MY_DOABLE_GET_IFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), MY_TYPE_DOABLE, MyDoableIface))

typedef struct _MyDoableIface MyDoableIface;

struct _MyDoableIface
{
	GTypeInterface g_iface;

	/* signals: */

    //void (* signal_member)(MyDoable *self);

    /* VTable: */
    //void (*method1)(MyDoable *self);
    //void (*method2)(MyDoable *self);
}

/* Interface functions */
//void my_doable_method1(MyDoable *self);
//void my_doable_method2(MyDoable *self);

G_END_DECLS

#endif /* MY_DOABLE_H */

