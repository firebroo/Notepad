#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <gtksourceview/gtksourceview.h>
#include <gtksourceview/gtksourcebuffer.h>
#include <gtksourceview/gtksourcelanguage.h>
#include <gtksourceview/gtksourcelanguagemanager.h>

#define POINT '.'
#define maxPage 1024

typedef struct {
    gchar      *filename;
    guchar     *content;
    GtkWidget  *label;
} OpendFile;

typedef enum _status {
    SUCCESS,
    FAIL,
} STATUS;



void gtk_notepad_cut (void);

void gtk_notepad_copy (void);

void gtk_notepad_paste (void);

void gtk_notepad_delete (void);

void gtk_notepad_select_all (void);

void create_new_file (GtkWidget *widget, gpointer notebook);

void deal_switch_page (GtkNotebook *notebook, gpointer page,
        guint page_num, gpointer data);

GdkPixbuf *create_pixbuf (const gchar *filename);

void set_buffer_language (const gchar * lang);

STATUS write_buf (FILE *pFile, gchar *text);

void update_line_color (GtkWidget *view);

gchar * get_file_suffix (const gchar* filename);

gchar* gtk_show_file_save (GtkWidget* parent_window, 
        gchar *text, GtkWidget **dialog);

void set_font (GtkWidget *widget, gchar *fontname);

void select_font (GtkWidget *widget);

void select_color (GtkWidget *widget);

void show_about (GtkWidget *widget, gpointer data);

void update_statusbar (GtkSourceBuffer *buffer, GtkStatusbar *statusbar);

STATUS save_file (GtkWidget *widget);

int open_file (GtkWidget *file);

void select_and_open_file (GtkWidget *widget);

void mark_set_callback (GtkSourceBuffer *buffer, const GtkTextIter \
         *new_location, GtkTextMark *mark, gpointer data);

void init_text_view(void);
