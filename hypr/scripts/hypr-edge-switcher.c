#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <wayland-client.h>
#include "wlr-layer-shell-unstable-v1-client-protocol.h"

#define MAX_MONITORS 16

typedef struct {
    int id;
    char name[64];
    int x;
    int y;
    int width;
    int height;
    float scale;
    int disabled;
} MonitorInfo;

static MonitorInfo monitors[MAX_MONITORS];
static int num_monitors = 0;
static char socket_path[512] = {0};
static int waybar_is_hidden = 1;

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
        write(fd, "\n", 1);
    }
    close(fd);
}

static pid_t find_waybar_pid(void) {
    FILE *fp = popen("pgrep -x waybar", "r");
    if (!fp) return 0;
    pid_t pid = 0;
    if (fscanf(fp, "%d", &pid) != 1) pid = 0;
    pclose(fp);
    return pid;
}

static int is_waybar_visible_on_screen(void) {
    if (socket_path[0] == '\0') init_socket_path();
    if (socket_path[0] == '\0') return 0;

    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) return 0;

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        close(fd);
        return 0;
    }

    // Hyprland IPC command "j/layers" (no newline) returns layer surface geometry JSON
    write(fd, "j/layers", 8);

    char buf[32768];
    ssize_t total = 0;
    ssize_t n = 0;
    while ((n = read(fd, buf + total, sizeof(buf) - 1 - total)) > 0) {
        total += n;
    }
    close(fd);

    if (total <= 0) return 0;
    buf[total] = '\0';

    char *ptr = buf;
    while ((ptr = strstr(ptr, "\"2\":")) != NULL) {
        char *l3 = strstr(ptr, "\"3\":");
        size_t len_to_check = l3 ? (size_t)(l3 - ptr) : strlen(ptr);
        char *wb = strstr(ptr, "\"namespace\": \"waybar\"");
        if (wb && (size_t)(wb - ptr) < len_to_check) {
            return 1; // Waybar layer surface is mapped on top layer level "2" (visible)
        }
        ptr += 4;
    }

    return 0; // Waybar is not visible on top layer
}

static void init_waybar_state(void) {
    char lock_path[512];
    const char *home = getenv("HOME");
    if (home) {
        snprintf(lock_path, sizeof(lock_path), "%s/.cache/waybar_toggle.lock", home);
        if (access(lock_path, F_OK) == 0) {
            waybar_is_hidden = 0; // Manual lock file active: force visible
            return;
        }
    }

    // Wait up to 1.5 seconds for Waybar process and layer surface to initialize
    for (int i = 0; i < 15; i++) {
        pid_t pid = find_waybar_pid();
        if (pid > 0) {
            if (is_waybar_visible_on_screen()) {
                kill(pid, SIGUSR1);
            }
            break;
        }
        usleep(100000); // 100ms
    }
    waybar_is_hidden = 1;
}

static void set_waybar_visible(int show) {
    char lock_path[512];
    const char *home = getenv("HOME");
    if (home) {
        snprintf(lock_path, sizeof(lock_path), "%s/.cache/waybar_toggle.lock", home);
        if (access(lock_path, F_OK) == 0) {
            return; // Manual lock file active: keep Waybar visible
        }
    }

    pid_t pid = find_waybar_pid();
    int currently_visible = is_waybar_visible_on_screen();

    if (show) {
        if (pid <= 0) {
            system("~/.config/waybar/launch.sh >/dev/null 2>&1 &");
            waybar_is_hidden = 0;
        } else if (!currently_visible) {
            kill(pid, SIGUSR1);
            waybar_is_hidden = 0;
        } else {
            waybar_is_hidden = 0;
        }
    } else {
        if (pid > 0 && currently_visible) {
            kill(pid, SIGUSR1);
            waybar_is_hidden = 1;
        } else {
            waybar_is_hidden = 1;
        }
    }
}

// Minimal JSON parser to query monitor geometry from Hyprland IPC
static void fetch_monitor_topology(void) {
    if (socket_path[0] == '\0') init_socket_path();
    if (socket_path[0] == '\0') return;

    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) return;

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        close(fd);
        return;
    }

    write(fd, "j/monitors\n", 11);

    char buf[16384];
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    close(fd);

    if (n <= 0) return;
    buf[n] = '\0';

    num_monitors = 0;
    char *ptr = buf;
    while ((ptr = strstr(ptr, "\"name\":")) != NULL && num_monitors < MAX_MONITORS) {
        MonitorInfo *m = &monitors[num_monitors];
        memset(m, 0, sizeof(MonitorInfo));

        sscanf(ptr, "\"name\": \"%63[^\"]\"", m->name);

        char *px = strstr(ptr, "\"x\":");
        if (px) m->x = atoi(px + 4);

        char *py = strstr(ptr, "\"y\":");
        if (py) m->y = atoi(py + 4);

        char *pw = strstr(ptr, "\"width\":");
        if (pw) m->width = atoi(pw + 8);

        char *ph = strstr(ptr, "\"height\":");
        if (ph) m->height = atoi(ph + 9);

        char *ps = strstr(ptr, "\"scale\":");
        if (ps) m->scale = atof(ps + 8);

        char *pd = strstr(ptr, "\"disabled\":");
        if (pd && strncmp(pd + 11, "true", 4) == 0) {
            m->disabled = 1;
        }

        if (m->scale > 0.01) {
            m->width = (int)(m->width / m->scale);
            m->height = (int)(m->height / m->scale);
        }

        if (!m->disabled) {
            num_monitors++;
        }
        ptr += 7;
    }
}

static int point_in_rect(int px, int py, MonitorInfo *m) {
    return (px >= m->x && px < (m->x + m->width) &&
            py >= m->y && py < (m->y + m->height));
}

// Checks if a monitor side boundary is exposed to empty space (far outer layout edge)
static int is_outer_side_edge(MonitorInfo *mon, int check_left) {
    int test_x = check_left ? (mon->x - 1) : (mon->x + mon->width);
    int sample_points[] = {
        mon->y + 5,
        mon->y + mon->height / 2,
        mon->y + mon->height - 5
    };

    for (int p = 0; p < 3; p++) {
        int test_y = sample_points[p];
        for (int i = 0; i < num_monitors; i++) {
            MonitorInfo *other = &monitors[i];
            if (other == mon || other->disabled) continue;
            if (point_in_rect(test_x, test_y, other)) {
                return 0; // Inter-monitor boundary
            }
        }
    }
    return 1; // Outer side layout edge
}

// Checks if a monitor top boundary is exposed to empty space (far outer top edge)
static int is_outer_top_edge(MonitorInfo *mon) {
    int test_y = mon->y - 1;
    int sample_points[] = {
        mon->x + 5,
        mon->x + mon->width / 2,
        mon->x + mon->width - 5
    };

    for (int p = 0; p < 3; p++) {
        int test_x = sample_points[p];
        for (int i = 0; i < num_monitors; i++) {
            MonitorInfo *other = &monitors[i];
            if (other == mon || other->disabled) continue;
            if (point_in_rect(test_x, test_y, other)) {
                return 0; // Adjacent monitor above -> inter-monitor boundary
            }
        }
    }
    return 1; // Outer top layout edge
}

typedef enum {
    EDGE_LEFT = 0,
    EDGE_RIGHT = 1,
    EDGE_TOP = 2
} EdgeType;

// Wayland State
struct wl_display *display = NULL;
struct wl_registry *registry = NULL;
struct wl_compositor *compositor = NULL;
struct wl_shm *shm = NULL;
struct zwlr_layer_shell_v1 *layer_shell = NULL;
struct wl_seat *seat = NULL;
struct wl_pointer *pointer = NULL;

typedef struct {
    struct wl_output *output;
    char name[64];
    EdgeType type;
    struct wl_surface *surface;
    struct zwlr_layer_surface_v1 *layer_surface;
    struct wl_buffer *buffer;
} EdgeSurface;

#define MAX_SURFACES 32
static EdgeSurface edge_surfaces[MAX_SURFACES];
static int num_surfaces = 0;
static struct wl_output *wl_outputs[MAX_MONITORS];
static int num_wl_outputs = 0;
static int active_side_trigger = 0;

static void pointer_enter(void *data, struct wl_pointer *pointer, uint32_t serial,
                          struct wl_surface *surface, wl_fixed_t sx, wl_fixed_t sy) {
    for (int i = 0; i < num_surfaces; i++) {
        if (edge_surfaces[i].surface == surface) {
            if (edge_surfaces[i].type == EDGE_LEFT) {
                if (!active_side_trigger) {
                    active_side_trigger = 1;
                    send_hyprland_cmd("eval hl.dispatch(hl.dsp.focus({ workspace = 'e-1' }))");
                }
            } else if (edge_surfaces[i].type == EDGE_RIGHT) {
                if (!active_side_trigger) {
                    active_side_trigger = 1;
                    send_hyprland_cmd("eval hl.dispatch(hl.dsp.focus({ workspace = 'e+1' }))");
                }
            } else if (edge_surfaces[i].type == EDGE_TOP) {
                set_waybar_visible(1); // Reveal Waybar on top edge enter
            }
            break;
        }
    }
}

static void pointer_leave(void *data, struct wl_pointer *pointer, uint32_t serial,
                          struct wl_surface *surface) {
    for (int i = 0; i < num_surfaces; i++) {
        if (edge_surfaces[i].surface == surface) {
            if (edge_surfaces[i].type == EDGE_LEFT || edge_surfaces[i].type == EDGE_RIGHT) {
                active_side_trigger = 0; // Re-arm workspace trigger upon inward movement
            } else if (edge_surfaces[i].type == EDGE_TOP) {
                set_waybar_visible(0); // Hide Waybar on top edge leave
            }
            break;
        }
    }
}

static void pointer_motion(void *data, struct wl_pointer *pointer, uint32_t time, wl_fixed_t sx, wl_fixed_t sy) {}
static void pointer_button(void *data, struct wl_pointer *pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state) {}
static void pointer_axis(void *data, struct wl_pointer *pointer, uint32_t time, uint32_t axis, wl_fixed_t value) {}
static void pointer_frame(void *data, struct wl_pointer *pointer) {}
static void pointer_axis_source(void *data, struct wl_pointer *pointer, uint32_t axis_source) {}
static void pointer_axis_stop(void *data, struct wl_pointer *pointer, uint32_t time, uint32_t axis) {}
static void pointer_axis_discrete(void *data, struct wl_pointer *pointer, uint32_t axis, int32_t discrete) {}

static const struct wl_pointer_listener pointer_listener = {
    .enter = pointer_enter,
    .leave = pointer_leave,
    .motion = pointer_motion,
    .button = pointer_button,
    .axis = pointer_axis,
    .frame = pointer_frame,
    .axis_source = pointer_axis_source,
    .axis_stop = pointer_axis_stop,
    .axis_discrete = pointer_axis_discrete,
};

static void seat_capabilities(void *data, struct wl_seat *seat, uint32_t caps) {
    if ((caps & WL_SEAT_CAPABILITY_POINTER) && !pointer) {
        pointer = wl_seat_get_pointer(seat);
        wl_pointer_add_listener(pointer, &pointer_listener, NULL);
    }
}

static void seat_name(void *data, struct wl_seat *seat, const char *name) {}

static const struct wl_seat_listener seat_listener = {
    .capabilities = seat_capabilities,
    .name = seat_name,
};

static struct wl_buffer *create_transparent_buffer(struct wl_shm *shm, int width, int height) {
    if (width <= 0) width = 1;
    if (height <= 0) height = 1;
    int stride = width * 4;
    int size = stride * height;

    int fd = memfd_create("shm-edge", MFD_CLOEXEC);
    if (fd < 0) return NULL;
    if (ftruncate(fd, size) < 0) { close(fd); return NULL; }

    uint32_t *data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) { close(fd); return NULL; }
    memset(data, 0, size); // Fully transparent ARGB8888 pixels
    munmap(data, size);

    struct wl_shm_pool *pool = wl_shm_create_pool(shm, fd, size);
    struct wl_buffer *buf = wl_shm_pool_create_buffer(pool, 0, width, height, stride, WL_SHM_FORMAT_ARGB8888);
    wl_shm_pool_destroy(pool);
    close(fd);
    return buf;
}

static void layer_surface_configure(void *data, struct zwlr_layer_surface_v1 *layer_surface,
                                    uint32_t serial, uint32_t width, uint32_t height) {
    zwlr_layer_surface_v1_ack_configure(layer_surface, serial);

    for (int i = 0; i < num_surfaces; i++) {
        if (edge_surfaces[i].layer_surface == layer_surface) {
            if (shm && !edge_surfaces[i].buffer) {
                edge_surfaces[i].buffer = create_transparent_buffer(shm, (int)width, (int)height);
            }
            if (edge_surfaces[i].buffer) {
                wl_surface_attach(edge_surfaces[i].surface, edge_surfaces[i].buffer, 0, 0);
                wl_surface_commit(edge_surfaces[i].surface);
            }
            break;
        }
    }
}

static void layer_surface_closed(void *data, struct zwlr_layer_surface_v1 *surface) {}

static const struct zwlr_layer_surface_v1_listener layer_surface_listener = {
    .configure = layer_surface_configure,
    .closed = layer_surface_closed,
};

static void registry_global(void *data, struct wl_registry *registry,
                            uint32_t name, const char *interface, uint32_t version) {
    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 4);
    } else if (strcmp(interface, wl_shm_interface.name) == 0) {
        shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
    } else if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0) {
        layer_shell = wl_registry_bind(registry, name, &zwlr_layer_shell_v1_interface, 1);
    } else if (strcmp(interface, wl_seat_interface.name) == 0) {
        seat = wl_registry_bind(registry, name, &wl_seat_interface, 1);
        wl_seat_add_listener(seat, &seat_listener, NULL);
    } else if (strcmp(interface, wl_output_interface.name) == 0) {
        struct wl_output *output = wl_registry_bind(registry, name, &wl_output_interface, 1);
        if (num_wl_outputs < MAX_MONITORS) {
            wl_outputs[num_wl_outputs++] = output;
        }
    }
}

static void registry_global_remove(void *data, struct wl_registry *registry, uint32_t name) {}

static const struct wl_registry_listener registry_listener = {
    .global = registry_global,
    .global_remove = registry_global_remove,
};

static void setup_edge_surface(struct wl_output *output, EdgeType type) {
    if (!compositor || !layer_shell || num_surfaces >= MAX_SURFACES) return;

    struct wl_surface *surface = wl_compositor_create_surface(compositor);
    uint32_t anchor = 0;
    uint32_t width = 0, height = 0;

    if (type == EDGE_LEFT) {
        anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM | ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT;
        width = 1; height = 0;
    } else if (type == EDGE_RIGHT) {
        anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT;
        width = 1; height = 0;
    } else if (type == EDGE_TOP) {
        anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT;
        width = 0; height = 3; // 3-pixel tall hover target along top edge
    }

    struct zwlr_layer_surface_v1 *layer_surface = zwlr_layer_shell_v1_get_layer_surface(
        layer_shell, surface, output, ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY, "edge-events");

    edge_surfaces[num_surfaces].surface = surface;
    edge_surfaces[num_surfaces].layer_surface = layer_surface;
    edge_surfaces[num_surfaces].type = type;
    edge_surfaces[num_surfaces].buffer = NULL;
    num_surfaces++;

    zwlr_layer_surface_v1_set_size(layer_surface, width, height);
    zwlr_layer_surface_v1_set_anchor(layer_surface, anchor);
    zwlr_layer_surface_v1_set_exclusive_zone(layer_surface, -1);
    zwlr_layer_surface_v1_set_keyboard_interactivity(layer_surface, 0);
    zwlr_layer_surface_v1_add_listener(layer_surface, &layer_surface_listener, NULL);

    wl_surface_commit(surface);
}

int main(int argc, char *argv[]) {
    int lock_fd = open("/tmp/hypr-edge-switcher.lock", O_CREAT | O_RDWR, 0666);
    if (lock_fd >= 0 && flock(lock_fd, LOCK_EX | LOCK_NB) != 0) {
        return 0; // Already running
    }

    init_socket_path();
    fetch_monitor_topology();
    init_waybar_state(); // Initialize Waybar state (hidden by default)

    display = wl_display_connect(NULL);
    if (!display) return 1;

    registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, NULL);
    wl_display_roundtrip(display);

    for (int i = 0; i < num_monitors; i++) {
        MonitorInfo *m = &monitors[i];
        struct wl_output *out = (i < num_wl_outputs) ? wl_outputs[i] : NULL;

        if (is_outer_side_edge(m, 1)) {
            setup_edge_surface(out, EDGE_LEFT);
        }
        if (is_outer_side_edge(m, 0)) {
            setup_edge_surface(out, EDGE_RIGHT);
        }
        if (is_outer_top_edge(m)) {
            setup_edge_surface(out, EDGE_TOP);
        }
    }

    while (wl_display_dispatch(display) != -1) {
        // Pure event-driven event loop sleeping in kernel epoll_wait
    }

    return 0;
}
