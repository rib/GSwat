#ifndef OGO_INTERFACE_H
#define OGO_INTERFACE_H

/* There is a state table specifies how to
 * handle certain events depending on what we up to
 */
typedef struct {
    
    gboolean (*on_display_area_motion_notify_event)(GtkWidget *widget,GdkEventMotion  *event, gpointer user_data);
    
}OGOStateTable;


gboolean re_draw_state(gpointer data);

void ogo_set_display_area_widget(GtkDrawingArea *display_area);
void ogo_set_muscle_widget(GtkDrawingArea *muscle_motivator);
int ogo_get_display_area_width(void);
int ogo_get_display_area_height(void);

#endif /* OGO_INTERFACE_H */

