#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "notepad.h"

static gint             last_page = 0;
static guint            curr_page_num = 0;

static GtkWidget        *view;
static GtkWidget        *window;
static GtkSourceBuffer  *buffer;

static OpendFile        *hash[maxPage];

static GdkAtom           atom;
static GtkClipboard     *clipboard;

void
gtk_notepad_cut (void)
{
    gtk_text_buffer_cut_clipboard (GTK_TEXT_BUFFER (buffer), clipboard, TRUE);
}


void
gtk_notepad_copy (void)
{
    gtk_text_buffer_copy_clipboard (GTK_TEXT_BUFFER (buffer), clipboard);
}


void
gtk_notepad_paste (void)
{
    gtk_text_buffer_paste_clipboard (GTK_TEXT_BUFFER (buffer), clipboard, NULL, TRUE);
}


void 
gtk_notepad_delete (void)
{
    gtk_text_buffer_delete_selection (GTK_TEXT_BUFFER (buffer), TRUE, TRUE);
}


void 
gtk_notepad_select_all (void)
{
    GtkTextIter     start;
    GtkTextIter     end;

    gtk_text_buffer_get_start_iter (GTK_TEXT_BUFFER (buffer), &start);
    gtk_text_buffer_get_end_iter (GTK_TEXT_BUFFER (buffer), &end);
       
    gtk_text_buffer_select_range (GTK_TEXT_BUFFER (buffer), &start, &end);
}

void 
create_new_file (GtkWidget *widget, gpointer notebook)
{
    GtkWidget     *box;
    GtkWidget     *label;

    last_page++;
    curr_page_num = last_page;  

    (hash[curr_page_num]) = (OpendFile *) malloc (sizeof (OpendFile));
    (hash[curr_page_num])->filename = NULL; 
    (hash[curr_page_num])->content = NULL; 

    box  = gtk_vbox_new (FALSE, 0);
    label  = gtk_label_new ("Unsaved Document");
    gtk_label_set_width_chars (GTK_LABEL (label), 30);
    gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);

    (hash[curr_page_num])->label = label; 

    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), box, label);
    select_and_open_file (NULL);
    gtk_widget_show (box);
    gtk_widget_show (label);
}

void 
deal_switch_page (GtkNotebook *notebook, gpointer page, 
                  guint page_num, gpointer data)
{
    gchar   *file_suffix;

    curr_page_num = page_num;
    if ((hash[curr_page_num]) != NULL) {
        if ((hash[curr_page_num])->content != NULL) {
            gtk_text_buffer_set_text (GTK_TEXT_BUFFER (buffer),
                                      (hash[curr_page_num])->content,
                                      -1);
            gtk_window_set_title (GTK_WINDOW (window), (hash[curr_page_num])->filename);
            if ( (file_suffix = get_file_suffix ((hash[curr_page_num])->filename))) {
                set_buffer_language (file_suffix + 1);
            }
        }
    } 
}


GdkPixbuf *
create_pixbuf (const gchar *filename)
{
    GdkPixbuf  *pixbuf;
    GError     *error = NULL;

    pixbuf  = gdk_pixbuf_new_from_file (filename, &error);
    if (!pixbuf) {
        fprintf (stderr, "%s\n", error->message);
        g_error_free (error);
    }

    return pixbuf;
}

void
set_buffer_language (const gchar *lang)
{
    GtkSourceLanguage        *language = NULL;
    GtkSourceLanguageManager *lm;

    if (0 == strcmp (lang, "py")) {
        lang = "python";
    } else if (0 == strcmp (lang, "js")) {
        lang = "js";
    } else if (0 == strcmp (lang, "hs")) {
        lang = "haskell";
    } else if (0 == strcmp (lang, "rb")) {
        lang = "ruby";
    } else if (0 == strcmp(lang, "pl")) {
        lang = "perl";
    }

    lm = gtk_source_language_manager_new ();
    language = gtk_source_language_manager_get_language (lm, lang); /*加载语言语法高亮格式*/
    gtk_source_buffer_set_language (buffer, language);
}

STATUS 
write_buf (FILE *pFile, gchar *text)
{
    size_t  	writen;

    if ( (writen = fwrite (text, 1, strlen (text), pFile)) == strlen (text)) {
        fflush (pFile);
        gtk_label_set_text (GTK_LABEL ((hash[curr_page_num])->label),
                            strrchr (((hash[curr_page_num])->filename), '/') + 1);
        gtk_window_set_title (GTK_WINDOW(window), (hash[curr_page_num])->filename);
        return SUCCESS;
    } 

    return FAIL;
}


void
update_line_color (GtkWidget *view)
{
    GdkColor 	color;

    //line number default is orange
    gdk_color_parse ("Violet", &color);
    gtk_widget_modify_fg (view, GTK_STATE_NORMAL, &color);
}

gchar * 
get_file_suffix (const gchar* filename)
{
    return strrchr (filename, POINT);
}

gchar *
gtk_show_file_save (GtkWidget* parent_window, gchar *text, 
                   GtkWidget **dialog)
{
    FILE       *pFile;
    STATUS      status;
    GtkWidget  *top_dialog;
    gchar      *file_suffix;

label:
    top_dialog = gtk_file_chooser_dialog_new ("Save File",
                                              GTK_WINDOW(parent_window),
                                              GTK_FILE_CHOOSER_ACTION_SAVE,
                                              GTK_STOCK_SAVE,
                                              GTK_RESPONSE_ACCEPT,
                                              NULL);

    gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (top_dialog),
                                                    TRUE);

    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER (top_dialog), "~/");
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER (top_dialog), "Unsaved Document");

    if (gtk_dialog_run (GTK_DIALOG(top_dialog)) == GTK_RESPONSE_ACCEPT) {
        (hash[curr_page_num])->filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (top_dialog));
        if ( (file_suffix = get_file_suffix ((hash[curr_page_num])->filename))) {
            set_buffer_language (file_suffix + 1);
        }
        if ( (pFile = fopen ((hash[curr_page_num])->filename, "wb")) == NULL) {
            *dialog = gtk_message_dialog_new (NULL,
                                              GTK_DIALOG_MODAL, 
                                              GTK_MESSAGE_ERROR,
                                              GTK_BUTTONS_OK,
                                             "Permission Denied");
            gtk_dialog_run (GTK_DIALOG (*dialog));
            gtk_widget_destroy (*dialog);          /*destroy*/
            gtk_widget_destroy (top_dialog);
            goto label;
        } else {
            if ( (status = write_buf(pFile, text)) == SUCCESS ) {
                *dialog = gtk_message_dialog_new (NULL,
                                                  GTK_DIALOG_MODAL,
                                                  GTK_MESSAGE_INFO,
                                                  GTK_BUTTONS_OK,
                                                  "Save Success!");

                gtk_dialog_run(GTK_DIALOG(*dialog));
                gtk_widget_destroy(*dialog);       /*destroy*/
            }
        }
        fclose (pFile);
    }
    gtk_widget_destroy(top_dialog);                /*destroy*/
}

void 
set_font(GtkWidget *widget, gchar *fontname)
{
    PangoFontDescription    *font_desc;

    font_desc = pango_font_description_from_string(fontname);
    pango_font_description_set_size (font_desc, 13 * PANGO_SCALE);
    gtk_widget_modify_font(widget, font_desc);
    pango_font_description_free (font_desc);
}

void select_font (GtkWidget *widget)
{
    GtkResponseType     result;
    GtkWidget          *dialog;

    dialog = gtk_font_selection_dialog_new ("Select Font");
    result = gtk_dialog_run(GTK_DIALOG (dialog));
    if (result == GTK_RESPONSE_OK || result == GTK_RESPONSE_APPLY)
    {
        gchar *fontname = gtk_font_selection_dialog_get_font_name ( \
                GTK_FONT_SELECTION_DIALOG(dialog));
        set_font (view, fontname);
        g_free (fontname);
    }
    gtk_widget_destroy (dialog);
}


void
select_color (GtkWidget *widget)
{
    GtkResponseType     result;
    GtkColorSelection  *colorsel;
    GtkWidget          *dialog;
    GdkColor            color;

    dialog = gtk_color_selection_dialog_new ("Font Color");
    if ( (result = gtk_dialog_run (GTK_DIALOG(dialog)) == GTK_RESPONSE_OK )) {

        colorsel = GTK_COLOR_SELECTION (GTK_COLOR_SELECTION_DIALOG (dialog)->colorsel);
        //gdk_color_parse ("red", &color);
        gtk_color_selection_get_current_color (colorsel, &color);
        gtk_widget_modify_fg (view, GTK_STATE_NORMAL, &color);
        //gtk_widget_modify_base(view, GTK_STATE_NORMAL, &color);
    }
    gtk_widget_destroy (dialog);
}

void show_about(GtkWidget *widget, gpointer data)
{
    GdkPixbuf   *pixbuf;
    GtkWidget   *dialog;

    pixbuf = gdk_pixbuf_new_from_file("icon.png", NULL);
    dialog = gtk_about_dialog_new();
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

void
update_statusbar (GtkSourceBuffer *buffer, GtkStatusbar *statusbar)
{
    gchar        *msg;
    gint          row;
    gint          col;
    GtkTextIter   iter;
    GtkTextMark  *mark;

    gtk_statusbar_pop (statusbar, 0);
    gtk_text_buffer_get_iter_at_mark (GTK_TEXT_BUFFER(buffer), &iter,
            gtk_text_buffer_get_insert (GTK_TEXT_BUFFER(buffer)));

    row = gtk_text_iter_get_line (&iter);
    col = gtk_text_iter_get_line_offset (&iter);
    msg = g_strdup_printf ("row %d col %d", row + 1, col + 1);
    gtk_statusbar_push (statusbar, 0, msg);
    g_free (msg);
}

STATUS
save_file (GtkWidget *widget)
{
    FILE        *pFile;
    gchar       *text;
    STATUS       status;
    GtkWidget   *dialog;
    GtkTextIter  start,end;

    gtk_text_buffer_get_bounds (GTK_TEXT_BUFFER(buffer), &start, &end);
    text = gtk_text_buffer_get_text (GTK_TEXT_BUFFER(buffer), &start, &end, TRUE);
    gchar *content = strdup (text);
    if ((hash[curr_page_num])->content != NULL) {
        free ((hash[curr_page_num])->content);
    }   
    (hash[curr_page_num])->content = content;
    if ((hash[curr_page_num])->filename) {
        if ( (pFile = fopen((hash[curr_page_num])->filename, "wb")) == NULL) {
            dialog = gtk_message_dialog_new (NULL,
                                             GTK_DIALOG_MODAL,
                                             GTK_MESSAGE_ERROR,
                                             GTK_BUTTONS_OK,
                                             "Permission Denied");
        } else {
            if ( (status = write_buf (pFile, text) == SUCCESS )) {
                dialog = gtk_message_dialog_new (NULL,
                                                 GTK_DIALOG_MODAL,
                                                 GTK_MESSAGE_INFO,
                                                 GTK_BUTTONS_OK,
                                                 "save success!");
            }
            fclose (pFile);
        }
        gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);
    } else {
        gtk_show_file_save (window, text, &dialog);
        //dialog = gtk_message_dialog_new(NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,"please fist open file");
    }
}

int
open_file (GtkWidget *file)
{
    long           len;
    FILE          *pFile;
    guchar        *pBuf;
    size_t         readn;
    GtkWidget     *dialog;
    GtkTextIter    start, end;
    gchar         *file_suffix;

    if (hash[curr_page_num] == NULL) {
        hash[curr_page_num] = (OpendFile *) malloc (sizeof (OpendFile));
    }   
    if ( ((hash[curr_page_num])->filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (file))) == NULL) {
        return FAIL;
    }
    if ( (file_suffix = get_file_suffix((hash[curr_page_num])->filename))) {
        set_buffer_language (file_suffix + 1);
    }
    if ( (pFile = fopen ((hash[curr_page_num])->filename, "rb"))) {
        fseek (pFile, 0, SEEK_END); /*把指针移动到文件的结尾 ，获取文件长度*/
        len = ftell (pFile);
        pBuf = (gchar *) malloc (len + 1);
        if (pBuf == NULL) {
            fprintf (stderr, "out of memory\n");
            exit(-1);
        }
        rewind (pFile);
        if ( (readn = fread (pBuf, 1, len, pFile)) == len){
            pBuf[len] = '\0';

            fclose (pFile);

            //gtk_text_buffer_get_bounds (GTK_TEXT_BUFFER (buffer), &start, &end);
            //clear view
            (hash[curr_page_num])->content = pBuf;

            gtk_text_buffer_set_text (GTK_TEXT_BUFFER (buffer), pBuf, -1);
            //gtk_text_buffer_delete(GTK_TEXT_BUFFER(buffer), &start, &end);
            //gtk_text_buffer_insert(GTK_TEXT_BUFFER(buffer),&start,pBuf,strlen(pBuf));
            //free (pBuf);
            gtk_widget_destroy (file);
            //gtk_label_set_text(GTK_LABEL(label), temp);
            return SUCCESS;
        } else {
            fprintf (stderr, "read fail\n");
            exit(-1);
        }
   } else {
        dialog = gtk_message_dialog_new (NULL,
                                         GTK_DIALOG_MODAL,
                                         GTK_MESSAGE_ERROR,
                                         GTK_BUTTONS_OK,
                                         "Permission Denied");
        gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);
        return FAIL;
    }
}

void
select_and_open_file (GtkWidget *widget)
{
    GtkWidget   *file;
    STATUS       result;

select_file:
    file = gtk_file_chooser_dialog_new ("SelectFile",
                                        NULL,
                                        GTK_FILE_CHOOSER_ACTION_OPEN,
                                        GTK_STOCK_CANCEL,GTK_RESPONSE_CANCEL,
                                        GTK_STOCK_OK,GTK_RESPONSE_ACCEPT,
                                        NULL);
    if (gtk_dialog_run (GTK_DIALOG(file)) == GTK_RESPONSE_ACCEPT) {
        if ( (result = open_file(file)) == FAIL) {
            gtk_widget_destroy(file);
            goto select_file;
        } else {
            if ((hash[curr_page_num])->filename) {
                gchar *result = strrchr ((hash[curr_page_num])->filename, '/');
                gtk_label_set_text (GTK_LABEL ((hash[curr_page_num])->label), result + 1);
                gtk_window_set_title (GTK_WINDOW(window), (hash[curr_page_num])->filename);
            }
            //gtk_label_set_text((GtkLabel *)(hash[curr_page_num])->label, strrchr ((hash[curr_page_num])->filename, '/') + 1);
        }
    } else {
        gtk_widget_destroy (file);
    }
}


void
mark_set_callback (GtkSourceBuffer *buffer, const GtkTextIter \
        *new_location, GtkTextMark *mark, gpointer data){
    update_statusbar (buffer, GTK_STATUSBAR (data));
}


void
init_text_view() {
    atom = gdk_atom_intern ("CLIPBOARD", TRUE);
    clipboard = gtk_clipboard_get (atom); /* get primary clipboard) */
    update_line_color (view);
    set_font (view, "Monospace Italic");
}

int
main( int argc, char *argv[])
{
    GtkWidget                *box;
    GtkWidget                *notebook;
    GtkWidget                *menubar;
    GtkWidget                *filemenu;
    GtkWidget                *editmenu;
    GtkWidget                *helpmenu;
    GtkWidget                *tog_stat;
    GtkWidget                *statusbar;
    GtkWidget                *scrolled;
    GtkWidget                *label;
    GtkWidget                *file;
    GtkWidget                *help;
    GtkWidget                *console;
    GtkWidget                *quit;
    GtkWidget                *export;
    GtkWidget                *vbox;
    GtkWidget                *toolbar;
    GtkWidget                *nnew;
    GtkWidget                *nopen;
    GtkWidget                *nsave;
    GtkWidget                *sep;
    GtkWidget                *sw;
    GtkWidget                *about;
    GtkWidget                *edit;
    GtkWidget                *cut;
    GtkWidget                *copy;
    GtkWidget                *paste;
    GtkWidget                *delete;
    GtkWidget                *selectall;
    GtkToolItem              *open;
    GtkToolItem              *new;
    GtkToolItem              *save;
    GtkToolItem              *font;
    GtkToolItem              *color;
    GtkToolItem              *exit;
    GtkAccelGroup            *accel_group = NULL;
    GdkScreen                *screen;
    GtkSourceLanguage        *language = NULL; /* 创建一个GtkSourceLanguagesManager 管理标记语言 */
    GtkSourceLanguageManager *lm;             /* 管理 */


    gtk_init (&argc, &argv);
    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_position (GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    //gtk_window_set_default_size(GTK_WINDOW(window), 600, 500);
    screen = gtk_window_get_screen (GTK_WINDOW (window));
    gtk_window_set_default_size (GTK_WINDOW(window),
                                 gdk_screen_get_width(screen),
                                 gdk_screen_get_height(screen));
    gtk_window_set_title (GTK_WINDOW (window), "Notepad");
    gtk_window_set_icon (GTK_WINDOW (window), create_pixbuf ("icon.png"));
    gtk_container_set_border_width (GTK_CONTAINER (window), 5);

    vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (window), vbox);
    toolbar = gtk_toolbar_new ();
    gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);
    open = gtk_tool_button_new_from_stock (GTK_STOCK_OPEN);
    gtk_toolbar_insert (GTK_TOOLBAR (toolbar), open, -1);
    new = gtk_tool_button_new_from_stock (GTK_STOCK_NEW);
    gtk_toolbar_insert (GTK_TOOLBAR(toolbar), new, -1);
    save = gtk_tool_button_new_from_stock (GTK_STOCK_SAVE);
    gtk_toolbar_insert (GTK_TOOLBAR(toolbar), save, -1);
    color = gtk_tool_button_new_from_stock (GTK_STOCK_SELECT_COLOR);
    gtk_toolbar_insert (GTK_TOOLBAR(toolbar), color, -1);
    font = gtk_tool_button_new_from_stock (GTK_STOCK_SELECT_FONT);
    gtk_toolbar_insert (GTK_TOOLBAR(toolbar), font, -1);
    exit = gtk_tool_button_new_from_stock (GTK_STOCK_QUIT);
    gtk_toolbar_insert (GTK_TOOLBAR(toolbar), exit, -1);

    menubar  = gtk_menu_bar_new ();
    filemenu = gtk_menu_new ();
    editmenu = gtk_menu_new();
    helpmenu = gtk_menu_new ();


    accel_group = gtk_accel_group_new ();
    gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);

    /*File menu*/
    file = gtk_menu_item_new_with_mnemonic ("_File");
    nopen = gtk_image_menu_item_new_from_stock (GTK_STOCK_OPEN, NULL);
    nnew = gtk_image_menu_item_new_from_stock (GTK_STOCK_NEW, NULL);
    sep = gtk_separator_menu_item_new ();
    quit = gtk_image_menu_item_new_from_stock (GTK_STOCK_QUIT, accel_group);
    nsave = gtk_image_menu_item_new_from_stock (GTK_STOCK_SAVE, accel_group);
    gtk_widget_add_accelerator (quit, 
                                "activate",
                                accel_group,
                                GDK_q,
                                GDK_CONTROL_MASK,
                                GTK_ACCEL_VISIBLE);

    gtk_widget_add_accelerator (nsave,
                                "activate",
                                accel_group,
                                GDK_s,
                                GDK_CONTROL_MASK, 
                                GTK_ACCEL_VISIBLE);



    /* Edit menu */
    edit = gtk_menu_item_new_with_mnemonic ("_Edit");
    cut  = gtk_menu_item_new_with_mnemonic ("Cu_t");
    copy = gtk_menu_item_new_with_mnemonic ("_Copy");
    paste = gtk_menu_item_new_with_mnemonic ("_Paste");
    delete = gtk_menu_item_new_with_mnemonic ("_Delete");
    selectall = gtk_menu_item_new_with_mnemonic ("_Select All");

    gtk_widget_add_accelerator (cut, 
                               "activate", 
                               accel_group, 
                               GDK_x,
                               GDK_CONTROL_MASK,
                               GTK_ACCEL_VISIBLE);

    gtk_widget_add_accelerator (copy, 
                                "activate",
                                accel_group,
                                GDK_c,
                                GDK_CONTROL_MASK,
                                GTK_ACCEL_VISIBLE);

    gtk_widget_add_accelerator (paste,
                                "activate",
                                accel_group,
                                GDK_v,
                                GDK_CONTROL_MASK,
                                GTK_ACCEL_VISIBLE);

    gtk_widget_add_accelerator (delete, 
                                "activate",
                                accel_group,
                                GDK_KEY_Delete,
                                0,
                                GTK_ACCEL_VISIBLE);

    gtk_widget_add_accelerator (selectall,
                                "activate",
                                accel_group,
                                GDK_a,
                                GDK_CONTROL_MASK,
                                GTK_ACCEL_VISIBLE);
  

    /* Help memu */
    help = gtk_menu_item_new_with_label ("Help");
    about = gtk_image_menu_item_new_from_stock (GTK_STOCK_ABOUT, NULL);


    gtk_menu_item_set_submenu (GTK_MENU_ITEM (file), filemenu);
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (edit), editmenu);
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (help), helpmenu);

    gtk_menu_shell_append (GTK_MENU_SHELL (menubar), file);
    gtk_menu_shell_append (GTK_MENU_SHELL (menubar), edit);
    gtk_menu_shell_append (GTK_MENU_SHELL (menubar), help);

    gtk_menu_shell_append (GTK_MENU_SHELL (filemenu), nopen);
    gtk_menu_shell_append (GTK_MENU_SHELL (filemenu), nnew);
    gtk_menu_shell_append (GTK_MENU_SHELL (filemenu), nsave);
    //分割线
    gtk_menu_shell_append (GTK_MENU_SHELL (filemenu), sep);
    gtk_menu_shell_append (GTK_MENU_SHELL (filemenu), quit);

    gtk_menu_shell_append (GTK_MENU_SHELL (editmenu), cut);
    gtk_menu_shell_append (GTK_MENU_SHELL (editmenu), copy);
    gtk_menu_shell_append (GTK_MENU_SHELL (editmenu), paste);
    gtk_menu_shell_append (GTK_MENU_SHELL (editmenu), delete);
    gtk_menu_shell_append (GTK_MENU_SHELL (editmenu),
                             gtk_separator_menu_item_new());
    gtk_menu_shell_append (GTK_MENU_SHELL (editmenu), selectall);
 
    gtk_menu_shell_append (GTK_MENU_SHELL (helpmenu), about);


    buffer = GTK_SOURCE_BUFFER (gtk_source_buffer_new (NULL));                  /*创建缓冲区 */
    view = gtk_source_view_new_with_buffer (buffer);
    gtk_source_view_set_show_line_marks (GTK_SOURCE_VIEW (view), TRUE);         /* 显示行号栏 */
    gtk_source_view_set_show_line_numbers (GTK_SOURCE_VIEW (view), TRUE);       /* 行号栏里面显示数字 */
    gtk_source_view_set_highlight_current_line (GTK_SOURCE_VIEW (view), TRUE);  /* 突显当前行 */
    gtk_source_view_set_indent_on_tab (GTK_SOURCE_VIEW (view), TRUE);
    gtk_source_view_set_auto_indent (GTK_SOURCE_VIEW (view), TRUE);             /* 启动自动缩进的文本 */
    gtk_source_view_set_tab_width (GTK_SOURCE_VIEW (view), 4);
    gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (view), GTK_WRAP_CHAR);          /* 设置自动换行的模式 */

    scrolled = gtk_scrolled_window_new (NULL, NULL);                            /* 创建滚动窗口构件 */
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
                                    GTK_POLICY_AUTOMATIC,
                                    GTK_POLICY_AUTOMATIC);
    gtk_widget_grab_focus (view);
    gtk_container_add (GTK_CONTAINER (scrolled), view);

    statusbar = gtk_statusbar_new ();

    notebook = gtk_notebook_new ();
    box = gtk_vbox_new (FALSE, 0);

    label = gtk_label_new ("Unsaved Document");
    gtk_label_set_width_chars(GTK_LABEL(label), 30);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);

    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), box, label);
    gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), 0);

    curr_page_num = 0;
    hash[curr_page_num] = (OpendFile *) malloc (sizeof (OpendFile));
    (hash[curr_page_num])->label = label ;
    (hash[curr_page_num])->filename = NULL ;
    (hash[curr_page_num])->content = NULL ;

    gtk_box_pack_start (GTK_BOX(vbox), menubar,   FALSE, FALSE, 5);
    gtk_box_pack_start (GTK_BOX(vbox), toolbar,   FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX(vbox), notebook,  FALSE, FALSE, 5);
    gtk_box_pack_start (GTK_BOX(vbox), scrolled,  TRUE,  TRUE,  5);
    gtk_box_pack_start (GTK_BOX(vbox), statusbar, FALSE, FALSE, 0);

    //gtk_widget_add_events(window, GDK_BUTTON_PRESS_MASK);
    g_signal_connect (G_OBJECT (cut), \
            "activate", G_CALLBACK(gtk_notepad_cut), NULL);
    g_signal_connect (G_OBJECT (copy), \
            "activate", G_CALLBACK (gtk_notepad_copy), NULL);
    g_signal_connect (G_OBJECT (paste), \
            "activate", G_CALLBACK (gtk_notepad_paste), NULL);
    g_signal_connect (G_OBJECT (delete), \
            "activate",G_CALLBACK (gtk_notepad_delete), NULL);
    g_signal_connect (G_OBJECT (selectall), \
            "activate", G_CALLBACK (gtk_notepad_select_all), NULL);
    g_signal_connect (G_OBJECT (about), \
            "button-press-event", G_CALLBACK(show_about), NULL);
    g_signal_connect (G_OBJECT (nopen), \
            "button-press-event", G_CALLBACK(select_and_open_file), NULL);
    g_signal_connect (G_OBJECT (nsave), \
            "activate",G_CALLBACK(save_file), NULL);
    g_signal_connect (G_OBJECT (nnew), \
            "activate",G_CALLBACK(create_new_file), notebook);
    g_signal_connect (G_OBJECT (new), \
            "clicked",G_CALLBACK(create_new_file), notebook);
    g_signal_connect (G_OBJECT (open), \
            "clicked",G_CALLBACK(select_and_open_file), NULL);
    g_signal_connect (G_OBJECT (save), \
            "clicked",G_CALLBACK(save_file), NULL);
    g_signal_connect (G_OBJECT (exit), \
            "clicked",G_CALLBACK (gtk_main_quit), NULL);
    g_signal_connect (buffer, \
            "changed", G_CALLBACK(update_statusbar), statusbar);
    g_signal_connect_object (buffer, \
            "mark_set", G_CALLBACK (mark_set_callback), statusbar, 0);
    g_signal_connect (G_OBJECT(quit), \
            "activate", G_CALLBACK (gtk_main_quit), NULL);
    g_signal_connect (G_OBJECT(color), \
            "clicked", G_CALLBACK (select_color), NULL);
    g_signal_connect (G_OBJECT(font), \
            "clicked", G_CALLBACK (select_font), NULL);
    g_signal_connect (notebook, \
            "switch-page", G_CALLBACK (deal_switch_page), NULL);
    g_signal_connect_swapped (G_OBJECT (window), \
            "destroy", G_CALLBACK (gtk_main_quit), NULL);
    gtk_widget_show_all (window);
    update_statusbar (buffer, GTK_STATUSBAR (statusbar));
    init_text_view ();
    gtk_main ();
    return 0;
}
