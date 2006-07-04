#ifndef __OGO_OBJECT_H__
#define __OGO_OBJECT_H__
G_BEGIN_DECLS

#define OGO_TYPE_OBJECT            (ogo_object_get_type (void))
#define OGO_OBJECT(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), OGO_TYPE_OBJECT, OgoObject)
#define OGO_OBJECT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), OGO_TYPE_OBJECT, OgoObjectClass))
#define OGO_IS_OBJECT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OGO_TYPE_OBJECT))
#define OGO_IS_OBJECT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), OGO_TYPE_OBJECT))
#define OGO_GET_OBJECT_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), OGO_TYPE_OBJECT, OgoObjectClass))

typedef struct _OgoObject OgoObject;
typedef struct _OgoObjectClass OgoObjectClass;

struct _OgoObject {
	GObject parent;
	/* define public instance variables here */
};

struct _OgoObjectClass {
	GObjectClass parent_class;
	/* define vtable methods and signals here */
};

GType ogo_object_get_type (void);

OgoObject* ogo_object_new (void);

G_END_DECLS

#endif /* __OGO_OBJECT_H__ */

