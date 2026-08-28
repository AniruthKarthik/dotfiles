#include <gtk/gtk.h>
#include <gtk-layer-shell/gtk-layer-shell.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char socket_path[512] = {0};

static void init_socket_path(void) {
    const char *xdg_runtime = getenv("XDG_RUNTIME_DIR");
    const char *hypr_sig = getenv("HYPRLAND_INSTANCE_SIGNATURE");
    if (xdg_runtime && hypr_sig) {
        snprintf(socket_path, sizeof(socket_path), "%s/hypr/%s/.socket.sock", xdg_runtime, hypr_sig);
    }
}

static void send_hyprland_cmd(const char *cmd) {
    if (socket_path[0] == '\0') init_socket_path();
    if (socket_path[0] == '\0') return;

    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) return;

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
        write(fd, cmd, strlen(cmd));
    }
    close(fd);
}

typedef struct {
    gboolean is_left;
    gboolean triggered;
} EdgeContext;

static gboolean on_enter_edge(GtkWidget *widget, GdkEventCrossing *event, gpointer data) {
    EdgeContext *ctx = (EdgeContext *)data;
    if (!ctx->triggered) {
        ctx->triggered = TRUE;
        if (ctx->is_left) {
            send_hyprland_cmd("eval hl.dispatch(hl.dsp.focus({ workspace = 'e-1' }))");
        } else {
            send_hyprland_cmd("eval hl.dispatch(hl.dsp.focus({ workspace = 'e+1' }))");
        }
    }
    return TRUE;
}

static gboolean on_leave_edge(GtkWidget *widget, GdkEventCrossing *event, gpointer data) {
    EdgeContext *ctx = (EdgeContext *)data;
    ctx->triggered = FALSE;
    return TRUE;
}

static gboolean point_in_rect(int px, int py, GdkRectangle *r) {
    return (px >= r->x && px < (r->x + r->width) &&
            py >= r->y && py < (r->y + r->height));
}

// Determines if a monitor boundary is exposed to empty space (far outer edge of layout)
static gboolean is_outer_edge(GdkDisplay *display, GdkRectangle *mon_geom, gboolean check_left) {
    int n = gdk_display_get_n_monitors(display);
    int test_x = check_left ? (mon_geom->x - 1) : (mon_geom->x + mon_geom->width);
    
    int sample_points[] = {
        mon_geom->y + 5,
        mon_geom->y + mon_geom->height / 2,
        mon_geom->y + mon_geom->height - 5
    };

    for (int p = 0; p < 3; p++) {
        int test_y = sample_points[p];
        for (int i = 0; i < n; i++) {
            GdkMonitor *m = gdk_display_get_monitor(display, i);
            GdkRectangle g;
            gdk_monitor_get_geometry(m, &g);

            if (g.x == mon_geom->x && g.y == mon_geom->y &&
                g.width == mon_geom->width && g.height == mon_geom->height) {
                continue;
            }

            if (point_in_rect(test_x, test_y, &g)) {
                return FALSE; // Inter-monitor boundary
            }
        }
    }
    return TRUE; // True outer layout edge
}

static void create_edge_surface(GdkDisplay *display, GdkMonitor *monitor, GdkRectangle *mon_geom, gboolean is_left) {
    GtkWidget *win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_layer_init_for_window(GTK_WINDOW(win));
    
    if (monitor) {
        gtk_layer_set_monitor(GTK_WINDOW(win), monitor);
    }
    
    gtk_layer_set_layer(GTK_WINDOW(win), GTK_LAYER_SHELL_LAYER_OVERLAY);
    gtk_layer_set_anchor(GTK_WINDOW(win), is_left ? GTK_LAYER_SHELL_EDGE_LEFT : GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);
    gtk_layer_set_anchor(GTK_WINDOW(win), GTK_LAYER_SHELL_EDGE_TOP, TRUE);
    gtk_layer_set_anchor(GTK_WINDOW(win), GTK_LAYER_SHELL_EDGE_BOTTOM, TRUE);
    
    // 1-pixel wide edge trigger surface
    gtk_widget_set_size_request(win, 1, -1);
    
    gtk_widget_set_events(win, GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK);
    
    EdgeContext *ctx = g_new0(EdgeContext, 1);
    ctx->is_left = is_left;
    ctx->triggered = FALSE;
    
    g_signal_connect(win, "enter-notify-event", G_CALLBACK(on_enter_edge), ctx);
    g_signal_connect(win, "leave-notify-event", G_CALLBACK(on_leave_edge), ctx);
    
    // Transparent background
    GdkScreen *screen = gdk_screen_get_default();
    GdkVisual *visual = gdk_screen_get_rgba_visual(screen);
    if (visual && gdk_screen_is_composited(screen)) {
        gtk_widget_set_visual(win, visual);
    }
    
    gtk_widget_show_all(win);
}

int main(int argc, char *argv[]) {
    // Single instance lock
    int lock_fd = open("/tmp/hypr-edge-switcher.lock", O_CREAT | O_RDWR, 0666);
    if (lock_fd >= 0 && flock(lock_fd, LOCK_EX | LOCK_NB) != 0) {
        return 0; // Already running
    }

    gtk_init(&argc, &argv);
    init_socket_path();
    
    GdkDisplay *display = gdk_display_get_default();
    if (!display) return 1;
    
    int n_monitors = gdk_display_get_n_monitors(display);
    for (int i = 0; i < n_monitors; i++) {
        GdkMonitor *m = gdk_display_get_monitor(display, i);
        GdkRectangle geom;
        gdk_monitor_get_geometry(m, &geom);
        
        if (is_outer_edge(display, &geom, TRUE)) {
            create_edge_surface(display, m, &geom, TRUE);
        }
        if (is_outer_edge(display, &geom, FALSE)) {
            create_edge_surface(display, m, &geom, FALSE);
        }
    }
    
    gtk_main();
    return 0;
}
