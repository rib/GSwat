#ifndef MY_OBJECT_H
#define MY_OBJECT_H

#include <glib.h>
#include <glib-object.h>
/* include your parent object here */

G_BEGIN_DECLS

#define MY_OBJECT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MY_TYPE_OBJECT, MyObject))
#define MY_TYPE_OBJECT            (my_object_get_type())
#define MY_OBJECT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MY_TYPE_OBJECT, MyObjectClass))
#define MY_IS_OBJECT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MY_TYPE_OBJECT))
#define MY_IS_OBJECT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MY_TYPE_OBJECT))
#define MY_OBJECT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MY_TYPE_OBJECT, MyObjectClass))

typedef struct _MyObject        MyObject;
typedef struct _MyObjectClass   MyObjectClass;
typedef struct _MyObjectPrivate MyObjectPrivate;

struct _MyObject
{
    /* add your parent type here */
    MyObjectParent parent;

    /* add pointers to new members here */
    
	/*< private > */
	GSwatViewPrivate *priv;
};

struct _MyObjectClass
{
    /* add your parent class here */
    MyObjectParentClass parent_class;

    /* add signals here */
    //void (* signal) (MyObject *object);
};

GType       my_object_get_type        (void);
MyObject*   my_object_new             (void);
void        my_object_clear           (MyObject *self);

/* add additional methods here */

G_END_DECLS

#endif /* MY_OBJECT_H */

