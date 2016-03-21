#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

GtkTextBuffer *buffer;
gchar *filename;
GtkWidget *view;
GtkWidget *window;

GdkPixbuf *create_pixbuf(const gchar *filename) {
    GdkPixbuf *pixbuf;
    GError *error = NULL;
    pixbuf  = gdk_pixbuf_new_from_file(filename, &error);
    if(!pixbuf) {
        fprintf(stderr, "%s\n", error->message);
        g_error_free(error);
    }
}

gchar* gtk_show_file_save(GtkWidget* parent_window, gchar *text, GtkWidget **dialog)
{
    GtkWidget *top_dialog;
    gchar *filename;
    FILE *pFile;

    top_dialog = gtk_file_chooser_dialog_new ("save file", GTK_WINDOW(parent_window), \
            GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);
    gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (top_dialog), TRUE);
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER (top_dialog), "~/");
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER (top_dialog), "Unsaved Document");

    if (gtk_dialog_run(GTK_DIALOG(top_dialog)) == GTK_RESPONSE_ACCEPT)
    {
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER (top_dialog));
        pFile = fopen(filename, "w");
        if(pFile == NULL) {
            *dialog = gtk_message_dialog_new(NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,"Permission denied");
        }else {
            size_t result = fwrite(text, 1, strlen(text), pFile);
            if(result == strlen(text)) {
                printf("%s\n", "success");
            }
            fflush(pFile);
            fclose(pFile);
            *dialog = gtk_message_dialog_new(NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_INFO,GTK_BUTTONS_OK,"save success!");
        }
        gtk_dialog_run(GTK_DIALOG(*dialog));
        //destroy
        gtk_widget_destroy(*dialog);
    }
    gtk_widget_destroy(top_dialog);
}

void select_font(GtkWidget *widget)
{
    GtkResponseType result;
    GtkWidget *dialog = gtk_font_selection_dialog_new("Select Font");
    result = gtk_dialog_run(GTK_DIALOG(dialog));
    if(result == GTK_RESPONSE_OK || result == GTK_RESPONSE_APPLY)
    {
        PangoFontDescription *font_desc;
        gchar *fontname = gtk_font_selection_dialog_get_font_name(GTK_FONT_SELECTION_DIALOG(dialog));
        font_desc = pango_font_description_from_string(fontname);
        gtk_widget_modify_font(view, font_desc);
        g_free(fontname);
    }
    gtk_widget_destroy(dialog);
}


void select_color(GtkWidget *widget, gpointer label)
{
    GtkResponseType result;
    GtkColorSelection *colorsel;
    GtkWidget *dialog = gtk_color_selection_dialog_new("Font Color");
    result = gtk_dialog_run(GTK_DIALOG(dialog));
    if (result == GTK_RESPONSE_OK) {
        GdkColor color;
        colorsel = GTK_COLOR_SELECTION(GTK_COLOR_SELECTION_DIALOG(dialog)->colorsel);
        gtk_widget_modify_fg(view, GTK_STATE_NORMAL, &color);
    }
    gtk_widget_destroy(dialog);
}

void show_about(GtkWidget *widget, gpointer data)
{
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file("icon.png", NULL);
    GtkWidget *dialog = gtk_about_dialog_new();
    gtk_about_dialog_set_name(GTK_ABOUT_DIALOG(dialog), "Notepad");
    gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(dialog), "1.0");
    gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(dialog), "(c) firebroo");
    gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(dialog), "Notepad is a tool to edit file.");
    gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(dialog),"http://www.firebroo.com");
    gtk_about_dialog_set_logo(GTK_ABOUT_DIALOG(dialog), pixbuf);
    g_object_unref(pixbuf), pixbuf = NULL;
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

void update_statusbar(GtkTextBuffer *buffer, GtkStatusbar *statusbar){
    gchar *msg;
    gint row, col;
    GtkTextIter iter;
    gtk_statusbar_pop(statusbar, 0);
    gtk_text_buffer_get_iter_at_mark(buffer, &iter,
            gtk_text_buffer_get_insert(buffer));
    row = gtk_text_iter_get_line(&iter);
    col = gtk_text_iter_get_line_offset(&iter);
    msg = g_strdup_printf("Col %d Ln %d", col+1, row+1);
    gtk_statusbar_push(statusbar, 0, msg);
    g_free(msg);
}

void save_file(GtkWidget *widget)
{
    GtkWidget *dialog;
    GtkTextIter start,end;
    gchar *text;
    FILE *pFile;
    gtk_text_buffer_get_bounds(GTK_TEXT_BUFFER(buffer),&start,&end);
    text = gtk_text_buffer_get_text(buffer, &start, &end, TRUE);
    if(filename) {
        pFile = fopen(filename, "w");
        if(pFile == NULL) {
            dialog = gtk_message_dialog_new(NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,"Permission denied");
        }else {
            size_t result = fwrite(text, 1, strlen(text), pFile);
            if(result == strlen(text)) {
                printf("%s\n", "success");
            }
            fflush(pFile);
            fclose(pFile);
            dialog = gtk_message_dialog_new(NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_INFO,GTK_BUTTONS_OK,"save success!");
        }
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    }else{
        gtk_show_file_save(window, text, &dialog);
        //dialog = gtk_message_dialog_new(NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,"please fist open file");
    }
}

void open_file(GtkWidget *file)
{
    GtkWidget *dialog;
    GtkWidget *label;
    GtkTextIter start,end;
    char *pBuf;
    filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(file));
    FILE *pFile = fopen(filename,"r");
    fseek(pFile,0,SEEK_END); //把指针移动到文件的结尾 ，获取文件长度
    int len = ftell(pFile);
    pBuf = (char *)malloc(len+1);
    rewind(pFile);
    fread(pBuf, 1, len, pFile);
    pBuf[len] = '\0';
    fclose(pFile);
    gtk_text_buffer_get_bounds(GTK_TEXT_BUFFER(buffer),&start,&end);
    //clear view
    gtk_text_buffer_delete(buffer, &start, &end);
    gtk_text_buffer_insert(GTK_TEXT_BUFFER(buffer),&start,pBuf,strlen(pBuf));
    free(pBuf);
    gtk_widget_destroy(file);
}

void select_and_open_file(GtkWidget *widget,gpointer data)
{
    GtkWidget *file;
    file = gtk_file_chooser_dialog_new("SelectFile",NULL,GTK_FILE_CHOOSER_ACTION_OPEN, \
            GTK_STOCK_CANCEL,GTK_RESPONSE_CANCEL,GTK_STOCK_OK,GTK_RESPONSE_ACCEPT,NULL);
    if(gtk_dialog_run(GTK_DIALOG(file)) == GTK_RESPONSE_ACCEPT) {
        open_file(file);
    }
    else {
        gtk_widget_destroy(file);
    }
}


static void mark_set_callback(GtkTextBuffer *buffer, const GtkTextIter \
        *new_location, GtkTextMark *mark, gpointer data){
    update_statusbar(buffer, GTK_STATUSBAR(data));
}


int main( int argc, char *argv[]){
    GtkWidget *menubar;
    GtkWidget *filemenu;
    GtkWidget *helpmenu;
    GtkWidget *tog_stat;
    GtkWidget *statusbar;
    GtkWidget *scrolled;
    GtkWidget *label;
    GtkWidget *file;
    GtkWidget *help;
    GtkWidget *console;
    GtkWidget *quit;
    GtkWidget *export;
    GtkWidget *vbox;
    GtkWidget *toolbar;
    GtkWidget *new;
    GtkWidget *nopen;
    GtkWidget *nsave;
    GtkWidget *sep;
    GtkWidget *sw;
    GtkWidget *about;
    GtkToolItem *open;
    GtkToolItem *save;
    GtkToolItem *font;
    GtkToolItem *color;
    GtkToolItem *exit;
    GtkAccelGroup *accel_group = NULL;
    gtk_init(&argc, &argv);
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 500);
    gtk_window_set_title(GTK_WINDOW(window), "Notepad");
    gtk_window_set_icon(GTK_WINDOW(window), create_pixbuf("icon.png"));
    gtk_container_set_border_width(GTK_CONTAINER(window), 5);

    //sw = gtk_scrolled_window_new(NULL, NULL);
    //gtk_container_add(GTK_CONTAINER(window), sw);
    //gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), \
    //        GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    //gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw), \
    //        GTK_SHADOW_IN);

    vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(window), vbox);
    toolbar = gtk_toolbar_new();
    gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);
    open = gtk_tool_button_new_from_stock(GTK_STOCK_OPEN);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), open, -1);
    save = gtk_tool_button_new_from_stock(GTK_STOCK_SAVE);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), save, -1);
    color = gtk_tool_button_new_from_stock(GTK_STOCK_SELECT_COLOR);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), color, -1);
    font = gtk_tool_button_new_from_stock(GTK_STOCK_SELECT_FONT);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), font, -1);
    exit = gtk_tool_button_new_from_stock(GTK_STOCK_QUIT);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), exit, -1);

    menubar = gtk_menu_bar_new();
    filemenu = gtk_menu_new();
    helpmenu = gtk_menu_new();


    accel_group = gtk_accel_group_new();
    gtk_window_add_accel_group(GTK_WINDOW(window), accel_group);
    //file = gtk_menu_item_new_with_label("File");
    file = gtk_menu_item_new_with_mnemonic("_File");
    nopen = gtk_image_menu_item_new_from_stock(GTK_STOCK_OPEN, NULL);
    //nsave = gtk_image_menu_item_new_from_stock(GTK_STOCK_SAVE, NULL);
    sep = gtk_separator_menu_item_new();
    quit = gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT, accel_group);
    nsave = gtk_image_menu_item_new_from_stock(GTK_STOCK_SAVE, accel_group);

    help = gtk_menu_item_new_with_label("Help");

    //quit = gtk_menu_item_new_with_label("quit");
    gtk_widget_add_accelerator(quit, "activate",  \
            accel_group, GDK_q, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(nsave, "activate", \
            accel_group, GDK_s, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    about = gtk_menu_item_new_with_label("about");

    gtk_menu_item_set_submenu(GTK_MENU_ITEM(file), filemenu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(help), helpmenu);

    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), file);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), help);

    gtk_menu_shell_append(GTK_MENU_SHELL(filemenu), nopen);
    gtk_menu_shell_append(GTK_MENU_SHELL(filemenu), nsave);
    //分割线
    gtk_menu_shell_append(GTK_MENU_SHELL(filemenu), sep);
    gtk_menu_shell_append(GTK_MENU_SHELL(filemenu), quit);

    gtk_menu_shell_append(GTK_MENU_SHELL(helpmenu), about);


    view = gtk_text_view_new();
    //gtk_box_pack_start(GTK_BOX(vbox), view, TRUE, TRUE, 0);
    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
    scrolled = gtk_scrolled_window_new(NULL, NULL); /*创建滚动窗口构件*/
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled), \
            GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
    gtk_widget_grab_focus(view);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled),view);

    statusbar = gtk_statusbar_new();

    gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(vbox), statusbar, FALSE, FALSE, 0);

    //gtk_widget_add_events(window, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(G_OBJECT(about), \
            "button-press-event",G_CALLBACK(show_about), NULL);
    g_signal_connect(G_OBJECT(nopen), \
            "button-press-event",G_CALLBACK(select_and_open_file), NULL);
    g_signal_connect(G_OBJECT(nsave), \
            "activate",G_CALLBACK(save_file), NULL);
    g_signal_connect(G_OBJECT(open), \
            "clicked",G_CALLBACK(select_and_open_file), NULL);
    g_signal_connect(G_OBJECT(save), \
            "clicked",G_CALLBACK(save_file), NULL);
    g_signal_connect(G_OBJECT(exit), \
            "clicked",G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(buffer, "changed", \
            G_CALLBACK(update_statusbar), statusbar);
    g_signal_connect_object(buffer,  \
            "mark_set",G_CALLBACK(mark_set_callback), statusbar, 0);
    g_signal_connect(G_OBJECT(quit), \
            "activate", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(G_OBJECT(color), \
            "clicked", G_CALLBACK(select_color), NULL);
    g_signal_connect(G_OBJECT(font), \
            "clicked", G_CALLBACK(select_font), NULL);
    g_signal_connect_swapped(G_OBJECT(window), "destroy", \
            G_CALLBACK(gtk_main_quit), NULL);
    gtk_widget_show_all(window);
    update_statusbar(buffer, GTK_STATUSBAR (statusbar));
    gtk_main();
    return 0;
}
