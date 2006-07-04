#include <stdio.h>
#include <sys/time.h>
#include <math.h>
#include <gtk/gtk.h>

#include "global.h"
#include "signals.h"
#include "simulator.h"
#include "interface.h"

static int display_area_width;
static int display_area_height;

static GtkDrawingArea *display_area;
static GdkGC *gc;
static GdkPixmap *display_area_back_buffer;

static OGOObject *last_created_object;
static OGOObject *highlighted_object;
static unsigned int state;

static void re_draw_display_area(void);

typedef struct {
    int width;
    int height;
    int wave_area_height;
    double frequency;
    double amplitude;
    GtkDrawingArea *drawing_area;
    GdkPixmap *wave_buffer;
    GdkPixmap *back_buffer;
}OGOMotivatorWidget;



static void re_draw_muscle_motivator(OGOMotivatorWidget *motivator);

static void set_motivator_frequency(OGOMotivatorWidget *motivator, double frequency);
static void set_motivator_amplitude(OGOMotivatorWidget *motivator, double amplitude);
static void draw_motivator_wave_buffer(OGOMotivatorWidget *motivator);


static OGOMotivatorWidget motivator;

enum {
    OGO_NO_TOOL                 =1L<<0,
    OGO_MASS_AND_SPRING_TOOL    =1L<<1,
    OGO_DRAGGING_MASS           =1L<<3
};

static int mass_type;

enum {
    OGO_FREE_MASS=0,
    OGO_FIXED_MASS=1
};

/* Editor state machine */

/* adding masses - tool */
/* simulator mode */
/*
OGOStateTable interface_state={
    on_display_area_motion_notify_event
};
*/

void
ogo_set_display_area_widget(GtkDrawingArea *widget)
{
    display_area=widget;

    gc=gdk_gc_new(GTK_WIDGET(display_area)->window);

    gdk_drawable_get_size(GTK_WIDGET(widget)->window,
                          &display_area_width,
                          &display_area_height);

    display_area_back_buffer = gdk_pixmap_new(GTK_WIDGET(display_area)->window,
                                 display_area_width,
                                 display_area_height,
                                 -1);/* take depth from drawable */
    state|=OGO_NO_TOOL;
    mass_type=OGO_FREE_MASS;
    last_created_object=NULL;
    highlighted_object=NULL;

}


void
ogo_set_muscle_widget(GtkDrawingArea *widget)
{
    motivator.drawing_area=widget;

    gdk_drawable_get_size(GTK_WIDGET(motivator.drawing_area)->window,
                          &(motivator.width),
                          &(motivator.height));

    motivator.back_buffer = gdk_pixmap_new(GTK_WIDGET(motivator.drawing_area)->window,
                                 motivator.width,
                                 motivator.height,
                                 -1);/* take depth from drawable */
    
    set_motivator_frequency(&motivator, 0.5);
    set_motivator_amplitude(&motivator, 1.0);
    draw_motivator_wave_buffer(&motivator);
}


gboolean
re_draw_state(gpointer data)
{
    re_draw_display_area();
    //re_draw_muscle_motivator(&motivator);

    return FALSE;
}



static void
re_draw_display_area(void)
{
    GdkDrawable *window=GTK_WIDGET(display_area)->window;
    const GdkColor background_color={0, 65535,65535,65535};
    const GdkColor foreground_color={0, 0,0,0};
    GSList *tmp;
    OGOObject *current_object;
    GdkWindow * ret;
    GdkDisplay *display;
    gint pointer_x_ret, pointer_y_ret;
    int x1_ret, y1_ret, x2_ret, y2_ret;
    int width_ret;

    gdk_gc_set_rgb_fg_color(gc, &background_color);
    gdk_draw_rectangle(display_area_back_buffer,
                       gc,
                       TRUE,
                       0,
                       0,
                       display_area_width,
                       display_area_height);

    gdk_gc_set_rgb_fg_color(gc, &foreground_color);

    /* iterate all mass objects and draw */
    for(tmp=ogo_get_world_objects();tmp!=NULL;tmp=tmp->next){
        current_object=(OGOObject *)tmp->data;

        if(! (current_object->flags & OGO_MASS)){
            continue;
        }

        ogo_sim_get_mass_dimensions(current_object, &x1_ret, &y1_ret, &width_ret);

        gdk_draw_rectangle(display_area_back_buffer,
                           gc,
                           TRUE,
                           x1_ret-(width_ret/2),
                           y1_ret-(width_ret/2),
                           width_ret,
                           width_ret);

        if(current_object->flags & OGO_HIGHLIGHTED){
            gdk_draw_rectangle(display_area_back_buffer,
                               gc,
                               FALSE,
                               x1_ret-(width_ret/2)-3,
                               y1_ret-(width_ret/2)-3,
                               width_ret+6,
                               width_ret+6);
        }

    }

    /* draw all spring objects */
    for(tmp=ogo_get_world_objects();tmp!=NULL;tmp=tmp->next){
        current_object=(OGOObject *)tmp->data;

        if(! (current_object->flags & OGO_SPRING)){
            continue;
        }

        ogo_sim_get_spring_position(current_object,
                                    &x1_ret,
                                    &y1_ret,
                                    &x2_ret,
                                    &y2_ret);

        gdk_draw_line(display_area_back_buffer,
                      gc,
                      x1_ret,
                      y1_ret,
                      x2_ret,
                      y2_ret);
    }

    /* preview next spring */
    if(state==OGO_MASS_AND_SPRING_TOOL){
        display=gdk_display_get_default();
        ret=gdk_display_get_window_at_pointer(display,&pointer_x_ret,&pointer_y_ret);
        if(ret == GTK_WIDGET(display_area)->window )
        {
            ogo_sim_get_mass_dimensions(last_created_object, &x1_ret, &y1_ret, &width_ret);
            gdk_draw_line(display_area_back_buffer,
                          gc,
                          x1_ret,
                          y1_ret,
                          pointer_x_ret,
                          pointer_y_ret);
        }
    }

    /* draw back buffer to window */
    gdk_draw_drawable(window,
                      gc,
                      display_area_back_buffer,
                      0,
                      0,
                      0,
                      0,
                      -1,
                      -1);

}


/* TODO
 * render wave to a pixmap which only needs to be redrawn
 * when frequency or amplitude get changed
 */
static void
re_draw_muscle_motivator(OGOMotivatorWidget *motivator)
{
    //const GdkColor background_color={0, 65535,65535,65535};
    const GdkColor foreground_color={0, 0,0,0};
    GdkDrawable *window=GTK_WIDGET(motivator->drawing_area)->window;
    struct timeval time;
    double time_step=((double)motivator->height/(double)1000000.0)*motivator->frequency;
    double current_phase_offset;
    long tmpmoo;
    int wavelength=motivator->height;
    
    gettimeofday(&time, NULL);
    current_phase_offset=time_step*(double)time.tv_usec;

    tmpmoo = current_phase_offset;
    tmpmoo=tmpmoo % motivator->height;
current_phase_offset=tmpmoo;
    gdk_gc_set_rgb_fg_color(gc, &foreground_color);

    /*
    gdk_draw_line(display_area_back_buffer,
                      gc,
                      0,
                      current_phase_offset,
                      motivator_width,
                      current_phase_offset);
                      */

    /* draw wave buffer to window */
    gdk_draw_drawable(motivator->back_buffer,
                      gc,
                      motivator->wave_buffer,
                      0,
                      0,
                      0,
                      (int)current_phase_offset,
                      -1,
                      -1);

    gdk_draw_drawable(motivator->back_buffer,
                      gc,
                      motivator->wave_buffer,
                      0,
                      0,
                      0,
                      (int)current_phase_offset-wavelength,
                      -1,
                      -1);


    /* draw back buffer to window */
    gdk_draw_drawable(window,
                      gc,
                      motivator->back_buffer,
                      0,
                      0,
                      0,
                      0,
                      -1,
                      -1);

}

/* TODO draw into transparent pixmap and only render the wave
 * with no background
 */
/* draws one wave length */
static void
draw_motivator_wave_buffer(OGOMotivatorWidget *motivator)
{
    const GdkColor background_color={0, 65535,65535,65535};
    const GdkColor foreground_color={0, 0,0,0};
    const double two_pie=M_PI*2;
    GdkDrawable *window=GTK_WIDGET(motivator->drawing_area)->window;
    int x,y;
    int offset=(motivator->width/2);
    double tmpx;
    double amplitude;


    gdk_pixmap_unref(motivator->wave_buffer);

    motivator->wave_buffer = gdk_pixmap_new(window,
                                           motivator->width,
                                           motivator->height,
                                           -1);/* take depth from drawable */

    gdk_gc_set_rgb_fg_color(gc, &background_color);
    gdk_draw_rectangle(motivator->wave_buffer,
                       gc,
                       TRUE,
                       0,
                       0,
                       motivator->width,
                       motivator->height);

    gdk_gc_set_rgb_fg_color(gc, &foreground_color);

    for(y=1;y<motivator->height;y++){
        amplitude=cos((two_pie/motivator->height)*y);
        tmpx=offset+amplitude*(motivator->amplitude*(motivator->width/2));
        x=(int)tmpx;

        gdk_draw_point(motivator->wave_buffer,
                       gc,
                       x,
                       y
                      );

        gdk_draw_point(motivator->wave_buffer,
                       gc,
                       offset,
                       y
                      );
    }

}



static void
set_motivator_frequency(OGOMotivatorWidget *motivator, double frequency)
{
    motivator->frequency=frequency;
    draw_motivator_wave_buffer(motivator);
}



static void
set_motivator_amplitude(OGOMotivatorWidget *motivator, double amplitude)
{
    motivator->amplitude=amplitude;
    draw_motivator_wave_buffer(motivator);
}



static void
use_mass_and_spring_tool_at(int x, int y)
{
    OGOObject *new_mass;
    OGOObject *new_spring;
    int x_ret, y_ret;
    int a, b, c;

    if(state & OGO_NO_TOOL){
        if(highlighted_object==NULL){
            last_created_object=ogo_sim_create_mass_object(x, y);
            if(mass_type==OGO_FIXED_MASS){
                ogo_sim_fix_mass(last_created_object);
            }
            ogo_sim_add_object_to_world(last_created_object);
        }else{
            last_created_object=highlighted_object;
        }
        state&=~OGO_NO_TOOL;
        state|=OGO_MASS_AND_SPRING_TOOL;
    }else if(state & OGO_MASS_AND_SPRING_TOOL){
        if(highlighted_object==NULL){
            new_mass=ogo_sim_create_mass_object(x, y);
            if(mass_type==OGO_FIXED_MASS){
                ogo_sim_fix_mass(new_mass);
            }
            ogo_sim_add_object_to_world(new_mass);
        }else{
            new_mass=highlighted_object;
        }

        /* measure distance between new mass and last created mass
         * since this determins the natural length of the
         * connecting spring
         */

        ogo_sim_get_mass_dimensions(last_created_object,
                                    &x_ret,
                                    &y_ret,
                                    NULL);

        a=x-x_ret;
        b=y-y_ret;
        c=sqrt(a*a + b*b);

        new_spring=ogo_sim_create_spring(last_created_object,
                                         new_mass,
                                         c,
                                         10);

        ogo_sim_add_object_to_world(new_spring);

        last_created_object=new_mass;
    }
}



gboolean
on_display_area_key_release_event(GtkWidget       *widget,
                                  GdkEventKey     *event,
                                  gpointer         user_data)
{
    GdkWindow * ret;
    GdkDisplay *display=gdk_display_get_default();
    gint x_ret, y_ret;

    ret=gdk_display_get_window_at_pointer(display,&x_ret,&y_ret);
    if(ret != GTK_WIDGET(display_area)->window ){
        return TRUE;
    }

    use_mass_and_spring_tool_at(x_ret, y_ret);

    return TRUE;
}

gboolean
on_display_area_button_press_event(GtkWidget       *widget,
                                   GdkEventButton  *event,
                                   gpointer         user_data)
{

    if(event->button==3){
        state|=OGO_NO_TOOL;
        return TRUE;
    }

    use_mass_and_spring_tool_at(event->x, event->y);

    /* allow dragging highlighted mass */
    state |= OGO_DRAGGING_MASS;
    if(highlighted_object!=NULL){
        ogo_sim_fix_mass(highlighted_object);
    }
    return TRUE;
}



gboolean
on_display_area_button_release_event(GtkWidget       *widget,
                                     GdkEventButton  *event, 
                                     gpointer         user_data)
{
    /* stop dragging highlighted mass */
    state &= ~OGO_DRAGGING_MASS;
    if(highlighted_object!=NULL){
        ogo_sim_free_mass(highlighted_object);
    }

    return TRUE;
}



gboolean
on_ogo_main_window_configure_event(GtkWidget       *widget,
                                   GdkEventConfigure *event,
                                   gpointer         user_data)
{

    display_area_width=event->width;
    display_area_height=event->height;
    printf("width=%d\nheight=%d\n",display_area_width, display_area_height);

    /* FIXME only necisary if window resized */

    g_object_unref(G_OBJECT(display_area_back_buffer));

    display_area_back_buffer=gdk_pixmap_new(GTK_WIDGET(display_area)->window,
                                            display_area_width,
                                            display_area_height,
                                            -1);/* take depth from drawable */
    return TRUE;
}

void
on_gravity_toggle_button_toggled(GtkToggleToolButton *toggletoolbutton,
                                 gpointer         user_data)
{

    gboolean gravity_on;
    gravity_on=gtk_toggle_tool_button_get_active(toggletoolbutton);

    if(gravity_on){
        ogo_sim_set_gravity(0,9.81);
    }else{
        ogo_sim_set_gravity(0,0);
    }
}


void
on_mass_type_button_toggled(GtkToggleToolButton *toggletoolbutton,
                            gpointer         user_data)
{
    gboolean fixed;
    fixed=gtk_toggle_tool_button_get_active(toggletoolbutton);

    if(fixed){
        mass_type=OGO_FIXED_MASS;
    }else{
        mass_type=OGO_FREE_MASS;
    }
}


void
on_simulator_toggle_button_toggled(GtkToggleToolButton *toggletoolbutton,
                                   gpointer         user_data)
{   
    gboolean simulate;
    simulate=gtk_toggle_tool_button_get_active(toggletoolbutton);

    if(simulate){
        ogo_sim_start();
    }else{
        ogo_sim_stop();
    }
}



gboolean
on_display_area_motion_notify_event(GtkWidget       *widget,
                                    GdkEventMotion  *event, 
                                    gpointer         user_data)
{
    OGOObject *object_match;

    printf("motion notify\n");
    fflush(stdout);

    if(state & OGO_DRAGGING_MASS){
        if(highlighted_object!=NULL){
            /* TODO FIXME () */

            ogo_sim_move_mass(highlighted_object, event->x, event->y);
        }
    }else{
        /* ask the sim if the pointer is close to any objects */
        object_match=ogo_sim_match_object(event->x, event->y);

        if(object_match!=NULL){
            ogo_sim_highlight_object(object_match);
            highlighted_object=object_match;
        }else{/* clear last highlight */
            highlighted_object=NULL;
            ogo_sim_highlight_object(NULL);
        }
    }

    return TRUE;
}


int
ogo_get_display_area_width(void)
{
    return display_area_width;
}


int
ogo_get_display_area_height(void)
{
    return display_area_height;
}


gboolean
on_muscle_motivator_button_press_event (GtkWidget       *widget,
                                        GdkEventButton  *event, 
                                        gpointer         user_data)
{
    int middle=motivator.width/2;
    int offset=event->x-middle;

    offset=event->x-middle;

    double amplitude=(double)offset/(double)middle;

    if(amplitude<0){
        amplitude=-amplitude;
    }

    set_motivator_amplitude(&motivator, amplitude);
    printf("CLICK *********************************\n");

    return TRUE;
}   

gboolean
on_vscale4_change_value                (GtkRange        *range,
                                        GtkScrollType    arg1,
                                        gdouble          arg2,
                                        gpointer         user_data)
{
    set_motivator_frequency(&motivator, arg2);

    return TRUE;
}

