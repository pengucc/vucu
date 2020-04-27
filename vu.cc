/*
    vu.cc, Copyright (C) 2020 Peng-Hsien Lai

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#define VUCU_VERSION_STRING "0.1"

#include<cstdio>
#include<cstdlib>
#include<errno.h>
#include<glob.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<regex.h>
#include<unistd.h>
#include<fcntl.h>
#include<pthread.h>
#include<sys/time.h>
#include<sys/resource.h>

#include<gtk/gtk.h>
#include<gdk/gdkkeysyms.h>

#include<cmath>
#include<vector>
#include<string>
#include<utility>
#include<set>
#include<map>

#include"cu.h"
#include"cu.tab.h"
#define p_assert g_assert

using std::vector;
using std::string;
using std::pair;
using std::make_pair;
using std::set;
using std::multiset;
using std::map;
using std::multimap;

// define missing function/macros in old gtk releases

#ifndef GDK_KEY_Return
#define GDK_KEY_Return GDK_Return
#endif
#ifndef GDK_KEY_Alt_L
#define GDK_KEY_Alt_L GDK_Alt_L
#endif
#ifndef GDK_KEY_Alt_R
#define GDK_KEY_Alt_R GDK_Alt_R
#endif

#if GTK_MINOR_VERSION < 12
void __attribute__((weak)) gtk_widget_set_tooltip_text(GtkWidget *widget, const char *tooltip){
}
#endif

gdouble __attribute__((weak)) gtk_adjustment_get_page_size(GtkAdjustment *adj){
    GValue value = {0};
    g_value_init(&value, G_TYPE_DOUBLE);
    g_object_get_property(G_OBJECT(adj), "page-size", &value);
    return g_value_get_double(&value);
}

gdouble __attribute__((weak)) gtk_adjustment_get_upper(GtkAdjustment *adj){
    GValue value = {0};
    g_value_init(&value, G_TYPE_DOUBLE);
    g_object_get_property(G_OBJECT(adj), "upper", &value);
    return g_value_get_double(&value);
}

gdouble __attribute__((weak)) gtk_adjustment_get_lower(GtkAdjustment *adj){
    GValue value = {0};
    g_value_init(&value, G_TYPE_DOUBLE);
    g_object_get_property(G_OBJECT(adj), "lower", &value);
    return g_value_get_double(&value);
}

void __attribute__((weak)) gtk_widget_set_can_focus(GtkWidget* widget, gboolean can_focus){
    GValue value = {0};
    g_value_init(&value, G_TYPE_BOOLEAN);
    g_value_set_boolean(&value, can_focus);
    g_object_set_property(G_OBJECT(widget), "can-focus", &value);
}

gboolean __attribute__((weak)) gtk_widget_self_sensitive(GtkWidget* widget){
    GValue value = {0};
    g_value_init(&value, G_TYPE_BOOLEAN);
    g_object_get_property(G_OBJECT(widget), "sensitive", &value);
    return g_value_get_boolean(&value);
}


int HsvBlock::block_limit = -1;
int HsvBlock::block_inuse = 0;
HsvStorage HsvStorage::unused;
raw_block_list_t* HsvBlock::freed=NULL;

class Hsview;

struct HsvIndirectlyScrollableTextView;

struct _app {
    int            window_width;
    int            window_height;
    int            closing;
    const char*    home;
    const char*    dotVU_history;
    GtkListStore*  recent_files_store;
    int            recent_files_removed;

    GtkWidget*     load_progress;
    HsvFile*       file;
    Hsview*        current_view;

    GtkWidget*     top_window;
    GtkWidget*     notebook;
    GtkWidget*     info_page;
    GtkFontButton* code_font_button;
    GtkWidget*     new_page;

    GtkWidget*    hsview;
    unsigned int  time_stamp;
    GtkWidget*     not_found;
    HsvIndirectlyScrollableTextView* execution_bind;
    int                              execution_bind_deleted;
} app; 

void app_main_quit(){
    app.closing = 1;
    gtk_main_quit();
}

HsvLine* hsv_app_get_line(long linenum){
    if(app.file && linenum>=0 && linenum<app.file->get_number_of_lines()){
        return new HsvLine(app.file->get_line(linenum));
    }
    return NULL;
}

HsvFile* HsvFile::load_file(int f, long file_size){
    HsvFile* file = new HsvFile(f);

    long num_line = 0;
    int line_started = 1;
    long file_offset = 0;
    int  end_of_file = 0;
    int eof = 0;
    int cnt = 0;
    while(1){
        char* buffer = HsvBlock::get_raw_data_block();
        int sz = 0;
        while(sz<HSV_BLOCK_SIZE){
            int status = read(f, buffer+sz, HSV_BLOCK_SIZE-sz);
            if(status>0){
                sz += status;
            }else if(status==0){
                eof = 1;
                break;
            }else{
                puts("read failed.");
                exit(1);
            }
        }
        if(sz == 0){
            HsvBlock::return_raw_data_block(buffer);
            break;
        }
        HsvBlock* block = new HsvBlock(buffer, sz, line_started);
        cnt++;
        file->file_offset_of_block.push_back(file_offset);
        file->first_line_of_block.push_back(num_line);
        file_offset += sz;
        for(int i=0; i<sz; ++i){
            int c;
            if( line_started ){
                line_started = 0;
                block->line_start_offset.push_back(i);
                num_line++;
            }
            c = block->data[i];
            if(!c || c=='\n'){
                block->data[i] = 0;
                line_started   = 1;
            }
            if(c>127){
                printf("Error: not an ASCII text file"); exit(1);
            }
        }
        block->number_of_newline = block->line_start_offset.size();
        block->newline_terminated = line_started;
        if(sz<HSV_BLOCK_SIZE && !line_started){
            block->data[sz] = 0;
            block->newline_terminated = 1;
        }
        if(block->newline_terminated){
            block->tail_line = block->data + block->line_start_offset.back(); 
        }
        if(!eof && block->number_of_newline==0){
            printf("Sorry: line too long"); exit(1);
        }
        file->blocks.push_back(block);
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(app.load_progress), ((gdouble)file_offset)/file_size);
        if(gtk_events_pending()){
            gtk_main_iteration();
        }
        if(cnt>1){
            block->unload();
        }
        if(app.closing){
            delete file;
            return NULL;
        }
    }
    file->first_line_of_block.push_back(num_line);
    file->number_of_lines = num_line;

    return file;
}

void memory_limit_changed(GtkRange* range, GtkWidget* label){
    if(!range || !label) return;
    const int N = 10;
    int choice;
    static const char* value_str[N+1] = {
        "50MB", "100MB", "150MB", "200MB", "300MB", "400MB", "600MB", "800MB", "1GB", "1.5GB", "unlimited"
    };
    static const int block_number_limit[N+1] = {
        50, 100, 150, 200, 300, 400, 600, 800, 1024, 1536, -1
    };
    choice = int(gtk_range_get_value(range)*N+0.5);
    gtk_label_set_text(GTK_LABEL(label), value_str[choice]); 
    HsvBlock::block_limit = block_number_limit[choice]*((1<<20)/HSV_BLOCK_SIZE);
    struct rusage usage;
    if((getrusage(RUSAGE_SELF,&usage))){
        printf("Error, getrusage failed\n"); exit(1); 
    }
    if(HsvBlock::block_limit>0 && 
      usage.ru_maxrss>(HSV_BLOCK_SIZE>>10)*int(HsvBlock::block_limit*1.1)){
        gtk_range_set_value(range, gtk_range_get_value(range)+1.0/N);
    }
}

gboolean set_widget_sensitive(GtkWidget* widget){
    gtk_widget_set_sensitive(widget, TRUE);
    return TRUE;
}
gboolean unset_widget_sensitive(GtkWidget* widget){
    gtk_widget_set_sensitive(widget, FALSE);
    return TRUE;
}

enum {
    RF_FILEPATH,
    RF_OPENDATE,
    RF_DATETIME,
    RF_COMMENTS,
    RF_KEEPFILE,
    RF_HIDENSEQ,
    RF_NUM_COLS
};


gboolean mouse_entered(GtkWidget* widget, GdkEvent* event, GtkWidget* object){
    printf("mouse_entered\n");
    return FALSE;
}
gboolean mouse_leaved(GtkWidget* widget, GdkEvent* event, GtkWidget* object){
    printf("mouse_leaved\n");
    return FALSE;
}
gboolean show_object(GtkWidget* widget, GdkEvent* event, GtkWidget* object){
    gtk_widget_show(object);
    return FALSE;
}
gboolean hide_object(GtkWidget* widget, GdkEvent* event, GtkWidget* object){
    gtk_widget_hide(object);
    return FALSE;
}

GtkWidget* python_text_view;
gboolean destroy_on_clicked(GtkWidget* widget, GdkEvent* event, GtkWidget* target){
    if(event->type==GDK_BUTTON_PRESS){
        gtk_container_remove(GTK_CONTAINER(python_text_view), target);
        return TRUE;
    }
    return FALSE;
}

gboolean destroy_anchor_on_clicked(GtkWidget* widget, GdkEvent* event, gpointer data){
    if(event->type==GDK_BUTTON_PRESS){
        GtkTextBuffer* buffer = GTK_TEXT_BUFFER((GtkWidget*) g_object_get_data(G_OBJECT(widget), "buffer"));
        GtkTextChildAnchor* anchor = (GtkTextChildAnchor*) g_object_get_data(G_OBJECT(widget), "anchor");
        GtkTextIter iter;
        gtk_text_buffer_get_iter_at_child_anchor(buffer, &iter, anchor);
        GtkTextIter end = iter;
        gtk_text_iter_forward_chars(&end, 2);
        gtk_text_buffer_delete(buffer, &iter, &end);
        return TRUE;
    }
    return FALSE;
}

struct VuGtkTextMark{
    static GtkTextBuffer* buffer;
    static int global_stamp;
    static int count_update;

    GtkTextMark*  mark;
    mutable int   stmp;
    mutable int   line;
    mutable char  edited;
    mutable char  open;
    mutable int   trait;

    mutable short closing_para, opening_para;
    mutable short closing_brak, opening_brak;
    mutable short closing_brac, opening_brac;
    mutable short closing_cmmt, opening_cmmt;

    VuGtkTextMark(){
        mark   = NULL;
        stmp   = global_stamp;
        edited = 0;
        open   = 0;
        trait  = 0;
        closing_para=0, opening_para=0;
        closing_brak=0, opening_brak=0;
        closing_brac=0, opening_brac=0;
        closing_cmmt=0, opening_cmmt=0;
    }
    int get_line() const{
        if(!stmp){
            printf("error on mark %p of line %d\n", mark, line);
        }
        p_assert(stmp);
        p_assert(!mark || !gtk_text_mark_get_deleted(mark));
        if(stmp<global_stamp){
            GtkTextIter iter;
            gtk_text_buffer_get_iter_at_mark(buffer, &iter, mark);
            line = gtk_text_iter_get_line(&iter);
            stmp = global_stamp;
            count_update++;
        }
        return line;
    }
    bool operator<(const VuGtkTextMark& rhs) const{
        return get_line() < rhs.get_line();
    }
    typedef multiset<VuGtkTextMark>::iterator iterator;
};

GtkTextBuffer* VuGtkTextMark::buffer = NULL;
int VuGtkTextMark::global_stamp = 1;
int VuGtkTextMark::count_update = 0;

struct MarkIterComp{
    bool operator()(const VuGtkTextMark::iterator& lhs, const VuGtkTextMark::iterator& rhs){
        return *lhs < *rhs; 
    }
};

struct HsvIndirectlyScrollableTextView{
    GtkAdjustment* h_adj;
    GtkAdjustment* v_adj;
    GtkWidget*     scrolled_container;
    GtkWidget*     output_box;
    GtkTextView*   text_view;
    GtkTextBuffer* text_buffer;
    GtkWidget*     occupied;
    GtkWidget*     stop_btn;
    gulong         font_set_handler;
    GtkTextTag*    tag1;
    int            flags;
    GtkTextTag*    red;
    GtkTextTag*    blue;
    GtkTextTag*    brown;
    GtkTextTag*    purple;
    GtkTextTag*    para_error;
    GtkTextTag*    syntax_err;
    int            quiet;
    int            insert_begin;
    int            delete_begin;
     multiset<VuGtkTextMark> marks;
     set<VuGtkTextMark::iterator,MarkIterComp> edited_groups;
     void add_edited(VuGtkTextMark::iterator&, int force=0);
     void remove_edited(VuGtkTextMark::iterator&);
     set<VuGtkTextMark::iterator,MarkIterComp> open_lines;
     int overall_para;
     int overall_brak;
     int overall_brac;
     int overall_cmmt;
     HsvIndirectlyScrollableTextView(){
         overall_para = 0;
         overall_brak = 0;
         overall_brac = 0;
         overall_cmmt = 0;
     }
};

void HsvIndirectlyScrollableTextView::add_edited(VuGtkTextMark::iterator& self, int force){
    if(self->edited &&!force) return;
    self->edited = 1;
    VuGtkTextMark::iterator prev = self; --prev;
    if(prev->edited){
        if(!force){
            return;
        }
    }else{
        this->edited_groups.insert(self);
    }
    VuGtkTextMark::iterator next = self; ++next;
    if(next->edited){
        set<VuGtkTextMark::iterator>::iterator itr = edited_groups.find(next);
        if(!force){
            p_assert(itr!=edited_groups.end());
            edited_groups.erase(itr);
        }else{
            if(itr!=edited_groups.end()){
                edited_groups.erase(itr);
            }
        }
    }
}

void HsvIndirectlyScrollableTextView::remove_edited(VuGtkTextMark::iterator& self){
    set<VuGtkTextMark::iterator>::iterator itr = edited_groups.find(self);
    p_assert(itr!=edited_groups.end());
    if(itr!=edited_groups.end()){
        edited_groups.erase(itr);
    }
}

void make_cursor_on_onscreen(HsvIndirectlyScrollableTextView* bind){
    GtkTextIter iter; 
    gtk_text_buffer_get_iter_at_mark(bind->text_buffer,
      &iter, gtk_text_buffer_get_insert(bind->text_buffer));

    GdkRectangle abs_loc;
    gtk_text_view_get_iter_location(bind->text_view, &iter,
      &abs_loc);
  
    gint win_x, win_y;
    gtk_widget_translate_coordinates((GtkWidget*) bind->text_view, bind->scrolled_container,
      abs_loc.x, abs_loc.y, &win_x, &win_y);

    win_x += 20;

    gdouble width     = gtk_adjustment_get_page_size(bind->h_adj);
    gdouble l_offset  = win_x - gtk_adjustment_get_value(bind->h_adj);
    gdouble r_offset  = gtk_adjustment_get_value(bind->h_adj) + width - win_x;
    if(l_offset < width/8){
        win_x -= int(width/8);
        if(win_x<0) win_x = 0;
        gtk_adjustment_set_value(bind->h_adj, win_x);
    }else if(r_offset < width/8){
        win_x -= int(width-width/8);
        if(win_x<0) win_x = 0;
        gdouble upper = gtk_adjustment_get_upper(bind->h_adj) - width;
        if(win_x>upper) win_x = (int)upper;
        gtk_adjustment_set_value(bind->h_adj, win_x);
    }

    gdouble height    = gtk_adjustment_get_page_size(bind->v_adj);
    gdouble t_offset  = win_y - gtk_adjustment_get_value(bind->v_adj);
    gdouble b_offset  = gtk_adjustment_get_value(bind->v_adj) + height - win_y;
    if(t_offset < height/8){
        win_y -= int(height/8);
        if(win_y<0) win_y = 0;
        gtk_adjustment_set_value(bind->v_adj, win_y);
    }else if(b_offset < height/8){
        win_y -= int(height-height/8);
        if(win_y<0) win_y = 0;
        gdouble upper = gtk_adjustment_get_upper(bind->v_adj) - height;
        if(win_y>upper) win_y = (int)upper;
        gtk_adjustment_set_value(bind->v_adj, win_y);
    }
}

gboolean onclick_remove_widget_from_parent(GtkWidget* eventbox, GdkEvent* event, GtkWidget* widget){
    if(event->type==GDK_BUTTON_PRESS){
        GtkWidget* parent = gtk_widget_get_parent(widget); 
        gtk_container_remove(GTK_CONTAINER(parent), widget);
        return TRUE;
    }
    return FALSE;
}
void handler_destroyed(gpointer data, GClosure *closure){
    printf("handler destroyed.\n");
}

int cu_pipe[2];
Hsview* cu_executing = NULL;
void* cu_execute(void* _stat){
    cu_abort = 0;
    statement_t* stat = (statement_t*) _stat; 
    stat->execute();
    fflush(cu_out);
    delete stat;
    cu_delete_scopes();
    cu_executing = NULL;
    return NULL;
}
void cu_stop(){
    cu_abort = 1;
}

int no_syntax_highlight = 0;
int ready_for_execution = 0;
gboolean console_key_release(GtkWidget *widget, GdkEventKey *event, HsvIndirectlyScrollableTextView *bind){
    if(event->type==GDK_KEY_RELEASE){
        make_cursor_on_onscreen(bind);
        if(event->keyval == GDK_KEY_Alt_L || event->keyval == GDK_KEY_Alt_R){
            if(ready_for_execution){
                ready_for_execution = 0;
                GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
                GtkTextIter start, end; 
                gtk_text_buffer_get_start_iter(buffer, &start);
                gtk_text_buffer_get_end_iter(buffer, &end);

                gchar* text_entered = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
                int last_idx = int(strlen(text_entered))-1;
                g_assert(last_idx>=0 && text_entered[last_idx]=='\n');
                text_entered[last_idx] = 0;
                for(;last_idx;){
                    char c= text_entered[last_idx-1];    
                    if(c==' '||c=='\t'||c=='\n'){
                        text_entered[--last_idx] = 0;
                    }else{
                        break;
                    }
                }

                int parse_error = cuparse_string(text_entered, last_idx);

                GtkWidget* output_box = NULL;
                int j=0;
                if(!parse_error && !cu_executing){
                    VuGtkTextMark::buffer = buffer;
                    for(VuGtkTextMark::iterator 
                       itr  = bind->marks.begin()
                     ; itr != bind->marks.end()
                     ; ++itr){
                        if(itr->open){
                            p_assert(bind->open_lines.find(itr)
                              != bind->open_lines.end());
                        }else{
                            p_assert(bind->open_lines.find(itr)
                              == bind->open_lines.end());
                        }
                    }
                    no_syntax_highlight = 1;
                    gtk_text_buffer_set_text(buffer, "", 0);
                    no_syntax_highlight = 0;
                    bind->edited_groups.clear();
                    bind->open_lines.clear();
                    for(VuGtkTextMark::iterator 
                       itr  = bind->marks.begin()
                     ; itr != bind->marks.end()
                     ; ++itr){
                        itr->edited = 0;
                        itr->open   = 0;
                        if(itr->line==INT_MAX){
                            itr->open = 1;
                            bind->open_lines.insert(itr);
                        }
                    }
                    VuGtkTextMark::buffer = NULL;

                    for(int i=0, c; c=text_entered[i]; ++i){
                        switch(c){
                          case '\n': j=i+1;
                          case '\t':
                          case ' ' : continue;
                        }
                        break;
                    }
                    if(text_entered[j]){
                        output_box = (GtkWidget*) g_object_get_data(G_OBJECT(widget), "output-box");
                    }
                }
                if(output_box){
                    GtkWidget* record = gtk_hbox_new(FALSE, 0);

                    GtkWidget* delete_box = gtk_event_box_new(); { 
                        GtkWidget* img_close = gtk_image_new_from_stock(GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU);
                        gtk_widget_set_tooltip_text(img_close, "delete");
                        gtk_event_box_set_above_child(GTK_EVENT_BOX(delete_box), FALSE);
                        gtk_event_box_set_visible_window(GTK_EVENT_BOX(delete_box), FALSE);
                        gtk_container_add(GTK_CONTAINER(delete_box), img_close);
                        g_signal_connect(G_OBJECT(delete_box), "button_press_event", 
                          G_CALLBACK(onclick_remove_widget_from_parent), record);
                    }
                    GtkWidget* img_aligned = gtk_alignment_new(0, 0, 0, 0);
                    gtk_container_add(GTK_CONTAINER(img_aligned), delete_box);

                    GtkWidget* txt_aligned = gtk_alignment_new(0, 0, 0, 0);
                    {
                        GtkWidget* label = gtk_label_new(text_entered+j);
                        PangoAttribute* mono = pango_attr_family_new("Monospace");
                        mono->start_index = 0;
                        mono->end_index   = strlen(text_entered+j);
                        PangoAttrList* markup = pango_attr_list_new();
                        pango_attr_list_insert(markup, mono);
                        gtk_label_set_attributes(GTK_LABEL(label), markup);
                        pango_attr_list_unref(markup);
                        gtk_label_set_selectable(GTK_LABEL(label), TRUE);
                        gtk_container_add(GTK_CONTAINER(txt_aligned), label);
                        GdkColor color = {0, 100<<8, 100<<8, 100<<8};
                        gtk_widget_modify_fg(label, GTK_STATE_NORMAL, &color);
                    }

                    gtk_box_pack_start(GTK_BOX(record), img_aligned, FALSE, FALSE, 0); 
                    GtkWidget* vbox = gtk_vbox_new(FALSE, 0);
                    gtk_box_pack_start(GTK_BOX(vbox), txt_aligned, FALSE, FALSE, 0); 
                    gtk_box_pack_start(GTK_BOX(record), vbox, FALSE, FALSE, 0); 

                    gtk_box_pack_start(GTK_BOX(output_box), record, FALSE, FALSE, 0); 

                    gtk_widget_show_all(record);

                    output_box = vbox;
                }

                if(!parse_error){
                    if(first_statement){
                        if(!cu_executing && !app.execution_bind){
                            cu_executing = app.current_view;
                            app.execution_bind = bind;
                            bind->output_box = output_box;
                            pthread_t thread;
                            if(pthread_create(&thread, NULL, cu_execute, first_statement)!=0){
                                puts("pthread_create failed.");
                                exit(1);
                            }
                            pthread_detach(thread);
                            first_statement = NULL;
                            gtk_widget_hide(bind->occupied);
                            gtk_widget_set_child_visible(bind->stop_btn, TRUE);
                        }else{
                            gtk_label_set_markup(GTK_LABEL(bind->occupied),
                              "<span foreground=\"red\" background=\"white\">Sorry, another CU program is still running.</span>"); 
                            gtk_widget_set_child_visible(bind->occupied, TRUE);
                            gtk_widget_show(bind->occupied);
                            delete first_statement;
                            first_statement = NULL;
                        }
                    }else{
                        printf("nothing compiled\n");
                    }
                }else{
                    if(cu_err[0]){
                        gtk_label_set_text(GTK_LABEL(bind->occupied), cu_err);
                        PangoAttribute* red = pango_attr_foreground_new(255<<8,0,0);
                        red->start_index = 0;
                        red->end_index   = strlen(cu_err);
                        PangoAttrList* markup = pango_attr_list_new();
                        pango_attr_list_insert(markup, red);
                        gtk_label_set_attributes(GTK_LABEL(bind->occupied), markup);
                        pango_attr_list_unref(markup);
                        gtk_widget_set_child_visible(bind->occupied, TRUE);
                        gtk_widget_show(bind->occupied);
                    }

                    if(!err_symbol_end){
                        err_symbol_begin = cu_symbol_begin;
                        err_symbol_end   = cu_symbol_end;
                    }
                    GtkTextIter begin, end;
                    gtk_text_buffer_get_iter_at_offset(buffer, &begin, err_symbol_begin);
                    gtk_text_buffer_get_iter_at_offset(buffer, &end  , err_symbol_end  );
                    gtk_text_buffer_apply_tag(buffer, bind->syntax_err, &begin, &end);
                }

                g_free(text_entered);
                return TRUE;
            }

        }else if(event->keyval != GDK_KEY_Return){
            ready_for_execution = 0;
        }else{
            if(bind->edited_groups.size()){

            }

        }
    }
    return FALSE;
}

void set_console_font(GtkFontButton* font_btn, GtkTextView* view);
gboolean console_key_press(GtkWidget *widget, GdkEventKey *event, HsvIndirectlyScrollableTextView *bind){
    if(!bind->flags){
        bind->flags = 1;
        set_console_font(app.code_font_button, GTK_TEXT_VIEW(widget));
    }
    python_text_view = widget;
    if(event->type==GDK_KEY_PRESS){
        make_cursor_on_onscreen(bind);
        if(event->state & GDK_MOD1_MASK && event->keyval == GDK_KEY_Return){
            if(0){
                GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
                GtkTextIter end;
                gtk_text_buffer_get_end_iter(buffer, &end);
                gtk_text_buffer_place_cursor(buffer, &end);
                ready_for_execution++;
            }else{
                GtkTextBuffer* buffer = bind->text_buffer;
                GtkTextIter end, end2;
                gtk_text_buffer_get_end_iter(buffer, &end); 
                gtk_text_buffer_get_iter_at_line_offset(buffer, &end2, 
                  gtk_text_buffer_get_line_count(buffer), 0);
                if(!gtk_text_iter_equal(&end, &end2)){
                    gtk_text_buffer_get_iter_at_mark(buffer, &end2, gtk_text_buffer_get_insert(buffer));
                    GtkTextMark* mark = gtk_text_buffer_create_mark(buffer, NULL, &end2, TRUE);
                    gtk_text_buffer_insert(buffer, &end, "\n", 1);
                    gtk_text_buffer_get_iter_at_mark(buffer, &end2, mark);
                    gtk_text_buffer_place_cursor(buffer, &end2);
                    gtk_text_buffer_delete_mark(buffer, mark);
                }else if(gtk_text_buffer_get_has_selection(buffer)){
                    gtk_text_buffer_get_iter_at_mark(buffer, &end2, gtk_text_buffer_get_insert(buffer));
                    gtk_text_buffer_place_cursor(buffer, &end2);
                }
                ready_for_execution++;
                return TRUE;
            }
        }else{
            ready_for_execution = 0;
        }
    }
    return FALSE;
}

HsvIndirectlyScrollableTextView* syntax_highlight_bind = NULL;

void console_insert_text(GtkTextBuffer* buffer, GtkTextIter* location, gchar* text, gint len, HsvIndirectlyScrollableTextView* bind){
    bind->insert_begin = gtk_text_iter_get_line(location);
    bind->quiet = 0;
    syntax_highlight_bind = bind;
}
void console_insert_text_after(GtkTextBuffer* buffer, GtkTextIter* location, gchar* text, gint len, HsvIndirectlyScrollableTextView* bind){
    VuGtkTextMark::buffer = buffer;
    VuGtkTextMark::count_update=0;
    GtkTextIter iter;
    int insert_end = gtk_text_iter_get_line(location);
    if(insert_end > bind->insert_begin){
        VuGtkTextMark::global_stamp++;
    }
    static VuGtkTextMark::iterator cached_itr;
    static GtkTextBuffer*          cached_for;
    static int                     cached_stp;
    VuGtkTextMark::iterator begin;
    if(cached_for == buffer &&
       cached_stp == VuGtkTextMark::global_stamp &&
       bind->insert_begin == cached_itr->get_line()
      ){
        begin = cached_itr;
    }else{
        VuGtkTextMark mark;
        mark.line = bind->insert_begin;
        if(bind->marks.size()){
            begin = bind->marks.find(mark);
        }else{
            {
                VuGtkTextMark mark;
                mark.mark   = NULL;
                mark.stmp   = INT_MAX;
                mark.line   = INT_MAX;
                mark.open   = 1;
                bind->marks.insert(mark); 
                bind->open_lines.insert(bind->marks.begin());
            }
            g_assert(bind->insert_begin==0);
            gtk_text_buffer_get_start_iter(buffer, &iter);
            mark.mark = gtk_text_buffer_create_mark(buffer, NULL, &iter, TRUE); 
            g_object_ref(mark.mark);
            bind->marks.insert(mark);
            begin = bind->marks.begin();
            {
                VuGtkTextMark mark;
                mark.mark   = NULL;
                mark.stmp   = INT_MAX;
                mark.line   = -1;
                mark.open   = 0;
                bind->marks.insert(mark); 
            }
        }
        cached_for = buffer;
        cached_stp = VuGtkTextMark::global_stamp;
        cached_itr = begin;
    }
    p_assert(begin!=bind->marks.end());

    bind->add_edited(begin);

    int ends_with_new_line = (len && text[len-1]=='\n');
    int inserted_last_line = (ends_with_new_line && 
                              (insert_end+1) == gtk_text_buffer_get_line_count(buffer));
    int b = bind->marks.size();
    for(int i=bind->insert_begin+1; i<=insert_end; ++i){
        VuGtkTextMark mark;
        gtk_text_buffer_get_iter_at_line_offset(buffer, &iter, i, 0);
        mark.mark   = gtk_text_buffer_create_mark(buffer, NULL, &iter, TRUE);
        mark.line   = i;
        mark.edited = 1;
        if(i==insert_end && inserted_last_line){
            mark.edited = 0;
        }
        g_object_ref(mark.mark);
        bind->marks.insert(begin, mark);
        ++begin;
        p_assert(begin->mark==mark.mark);
    }
    p_assert((bind->marks.size()-b)==(insert_end-bind->insert_begin));
    if(insert_end > bind->insert_begin){
        if(!inserted_last_line){
            bind->add_edited(begin, 1);
        }else{
        }
    }

    VuGtkTextMark::buffer = NULL;
}
void console_delete_text(GtkTextBuffer* buffer, GtkTextIter* start, GtkTextIter* end, HsvIndirectlyScrollableTextView* bind){
    bind->quiet = 0;
    syntax_highlight_bind = bind;
    VuGtkTextMark::buffer = buffer;

    bind->delete_begin = gtk_text_iter_get_line(start);
    int delete_end = gtk_text_iter_get_line(end);
    static VuGtkTextMark::iterator cached_itr;
    static GtkTextBuffer*          cached_for;
    static int                     cached_stp;

    VuGtkTextMark::iterator begin;
    VuGtkTextMark mark;
    mark.line = bind->delete_begin;
    if(cached_for == buffer &&
       cached_stp == VuGtkTextMark::global_stamp &&
       bind->delete_begin == cached_itr->get_line()
      ){
        begin = cached_itr;
    }else{
        begin = bind->marks.find(mark);
        cached_for = buffer;
        cached_stp = VuGtkTextMark::global_stamp;
        cached_itr = begin;
    }
    p_assert(begin!=bind->marks.end());
    bind->add_edited(begin);

    if(delete_end > bind->delete_begin){
        VuGtkTextMark::iterator begin_backup = begin;
        VuGtkTextMark::iterator itr, end;
        ++begin;
        p_assert(begin!=bind->marks.end());

        mark.line = delete_end;
        end   = bind->marks.find(mark);
        p_assert(end!=bind->marks.end());
        ++end;

        int cnt=0;
        int previous_edited = 1;
        for(itr=begin; itr!=end; ++itr){
            cnt++;
            p_assert(itr->mark);
            p_assert(itr->stmp);
            itr->get_line();
            gtk_text_buffer_delete_mark(buffer, itr->mark); 
            g_object_unref(itr->mark);
            const_cast<VuGtkTextMark&>(*itr).mark = NULL;
            p_assert(itr->mark==NULL);
            if(itr->open){
                bind->open_lines.erase(itr);
            }
            if(!previous_edited && itr->edited){
                bind->remove_edited(itr);
            }
            previous_edited = itr->edited;
            bind->overall_para -= - itr->closing_para + itr->opening_para;;
            bind->overall_brak -= - itr->closing_brak + itr->opening_brak;;
            bind->overall_brac -= - itr->closing_brac + itr->opening_brac;;
            bind->overall_cmmt -= - itr->closing_cmmt + itr->opening_cmmt;;
        }
        p_assert((cnt)==(delete_end-bind->delete_begin));

        bind->marks.erase(begin, end);

        VuGtkTextMark::global_stamp++;
        cached_stp = VuGtkTextMark::global_stamp;

        bind->add_edited(begin_backup, 1);
    }

    VuGtkTextMark::buffer = NULL;
}

void console_syntax_highlight_line_para(
    int opening_para,
    int opening_brak,
    int opening_brac,
    int opening_cmmt,
    VuGtkTextMark::iterator& mark_itr,
    VuGtkTextMark::iterator& next,
    gchar* line,
    int    len ,
    HsvIndirectlyScrollableTextView* bind,
    int closing_para,
    int closing_brak,
    int closing_brac,
    int closing_cmmt
){
    const VuGtkTextMark& mark = *mark_itr;
    if(opening_para<0) opening_para=0;
    if(opening_brak<0) opening_brak=0;
    if(opening_brac<0) opening_brac=0;
    if(opening_cmmt<0) opening_cmmt=0;
    if(mark_itr->closing_para==0) opening_para=0;
    if(mark_itr->closing_brak==0) opening_brak=0;
    if(mark_itr->closing_brac==0) opening_brac=0;
    if(mark_itr->closing_cmmt==0) opening_cmmt=0;

    closing_para = (closing_para<0?-closing_para:0);
    closing_brak = (closing_brak<0?-closing_brak:0);
    closing_brac = (closing_brac<0?-closing_brac:0);
    closing_cmmt = (closing_cmmt<0?-closing_cmmt:0);
    if(mark_itr->opening_para==0) closing_para=0;
    if(mark_itr->opening_brak==0) closing_brak=0;
    if(mark_itr->opening_brac==0) closing_brac=0;
    if(mark_itr->opening_cmmt==0) closing_cmmt=0;

    int trait = opening_para<<28^opening_brak<<24^opening_brac<<20^opening_cmmt<<16^
                closing_para<<12^closing_brak<< 8^closing_brac<< 4^closing_cmmt<< 0^
                opening_para>> 4^opening_brak>> 8^opening_brac>>12^opening_cmmt>>16^
                closing_para>>20^closing_brak>>24^closing_brac>>28^-1;

    if(line || trait!=mark_itr->trait){
        mark_itr->trait = trait;
    }else{
        return;
    }

    GtkTextBuffer* buffer = bind->text_buffer;
    GtkTextIter start, end;

    int free_line = !line;
    if(!line){
        gtk_text_buffer_get_iter_at_mark(buffer, &start, mark.mark);
        gtk_text_buffer_get_iter_at_mark(buffer, &end, next->mark);
        line = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
        len=strlen(line);
        line[len-1]=0;
        gtk_text_buffer_remove_tag(buffer, bind->para_error, &start, &end);
    }

    static vector<pair<short,short> > para;
    para.clear();
    culex_begin(line, len-1);
    for(int t;t=culex();){
        int error = 0;
        switch(t){
          case '{': para.push_back(make_pair<short,short>(cu_symbol_begin,'{')); break;
          case '[': para.push_back(make_pair<short,short>(cu_symbol_begin,'[')); break;
          case '(': para.push_back(make_pair<short,short>(cu_symbol_begin,'(')); break;
          case '}': if(para.size() && para.back().second=='{') para.pop_back();
                      else if(opening_brac) --opening_brac;    else error = 1;   break;
          case ']': if(para.size() && para.back().second=='[') para.pop_back();
                      else if(opening_brak) --opening_brak;    else error = 1;   break;
          case ')': if(para.size() && para.back().second=='(') para.pop_back();
                      else if(opening_para) --opening_para;    else error = 1;   break;
        }
        if(error){
            gtk_text_buffer_get_iter_at_line_index(buffer, &start, mark.get_line(), cu_symbol_begin);
            gtk_text_buffer_get_iter_at_line_index(buffer, &end  , mark.get_line(), cu_symbol_begin+1);
            gtk_text_buffer_apply_tag(buffer, bind->para_error, &start, &end);
        }
    }
    while(para.size()){
        int error = 0;
        switch(para.back().second){
          case '{': if(closing_brac) --closing_brac; else error=1; break;
          case '[': if(closing_brak) --closing_brak; else error=1; break;
          case '(': if(closing_para) --closing_para; else error=1; break;
        }
        if(error){
            int loc = para.back().first;
            gtk_text_buffer_get_iter_at_line_index(buffer, &start, mark.get_line(), loc);
            gtk_text_buffer_get_iter_at_line_index(buffer, &end  , mark.get_line(), loc+1);
            gtk_text_buffer_apply_tag(buffer, bind->para_error, &start, &end);
        }
        para.pop_back();
    }
    culex_end();

    if(free_line) g_free(line);
}

void console_syntax_highlight_line(
    VuGtkTextMark::iterator& mark_itr,
    VuGtkTextMark::iterator& next,
    HsvIndirectlyScrollableTextView* bind,
    int* i=NULL
){
    GtkTextBuffer* buffer = bind->text_buffer;
    const VuGtkTextMark& mark = *mark_itr;

    GtkTextIter start, end;
    gtk_text_buffer_get_iter_at_mark(buffer, &start, mark.mark);
    gtk_text_buffer_get_iter_at_mark(buffer, &end, next->mark);

    gtk_text_buffer_remove_all_tags(buffer, &start, &end);
    gchar* line = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

    int len=strlen(line);
    p_assert(len>0);
    p_assert(line[len-1]=='\n');
    line[len-1] = 0;

    int closing_para=0, opening_para=0;
    int closing_brak=0, opening_brak=0;
    int closing_brac=0, opening_brac=0;
    int closing_cmmt=0, opening_cmmt=0;
    culex_begin(line, len-1);
    for(int t;t=culex();){
        switch(t){
          case '{': ++opening_brac; break;
          case '[': ++opening_brak; break;
          case '(': ++opening_para; break;
          case '}': if(opening_brac) --opening_brac; else ++closing_brac; break;
          case ']': if(opening_brak) --opening_brak; else ++closing_brak; break;
          case ')': if(opening_para) --opening_para; else ++closing_para; break;
          case K_s: ++opening_cmmt; break;
          case K_e: if(opening_cmmt) --opening_cmmt; else ++closing_cmmt; break;
        }
        if(t==K_comment){
            gtk_text_buffer_get_iter_at_line_index(buffer, &start, mark.get_line(), cu_symbol_begin);
            gtk_text_buffer_get_iter_at_line_index(buffer, &end  , mark.get_line(), cu_symbol_end);
            gtk_text_buffer_apply_tag(buffer, bind->purple, &start, &end);
        }
        if(t>K_keyword_start && t<K_keyword_end){
            gtk_text_buffer_get_iter_at_line_index(buffer, &start, mark.get_line(), cu_symbol_begin);
            gtk_text_buffer_get_iter_at_line_index(buffer, &end  , mark.get_line(), cu_symbol_end);
            gtk_text_buffer_apply_tag(buffer, bind->blue, &start, &end);
        }
        if(t=='"' || t>K_reserved_start && t<K_reserved_end){
            gtk_text_buffer_get_iter_at_line_index(buffer, &start, mark.get_line(), cu_symbol_begin);
            gtk_text_buffer_get_iter_at_line_index(buffer, &end  , mark.get_line(), cu_symbol_end);
            gtk_text_buffer_apply_tag(buffer, bind->red, &start, &end);
        }
        if(t==T_str){
            gtk_text_buffer_get_iter_at_line_index(buffer, &start, mark.get_line(), cu_symbol_begin);
            gtk_text_buffer_get_iter_at_line_index(buffer, &end  , mark.get_line(), cu_symbol_end);
            gtk_text_buffer_apply_tag(buffer, bind->brown, &start, &end);
        }
        if(t==T_regex){
            gtk_text_buffer_get_iter_at_line_index(buffer, &start, mark.get_line(), cu_symbol_begin);
            gtk_text_buffer_get_iter_at_line_index(buffer, &end  , mark.get_line(), cu_symbol_begin+1);
            gtk_text_buffer_apply_tag(buffer, bind->blue, &start, &end);
            const char* highlight_re(char* str, GtkTextBuffer* tb, int line, int offset);
            line[cu_symbol_end-1] = 0;
            highlight_re(line+cu_symbol_begin+1, buffer, mark.get_line(), cu_symbol_begin+1);
            line[cu_symbol_end-1] = '/';
            gtk_text_buffer_get_iter_at_line_index(buffer, &start, mark.get_line(), cu_symbol_end-1);
            gtk_text_buffer_get_iter_at_line_index(buffer, &end  , mark.get_line(), cu_symbol_end);
            gtk_text_buffer_apply_tag(buffer, bind->blue, &start, &end);
        }
    }
    culex_end();

    int is_open = !!( closing_para|opening_para
                     |closing_brak|opening_brak
                     |closing_brac|opening_brac
                     |closing_cmmt|opening_cmmt);
    if(is_open){
        if(!mark.open){
            mark.open = 1;
            bind->open_lines.insert(mark_itr);
        }
    }else{
        if(mark.open){
            mark.open = 0;
            bind->open_lines.erase(mark_itr);
        }
    }
    if(!is_open){
        console_syntax_highlight_line_para(0, 0, 0, 0,
          mark_itr, next, line, len, bind, 0, 0, 0, 0); 
    }else{
        mark_itr->trait = 0;
    }

    g_free(line);

    mark.closing_brac = closing_brac;   mark.opening_brac = opening_brac;
    mark.closing_brak = closing_brak;   mark.opening_brak = opening_brak;
    mark.closing_para = closing_para;   mark.opening_para = opening_para;
    mark.closing_cmmt = closing_cmmt;   mark.opening_cmmt = opening_cmmt;
}

gboolean console_refresh(void*){
    if(app.execution_bind){
        HsvIndirectlyScrollableTextView* bind = app.execution_bind;
        if(app.execution_bind_deleted){
            bind = NULL; 
        }
        static char buffer[4097];
        static GtkWidget* incomplete_line=NULL;
        while(1){
            int bytes = read(cu_pipe[0], buffer, 4096);
            buffer[bytes]=0;
            if(!bind) continue;
            if(bytes<=0) break;
            int len=0, start=0, end=0;
            if(buffer[bytes-1]=='\n'){
                end = bytes-1;
            }else{
                char* n = strrchr(buffer, '\n');
                if(n) end = n - buffer;
            }
            for(; end<bytes; ++end){
                char c;
                len++;
                if((c=buffer[end])=='\n'){
                    buffer[end]=0;
                    if(incomplete_line){
                        string str = gtk_label_get_text(GTK_LABEL(incomplete_line));
                        str.append(buffer+start);
                        gtk_label_set_text(GTK_LABEL(incomplete_line), str.c_str()); 
                        if(c=='\n'){
                            incomplete_line=NULL;
                        }
                    }else{
                        GtkWidget* label = gtk_label_new(buffer+start);
                        gtk_label_set_selectable(GTK_LABEL(label), TRUE);
                        GtkWidget* align = gtk_alignment_new(0, 0, 0, 0);
                        gtk_container_add(GTK_CONTAINER(align), label);
                        gtk_widget_show_all(align);
                        gtk_box_pack_start(GTK_BOX(app.execution_bind->output_box), align, FALSE, FALSE, 0);
                    }
                    if(c=='\n'){
                        start = end+1;
                        len = 0;
                    }else{
                        buffer[end] = c;
                        start = end;
                        len = 1;
                    }
                }
            }
            if(start<bytes){
                incomplete_line = gtk_label_new(buffer+start);
                gtk_label_set_selectable(GTK_LABEL(incomplete_line), TRUE);
                GtkWidget* align = gtk_alignment_new(0, 0, 0, 0);
                gtk_container_add(GTK_CONTAINER(align), incomplete_line);
                gtk_widget_show_all(align);
                gtk_box_pack_start(GTK_BOX(app.execution_bind->output_box), align, FALSE, FALSE, 0);
            }
        }
        if(!cu_executing){
            gtk_widget_set_child_visible(bind->stop_btn, FALSE);
            gtk_widget_set_child_visible(bind->occupied, FALSE);
            gtk_widget_hide(bind->occupied);
            app.execution_bind = NULL;
            app.execution_bind_deleted = 0;
            incomplete_line = NULL;
        }
    }

    HsvIndirectlyScrollableTextView* bind = syntax_highlight_bind;
    if(!bind) return TRUE;
    if(no_syntax_highlight) return TRUE;
    if(!bind->quiet){
        bind->quiet = 2;
        return TRUE;
    }else{
        if(--bind->quiet){
            return TRUE;
        }
    }
    p_assert(bind->quiet==0);

    GtkTextBuffer* buffer = bind->text_buffer;
    if(gtk_text_buffer_get_has_selection(buffer)){
        bind->quiet = 1;
        return TRUE;
    }
    
    GtkTextIter end, end2;
    gtk_text_buffer_get_end_iter(buffer, &end); 
    gtk_text_buffer_get_iter_at_line_offset(buffer, &end2, 
      gtk_text_buffer_get_line_count(buffer), 0);
    if(!gtk_text_iter_equal(&end, &end2)){
        gtk_text_buffer_get_iter_at_mark(buffer, &end2, gtk_text_buffer_get_insert(buffer));
        GtkTextMark* mark = gtk_text_buffer_create_mark(buffer, NULL, &end2, TRUE);
        gtk_text_buffer_insert(buffer, &end, "\n", 1);
        gtk_text_buffer_get_iter_at_mark(buffer, &end2, mark);
        gtk_text_buffer_place_cursor(buffer, &end2);
        gtk_text_buffer_delete_mark(buffer, mark);
    }else{
        if(gtk_text_buffer_get_line_count(buffer)==1){
            bind->quiet = -1;
            return TRUE;
        }
    }

    VuGtkTextMark::buffer = buffer;

    int overall_para_prior=0, overall_para_post=0;
    int overall_brak_prior=0, overall_brak_post=0;
    int overall_brac_prior=0, overall_brac_post=0;
    int overall_cmmt_prior=0, overall_cmmt_post=0;

    for(set<VuGtkTextMark::iterator,MarkIterComp>::iterator 
        itr  = bind->edited_groups.begin()
    ;   itr != bind->edited_groups.end()
    ; ++itr
    ){
        VuGtkTextMark::iterator mark_itr = *itr;

        int i=0;
        VuGtkTextMark::iterator next = mark_itr;
        for(; mark_itr->edited; mark_itr = next){
            overall_para_prior += - mark_itr->closing_para + mark_itr->opening_para;;
            overall_brak_prior += - mark_itr->closing_brak + mark_itr->opening_brak;;
            overall_brac_prior += - mark_itr->closing_brac + mark_itr->opening_brac;;
            overall_cmmt_prior += - mark_itr->closing_cmmt + mark_itr->opening_cmmt;;

            ++next;
            mark_itr->edited = 0;
            console_syntax_highlight_line(mark_itr, next, bind, &i);
            ++i;

            overall_para_post += - mark_itr->closing_para + mark_itr->opening_para;;
            overall_brak_post += - mark_itr->closing_brak + mark_itr->opening_brak;;
            overall_brac_post += - mark_itr->closing_brac + mark_itr->opening_brac;;
            overall_cmmt_post += - mark_itr->closing_cmmt + mark_itr->opening_cmmt;;
        }
    }
    bind->edited_groups.clear();

    bind->overall_para += overall_para_post - overall_para_prior;
    bind->overall_brak += overall_brak_post - overall_brak_prior;
    bind->overall_brac += overall_brac_post - overall_brac_prior;
    bind->overall_cmmt += overall_cmmt_post - overall_cmmt_prior;

    int overall_para = 0;
    int overall_brak = 0;
    int overall_brac = 0;
    int overall_cmmt = 0;
    for(set<VuGtkTextMark::iterator,MarkIterComp>::iterator
        open_itr = bind->open_lines.begin()
    ; (*open_itr)->line < INT_MAX
    ; ++open_itr
    ){
        const VuGtkTextMark::iterator& open_line = *open_itr;
        p_assert(!open_line->edited);

        int overall_para_post = overall_para - open_line->closing_para + open_line->opening_para;;
        int overall_brak_post = overall_brak - open_line->closing_brak + open_line->opening_brak;;
        int overall_brac_post = overall_brac - open_line->closing_brac + open_line->opening_brac;;
        int overall_cmmt_post = overall_cmmt - open_line->closing_cmmt + open_line->opening_cmmt;;

        VuGtkTextMark::iterator mark_itr = *open_itr;
        VuGtkTextMark::iterator     next = mark_itr; 
                                  ++next;
        console_syntax_highlight_line_para(
          overall_para, overall_brak, overall_brac, overall_cmmt,
          mark_itr, next, NULL, 0, bind, 
          bind->overall_para - overall_para_post,
          bind->overall_brak - overall_brak_post,
          bind->overall_brac - overall_brac_post,
          bind->overall_cmmt - overall_cmmt_post
        );

        overall_para = overall_para_post;
        overall_brak = overall_brak_post;
        overall_brac = overall_brac_post;
        overall_cmmt = overall_cmmt_post;
    }

    VuGtkTextMark::buffer = NULL;

    bind->quiet = -1;
    return TRUE;
}

void set_console_font(GtkFontButton* font_btn, GtkTextView* view){
    PangoFontDescription* font_desc = pango_font_description_from_string(gtk_font_button_get_font_name(font_btn));
    gtk_widget_modify_font(GTK_WIDGET(view), font_desc);
    pango_font_description_free(font_desc);

    PangoContext* context = gtk_widget_get_pango_context(GTK_WIDGET(view));
    PangoLayout* layout = pango_layout_new(context);
    pango_layout_set_text(layout, "    ", 4);
    int width, height;
    pango_layout_get_size(layout, &width, &height);
    g_object_unref(layout); 
    PangoTabArray* ta = pango_tab_array_new(1, FALSE);
    pango_tab_array_set_tab(ta, 0,  PANGO_TAB_LEFT,  width);
    gtk_text_view_set_tabs(GTK_TEXT_VIEW(view), ta);
    pango_tab_array_free(ta);
}

void font_set_handler_disconnect(void*, gulong handler){
    if(app.code_font_button){
        g_signal_handler_disconnect(app.code_font_button, handler);    
    }
}
void console_text_view_destroyed(void*, HsvIndirectlyScrollableTextView* bind){
    font_set_handler_disconnect(NULL, bind->font_set_handler);
    if(syntax_highlight_bind==bind) syntax_highlight_bind=NULL;
    if(app.execution_bind == bind){
        app.execution_bind_deleted = 1;
    }
    delete bind;
}

GtkWidget* new_console(){

    GtkWidget* vbox = gtk_vbox_new(FALSE, 0);

    GtkWidget* output_box = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), output_box, FALSE, FALSE, 0);

    GtkWidget* text_view = gtk_text_view_new();

    GtkWidget* prompt_label = gtk_image_new_from_stock(GTK_STOCK_GO_FORWARD, GTK_ICON_SIZE_MENU);
    gtk_text_view_add_child_in_window(GTK_TEXT_VIEW(text_view), prompt_label, GTK_TEXT_WINDOW_LEFT, 0, 0);
    gtk_text_view_set_border_window_size(GTK_TEXT_VIEW(text_view), GTK_TEXT_WINDOW_LEFT, 20);

    GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    gtk_text_buffer_set_text(buffer, "", -1);

    GtkWidget* img_info = gtk_image_new_from_stock(GTK_STOCK_DIALOG_INFO, GTK_ICON_SIZE_MENU);
    GtkWidget* img_align = gtk_alignment_new(0, 0, 0, 0);
    gtk_container_add(GTK_CONTAINER(img_align), img_info);
    GtkWidget* welcome_box = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(welcome_box), img_align, FALSE, FALSE, 3); 
    GtkWidget* welcome_msg = gtk_label_new("Press Alt+Enter to evaluate.");
    GtkWidget* wel_align = gtk_alignment_new(0, 0, 0, 0);
    gtk_container_add(GTK_CONTAINER(wel_align), welcome_msg);
    gtk_box_pack_start_defaults(GTK_BOX(welcome_box), wel_align);
    
    gtk_box_pack_start(GTK_BOX(output_box), welcome_box, FALSE, FALSE, 0);

    HsvIndirectlyScrollableTextView* bind = new HsvIndirectlyScrollableTextView;

    GdkColor red    = {0, 255<<8, 0, 0};
    GdkColor blue   = {0, 0, 0, 216<<8};
    GdkColor purple = {0, 156<<8, 0, 143<<8};
    GdkColor grey = {0, 30000, 30000, 30000};
    GdkColor green= {0, 0, 65535, 0};
    GdkColor brown =  {0, 100<<8, 30<<8, 10<<8};
    GdkColor purple2= {0, 200<<8, 0, 216<<8};
    bind->red    = gtk_text_buffer_create_tag(buffer, "red", "foreground-gdk", &red, "foreground-set", TRUE, NULL);
    bind->blue   = gtk_text_buffer_create_tag(buffer, "blue", "foreground-gdk", &blue, "foreground-set", TRUE, NULL);
                   gtk_text_buffer_create_tag(buffer, "purple", "foreground-gdk", &purple, "foreground-set", TRUE, NULL);
                   gtk_text_buffer_create_tag(buffer, "grey", "foreground-gdk", &grey, "foreground-set", TRUE, NULL);
                   gtk_text_buffer_create_tag(buffer, "green", "foreground-gdk", &green, "foreground-set", TRUE, NULL);
    bind->brown  = gtk_text_buffer_create_tag(buffer, "brown", "foreground-gdk", &brown, "foreground-set", TRUE, NULL);
    bind->purple = gtk_text_buffer_create_tag(buffer, "purple2", "foreground-gdk", &purple2, "foreground-set", TRUE, NULL);
    bind->para_error = gtk_text_buffer_create_tag(buffer, "para_error", "foreground-gdk", &red, "foreground-set", TRUE, NULL);
    GdkColor synerr = {0, 255<<8, 200<<8, 200<<8};
    bind->syntax_err = gtk_text_buffer_create_tag(buffer, "synerr", "background-gdk", &synerr, "background-set", TRUE, NULL);
    
    g_signal_connect(G_OBJECT(text_view), "key_press_event",   G_CALLBACK(console_key_press      ),   bind);
    g_signal_connect(G_OBJECT(text_view), "key_release_event", G_CALLBACK(console_key_release), bind);
    if(app.code_font_button){
        set_console_font(app.code_font_button, GTK_TEXT_VIEW(text_view));
        gulong handler = g_signal_connect(G_OBJECT(app.code_font_button), "font-set", G_CALLBACK(set_console_font), text_view);
        bind->font_set_handler = handler;
    }
    g_signal_connect(G_OBJECT(text_view), "destroy", G_CALLBACK(console_text_view_destroyed), bind);

    g_signal_connect      (G_OBJECT(buffer), "insert-text",  G_CALLBACK(console_insert_text      ), bind);
    g_signal_connect_after(G_OBJECT(buffer), "insert-text",  G_CALLBACK(console_insert_text_after), bind);
    g_signal_connect      (G_OBJECT(buffer), "delete-range", G_CALLBACK(console_delete_text      ), bind);

    GtkWidget* scrolled = gtk_scrolled_window_new(NULL, NULL);
    bind->h_adj = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(scrolled)); 
    bind->v_adj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scrolled)); 
    bind->text_view = GTK_TEXT_VIEW(text_view);
    bind->text_buffer = buffer;
    bind->scrolled_container = vbox;
    bind->flags = 0;

    gtk_box_pack_start(GTK_BOX(vbox), text_view, FALSE, FALSE, 0);

    bind->occupied = gtk_label_new(NULL);
    gtk_widget_set_child_visible(bind->occupied, FALSE);
    gtk_box_pack_start(GTK_BOX(vbox), bind->occupied, FALSE, FALSE, 3);

    GtkWidget* stop_btn = gtk_button_new_from_stock(GTK_STOCK_STOP);
    g_signal_connect(G_OBJECT(stop_btn), "clicked", G_CALLBACK(cu_stop), NULL);
    GtkWidget* btn_algn = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(btn_algn), stop_btn, 0, 0, 20);
    gtk_box_pack_start(GTK_BOX(vbox), btn_algn, FALSE, FALSE, 3);
    gtk_widget_set_child_visible(stop_btn, FALSE);
    bind->stop_btn = stop_btn;

    g_object_set_data(G_OBJECT(text_view), "output-box", output_box);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled), vbox);


    return scrolled;
}


class HsvColorButton{
    GtkWidget* da;
    GdkColor*  color;

    static const int width  = 30;
    static const int height = 16;

static 
    gboolean button_pressed(GtkWidget* widget, GdkEventButton* event, HsvColorButton* self){
        g_assert( widget == self->da );
        if(event->type==GDK_2BUTTON_PRESS && event->button==1){
            static GtkWidget* dialog = NULL;
            if(!dialog) dialog = gtk_color_selection_dialog_new("Choose a color");
            gtk_color_selection_set_current_color(GTK_COLOR_SELECTION(GTK_COLOR_SELECTION_DIALOG(dialog)->colorsel), self->color);
            int reponse = gtk_dialog_run(GTK_DIALOG(dialog));
            if(reponse==GTK_RESPONSE_OK){
                gtk_color_selection_get_current_color(GTK_COLOR_SELECTION(GTK_COLOR_SELECTION_DIALOG(dialog)->colorsel), self->color);
                gtk_widget_queue_draw(self->da);
            }
            gtk_widget_hide(dialog);
        }
        return TRUE;
    }
static 
    gboolean exposed(GtkWidget* widget, GdkEventExpose* event, HsvColorButton* self){
        g_assert( widget == self->da );

        GdkWindow* window = self->da->window;
        cairo_t*   cr     = gdk_cairo_create(window); 

            gdk_cairo_set_source_color(cr, self->color);
            cairo_rectangle(cr, 0, 0, widget->allocation.width, widget->allocation.height);
            cairo_fill(cr);

        cairo_set_source_rgb(cr, 0, 0, 0);
        cairo_set_line_width(cr, 1);
        cairo_rectangle(cr, 0.5, 0.5, widget->allocation.width-1, widget->allocation.height-1);
        cairo_stroke(cr);

        cairo_destroy(cr);
        return TRUE;
    }

public:
    HsvColorButton(GdkColor* p_color){
        da = gtk_drawing_area_new(); 
        gtk_widget_set_size_request(da, width, height);
        gtk_widget_add_events(da, GDK_BUTTON_PRESS_MASK);
        g_signal_connect(da, "button-press-event", G_CALLBACK(HsvColorButton::button_pressed), this);
        g_signal_connect(da,       "expose-event", G_CALLBACK(HsvColorButton::exposed)       , this);
        color = p_color;

        static int i = 0;
        i++;
        color->pixel = 0;
        color->red   = (unsigned short)(32000*(1+cos(G_PI*2/10*i)));
        color->blue  = (unsigned short)(32000*(1+cos(G_PI*2/10*i+G_PI*2/3)));
        color->green = (unsigned short)(33000*(1+cos(G_PI*2/10*i+G_PI*4/3)));
    }
    GtkWidget* get_widget(){
        return da;
    }
};

GtkWidget* event_listening(GtkWidget* widget){
    GtkWidget* eb = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(eb), widget);
    return eb;
}


class HsvImageButton{
    GtkWidget* eb;
    GtkWidget* da;
    GdkPixbuf* pixbuf;
    double     alpha;

static 
    gboolean button_pressed(GtkWidget* widget, GdkEventButton* event, HsvImageButton* self){
        g_assert( widget == self->da );
        if(event->type==GDK_BUTTON_PRESS && event->button==1){
        }
        return FALSE;
    }
static 
    gboolean mouse_moved(GtkWidget* widget, GdkEventCrossing* event, HsvImageButton* self){
        g_assert( widget == self->eb );
        if(event->type==GDK_ENTER_NOTIFY){
            self->alpha = 1;
        }else if(event->type==GDK_LEAVE_NOTIFY){
            self->alpha = 0.7;
        }
        gtk_widget_queue_draw(self->da);
        return TRUE;
    }
static 
    gboolean exposed(GtkWidget* widget, GdkEventExpose* event, HsvImageButton* self){
        g_assert( widget == self->da );
        GdkWindow* window = self->da->window;
        cairo_t*   cr     = gdk_cairo_create(window);
        if(self->alpha>0.9){
            cairo_set_source_rgb(cr, self->alpha, self->alpha, self->alpha);
            cairo_paint_with_alpha(cr, 0.5);
        }
        gdk_cairo_set_source_pixbuf(cr, self->pixbuf, 0, 0);
        cairo_paint_with_alpha(cr, self->alpha);
        cairo_destroy(cr);
        return TRUE;
    }

public:
    HsvImageButton(GtkWidget* widget){
        g_object_ref_sink(widget);
        GtkImage* image = GTK_IMAGE(widget);
        da = gtk_drawing_area_new(); 
        pixbuf = gtk_widget_render_icon(widget,
                                        image->data.stock.stock_id,
                                        image->icon_size,
                                        NULL);
        gtk_widget_set_size_request(da, gdk_pixbuf_get_width(pixbuf), gdk_pixbuf_get_height(pixbuf));
        g_signal_connect(da, "expose-event", G_CALLBACK(HsvImageButton::exposed)       , this);
        eb = event_listening(da);
        gtk_event_box_set_visible_window(GTK_EVENT_BOX(eb), FALSE);
        gtk_event_box_set_above_child(GTK_EVENT_BOX(eb), TRUE);
        g_signal_connect(eb, "enter-notify-event", G_CALLBACK(HsvImageButton::mouse_moved)   , this);
        g_signal_connect(eb, "leave-notify-event", G_CALLBACK(HsvImageButton::mouse_moved)   , this);
        g_object_unref(image);
        alpha = 0.7;
    }
   ~HsvImageButton(){
        g_object_unref(pixbuf);
    }
    GtkWidget* get_widget(){
        return eb;
    }
};

GtkWidget* ceiling(GtkWidget* widget){
    return widget;
    GtkWidget* align = gtk_alignment_new(0.0, 0.0, 0, 0);
    gtk_container_add(GTK_CONTAINER(align), widget);
    return align;
}

GtkWidget* center(GtkWidget* widget){
    GtkWidget* align = gtk_alignment_new(0.4, 0.2, 0.0, 0.0);
    gtk_container_add(GTK_CONTAINER(align), widget);
    return align;
}

GtkWidget* on_floor(GtkWidget* widget){
    GtkWidget* align = gtk_alignment_new(0.0, 1.0, 1, 0);
    gtk_container_add(GTK_CONTAINER(align), widget);
    return align;
}

GtkWidget* at_front(GtkWidget* widget){
    GtkWidget* align = gtk_alignment_new(0.0, 0.0, 0, 0);
    gtk_container_add(GTK_CONTAINER(align), widget);
    return align;
}

gboolean hsv_label_exposed(GtkWidget* da, GdkEventExpose* event, PangoLayout* layout){
    int width, height;
    pango_layout_get_size(layout, &width, &height);
    
    cairo_t* cr = gdk_cairo_create(da->window); 
    cairo_rotate(cr, G_PI_4);
    cairo_move_to(cr, +height*0.7/PANGO_SCALE, -height*0.7/PANGO_SCALE); 
    pango_cairo_show_layout(cr, layout);
    cairo_destroy(cr);
    return TRUE;
}

class HsvItemLables{
    vector<GtkWidget*>   anchors;
    vector<PangoLayout*> layouts;
    GtkWidget* da; 

static
    gboolean exposed(GtkWidget* da, GdkEventExpose* event, HsvItemLables* self){
        if( self->anchors.size() < self->layouts.size() )
            self->anchors.resize ( self->layouts.size() );

        int x_borrow = 0;

        int width  = da->allocation.width;
        int height = da->allocation.height;
        for(int i=self->layouts.size()-1; i; --i){
            if(!self->anchors[i]) continue;

            int anchor_x = self->anchors[i]->allocation.x;
            int text_w, text_h;
            PangoLayout* layout = self->layouts.back();
            pango_layout_get_size(layout, &text_w, &text_h);

            int draw_w = int(anchor_x + 0.7*(text_w+text_h)/PANGO_SCALE);
            if(draw_w>width){
                x_borrow = (draw_w-width)/i; 
            }

            break;
        }

        cairo_t* cr = gdk_cairo_create(da->window); 

        for(int i=0; i<self->layouts.size(); ++i){
            if(!self->anchors[i]) continue;
            PangoLayout* layout = self->layouts[i];
            int anchor_x = self->anchors[i]->allocation.x;
            int anchor_y = self->anchors[i]->allocation.y;
            int text_w, text_h;
            pango_layout_get_size(layout, &text_w, &text_h);
            cairo_save(cr);
            cairo_translate(cr, anchor_x-x_borrow*i, height-text_h*0.7/PANGO_SCALE);
            cairo_rotate(cr, -G_PI_4);
            cairo_move_to(cr, 0, 0); 
            pango_cairo_show_layout(cr, layout);
            cairo_restore(cr);
        }
        cairo_destroy(cr);
        return TRUE;
    }

   ~HsvItemLables(){
        for(int i=0; i<layouts.size(); ++i){
            g_object_unref(layouts[i]); 
        }
    }
    static void destroy(HsvItemLables* self){ delete self;}

public:
    HsvItemLables(const char** texts){
        da = gtk_drawing_area_new();
        PangoContext* context = gtk_widget_get_pango_context(da);
        int max_width  = 0;
        int max_height = 0;
        int total_height = 0;
        int last_width = 0;
        for(int i=0; texts[i]; ++i){
            PangoLayout* layout = pango_layout_new(context);
            pango_layout_set_text(layout, texts[i], -1);
            int width, height;
            pango_layout_get_size(layout, &width, &height);
            if(width >max_width)  max_width  = width;
            if(height>max_height) max_height = height;
            last_width    = width;
            total_height += height;
            layouts.push_back(layout);
        }

        int height = (int)(0.7*(max_width+max_height)/PANGO_SCALE);
        int width  = (int)(0.7*(total_height+last_width)/PANGO_SCALE);
        gtk_widget_set_size_request(da, width, height);
        g_signal_connect(da, "expose-event", G_CALLBACK(HsvItemLables::exposed), this);

        g_signal_connect_swapped(da, "destroy", G_CALLBACK(HsvItemLables::destroy), this);
    }

    GtkWidget* get_widget(){ return da;}
    void set_anchor(int i, GtkWidget* widget){
        if(i>=anchors.size()) anchors.resize(i+1);
        anchors[i] = widget;
    }

};

void highlight(GtkTextBuffer* tb, const char* tag, int i, int j, int line, int offset){
    if(!(i<j)){
        printf("i(%d) must < j(%d)\n", i, j);
        return;
    }
    if(i<0){
        printf("i(%d) must >= 0\n", i);
        return;
    }
    GtkTextIter begin, end;
    gtk_text_buffer_get_iter_at_line_offset(tb, &begin, line, offset+i);
    gtk_text_buffer_get_iter_at_line_offset(tb, &end  , line, offset+j);
    gtk_text_buffer_apply_tag_by_name(tb, tag, &begin, &end);
}

const char* highlight_re(char* str, GtkTextBuffer* tb, int line=0, int offset=0){
    static char errmsg[1024];
    char* ptr = str;
    int   is_atom = 0;
    vector<char*> open_parens;
    while(*ptr){ 
        switch(*ptr++)
        {
          case '*':
          case '+':
          case '?': {
              if(!is_atom){
                  highlight(tb, "red", ptr-str-1, ptr-str, line, offset);
                  sprintf(errmsg, "%c must follow an atom", ptr[-1]);
                  return errmsg; 
              }
              highlight(tb, "green", ptr-str-1, ptr-str, line, offset);
          }
          break;

          case '{': {
              if(!is_atom){
                  highlight(tb, "red", ptr-str-1, ptr-str, line, offset);
                  sprintf(errmsg, "%c must follow an atom", ptr[-1]);
                  return errmsg;
              }
              if(!isdigit(*ptr)){
                  highlight(tb, "red", ptr-str-1, ptr-str, line, offset);
                  return "{ must be followed by an integer";
              }
              char* begin = ptr;
              int i = 0;
              while(isdigit(*ptr)){
                  i = i*10 + *ptr++ - '0';
              }
              if(*ptr==',' && isdigit(*++ptr)){
                  int j = 0;
                  while(isdigit(*ptr)){
                      j = j*10 + *ptr++ - '0';
                  }
                  if(i>j){
                      highlight(tb, "red", begin-str, ptr-str, line, offset);
                      sprintf(errmsg, "invalid range");
                      return errmsg;
                  }
              }
              if(*ptr++!='}'){
                  highlight(tb, "red", begin-str-1, ptr-str-1, line, offset);
                  sprintf(errmsg, "missing }");
                  return errmsg;
              }
              highlight(tb, "green", begin-str-1, ptr-str, line, offset);
          }
          break;

          case '|' : {
              is_atom = 0;
              highlight(tb, "blue", ptr-str-1, ptr-str, line, offset);
          }
          break;

          case ')' : {
              is_atom = 1;
              if(open_parens.size()){
                  char* left = open_parens.back();
                  open_parens.pop_back();
                  highlight(tb, "blue", left-str, left-str+1, line, offset);
                  highlight(tb, "blue", ptr-str-1, ptr-str  , line, offset);
              }else{
              }
          }
          break;

          case '(' : {
              if(!*ptr){
                  highlight(tb, "red", ptr-str-1, ptr-str, line, offset);
                  return "trailing (";
              }
              is_atom = 0;
              open_parens.push_back(ptr-1);
          }
          break;

          case '[' : {
              char* begin = ptr-1;
              int n=0;
              if(*ptr=='^'){
                  ptr++;
              }
              if(*ptr==']'){
                  ptr++;
                  n++;
              }
              while(*ptr && *ptr!=']'){
                  char c = ptr[1];
                  if(*ptr=='[' && (c=='.'||c=='='||c==':')){
                      char* begin = ptr+2;
                      if(ptr[2]&&ptr[3]){
                          ptr += 3;
                          while(*ptr && (*ptr!=']' || ptr[-1]!=c)) ptr++;
                          if(!*ptr){
                            highlight(tb, "red", begin-str-2, ptr-str, line, offset);
                            sprintf(errmsg, "missing %c]", c);
                            return errmsg;
                          }
                          ptr++;
                      }else{
                          highlight(tb, "red", begin-str-2, ptr-str, line, offset);
                          sprintf(errmsg, "missing %c]\n", c);
                          return errmsg;
                      }
                      if(c==':'){
                          highlight(tb, "blue", begin-str, ptr-str-2, line, offset);
                          for(;begin<ptr-2;++begin){
                          }
                      }
                      if(n==2){
                          highlight(tb, "blue", begin-str-3, begin-str-2, line, offset);
                          n=0;
                      }else{
                          n=1;
                      }
                  }else if(*ptr=='-'){
                      if(n==0){
                          n = 1;
                      }else if(n==1){
                          n = 2;
                          if(ptr[1]==']'){
                          }
                      }else if(n==2){
                          highlight(tb, "blue", ptr-str-1, ptr-str, line, offset);
                          n=0;
                      }
                      ptr++;
                  }else{
                      if(n==2){
                          highlight(tb, "blue", ptr-str-1, ptr-str, line, offset);
                          n=0;
                      }else{
                          n=1;
                      }
                      ptr++;
                  }
              }
              if(*ptr++!=']'){
                  highlight(tb, "red", begin-str, ptr-str-1, line, offset);
                  return "missing ]"; 
              }
              highlight(tb, "purple", begin-str, begin-str+1, line, offset);
              highlight(tb, "purple", ptr-str-1, ptr-str    , line, offset);
              is_atom = 1;
          }
          break;

          case ']' : {
              is_atom = 1;
          }
          break;

          case '\\' : {
              if(!*ptr){
                  highlight(tb, "red", ptr-str-1, ptr-str, line, offset);
                  return "trailing \\";
              }
              int match_literally = 0;
              switch(*ptr++){
                  case 'w':
                  case 'W':
                  case 's':
                  case 'S':
                  case 'b':
                  case 'B':
                  case '<':
                  case '>':
                  case '\'':
                  case '`':
                      break;
                  default:
                      match_literally = 1;
              }
                  
              if(match_literally){
                  highlight(tb, "grey", ptr-str-2, ptr-str-1, line, offset);
              }else{
                  highlight(tb, "blue", ptr-str-2, ptr-str  , line, offset);
              }
              is_atom = 1;
          }
          break;
            
          default:
              is_atom = 1;
        }
    }
    if(open_parens.size()){
        char* left = open_parens.back();
        highlight(tb, "red", left-str, ptr-str, line, offset);
        return "missing )";
    }
    return NULL;
}

struct HsvPattern{
    unsigned int enable     : 1;
    unsigned int casesn     : 1;
    unsigned int regex      : 1;
    unsigned int re_X       : 1;
    unsigned int error_bg   : 1;
    unsigned int regexp_set : 1;
    unsigned int rawstr_set : 1;
    unsigned int tstamp     :25;
    GdkColor     color;
    GtkWidget*   tv;
    union{
        char*    rawstr;
        regex_t  regexp;
    };
    GtkWidget*   ch_re_X;
    GtkWidget*   error_msg;

    HsvPattern(){
        enable     = 1;
        casesn     = 1;
        regex      = 0;
        re_X       = 0;
        error_bg   = 0;
        regexp_set = 0;
        rawstr_set = 0;
        tv         = NULL;
        rawstr     = NULL;
        ch_re_X    = NULL;
        error_msg  = NULL;
    }
   ~HsvPattern(){
        clear();
    }

    void clear(){
        if(rawstr_set){
            g_free(rawstr);
            rawstr     = NULL;
            rawstr_set = 0;
        }
        if(regexp_set){
            regfree(&regexp);
            regexp_set = 0;
        }
    }

    void clear_error_msg(){
        if(error_bg){
            error_bg = 0;
            GdkColor color = {0, 65535, 65535, 65535};
            gtk_widget_modify_base(tv, GTK_STATE_NORMAL, &color);
            gtk_widget_set_size_request(error_msg, 0, 0);
        }
    }

    void compile(){
        clear();
        GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tv));
        GtkTextIter start, end;
        gtk_text_buffer_get_start_iter (buffer, &start);
        gtk_text_buffer_get_end_iter   (buffer, &end);
        gtk_text_buffer_remove_all_tags(buffer, &start, &end);
        char* text = gtk_text_buffer_get_text(buffer, &start, &end, TRUE);
        if(*text==0){
            g_free(text);
            clear_error_msg();
            return;
        }
        if(!regex){
            rawstr     = text;
            rawstr_set = 1;
            clear_error_msg();
        }else{
            int ec=regcomp(&regexp, text, REG_EXTENDED | (casesn?0:REG_ICASE) );
            const char* msg = highlight_re(text, buffer);
            if(!ec){
                regexp_set = 1;
                clear_error_msg();
            }else{
                char errbuf[256];
                regerror(ec, &regexp, errbuf, 256);
                gtk_label_set_text(GTK_LABEL(error_msg), msg?msg:errbuf);
                gtk_misc_set_alignment(GTK_MISC(error_msg), 0, 0);
                if(!error_bg){
                    error_bg = 1;
                    GdkColor color = {0, 255<<8, 220<<8, 220<<8};
                    gtk_widget_modify_base(tv, GTK_STATE_NORMAL, &color);
                    gtk_widget_set_size_request(error_msg, -1, -1);
                }
            }
        }
        tstamp  = app.time_stamp;
    }
};

gboolean search_forward(GtkWidget* widget, GdkEventButton* event, HsvPattern* pattern);
gboolean search_backward(GtkWidget* widget, GdkEventButton* event, HsvPattern* pattern);

class HsvSearch{
    static const GtkAttachOptions GTK_GROW = GtkAttachOptions(GTK_EXPAND|GTK_FILL);
    static const GtkAttachOptions GTK_FIXD = GTK_SHRINK;

    GtkWidget* top_fr;
    GtkWidget* top_sw;
    GtkWidget* top_vp;
    GtkWidget* top_bx;
    GtkWidget* top_tb;
    GtkWidget* grid;

    GtkWidget* hsview;
    
    HsvItemLables* option_label;


    int        num_row;
public:
    GtkWidget* not_found;

    vector<HsvPattern*> patterns;
    GtkWidget* get_top_widget(){ return top_fr;}
    void       set_hsview(GtkWidget* v){ hsview = v;}
    
    HsvSearch(){
        num_row = 0;

        top_sw = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(top_sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
        top_vp = gtk_viewport_new(gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(top_sw)), 
                                  gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(top_sw)));
        top_bx = gtk_vbox_new(FALSE, 0);
        gtk_container_add(GTK_CONTAINER(top_vp), top_bx);
        gtk_container_add(GTK_CONTAINER(top_sw), top_vp);

        top_fr = gtk_vbox_new(FALSE, 0);
        {
            GtkWidget* align = gtk_alignment_new(0, 0, 0, 0);
            GtkWidget* hbox  = gtk_hbox_new(FALSE, 0);
            GtkWidget* img   = gtk_image_new_from_stock(GTK_STOCK_FIND, GTK_ICON_SIZE_MENU);
            GtkWidget* label = gtk_label_new("Highlight and Search");

            gtk_box_pack_start(GTK_BOX(hbox), img,   FALSE, FALSE, 3);
            gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
            gtk_container_add(GTK_CONTAINER(align), hbox);
            gtk_box_pack_start(GTK_BOX(top_fr), align, FALSE, FALSE, 3);
            gtk_box_pack_start_defaults(GTK_BOX(top_fr), top_sw);
        }

        grid = gtk_table_new(30, 9, FALSE);
        gtk_box_pack_start(GTK_BOX(top_bx), grid, FALSE, FALSE, 0);
        
        const char* labels[] = {"Enable", "Color", "Case", "RegEx", NULL, "re.X", NULL};
        option_label = new HsvItemLables(labels);
        gtk_table_attach(GTK_TABLE(grid), option_label->get_widget(), 0, 5, 0, 1, GTK_FILL, GTK_SHRINK, 3, 1);

        GtkWidget* hd_pattern= gtk_label_new("Pattern");
        GtkWidget* hd_search = gtk_label_new("Search");
        gtk_table_attach(GTK_TABLE(grid), hd_pattern, 5, 6, 0, 1, GTK_EXPAND, GTK_SHRINK, 3, 1);
        gtk_table_attach(GTK_TABLE(grid), hd_search , 6, 8, 0, 1, GTK_SHRINK, GTK_SHRINK, 3, 1);

        gtk_table_attach(GTK_TABLE(grid), gtk_hseparator_new(), 0, 8, 1, 2, GTK_GROW, GTK_FIXD, 2, 2);

        add_row();
        add_row();
        add_row();

        g_signal_connect_swapped(G_OBJECT(get_top_widget()), "destroy", G_CALLBACK(delete_self), this);
    }
    static void delete_self(HsvSearch* self){delete self;}
   ~HsvSearch(){}

    void add_row(){
        HsvPattern* pattern = new HsvPattern();
        patterns.push_back(pattern);

        int i=num_row;
        int SPACE_PER_ROW = 3;

        GtkWidget* ch_enable = gtk_check_button_new();
        GtkWidget* ch_case   = gtk_check_button_new();
        GtkWidget* ch_regex  = gtk_check_button_new();
        GtkWidget* ch_re_X   = gtk_check_button_new();
        gtk_table_attach(GTK_TABLE(grid), ch_enable, 0, 1, 2+i*SPACE_PER_ROW, 3+i*SPACE_PER_ROW, GTK_SHRINK, GTK_SHRINK, 3, 1);
        gtk_table_attach(GTK_TABLE(grid), ch_case  , 2, 3, 2+i*SPACE_PER_ROW, 3+i*SPACE_PER_ROW, GTK_SHRINK, GTK_SHRINK, 3, 1);
        gtk_table_attach(GTK_TABLE(grid), ch_regex , 3, 4, 2+i*SPACE_PER_ROW, 3+i*SPACE_PER_ROW, GTK_SHRINK, GTK_SHRINK, 3, 1);
        gtk_table_attach(GTK_TABLE(grid), ch_re_X  , 4, 5, 2+i*SPACE_PER_ROW, 3+i*SPACE_PER_ROW, GTK_SHRINK, GTK_SHRINK, 0, 1);
        gtk_widget_set_can_focus(ch_enable, FALSE);
        gtk_widget_set_can_focus(ch_case  , FALSE);
        gtk_widget_set_can_focus(ch_regex , FALSE);
        gtk_widget_set_can_focus(ch_re_X  , FALSE);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ch_enable), TRUE);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ch_case  ), TRUE);
        gtk_widget_set_sensitive(ch_re_X, FALSE);
        gtk_widget_set_size_request(ch_re_X, 0, 0);

        pattern->ch_re_X = ch_re_X;

        HsvColorButton* hs_color = new HsvColorButton(&pattern->color);
        GtkWidget*      ch_color = hs_color->get_widget();
        gtk_table_attach(GTK_TABLE(grid), ch_color, 1, 2, 2+i*SPACE_PER_ROW, 3+i*SPACE_PER_ROW, GTK_SHRINK, GTK_SHRINK, 3, 1);

        if(i==0){
            option_label->set_anchor(0, ch_enable);
            option_label->set_anchor(1, ch_color);
            option_label->set_anchor(2, ch_case);
            option_label->set_anchor(3, ch_regex);
            option_label->set_anchor(4, ch_re_X);
        }

        #if 0
        #else
        HsvImageButton* forward_btn = new HsvImageButton(gtk_image_new_from_stock(GTK_STOCK_GO_FORWARD, GTK_ICON_SIZE_MENU));
        HsvImageButton*    back_btn = new HsvImageButton(gtk_image_new_from_stock(GTK_STOCK_GO_BACK,    GTK_ICON_SIZE_MENU));
        GtkWidget* forward = forward_btn->get_widget();
        GtkWidget*    back =    back_btn->get_widget();
        g_signal_connect(forward, "button-press-event", G_CALLBACK(search_forward) , pattern);
        g_signal_connect(   back, "button-press-event", G_CALLBACK(search_backward), pattern);
        #endif

        gtk_table_attach(GTK_TABLE(grid),    back, 6, 7, 2+i*SPACE_PER_ROW, 3+i*SPACE_PER_ROW, GTK_SHRINK, GTK_SHRINK, 3, 1);
        gtk_table_attach(GTK_TABLE(grid), forward, 7, 8, 2+i*SPACE_PER_ROW, 3+i*SPACE_PER_ROW, GTK_SHRINK, GTK_SHRINK, 3, 1);

        GtkWidget* error_msg = gtk_label_new("OK");
        gtk_misc_set_alignment(GTK_MISC(error_msg), 0, 0);
        gtk_widget_set_size_request(error_msg, 0, 0);
        GdkColor error_color = {0, 65535, 0, 0};
        gtk_widget_modify_fg(error_msg, GTK_STATE_NORMAL, &error_color);
        pattern->error_msg = error_msg;

        if(SPACE_PER_ROW==3){
            gtk_table_attach(GTK_TABLE(grid), on_floor(gtk_hseparator_new()), 0, 5, 2+(i+1)*SPACE_PER_ROW-2, 3+(i+1)*SPACE_PER_ROW-2, GTK_SHRINK, GTK_GROW, 0, 1);
            gtk_table_attach(GTK_TABLE(grid), on_floor(gtk_hseparator_new()), 6, 8, 2+(i+1)*SPACE_PER_ROW-2, 3+(i+1)*SPACE_PER_ROW-2, GTK_SHRINK, GTK_GROW, 0, 1);
            gtk_table_attach(GTK_TABLE(grid), on_floor(gtk_hseparator_new()), 0, 8, 2+(i+1)*SPACE_PER_ROW-1, 3+(i+1)*SPACE_PER_ROW-1, GTK_FILL, GTK_GROW, 0, 1);
        }else{
            gtk_table_attach(GTK_TABLE(grid), on_floor(gtk_hseparator_new()), 0, 5, 2+(i+1)*SPACE_PER_ROW-1, 3+(i+1)*SPACE_PER_ROW-1, GTK_FILL, GTK_GROW, 0, 1);
            gtk_table_attach(GTK_TABLE(grid), on_floor(gtk_hseparator_new()), 6, 8, 2+(i+1)*SPACE_PER_ROW-1, 3+(i+1)*SPACE_PER_ROW-1, GTK_FILL, GTK_GROW, 0, 1);
        }

        GtkWidget* pattern_box = gtk_vbox_new(FALSE, 0);
        GtkWidget* ch_text = gtk_text_view_new();
        gtk_box_pack_start(GTK_BOX(pattern_box), ceiling(ch_text), FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(pattern_box), error_msg, FALSE, FALSE, 0);
        gtk_container_set_border_width(GTK_CONTAINER(pattern_box),5);
        gtk_table_attach(GTK_TABLE(grid), pattern_box, 5, 6, 2+i*SPACE_PER_ROW, 4+i*SPACE_PER_ROW, GTK_GROW, GTK_FILL, 0, 0);
        pattern->tv = ch_text;
        if(app.code_font_button){
            gtk_widget_modify_font(ch_text, 
              pango_font_description_from_string(gtk_font_button_get_font_name(app.code_font_button)));
        }

        g_signal_connect(ch_enable, "toggled", G_CALLBACK(HsvSearch::toggled_enable), pattern);
        g_signal_connect_swapped
                        (ch_enable, "toggled", G_CALLBACK(gtk_widget_queue_draw), ch_color);
        g_signal_connect(ch_case  , "toggled", G_CALLBACK(HsvSearch::toggled_case  ), pattern);
        g_signal_connect(ch_regex , "toggled", G_CALLBACK(HsvSearch::toggled_regex ), pattern);
        g_signal_connect(ch_re_X  , "toggled", G_CALLBACK(HsvSearch::toggled_re_X  ), pattern);
        g_signal_connect(ch_text, "button-press-event", G_CALLBACK(HsvSearch::button_pressed), this);
        g_signal_connect(ch_text,    "key-press-event", G_CALLBACK(HsvSearch::key_pressed)   , this);
        g_signal_connect(ch_text,  "key-release-event", G_CALLBACK(HsvSearch::key_released)  , this);

        GtkTextBuffer* pattern_buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ch_text));
        GdkColor red  = {0, 65535, 0, 0};
        GdkColor blue = {0, 0, 0, 55535};
        GdkColor purple = {0, 156<<8, 0, 143<<8};
        GdkColor grey = {0, 30000, 30000, 30000};
        GdkColor green= {0, 0, 65535, 0};
        gtk_text_buffer_create_tag(pattern_buf, "red", "foreground-gdk", &red, "foreground-set", TRUE, NULL);
        gtk_text_buffer_create_tag(pattern_buf, "blue", "foreground-gdk", &blue, "foreground-set", TRUE, NULL);
        gtk_text_buffer_create_tag(pattern_buf, "purple", "foreground-gdk", &purple, "foreground-set", TRUE, NULL);
        gtk_text_buffer_create_tag(pattern_buf, "grey", "foreground-gdk", &grey, "foreground-set", TRUE, NULL);
        gtk_text_buffer_create_tag(pattern_buf, "green", "foreground-gdk", &green, "foreground-set", TRUE, NULL);

        num_row += 1; 
    }

private:

static
    void toggled_enable(GtkToggleButton* btn, HsvPattern* pattern){
        pattern->enable = gtk_toggle_button_get_active(btn)?1:0;
        pattern->tstamp = app.time_stamp;
        pattern->compile();
        gtk_widget_queue_draw(app.hsview);
    }
static
    void toggled_case(GtkToggleButton* btn, HsvPattern* pattern){
        pattern->casesn = gtk_toggle_button_get_active(btn)?1:0;
        pattern->tstamp = app.time_stamp;
        pattern->compile();
        gtk_widget_queue_draw(app.hsview);
    }
static
    void toggled_regex(GtkToggleButton* btn, HsvPattern* pattern){
        pattern->regex  = gtk_toggle_button_get_active(btn)?1:0;
        pattern->tstamp = app.time_stamp;
        gtk_widget_set_sensitive(pattern->ch_re_X, pattern->regex);
        pattern->compile();
        gtk_widget_queue_draw(app.hsview);
    }
static
    void toggled_re_X(GtkToggleButton* btn, HsvPattern* pattern){
        pattern->re_X   = gtk_toggle_button_get_active(btn)?1:0;
        pattern->tstamp = app.time_stamp;
    }
static
    gboolean button_pressed(GtkWidget* widget, GdkEventButton* event, HsvSearch* self){
        if(event->type==GDK_BUTTON_PRESS && event->button==1){
            for(int i=0; i<self->patterns.size(); ++i){
                if(widget==self->patterns[i]->tv){
                    if(i>=(self->patterns.size()-1)){
                        self->add_row();
                        gtk_widget_show_all(self->grid);
                    }
                    break; 
                }
            }

        }
        return FALSE;
    }
static
    gboolean key_pressed(GtkWidget* widget, GdkEventKey* event, HsvSearch* self){
        HsvPattern* pattern = NULL;
        for(int i=0; i<self->patterns.size(); ++i){
            if(self->patterns[i]->tv==widget){ 
                pattern = self->patterns[i];
                break;
            }
        }
        g_assert(pattern);
        if(event->keyval == GDK_KEY_Return || event->keyval == GDK_KP_Enter){
            if(event->state & GDK_SHIFT_MASK){
                search_backward(NULL, NULL, pattern);
            }else{
                search_forward(NULL, NULL, pattern);
            }
            return TRUE;
        }
        return FALSE;
    }
static
    gboolean key_released(GtkWidget* widget, GdkEventKey* event, HsvSearch* self){
        HsvPattern* pattern = NULL;
        for(int i=0; i<self->patterns.size(); ++i){
            if(self->patterns[i]->tv==widget){ 
                pattern = self->patterns[i];
                break;
            }
        }
        g_assert(pattern);
        pattern->compile();
        gtk_widget_queue_draw(self->hsview);
        return FALSE;
    }
    
};


struct HsviewLine{
    HsviewLine*  next;
    HsviewLine*  prev;
    PangoLayout* layout;
    PangoLayout* gutter;
    long         linenum;
    int          width;
    int          height;
    int          gutter_width;
    int          gutter_height;
    unsigned int used:1;
    HsviewLine(PangoContext* context, PangoFontDescription* font=NULL){
        next    = this;
        prev    = this;
        linenum = -1;
        used    = context?0:1;
        layout  = context?pango_layout_new(context):NULL;
        gutter  = context?pango_layout_new(context):NULL;
        width   = 0;
        height  = 0;
        gutter_width = 0;
        gutter_height= 0;
        if(font){
            pango_layout_set_font_description(layout, font);
            pango_layout_set_font_description(gutter, font);
        }
    }
    void set_line(long newlinenum){
        if(linenum != newlinenum){
            linenum = newlinenum;
            if(app.file){
                pango_layout_set_text(layout, app.file->get_line(linenum).c_str(), -1);
            }else{
                pango_layout_set_text(layout, "no text", -1);
            }
            static char buffer[32];
            sprintf(buffer, "%ld", linenum+1);
            pango_layout_set_text(gutter, buffer, -1);
            pango_layout_get_size(layout, &width, &height); 
            pango_layout_get_size(gutter, &gutter_width, &gutter_height);
            width  /= PANGO_SCALE;
            height /= PANGO_SCALE;
            gutter_width  /= PANGO_SCALE;
            gutter_height /= PANGO_SCALE;
        }
    }
   ~HsviewLine(){
        if(layout) g_object_unref(layout);
        if(gutter) g_object_unref(gutter);
    }
};

class HsvMarks;
void add_mark_from_menu_item_activation(GtkWidget* item, long linenum);

long requested_goto = 0;
void set_goto_request(int r){
    if(r<1) r=1;
    if(r>app.file->get_number_of_lines())
       r=app.file->get_number_of_lines();
    requested_goto = r;
    gtk_widget_queue_draw(app.hsview);
}

class Hsview{
public:
    GtkWidget*    top;
    GtkWidget*    layout;
    GtkWidget*    header;
    GtkEventBox*  h_scroll;
    GtkEventBox*  v_scroll;
    PangoContext* context;
    PangoFontDescription* font;
    HsvSearch*    searches;
    HsvMarks*     marks;

    HsviewLine             vline_cache;
    map<long, HsviewLine*> vline_map;
    long                   start_line;
    int                    scroll_speed;
    int                    preview;
  
    int                    h_scroll_static_offset;
    int                    h_scroll_dynamic_offset;
    int                    h_scroll_offset_max;
    int                    h_scroll_drag_from;
    int                    h_scroll_drag_to;
    float                  h_scroll_scale;
    int                    on_h_scroll;
    int                    text_offset;

    int                    v_scroll_offset;
    int                    v_scroll_offset_max;
    int                    v_scroll_drag_from_offset;
    int                    v_scroll_drag_from;
    int                    v_scroll_drag_to;
    int                    v_scroll_height;
    double                 v_scroll_scale;
    int                    on_v_scroll;

    vector<HsviewLine*>    drawn_lines;

    long                   line_of_focus;
    int                    char_of_focus;
    vector<PangoRectangle> rect_of_focus;

    HsviewLine* new_vline(){
        HsviewLine* vline = vline_cache.next;
        if(vline_map.size()>1000){
            if(vline == &vline_cache){
                printf("error\n");
                exit(1);
            }
            if(vline->linenum>=0){
                vline_map.erase(vline->linenum);
                vline->linenum = -1;
            }
            vline_cache.next  = vline->next;
            vline->next->prev = &vline_cache;

            vline->next = &vline_cache;
            vline->prev =  vline_cache.prev;
            vline_cache.prev->next = vline;
            vline_cache.prev       = vline;

            return vline;
        }
        vline = new HsviewLine(context, font);
        vline->next = &vline_cache;
        vline->prev =  vline_cache.prev;
        vline_cache.prev->next = vline;
        vline_cache.prev       = vline;
        return vline;
    }

    HsviewLine* get_vline(long linenum){
        HsviewLine*& vline = vline_map[linenum]; 
        if(vline){
            vline->prev->next = vline->next;
            vline->next->prev = vline->prev;
            vline->next = &vline_cache;
            vline->prev =  vline_cache.prev;
            vline_cache.prev->next = vline;
            vline_cache.prev       = vline;
        }else{
            vline = new_vline();
            vline->used = 1;
            vline->set_line(linenum);
        }
        return vline;
    }

    int width;
    int height; 
    int line_col_width;
    int header_height;
    int footer_height;
    int ever_exposed;
    
    Hsview() : vline_cache(NULL){
        
        searches     = NULL;
        marks        = NULL;
        start_line   = 0;
        scroll_speed = 1;
        preview      = 3;
        layout = gtk_layout_new(NULL, NULL);
        if(app.code_font_button){
            font = pango_font_description_from_string(gtk_font_button_get_font_name(app.code_font_button));
        }else{
            font = NULL;
        }

        char buffer[32];
        sprintf(buffer, "Total %ld lines / Go to", app.file->get_number_of_lines());
        GtkWidget* hdrbx = gtk_hbox_new(FALSE, 2);
        GtkWidget* label = gtk_label_new(buffer);
        GtkWidget* entry = gtk_entry_new();
        gtk_entry_set_has_frame(GTK_ENTRY(entry), FALSE);
        gtk_entry_set_text(GTK_ENTRY(entry), "1");
        gtk_entry_set_width_chars(GTK_ENTRY(entry), 2);
        g_signal_connect(entry, "key-press-event", G_CALLBACK(Hsview::set_line_from_entry),this);
        g_signal_connect(entry, "changed"        , G_CALLBACK(Hsview::set_line_from_entry),this);
        gtk_box_pack_start(GTK_BOX(hdrbx), label, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(hdrbx), entry, FALSE, FALSE, 3);
        GtkWidget* hdlgn  = gtk_alignment_new(1, 0, 0, 0);
        gtk_container_add(GTK_CONTAINER(hdlgn), hdrbx);
        gtk_layout_put(GTK_LAYOUT(layout), hdlgn, 0, 3);
        gtk_widget_set_size_request(hdlgn, 400, -1);
        header = hdlgn;

        g_signal_connect(layout, "size-allocate",       G_CALLBACK(Hsview::size_changed)  ,this);
        g_signal_connect(layout, "expose-event",        G_CALLBACK(Hsview::exposed)       ,this);
        g_signal_connect(layout, "scroll-event",        G_CALLBACK(Hsview::scrolled)      ,this);
        g_signal_connect(layout, "button-press-event",  G_CALLBACK(Hsview::button_pressed),this);
        g_signal_connect(layout, "key-press-event",     G_CALLBACK(Hsview::key_pressed)   ,this);

        ever_exposed = 0;

        h_scroll = NULL;
        h_scroll_static_offset  = 0;
        h_scroll_dynamic_offset = 0;
        h_scroll_offset_max     = 0;
        h_scroll_drag_from = 0;
        h_scroll_drag_to   = 0;
        h_scroll_scale     = 1;
        on_h_scroll = 0;

        v_scroll = NULL;
        v_scroll_offset = 0;
        v_scroll_offset_max = 0;
        v_scroll_drag_from_offset = 0;
        v_scroll_drag_from = 0;
        v_scroll_drag_to   = 0;
        v_scroll_height    = 0;
        v_scroll_scale     = 0;
        on_v_scroll = 0;

        gtk_widget_set_size_request(layout, 400, 300);

        top = layout;

        context = gtk_widget_create_pango_context(top);

        gtk_widget_set_can_focus(layout, TRUE);
        gtk_widget_grab_focus(layout);
        
        line_of_focus = -1;
        char_of_focus = -1;

        line_col_width = 40;

        g_signal_connect_swapped(G_OBJECT(top), "destroy", G_CALLBACK(delete_hsview), this);
    }
   ~Hsview(){
       for(HsviewLine* ptr=vline_cache.next; ptr!=&vline_cache;){
           ptr = ptr->next;
           delete ptr->prev;
       }
       g_object_unref(G_OBJECT(context));
       pango_font_description_free(font);
    }
    static void delete_hsview(Hsview* self){
        delete self;
    }

    void set_line_of_focus(long linenum){
        if(line_of_focus != linenum){

            line_of_focus = linenum;    
            rect_of_focus.clear();

            HsviewLine* vline = get_vline(linenum);
            PangoLayoutIter* iter = pango_layout_get_iter(vline->layout);
            do{
                rect_of_focus.resize(rect_of_focus.size()+1);
                pango_layout_iter_get_char_extents(iter, &rect_of_focus.back());
                rect_of_focus.back().x      /= PANGO_SCALE;
                rect_of_focus.back().width  /= PANGO_SCALE;
                rect_of_focus.back().y       = rect_of_focus.back().x + 
                                               rect_of_focus.back().width;
            }while(pango_layout_iter_next_char(iter));

            pango_layout_iter_free(iter);
        }
    }
    void set_char_of_focus(int x){
        char_of_focus = 0;
        for(int i=0; i<rect_of_focus.size(); ++i){
            char_of_focus = i;
            if(x >= rect_of_focus[i].x && x <= rect_of_focus[i].y){
                return;
            }
        }
    }
   
    GtkWidget* get_top_widget(){
        return top;    
    }

static
    void size_changed(GtkWidget* widget, GdkRectangle* allocation, Hsview* self){
        g_assert(self->layout==widget);
        if(self->h_scroll){
            int y = allocation->height - self->footer_height - 3;
            if(((GtkWidget*) self->h_scroll)->allocation.y != y){
                gtk_layout_move(GTK_LAYOUT(self->layout), GTK_WIDGET(self->h_scroll), self->line_col_width, y);
            }
            int width = allocation->width - self->line_col_width;
            if( width > 0 && ((GtkWidget*) self->h_scroll)->allocation.width != width){
                gtk_widget_set_size_request(GTK_WIDGET(self->h_scroll), width, 7);
            }
        }
        if(self->v_scroll){
            int x = allocation->width - 7;
            if(((GtkWidget*) self->v_scroll)->allocation.x != x){
                gtk_layout_move(GTK_LAYOUT(self->layout), GTK_WIDGET(self->v_scroll), x, self->header_height);
            }
            int height = allocation->height - self->header_height - self->footer_height; 
            if(height > 0 && ((GtkWidget*) self->v_scroll)->allocation.height != height){
                gtk_widget_set_size_request(GTK_WIDGET(self->v_scroll), 7, height);
            }
        }
        if(self->header){
            gtk_widget_set_size_request(self->header, allocation->width, -1);
        }
    }

static
    gboolean exposed(GtkWidget* widget, GdkEventExpose* event, Hsview* self){
        if(!self->ever_exposed){
            self->ever_exposed = 1;
            GdkWindow* bin_window = GTK_LAYOUT(self->layout)->bin_window;
            gdk_window_set_events(bin_window, GdkEventMask(gdk_window_get_events(bin_window) | GDK_BUTTON_PRESS_MASK));
        }
        app.hsview = self->top;
        g_assert (self->layout==widget);

        if(requested_goto){
            self->start_line = requested_goto-1;
            requested_goto = 0;
        }
        if(self->v_scroll_drag_from || self->v_scroll_drag_to){
            self->v_scroll_offset = self->v_scroll_drag_from_offset
              + self->v_scroll_drag_to - self->v_scroll_drag_from;
            if(self->v_scroll_offset<0) self->v_scroll_offset = 0;
            if(self->v_scroll_offset > self->v_scroll_offset_max)
               self->v_scroll_offset = self->v_scroll_offset_max;
            self->start_line = int(self->v_scroll_offset*self->v_scroll_scale);
            if(self->start_line >= app.file->get_number_of_lines()){
               self->start_line  = app.file->get_number_of_lines()-1;
            }
        }

        GtkLayout* layout = GTK_LAYOUT(widget);
        GdkWindow* window = layout->bin_window;
        GtkStyle*  style  = widget->style;
        GdkColor   color  = style->light[0];
        cairo_t*   cr     = gdk_cairo_create(window); 

        int width  = self->width  = widget->allocation.width;
        int height = self->height = widget->allocation.height;

        int line_col_width = self->line_col_width;
        int header_height  = self->header_height  = 30;
        int footer_height  = self->footer_height  = 30;

        int viewable_width = width-line_col_width-5;

        cairo_set_line_width(cr, 1);

        gdk_cairo_set_source_color(cr, &style->dark[GTK_STATE_NORMAL]);
        cairo_rectangle(cr, line_col_width-0.5, header_height-0.5, width-line_col_width, height-header_height-footer_height+1);
        cairo_stroke(cr);

        self->drawn_lines.clear();

        cairo_rectangle(cr, line_col_width, 0, width-line_col_width, height);
        cairo_clip(cr);

#if 0
#else
        int x_offset = 0;
        if(self->h_scroll_drag_to || self->h_scroll_drag_from){
            self->h_scroll_dynamic_offset = self->h_scroll_static_offset + (self->h_scroll_drag_to - self->h_scroll_drag_from);
            if( self->h_scroll_dynamic_offset < 0) self->h_scroll_dynamic_offset = 0;
            if( self->h_scroll_dynamic_offset > self->h_scroll_offset_max ) 
                self->h_scroll_dynamic_offset = self->h_scroll_offset_max ;
        }else{
            self->h_scroll_static_offset = self->h_scroll_dynamic_offset;
        }

        x_offset = (int) (self->h_scroll_dynamic_offset * self->h_scroll_scale);
#endif

        self->text_offset = x_offset;

        int y;
        gdk_cairo_set_source_color(cr, &style->text[GTK_STATE_INSENSITIVE]);
        y = header_height;
        for(int i=1; i<=self->preview; ++i){
            if(self->start_line < i) break;
            HsviewLine* vline = self->get_vline(self->start_line - i);
            y -= vline->height;
            cairo_move_to(cr, line_col_width-x_offset, y);
            pango_layout_set_attributes(vline->layout, NULL);
            pango_cairo_show_layout(cr, vline->layout);
        }

        int max_width = self->text_offset+viewable_width;

        gdk_cairo_set_source_color(cr, &style->text[GTK_STATE_NORMAL]);
        y = header_height;
        int viewable_lines = 0;
        HsviewLine* vline = NULL;
        for(int i=self->start_line; i<app.file->get_number_of_lines(); ++i){
            vline = self->get_vline(i);

            if(y+vline->gutter_height*4/5 < height-footer_height){
                viewable_lines += 1;

                if(vline->width > viewable_width+x_offset){
                    gdk_cairo_set_source_color(cr, &style->bg[GTK_STATE_NORMAL]);
                    cairo_move_to(cr, width-0.5, y);
                    cairo_line_to(cr, width-0.5, y+vline->height);
                    cairo_stroke(cr);
                }

                HsvLine line = app.file->get_line(i);
                const char* str = line.c_str();

                PangoAttrList* attrs = pango_attr_list_new();
                for(int p=0; p<self->searches->patterns.size(); ++p){
                    HsvPattern* pattern = self->searches->patterns[p];
                    if(!pattern->enable) continue;
                    if(!pattern->rawstr_set && !pattern->regexp_set) continue;
                    const char* found = str;
                    if(!pattern->regex){
                        int pattern_len = strlen(pattern->rawstr);
                        do{
                            if(pattern->casesn){
                                found = strstr(found, pattern->rawstr);
                            }else{
                                found = strcasestr(found, pattern->rawstr);
                            }
                            if(!found) break;
                            PangoAttribute* bg = pango_attr_background_new(pattern->color.red, pattern->color.green, pattern->color.blue);
                            bg->start_index = found - str;
                            bg->end_index   = bg->start_index + pattern_len;
                            pango_attr_list_insert(attrs, bg);
                            found += pattern_len;
                        }while(1);
                    }else{
                        regmatch_t match;
                        int offset = 0;
                        while(regexec(&pattern->regexp, str+offset, 1, &match, 0)==0){
                            if(match.rm_so==match.rm_eo){
                                offset+=1;
                                if(str[offset]) continue; 
                                else            break;
                            }
                            PangoAttribute* bg = pango_attr_background_new(pattern->color.red, pattern->color.green, pattern->color.blue);
                            bg->start_index = match.rm_so+offset;
                            bg->end_index   = match.rm_eo+offset;
                            pango_attr_list_insert(attrs, bg);
                            offset += match.rm_eo;
                        }

                    }
                }
                if(i==self->line_of_focus){
                    int len = strlen(str);
                    if(self->char_of_focus < len && isprint(str[self->char_of_focus])){
                        int begin = self->char_of_focus;
                        int end   = self->char_of_focus+1;
                        char c;
                        if(isalnum(c=str[begin]) || c=='_'){
                            while(begin && (isalnum(c=str[begin-1]) || c=='_')) --begin;
                            while(isalnum(c=str[end]) || c=='_') ++end;
                        }
                        GdkColor color = style->bg[GTK_STATE_SELECTED];
                        PangoAttribute* fg = pango_attr_foreground_new(color.red, color.green, color.blue);
                        fg->start_index = begin;
                        fg->end_index   = end;
                        pango_attr_list_insert(attrs, fg);
                    }
                }
                pango_layout_set_attributes(vline->layout, attrs);
                pango_attr_list_unref(attrs);

                if(i==self->line_of_focus){
                    gdk_cairo_set_source_color(cr, &style->text[GTK_STATE_NORMAL]);

                }else{
                    gdk_cairo_set_source_color(cr, &style->text[GTK_STATE_NORMAL]);
                }

                cairo_move_to(cr, line_col_width-x_offset, y);
                pango_cairo_show_layout(cr, vline->layout);
                self->drawn_lines.push_back(vline);
                y += vline->height;

                if(vline->width > max_width){
                    max_width = vline->width;
                }

            }else{
                vline = NULL;
                break;
            }
        }
        if(vline){
            int Y = y;
            while(Y+vline->gutter_height*4/5 < height-footer_height){
                viewable_lines++;
                Y += vline->height;
            }
            vline = NULL;
        }

        gdk_cairo_set_source_color(cr, &style->text[GTK_STATE_INSENSITIVE]);
        for(int i=0; i<self->preview; ++i){
            long linenum = self->start_line + self->drawn_lines.size()+i;
            if(linenum >= app.file->get_number_of_lines()) break;
            HsviewLine* vline = self->get_vline(linenum);
            cairo_move_to(cr, line_col_width-x_offset, y);
            pango_layout_set_attributes(vline->layout, NULL);
            pango_cairo_show_layout(cr, vline->layout);
            y += vline->height;
        }

        cairo_reset_clip(cr);

        gdk_cairo_set_source_color(cr, &style->dark[GTK_STATE_NORMAL]);
        y = header_height;
        for(int i=0; i<self->drawn_lines.size(); ++i){
            HsviewLine* vline = self->drawn_lines[i];
            int x;
            cairo_move_to(cr, x=(line_col_width-vline->gutter_width-5), y);
            if(x<0){
                self->line_col_width = vline->gutter_width+5;
            }
            if(self->line_of_focus != self->start_line+i){
                pango_cairo_show_layout(cr, vline->gutter);
                y += vline->height;
            }else{
                GdkColor color = {0, 0, 0, 0};
                gdk_cairo_set_source_color(cr, &color);
              { pango_cairo_show_layout(cr, vline->gutter);
                y += vline->height; }
                gdk_cairo_set_source_color(cr, &style->dark[GTK_STATE_NORMAL]);
            }
        }

        if(self->h_scroll_dynamic_offset || max_width > viewable_width){
            if(self->on_h_scroll){
                gdk_cairo_set_source_color(cr, &style->dark[GTK_STATE_PRELIGHT]);
            }else{
                gdk_cairo_set_source_color(cr, &style->dark[GTK_STATE_NORMAL]);
            }
            int offset = self->h_scroll_dynamic_offset;
            int scrollbar_width = viewable_width*viewable_width/max_width;
            self->h_scroll_offset_max = viewable_width - scrollbar_width;
            self->h_scroll_scale = float(max_width)/viewable_width;
            cairo_rectangle(cr, line_col_width+offset, height-footer_height-2, scrollbar_width, 5);
            cairo_fill(cr);
        }
        if(!self->h_scroll){
            self->h_scroll = GTK_EVENT_BOX(gtk_event_box_new());
            gtk_layout_put(GTK_LAYOUT(self->layout), GTK_WIDGET(self->h_scroll), line_col_width, height-footer_height-3);
            gtk_widget_set_size_request(GTK_WIDGET(self->h_scroll), width-line_col_width, 7);
            g_signal_connect(G_OBJECT(self->h_scroll), "motion-notify-event", G_CALLBACK(Hsview::hscrollbar_dragged), self);
            g_signal_connect(G_OBJECT(self->h_scroll),  "button-press-event", G_CALLBACK(Hsview::hscrollbar_dragged), self);
            g_signal_connect(G_OBJECT(self->h_scroll),"button-release-event", G_CALLBACK(Hsview::hscrollbar_dragged), self);
            g_signal_connect(G_OBJECT(self->h_scroll),  "enter-notify-event", G_CALLBACK(Hsview::hscrollbar_dragged), self);
            g_signal_connect(G_OBJECT(self->h_scroll),  "leave-notify-event", G_CALLBACK(Hsview::hscrollbar_dragged), self);
            gtk_event_box_set_visible_window(self->h_scroll, FALSE);
            gtk_widget_show(GTK_WIDGET(self->h_scroll));
        }

        if(viewable_lines>3 && self->drawn_lines.size() < app.file->get_number_of_lines()){
            if(self->v_scroll_drag_from || self->v_scroll_drag_to){
            }else{
                self->v_scroll_height = (height - self->header_height - self->footer_height)
                  * viewable_lines / (app.file->get_number_of_lines()+viewable_lines-1);
                if(self->v_scroll_height < 20) self->v_scroll_height = 20;
                self->v_scroll_offset_max = height - self->header_height
                  - self->footer_height - self->v_scroll_height;
                if(self->v_scroll_offset_max>0){
                    self->v_scroll_scale = double(app.file->get_number_of_lines())/self->v_scroll_offset_max;
                    self->v_scroll_offset = int(self->start_line / self->v_scroll_scale);
                }else{
                    self->v_scroll_height = 0;
                    self->v_scroll_offset = 0;
                }
            }
            if(self->on_v_scroll){
                gdk_cairo_set_source_color(cr, &style->dark[GTK_STATE_PRELIGHT]);
                cairo_rectangle(cr, width-9, self->header_height + self->v_scroll_offset, 7, self->v_scroll_height);
            }else{
                gdk_cairo_set_source_color(cr, &style->dark[GTK_STATE_NORMAL]);
                cairo_rectangle(cr, width-5, self->header_height + self->v_scroll_offset, 3, self->v_scroll_height);
            }
            cairo_fill(cr);
        }else{
            self->v_scroll_height = 0;
            self->v_scroll_offset = 0;
        }
        if(!self->v_scroll){
            self->v_scroll = GTK_EVENT_BOX(gtk_event_box_new());
            gtk_layout_put(GTK_LAYOUT(self->layout), GTK_WIDGET(self->v_scroll), width-7, header_height);
            gtk_widget_set_size_request(GTK_WIDGET(self->v_scroll), 7, height-header_height-footer_height);
            g_signal_connect(G_OBJECT(self->v_scroll), "motion-notify-event", G_CALLBACK(Hsview::vscrollbar_dragged), self);
            g_signal_connect(G_OBJECT(self->v_scroll),  "button-press-event", G_CALLBACK(Hsview::vscrollbar_dragged), self);
            g_signal_connect(G_OBJECT(self->v_scroll),"button-release-event", G_CALLBACK(Hsview::vscrollbar_dragged), self);
            g_signal_connect(G_OBJECT(self->v_scroll),  "enter-notify-event", G_CALLBACK(Hsview::vscrollbar_dragged), self);
            g_signal_connect(G_OBJECT(self->v_scroll),  "leave-notify-event", G_CALLBACK(Hsview::vscrollbar_dragged), self);
            gtk_event_box_set_visible_window(self->v_scroll, FALSE);
            gtk_widget_show(GTK_WIDGET(self->v_scroll));
        }

        cairo_destroy(cr);
        app.time_stamp++;
        return FALSE;
    }

static
    gboolean scrolled(GtkWidget* widget, GdkEvent* event, Hsview* self){
        g_assert (self->layout==widget);
        if(event->type == GDK_SCROLL){
            GdkEventScroll* scroll = (GdkEventScroll*) event;
            if(scroll->direction == GDK_SCROLL_UP){
                if( self->start_line >= self->scroll_speed )
                    self->start_line -= self->scroll_speed ;
                else 
                    self->start_line  = 0;
            }
            if(scroll->direction == GDK_SCROLL_DOWN){
                self->start_line += self->scroll_speed;
                if( self->start_line >= app.file->get_number_of_lines() )
                    self->start_line  = app.file->get_number_of_lines()-1 ;
            }
            gtk_widget_queue_draw(widget);
        }

        return TRUE;
    }

static
    gboolean button_pressed(GtkWidget* widget, GdkEventButton* event, Hsview* self){
        g_assert (self->layout==widget);
        if(!gtk_widget_is_focus(widget)){
            gtk_widget_grab_focus(widget);
        }

        if(event->type==GDK_2BUTTON_PRESS && event->button==1){
        }

        if(event->type==GDK_BUTTON_PRESS && event->button==1){
            if( event->y > self->header_height && event->y < self->height - self->footer_height && 
                event->x > self->line_col_width && event->x < self->width){
                int y = self->header_height;
                int i;
                for(i=0; i<self->drawn_lines.size(); ++i){
                    y += self->drawn_lines[i]->height;
                    if(y>=event->y) break;
                }
                if( i==self->drawn_lines.size() )
                    i =self->drawn_lines.size() - 1;   
                int x = int(event->x) - self->line_col_width + self->text_offset;
                self->set_line_of_focus(self->start_line + i);
                self->set_char_of_focus(x);
                gtk_widget_queue_draw(self->layout);
            }

        }else if(event->type==GDK_BUTTON_PRESS && event->button==3){
            if( event->y > self->header_height && event->y < self->height - self->footer_height){
                int y = self->header_height;
                int i;
                for(i=0; i<self->drawn_lines.size(); ++i){
                    y += self->drawn_lines[i]->height;
                    if(y>=event->y) break;
                }
                if( i==self->drawn_lines.size() )
                    i =self->drawn_lines.size() - 1;

                GtkWidget* menu = gtk_menu_new();
                GtkWidget* item = gtk_image_menu_item_new_with_mnemonic("_Add a mark to this line");
                g_signal_connect(item, "activate", G_CALLBACK(add_mark_from_menu_item_activation), (void*) (self->start_line+i));
                gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

                g_signal_connect(G_OBJECT(menu), "hide", G_CALLBACK(gtk_menu_detach), NULL);

                gtk_widget_show_all(menu);
                gtk_menu_attach_to_widget(GTK_MENU(menu), widget, NULL);
                gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, event->button, event->time);
            }
        }
        return TRUE;
    }

static
    gboolean hscrollbar_dragged(GtkWidget* widget, GdkEvent* event, Hsview* self){
        g_assert(self->h_scroll == (GTK_EVENT_BOX(widget)));
        static int y_begin = 0;
        if(event->type==GDK_ENTER_NOTIFY){
            self->on_h_scroll = 1;
            gtk_widget_queue_draw(self->layout);
        }else if(event->type==GDK_LEAVE_NOTIFY){
            self->on_h_scroll = 0;
            gtk_widget_queue_draw(self->layout);
        }else if(event->type==GDK_BUTTON_PRESS){
            GdkEventButton* button = (GdkEventButton*) event;
            if(button->button==1){
                self->h_scroll_drag_from = (int) button->x;
            }
        }else if(event->type==GDK_BUTTON_RELEASE){
            GdkEventButton* button = (GdkEventButton*) event;
            self->h_scroll_drag_from = 0;
            self->h_scroll_drag_to   = 0;
        }else if(event->type==GDK_MOTION_NOTIFY){
            GdkEventMotion* motion = (GdkEventMotion*) event;
            if(motion->state & GDK_BUTTON1_MASK){
                self->h_scroll_drag_to   = (int) motion->x;
                gtk_widget_queue_draw(self->layout);
            }
        }
        return TRUE;
    }

static
    gboolean vscrollbar_dragged(GtkWidget* widget, GdkEvent* event, Hsview* self){
        g_assert(self->v_scroll == (GTK_EVENT_BOX(widget)));
        static int y_begin = 0;
        if(event->type==GDK_ENTER_NOTIFY){
            GdkEventCrossing* button = (GdkEventCrossing*) event;
            int inner_offset = self->v_scroll_offset + self->v_scroll_height - (int)button->y;
            if((inner_offset>0) && inner_offset < self->v_scroll_height){
                if(!self->on_v_scroll){
                    self->on_v_scroll = 1;
                    gtk_widget_queue_draw(self->layout);
                }
            }
        }else if(event->type==GDK_LEAVE_NOTIFY){
            if((self->on_v_scroll)){
                self->on_v_scroll = 0;
                gtk_widget_queue_draw(self->layout);
            }
        }else if(event->type==GDK_BUTTON_PRESS){
            GdkEventButton* button = (GdkEventButton*) event;
            if(button->button==1){
                int inner_offset = self->v_scroll_offset + self->v_scroll_height - (int)button->y;
                if(inner_offset>0 && inner_offset < self->v_scroll_height){
                    self->v_scroll_drag_from = (int) button->y;
                    self->v_scroll_drag_from_offset = self->v_scroll_offset;
                }
            }
        }else if(event->type==GDK_BUTTON_RELEASE){
            self->v_scroll_drag_from = 0;
            self->v_scroll_drag_to   = 0;
        }else if(event->type==GDK_MOTION_NOTIFY && self->v_scroll_drag_from){
            GdkEventMotion* motion = (GdkEventMotion*) event;
            if(motion->state & GDK_BUTTON1_MASK){
                self->v_scroll_drag_to   = (int) motion->y;
                gtk_widget_queue_draw(self->layout);
            }
        }
        return TRUE;
    }

static
    gboolean key_pressed(GtkWidget* widget, GdkEventKey* event, Hsview* self){
        g_assert (self->layout==widget);
        if(event->type == GDK_KEY_PRESS){
            if(event->keyval == GDK_Page_Up){
                if(self->drawn_lines.size() >= 3){
                    self->start_line -= self->drawn_lines.size()-1;
                }else{
                    self->start_line --;
                }
            }else if(event->keyval == GDK_Page_Down){
                if(self->drawn_lines.size() >= 3){
                    self->start_line += self->drawn_lines.size()-1;
                }else{
                    self->start_line ++;
                }
            }else if(event->keyval == GDK_Down){
                self->start_line += 1;
            }else if(event->keyval == GDK_Up){
                self->start_line -= 1;
            }
            if( self->start_line >= app.file->get_number_of_lines() ){
                self->start_line =  app.file->get_number_of_lines()-1;
            }
            if( self->start_line < 0 )
                self->start_line = 0 ;
            gtk_widget_queue_draw(widget);
        }
        return TRUE;
    }

static
    gboolean set_line_from_entry(GtkWidget* entry, GdkEventKey* event, Hsview* self){
        const char* text = gtk_entry_get_text(GTK_ENTRY(entry));
        int len = strlen(text)+1;
        if(event->type == GDK_KEY_PRESS){
            if(event->keyval == GDK_KEY_Return){
                long line; char res[2];
                if(sscanf(text, " %ld %1s", &line, res)==1){
                    if(line<=1) line=1;
                    if(line>=app.file->get_number_of_lines())
                      line = app.file->get_number_of_lines();
                    char buffer[32]; sprintf(buffer, "%ld", line);
                    gtk_entry_set_text(GTK_ENTRY(entry), buffer);
                    gtk_editable_set_position(GTK_EDITABLE(entry), -1);
                    self->start_line = line-1;
                    gtk_widget_queue_draw(self->layout);
                    GdkColor color = {0, 255<<8, 255<<8, 255<<8};
                    gtk_widget_modify_base(GTK_WIDGET(entry), GTK_STATE_NORMAL, &color);
                }else{
                    GdkColor color = {0, 255<<8, 220<<8, 220<<8};
                    gtk_widget_modify_base(GTK_WIDGET(entry), GTK_STATE_NORMAL, &color);
                }
                return TRUE;
            }
        }else{
            gtk_entry_set_width_chars(GTK_ENTRY(entry), len);
        }
        return FALSE;
    }

};

gboolean search_forward_or_backward(GtkWidget* widget, GdkEventButton* event, HsvPattern* pattern, int direction){
    pattern->compile();
    if(!pattern->rawstr_set && !pattern->regexp_set){
        return TRUE;
    }
    const char* found = NULL;
    long i;
    const char* error = NULL;
    long end = app.file->get_number_of_lines();
    if(direction<0) end = 0;
    for(i=app.current_view->start_line+direction; i!=end; i+=direction){
        HsvLine line = app.file->get_line(i);
        const char* str = line.c_str();
        if(pattern->rawstr_set){
            if(pattern->casesn){
                found = strstr(str, pattern->rawstr);
            }else{
                found = strcasestr(str, pattern->rawstr);
            }
            if(found){
                app.current_view->start_line = i;
                break;
            }
        }else if(pattern->regexp_set){
            regmatch_t match;
            if(regexec(&pattern->regexp, str, 1, &match, 0)==0){
                if(match.rm_so==match.rm_eo){
                    error = "matches strings of zero length";
                    break;
                }
                found = str + match.rm_so;
                app.current_view->start_line = i;
                break;
            }
        }
    }
    if(app.current_view->searches->not_found){
        if(!strcmp(gtk_label_get_text(GTK_LABEL(pattern->error_msg)), "not found")){
            gtk_widget_set_size_request(pattern->error_msg, 0, 0);
        }
        app.current_view->searches->not_found = NULL;
    }
    if(!found){
        gtk_label_set_text(GTK_LABEL(pattern->error_msg), "not found");
        gtk_misc_set_alignment(GTK_MISC(pattern->error_msg), 1, 0);
        gtk_widget_set_size_request(pattern->error_msg, -1, -1);
        app.current_view->searches->not_found = pattern->error_msg;
        pattern->error_bg = 1;
    }
    if(error){
        gtk_label_set_text(GTK_LABEL(pattern->error_msg), error);
        gtk_misc_set_alignment(GTK_MISC(pattern->error_msg), 1, 0);
        gtk_widget_set_size_request(pattern->error_msg, -1, -1);
        pattern->error_bg = 1;
    }
    gtk_widget_queue_draw(app.hsview);
    return TRUE;
}

gboolean search_forward(GtkWidget* widget, GdkEventButton* event, HsvPattern* pattern){
    return search_forward_or_backward(widget, event, pattern, 1);
}

gboolean search_backward(GtkWidget* widget, GdkEventButton* event, HsvPattern* pattern){
    return search_forward_or_backward(widget, event, pattern, -1);
}

struct vu_mark{
    vu_mark*     parent;
    vu_mark*     prev;
    vu_mark*     next;
    vu_mark*     first_child;
    PangoLayout* layout;
    PangoLayout* linenum_layout;
    int          width;
    int          height;
    long         linenum;
    long         x;
    long         y;
    int          track;
    int          obstacle;
    unsigned int drag_dst_ancestor   : 1;
    unsigned int drag_dst_senior     : 1;
    unsigned int drag_src_descendant : 1;
    int          linenum_width;

    vu_mark(PangoContext* context, long _linenum, const char* label){
        parent = NULL;
        prev   = NULL;
        next   = NULL;
        first_child = NULL;
        layout = pango_layout_new(context);
        pango_layout_set_text(layout, label, -1);
        pango_layout_get_size(layout, &width, &height);
        width  /= PANGO_SCALE;
        height /= PANGO_SCALE;
        linenum = _linenum;
        x = y = 0;
        track = 0;
        obstacle = 0;
        drag_dst_ancestor   = 0;
        drag_dst_senior     = 0;
        drag_src_descendant = 0;

        char buf[32];
        sprintf(buf, "%ld", linenum+1);
        linenum_layout = pango_layout_new(context);
        pango_layout_set_text(linenum_layout, buf, -1);
        pango_layout_get_size(linenum_layout, &linenum_width, NULL);
        linenum_width /= PANGO_SCALE;
    }

    void attach(vu_mark* new_parent){
        if(prev) prev->next = next;
        if(next) next->prev = prev;
        if(parent && parent->first_child==this) parent->first_child = next;
        parent = new_parent;
        next   = NULL;
        prev   = NULL;
        if(!parent) return;
        if(!parent->first_child){
            parent->first_child = this; 
        }else{
            vu_mark* iter = parent->first_child;
            for(; iter->next; iter=iter->next){
                if(iter->next->linenum > linenum) break;
            }
            if(iter->linenum > linenum){
                g_assert(iter==parent->first_child);
                parent->first_child = this;
                next = iter;
                prev = NULL;
                next->prev = this;
            }else{
                g_assert(iter->linenum < linenum);
                prev = iter;
                next = iter->next;
                iter->next = this;
                if(next){
                    next->prev = this;
                    g_assert(next->linenum > linenum);
                }
            }
        }
    }

};

class HsvMarks{
    GtkWidget* layout;
    PangoFontDescription* font;
    map<long, vu_mark*> marks;

    long x_offset;
    long y_offset;
    long drag_from_x;
    long drag_from_y;
    long drag_to_x;
    long drag_to_y;
    vu_mark* drag_src;
    vu_mark* drag_dst;

    GtkWidget* rename_mi;
    GtkWidget* remove_mi;
    GtkWidget* rmchld_mi;

    GtkWidget* hscroll;
    GtkWidget* vscroll;
    int scroll_width;
    int scroll_height;
    int scroll_drag_from;
    int scroll_drag_from_offset;
    int hscroll_offset;
    int vscroll_offset;
    int hscroll_offset_max;
    int vscroll_offset_max;
    int hscroll_factor;
    int vscroll_factor;
    unsigned int pack_method      : 3;
    unsigned int show_linenum     : 1;
    unsigned int show_vscroll     : 1;
    unsigned int show_hscroll     : 1;
    unsigned int mouse_on_vscroll : 1;
    unsigned int mouse_on_hscroll : 1;
    unsigned int button_press_on_vscroll_area : 1;
    unsigned int button_press_on_hscroll_area : 1;

    static int const S = 10;

public:
    HsvMarks(){
        layout = gtk_layout_new(NULL, NULL);
        font   = pango_font_description_from_string(gtk_font_button_get_font_name(app.code_font_button));
        gtk_widget_add_events(layout, GDK_POINTER_MOTION_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
        g_signal_connect(layout,         "expose-event", G_CALLBACK(HsvMarks::exposed), this);
        g_signal_connect(layout,  "motion-notify-event", G_CALLBACK(HsvMarks::mouse_moved), this);
        g_signal_connect(layout,   "button-press-event", G_CALLBACK(HsvMarks::mouse_moved), this);
        g_signal_connect(layout, "button-release-event", G_CALLBACK(HsvMarks::mouse_moved), this);
        g_signal_connect(layout,      "key-press-event", G_CALLBACK(HsvMarks::key_pressed), this);
        drag_from_x = drag_from_y = drag_to_x = drag_to_y = -1; 
        x_offset = y_offset = 0;
        drag_src = drag_dst = NULL;
        rename_mi = remove_mi = rmchld_mi = NULL;
        g_signal_connect(layout, "destroy", G_CALLBACK(HsvMarks::destroy), this);
        hscroll = vscroll  = NULL;
        scroll_width       = scroll_height      =  0;
        mouse_on_vscroll   = mouse_on_hscroll   =  0;
        button_press_on_vscroll_area 
                           = button_press_on_hscroll_area 
                           = 0;
        scroll_drag_from   = scroll_drag_from_offset = -1;
        hscroll_offset     = vscroll_offset     =  0;
        hscroll_offset_max = vscroll_offset_max =  0;
        hscroll_factor     = vscroll_factor     =  1;
        show_vscroll       = show_hscroll       =  0;
        show_linenum       = 1;
        pack_method        = 2;
        gtk_widget_set_can_focus(layout, TRUE);
    }
   ~HsvMarks(){
        pango_font_description_free(font);
        map<long, vu_mark*>::iterator itr;
        for(itr=marks.begin(); itr!=marks.end(); ++itr){
            delete itr->second;
        }
    }
    GtkWidget* get_top_widget(){
        return layout;
    }
    void add_mark(long linenum){
        vu_mark*& mark = marks[linenum];
        if(!mark){
            PangoContext* context = gtk_widget_get_pango_context(layout);
            pango_context_set_font_description(context, font);
            HsvLine line = app.file->get_line(linenum);
            mark = new vu_mark(context, linenum, line.c_str());
        }
    }
    void remove_mark(long linenum){
        map<long, vu_mark*>::iterator itr = marks.find(linenum);
        if(itr!=marks.end()){
            vu_mark* mark = itr->second;
            g_assert(mark->linenum == linenum);
            marks.erase(itr);
            vu_mark* parent = mark->parent;
            mark->attach(NULL);
            vu_mark* child = mark->first_child;
            if(child){
                if(child->next){
                    while(child->next) child=child->next;
                    for(child=child->prev; child; child=child->prev){
                        child->next->attach(parent);
                    }
                }
                mark->first_child->attach(parent);
            }
            delete mark;
        }
    }
    void remove_recursively(vu_mark* mark){
        g_assert(mark); 
        while(mark->first_child){
            remove_recursively(mark->first_child);    
        }
        remove_mark(mark->linenum); 
    }

private:

static
    void destroy(GtkWidget* widget, HsvMarks* self){
        g_assert(widget == self->layout);
        delete self;
    }
static
    void rename_mi_activated(GtkWidget* mi, HsvMarks* self){
        vu_mark* mark = self->drag_dst;
        if(mark){
            GtkWidget* dialog = gtk_dialog_new_with_buttons("Edit mark", 
              GTK_WINDOW(app.top_window), GTK_DIALOG_MODAL,
              GTK_STOCK_APPLY, GTK_RESPONSE_APPLY, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
            GtkWidget* entry = gtk_entry_new();
            gtk_entry_set_text(GTK_ENTRY(entry), pango_layout_get_text(mark->layout));
            gtk_box_pack_start_defaults(GTK_BOX(GTK_DIALOG(dialog)->vbox), entry);
            gtk_widget_show_all(dialog);
            if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_APPLY){
                const char* new_label = gtk_entry_get_text(GTK_ENTRY(entry));
                if(new_label && strlen(new_label)){
                    pango_layout_set_text(mark->layout, new_label, -1);
                    pango_layout_get_size(mark->layout, &mark->width, &mark->height);
                    mark->width  /= PANGO_SCALE;
                    mark->height /= PANGO_SCALE;
                }
            }
            gtk_widget_destroy(dialog);
            gtk_widget_queue_draw(self->layout);
        }
    }
static
    void remove_mi_activated(GtkWidget* mi, HsvMarks* self){
        if(self->drag_dst){
            self->remove_mark(self->drag_dst->linenum);
            gtk_widget_queue_draw(self->layout);
        }
    }
static
    void rmchld_mi_activated(GtkWidget* mi, HsvMarks* self){
        vu_mark* mark = self->drag_dst;
        while(mark && mark->first_child){
            self->remove_recursively(mark->first_child);
        }
        gtk_widget_queue_draw(self->layout);
    }
static
    void toggle_linenum(GtkWidget* mi, HsvMarks* self){
        self->show_linenum = !self->show_linenum;
        gtk_widget_queue_draw(self->layout);
    }
static
    void toggle_packmethod(GtkWidget* mi, HsvMarks* self){
        self->pack_method = self->pack_method + 1;
        gtk_widget_queue_draw(self->layout);
    }
static
    gboolean exposed(GtkWidget* widget, GdkEventExpose* event, HsvMarks* self){
        g_assert (self->layout==widget);
        GdkWindow* window = GTK_LAYOUT(self->layout)->bin_window;
        cairo_t*   cr     = gdk_cairo_create(window); 
        GtkStyle*  style  = widget->style;
        GdkColor      light_color = style->light[GTK_STATE_NORMAL];
        GdkColor       dark_color = style->dark[GTK_STATE_NORMAL];
        GdkColor   selected_color = style->bg[GTK_STATE_SELECTED];
        GdkColor         bg_color = style->bg[GTK_STATE_NORMAL];
        cairo_set_line_width(cr, 1);

        int win_width  = widget->allocation.width;
        int win_height = widget->allocation.height;

        vector<bool> track_used;
        vector<long> track_end;
        track_used.push_back(true);
        track_end.push_back(0);

        long total_height = 0;

        self->y_offset = self->vscroll_offset * self->vscroll_factor;
        if(self->vscroll_offset < 0) self->y_offset = (int) -sqrt(-self->vscroll_offset);
        if(self->vscroll_offset > self->vscroll_offset_max){
            self->y_offset = self->vscroll_offset_max * self->vscroll_factor;
        }
        long max_width = 0;
        int  max_linenum_width = 0;

        long x=60, y=3;
        map<long, vu_mark*>::iterator itr, end = self->marks.end();
        map<long, vu_mark*>::iterator begin = end;
        int found_begin = 0;
        self->drag_dst  = NULL;
        for(itr=self->marks.begin(); itr!=end; ++itr){
            vu_mark* mark = itr->second;

            if(max_width < mark->width)
                max_width = mark->width;
            if(max_linenum_width < mark->linenum_width)
                max_linenum_width = mark->linenum_width;

            int height = mark->height;
            total_height += height;

            mark->x = 0;
            mark->y = y;
            if(mark->parent){
                mark->x = mark->parent->x + 1;
            }

            mark->drag_dst_ancestor   = 0;
            mark->drag_dst_senior     = 0;
            mark->drag_src_descendant = 0;
            if(self->drag_from_x>-1 && !self->drag_src){
                if(y < self->drag_from_y && (y+height) > self->drag_from_y){
                    self->drag_src = mark;
                }
            }
            if(mark==self->drag_src) 
                mark->drag_src_descendant = 1;
            if(mark->parent && mark->parent->drag_src_descendant)
                mark->drag_src_descendant = 1;
            if(self->drag_to_x>-1 && !self->drag_dst){
                if(y < self->drag_to_y && (y+height) > self->drag_to_y){
                    self->drag_dst = mark;
                    for(vu_mark* senior=mark; senior; senior=senior->prev)
                        senior->drag_dst_senior = 1;
                    for(vu_mark* parent=mark->parent; parent; parent=parent->parent){
                        parent->drag_dst_ancestor = 1;
                        for(vu_mark* senior=parent; senior; senior=senior->prev)
                            senior->drag_dst_senior = 1;
                    }
                    if(self->remove_mi && !GTK_WIDGET_SENSITIVE(self->remove_mi)){
                        g_signal_connect(self->rename_mi, "activate", G_CALLBACK(HsvMarks::rename_mi_activated), self); 
                        g_signal_connect(self->remove_mi, "activate", G_CALLBACK(HsvMarks::remove_mi_activated), self); 
                        g_signal_connect(self->rmchld_mi, "activate", G_CALLBACK(HsvMarks::rmchld_mi_activated), self); 
                        gtk_widget_set_sensitive(self->rename_mi, TRUE);
                        gtk_widget_set_sensitive(self->remove_mi, TRUE);
                        gtk_widget_set_sensitive(self->rmchld_mi, mark->first_child!=NULL);
                    }
                }
            }

            y += height;

            if(!found_begin && y > self->y_offset){
                found_begin = 1;
                begin = itr;
            }

            #if 0
            #else
            if(mark->prev){
                mark->track = mark->prev->track;
                g_assert(mark->parent);
                g_assert(mark->parent == mark->prev->parent);
            }else if(!mark->parent){
                g_assert(!mark->prev && !mark->next);
                mark->track = 0;
            }
            if(mark->first_child){
                mark->first_child->track = 0;
                for(int i=mark->track+1; i<track_used.size(); ++i){
                    if(!track_used[i]){
                        track_used[i] = true;
                        mark->first_child->track = i;
                        break;
                    }
                }
                if(!mark->first_child->track){
                    mark->first_child->track = track_used.size();
                    track_used.push_back(true);
                    track_end.push_back(0);
                }
            }
            #endif

            mark->obstacle = track_used.size();
            for(int i=track_used.size()-1; !track_used[i]; --i){
                mark->obstacle = i;
            }
            if(mark->parent && mark->obstacle <= mark->parent->obstacle){
                mark->obstacle = mark->parent->obstacle+1;
            }

            if(mark->parent && !mark->next){
                track_used[mark->track] = false;
                track_end [mark->track] = mark->linenum;
            }
        }
        self->remove_mi = NULL;

        if(self->drag_dst && self->drag_src){
            if(self->drag_dst->linenum > self->drag_src->linenum)
                self->drag_dst = NULL;
        }

        for(itr=begin; itr!=end; ++itr){
            vu_mark* mark = itr->second;

            switch(self->pack_method = self->pack_method % 4){
                case 1:
                mark->x = 15;
                if(mark->first_child){
                    mark->x = mark->first_child->track*15 + 15;
                }else{
                    if(mark->parent){
                        mark->x = mark->parent->x + 15;
                    }
                }
                break;
                case 2:
                mark->x = mark->obstacle*15;
                break;
                case 3:
                if(mark->prev && mark->obstacle < mark->prev->obstacle){
                    mark->obstacle = mark->prev->obstacle;
                }
                mark->x = mark->obstacle*15;
                break;
                default:
                mark->x = mark->x*15 + track_used.size()*15;
            }

            if(self->show_linenum){
                mark->x += max_linenum_width;
            }

            long x = mark->x - self->x_offset;
            long y = mark->y - self->y_offset;

            int width  = mark->width;
            int height = mark->height;

            if(mark==self->drag_src){
                gdk_cairo_set_source_color(cr, &selected_color);
                cairo_rectangle(cr, x, y+1, width, height-2);
                cairo_fill(cr);
            }
            if(mark==self->drag_dst){
                gdk_cairo_set_source_color(cr, &selected_color);
                cairo_rectangle(cr, x-0.5, y-0.5, width, height);
                cairo_stroke(cr);
            }

            if(y < win_height){
                cairo_move_to(cr, x, y); 
                cairo_set_source_rgb(cr, 0, 0, 0);
                pango_cairo_show_layout(cr, mark->layout);
                if(self->show_linenum){
                    cairo_move_to(cr, max_linenum_width-mark->linenum_width-self->x_offset, y);
                    pango_cairo_show_layout(cr, mark->linenum_layout);
                }
            }else{
                if(mark->parent){
                    if(mark->parent->y > self->y_offset + win_height) continue;
                }else{
                    continue;
                }
            }

            y += height/2;
            if(mark->parent){
                int track_x = mark->track*15 - self->x_offset;
                track_x += self->show_linenum?max_linenum_width:0;

                int track_y_from = mark->parent->y + mark->parent->height/2 - self->y_offset;
                if(mark->prev){
                    track_y_from = mark->prev->y + mark->prev->height/2 - self->y_offset;
                }

                gdk_cairo_set_source_color(cr, &dark_color);
                if(mark->drag_src_descendant && mark!=self->drag_src)
                    gdk_cairo_set_source_color(cr, &selected_color);
                if(mark->drag_dst_senior)
                    gdk_cairo_set_source_color(cr, &selected_color);

                cairo_move_to(cr, track_x + 0.5, track_y_from);
                cairo_line_to(cr, track_x + 0.5, y); 
                cairo_stroke(cr);
                gdk_cairo_set_source_color(cr, &bg_color);
                cairo_move_to(cr, track_x - 0.5, track_y_from + 1);
                cairo_line_to(cr, track_x - 0.5, y); 
                cairo_stroke(cr);
                cairo_set_line_width(cr, 2);
                cairo_move_to(cr, track_x + 2, track_y_from + 1);
                cairo_line_to(cr, track_x + 2, y); 
                cairo_stroke(cr);
                cairo_set_line_width(cr, 1);

                if(mark==self->drag_dst || mark->drag_src_descendant)
                    gdk_cairo_set_source_color(cr, &selected_color);
                else
                    gdk_cairo_set_source_color(cr, &dark_color);
                cairo_move_to(cr, track_x, y+0.5); 
                cairo_line_to(cr, x, y+0.5); 
                cairo_stroke(cr);
                gdk_cairo_set_source_color(cr, &light_color);
                cairo_move_to(cr, track_x, y+1.5); 
                cairo_line_to(cr, x, y+1.5); 
                cairo_stroke(cr);
                if(mark->drag_dst_ancestor){
                    gdk_cairo_set_source_color(cr, &selected_color);
                    int x = mark->first_child->track*15 - self->x_offset;
                        x+= self->show_linenum?max_linenum_width:0;
                    cairo_move_to(cr, track_x, y+0.5); 
                    cairo_line_to(cr, x, y+0.5); 
                    cairo_stroke(cr);
                }

            }else{
                if(mark==self->drag_dst || mark->drag_src_descendant)
                    gdk_cairo_set_source_color(cr, &selected_color);
                else
                    gdk_cairo_set_source_color(cr, &dark_color);
                int gutter_width = self->show_linenum?max_linenum_width:0;
                cairo_move_to(cr, gutter_width - self->x_offset, y+0.5); 
                cairo_line_to(cr, x, y+0.5); 
                cairo_stroke(cr);
                gdk_cairo_set_source_color(cr, &light_color);
                cairo_move_to(cr, gutter_width - self->x_offset, y+1.5);
                cairo_line_to(cr, x, y+1.5); 
                cairo_stroke(cr);
                if(mark->drag_dst_ancestor){
                    gdk_cairo_set_source_color(cr, &selected_color);
                    int track_x = mark->first_child->track*15 - self->x_offset;
                        track_x+= self->show_linenum?max_linenum_width:0;
                    cairo_move_to(cr, gutter_width + 0, y+0.5); 
                    cairo_line_to(cr, track_x, y+0.5); 
                    cairo_stroke(cr);
                }
            }

        }
        
        max_width += track_used.size()*15;
        max_width += self->show_linenum?max_linenum_width:0;

        if(self->drag_to_x != -1 && self->drag_from_x != -1 && self->drag_src){
            gdk_cairo_set_source_color(cr, &selected_color);
            if(self->drag_dst){
                cairo_move_to(cr, self->drag_from_x+0.5 - self->x_offset, self->drag_from_y + 0.5 - self->y_offset);
                cairo_line_to(cr, self->drag_to_x  +0.5 - self->x_offset, self->drag_to_y   + 0.5 - self->y_offset);
            }else if(self->drag_to_y > self->drag_src->y){
                cairo_move_to(cr, self->drag_src->x - self->x_offset, self->drag_src->y - self->y_offset + self->drag_src->height/2 + 0.5);
                cairo_line_to(cr,                 0 - self->x_offset, self->drag_src->y - self->y_offset + self->drag_src->height/2 + 0.5);
            }
            cairo_stroke(cr);
        }

        if(!self->hscroll){
            self->hscroll = gtk_event_box_new();
            self->vscroll = gtk_event_box_new();
            gtk_event_box_set_above_child(GTK_EVENT_BOX(self->hscroll), TRUE);
            gtk_event_box_set_above_child(GTK_EVENT_BOX(self->vscroll), TRUE);
            gtk_event_box_set_visible_window(GTK_EVENT_BOX(self->hscroll), FALSE);
            gtk_event_box_set_visible_window(GTK_EVENT_BOX(self->vscroll), FALSE);
            self->scroll_width  = win_width  - S;
            self->scroll_height = win_height - S;
            if(self->scroll_width  < 0) self->scroll_width  = 0;
            if(self->scroll_height < 0) self->scroll_height = 0;
            gtk_widget_set_size_request(self->hscroll, self->scroll_width, S);
            gtk_widget_set_size_request(self->vscroll, S, self->scroll_height);
            gtk_layout_put(GTK_LAYOUT(self->layout), self->hscroll, 0, self->scroll_height);
            gtk_layout_put(GTK_LAYOUT(self->layout), self->vscroll, self->scroll_width, 0);
            g_signal_connect(self->hscroll, "motion-notify-event", G_CALLBACK(HsvMarks::hscroll_dragged), self);
            g_signal_connect(self->hscroll,  "button-press-event", G_CALLBACK(HsvMarks::hscroll_dragged), self);
            g_signal_connect(self->hscroll,"button-release-event", G_CALLBACK(HsvMarks::hscroll_dragged), self);
            g_signal_connect(self->hscroll,  "enter-notify-event", G_CALLBACK(HsvMarks::hscroll_dragged), self);
            g_signal_connect(self->hscroll,  "leave-notify-event", G_CALLBACK(HsvMarks::hscroll_dragged), self);
            g_signal_connect(self->vscroll, "motion-notify-event", G_CALLBACK(HsvMarks::vscroll_dragged), self);
            g_signal_connect(self->vscroll,  "button-press-event", G_CALLBACK(HsvMarks::vscroll_dragged), self);
            g_signal_connect(self->vscroll,"button-release-event", G_CALLBACK(HsvMarks::vscroll_dragged), self);
            g_signal_connect(self->vscroll,  "enter-notify-event", G_CALLBACK(HsvMarks::vscroll_dragged), self);
            g_signal_connect(self->vscroll,  "leave-notify-event", G_CALLBACK(HsvMarks::vscroll_dragged), self);
            g_signal_connect(self->layout ,        "scroll-event", G_CALLBACK(HsvMarks::vscroll_dragged), self);
            gtk_widget_show(self->hscroll);
            gtk_widget_show(self->vscroll);
        }
        int win_resized = 0;
        if(self->scroll_width != win_width-S || self->scroll_height != win_height-S){
            self->scroll_width  = win_width  - S;
            self->scroll_height = win_height - S;
            if(self->scroll_width  < 0) self->scroll_width  = 0;
            if(self->scroll_height < 0) self->scroll_height = 0;
            gtk_widget_set_size_request(self->hscroll, self->scroll_width, S);
            gtk_widget_set_size_request(self->vscroll, S, self->scroll_height);
            gtk_layout_move(GTK_LAYOUT(self->layout), self->hscroll, 0, self->scroll_height);
            gtk_layout_move(GTK_LAYOUT(self->layout), self->vscroll, self->scroll_width, 0);
            win_resized = 1;
        }
        if(win_height < 2*S || win_width < 2*S){
            self->show_hscroll = 0;
        }else{
            if(max_width > win_width-S){
                self->show_hscroll = 1;
            }else if(self->button_press_on_hscroll_area){
                self->show_hscroll = 1; 
            }else{
                self->show_hscroll = 0;
            }
        }
        if(self->show_hscroll){
            int w = (win_width-S)*(win_width-S)/max_width; 
            if(!self->button_press_on_hscroll_area){
                self->hscroll_factor = (max_width+win_width-S-1)/(win_width-S);
                self->hscroll_offset_max = (win_width-S)-w;
                if(win_resized){
                    self->hscroll_offset = self->x_offset / self->hscroll_factor;
                }
            }
            gdk_cairo_set_source_color(cr, &dark_color);
            if(self->button_press_on_hscroll_area)
                gdk_cairo_set_source_color(cr, &selected_color);
            int shrink = (self->mouse_on_hscroll||self->button_press_on_hscroll_area)?0:S/2;
            int x = self->hscroll_offset;
            if(x < 0) x = 0;
            if(x > self->hscroll_offset_max) x = self->hscroll_offset_max;
            cairo_rectangle(cr, x, self->scroll_height+shrink, self->scroll_width - self->hscroll_offset_max, S-1-shrink);
            cairo_fill(cr);
        }
        if(win_height < 2*S || win_width < 2*S){
            self->show_vscroll = 0;
        }else{
            if(total_height > win_height-S){
                self->show_vscroll = 1;
            }else if(self->button_press_on_vscroll_area){
                self->show_vscroll = 1; 
            }else{
                self->show_vscroll = 0;
            }
        }
        if(self->show_vscroll){
            int h = (win_height-S)*(win_height-S)/total_height; 
            if(h<1) h = 1;
            if(!self->button_press_on_vscroll_area){
                self->vscroll_offset_max = (win_height-S)-h;
                g_assert(self->vscroll_offset_max > 0);
                self->vscroll_factor     = (total_height - (win_height-S) + self->vscroll_offset_max - 1)/self->vscroll_offset_max;
                if(win_resized){
                    self->vscroll_offset = self->y_offset / self->vscroll_factor;
                    if(self->vscroll_offset > self->vscroll_offset_max)
                        self->vscroll_offset = self->vscroll_offset_max;
                }
            }
            gdk_cairo_set_source_color(cr, &dark_color);
            if(self->button_press_on_vscroll_area)
                gdk_cairo_set_source_color(cr, &selected_color);
            int shrink = (self->mouse_on_vscroll||self->button_press_on_vscroll_area)?0:S/2;
            int y = self->vscroll_offset;
            if(y < 0) y = 0;
            if(y > self->vscroll_offset_max) y = self->vscroll_offset_max;
            cairo_rectangle(cr, self->scroll_width+shrink, y, S-1-shrink, self->scroll_height - self->vscroll_offset_max);
            cairo_fill(cr);
        }else{
            self->vscroll_offset = 0;
        }

        if(self->marks.size()==0){
            gdk_cairo_set_source_color(cr, &dark_color);
            cairo_move_to(cr, 0, win_height-5);
            cairo_show_text(cr, "(Right-click on a line to add)");
        }

        cairo_destroy(cr);
        return FALSE;
    }
static
    gboolean hscroll_dragged(GtkWidget* widget, GdkEvent* event, HsvMarks* self){
        if(event->type==GDK_ENTER_NOTIFY){
            self->mouse_on_hscroll = 1;
        }else if(event->type==GDK_LEAVE_NOTIFY){
            self->mouse_on_hscroll = 0;
        }else if(event->type==GDK_BUTTON_PRESS && event->button.button == 1){
            if(self->show_hscroll){
                self->button_press_on_hscroll_area = 1;
                self->scroll_drag_from        = int(event->button.x);
                self->scroll_drag_from_offset = self->hscroll_offset;
            }
        }else if(event->type==GDK_BUTTON_RELEASE && event->button.button == 1){
            self->button_press_on_hscroll_area = 0;
            if(self->hscroll_offset < 0){
                self->hscroll_offset = 0;
                self->x_offset = 0;
            }
            self->scroll_drag_from = -1;
        }else if(event->type==GDK_MOTION_NOTIFY && event->motion.state&GDK_BUTTON1_MASK){
            g_assert(self->scroll_drag_from > -1);
            if(self->show_hscroll){
                self->hscroll_offset = int(event->motion.x) - self->scroll_drag_from + self->scroll_drag_from_offset;
                self->x_offset = self->hscroll_offset * self->hscroll_factor;
                if(self->hscroll_offset < 0) self->x_offset = (int) -sqrt(-self->hscroll_offset);
                if(self->hscroll_offset > self->hscroll_offset_max){
                    self->x_offset = self->hscroll_offset_max * self->hscroll_factor;
                }
            }
        }
        gtk_widget_queue_draw(self->layout);
        return TRUE;
    }
static
    gboolean vscroll_dragged(GtkWidget* widget, GdkEvent* event, HsvMarks* self){
        if(event->type==GDK_ENTER_NOTIFY){
            self->mouse_on_vscroll = 1;
        }else if(event->type==GDK_LEAVE_NOTIFY){
            self->mouse_on_vscroll = 0;
        }else if(event->type==GDK_BUTTON_PRESS && event->button.button == 1){
            if(self->show_vscroll){
                self->button_press_on_vscroll_area = 1;
                self->scroll_drag_from        = int(event->button.y);
                self->scroll_drag_from_offset = self->vscroll_offset;
            }
        }else if(event->type==GDK_BUTTON_RELEASE && event->button.button == 1){
            self->button_press_on_vscroll_area = 0;
            self->scroll_drag_from = -1;
        }else if(event->type==GDK_MOTION_NOTIFY && event->motion.state&GDK_BUTTON1_MASK){
            if(self->show_vscroll){
                g_assert(self->scroll_drag_from > -1);
                self->vscroll_offset = int(event->motion.y) - self->scroll_drag_from + self->scroll_drag_from_offset;
                if(self->vscroll_offset <0) self->vscroll_offset = 0;
                if(self->vscroll_offset > self->vscroll_offset_max)
                    self->vscroll_offset = self->vscroll_offset_max;
                self->y_offset = self->vscroll_offset * self->vscroll_factor;
            }
        }else if(event->type==GDK_SCROLL){
            g_assert(self->layout==widget);
            if(self->show_vscroll){
                g_assert(self->vscroll_factor>0);
                if(event->scroll.direction == GDK_SCROLL_UP){
                    self->vscroll_offset--;
                    self->vscroll_offset-=self->scroll_height/4/self->vscroll_factor;
                    if(self->vscroll_offset < 0)
                        self->vscroll_offset = 0;
                    self->y_offset = self->vscroll_offset * self->vscroll_factor;
                }else if(event->scroll.direction == GDK_SCROLL_DOWN){
                    self->vscroll_offset++;
                    self->vscroll_offset+=self->scroll_height/4/self->vscroll_factor;
                    if(self->vscroll_offset > self->vscroll_offset_max)
                        self->vscroll_offset = self->vscroll_offset_max;
                    self->y_offset = self->vscroll_offset * self->vscroll_factor;
                }
            }
        }
        gtk_widget_queue_draw(self->layout);
        return TRUE;
    }
static
    gboolean mouse_moved(GtkWidget* widget, GdkEvent* event, HsvMarks* self){
        static int y_begin = 0;
        if(event->type==GDK_ENTER_NOTIFY){
        }else if(event->type==GDK_LEAVE_NOTIFY){
        }else if(event->type==GDK_2BUTTON_PRESS){
            vu_mark* mark = self->drag_dst;
            if(mark){
                app.current_view->start_line = mark->linenum;
                gtk_widget_queue_draw(app.current_view->get_top_widget());
            }
        }else if(event->type==GDK_BUTTON_PRESS){
            if(!gtk_widget_is_focus(widget)){
                gtk_widget_grab_focus(widget);
            }
            GdkEventButton* button = (GdkEventButton*) event;
            if(button->button == 1 && !(button->state&GDK_BUTTON3_MASK)){
                self->drag_from_x = self->x_offset + (int) button->x;
                self->drag_from_y = self->y_offset + (int) button->y;
                self->drag_to_x = -1;
                self->drag_to_y = -1;
            }else if(button->button == 3 && !(button->state&GDK_BUTTON1_MASK)){
                self->drag_to_x = self->x_offset + (long) button->x;
                self->drag_to_y = self->y_offset + (long) button->y;

                GtkWidget*   menu = gtk_menu_new(); 
                self->rename_mi = gtk_image_menu_item_new_with_mnemonic("_Edit");
                self->remove_mi = gtk_image_menu_item_new_with_mnemonic("_Delete");
                self->rmchld_mi = gtk_image_menu_item_new_with_mnemonic("Delete _childrens");
                g_signal_connect(self->rename_mi, "destroy", G_CALLBACK(gtk_widget_destroyed), &(self->rename_mi));
                g_signal_connect(self->remove_mi, "destroy", G_CALLBACK(gtk_widget_destroyed), &(self->remove_mi));
                g_signal_connect(self->rmchld_mi, "destroy", G_CALLBACK(gtk_widget_destroyed), &(self->rmchld_mi));

                g_signal_connect_swapped(self->rename_mi, "destroy", G_CALLBACK(puts), (void*) "rename_mi destroyed");

                gtk_widget_set_sensitive(self->rename_mi, FALSE);
                gtk_widget_set_sensitive(self->remove_mi, FALSE);
                gtk_widget_set_sensitive(self->rmchld_mi, FALSE);
                gtk_menu_shell_append(GTK_MENU_SHELL(menu), self->rename_mi);
                gtk_menu_shell_append(GTK_MENU_SHELL(menu), self->remove_mi);
                gtk_menu_shell_append(GTK_MENU_SHELL(menu), self->rmchld_mi);
                gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
                GtkWidget* show_linenum = gtk_image_menu_item_new_with_mnemonic(
                  self->show_linenum?"Hide _line numbers":"Show _line numbers");
                g_signal_connect(show_linenum, "activate", G_CALLBACK(HsvMarks::toggle_linenum), self);
                gtk_menu_shell_append(GTK_MENU_SHELL(menu), show_linenum);

                g_signal_connect(G_OBJECT(menu), "hide", G_CALLBACK(gtk_menu_detach), NULL);

                gtk_widget_show_all(menu);
                gtk_menu_attach_to_widget(GTK_MENU(menu), widget, NULL);
                gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, button->button, button->time);

            }else if(button->button == 3 && (button->state&GDK_BUTTON1_MASK)){
                self->drag_from_x = self->drag_from_y = self->drag_to_x = self->drag_to_y = -1;
                self->drag_src = self->drag_dst = NULL;
            }
        }else if(event->type==GDK_BUTTON_RELEASE){
            GdkEventButton* button = (GdkEventButton*) event;
            if(button->button == 1){
                if(self->drag_src && self->drag_src != self->drag_dst && self->drag_to_x != -1
                                                                      && self->drag_to_y != -1){
                    if(self->drag_dst){ 
                        self->drag_src->attach(self->drag_dst);
                    }else{
                        if(self->drag_to_y > self->drag_src->y){
                            self->drag_src->attach(NULL);
                        }
                    }
                }
                #define MARK_KEEP_HIGHLIGHT
                #ifndef MARK_KEEP_HIGHLIGHT
                self->drag_from_x = self->drag_from_y = self->drag_to_x = self->drag_to_y = -1;
                self->drag_src = self->drag_dst = NULL;
                #else
                self->drag_to_x = self->drag_from_x;
                self->drag_to_y = self->drag_from_y;
                self->drag_from_x = self->drag_from_y = -1;
                self->drag_src  = self->drag_dst = NULL;
                #endif
            }else if(button->button == 3 && !(button->state&GDK_BUTTON1_MASK)){
                self->drag_from_x = self->drag_from_y = self->drag_to_x = self->drag_to_y = -1;
                self->drag_src = self->drag_dst = NULL;
            }
        }else if(event->type==GDK_MOTION_NOTIFY){
            GdkEventMotion* motion = (GdkEventMotion*) event;
            if(motion->state & GDK_BUTTON1_MASK){
                if(self->drag_from_x != -1 && self->drag_from_y != -1){
                    self->drag_to_x = self->x_offset + (long) motion->x;
                    self->drag_to_y = self->y_offset + (long) motion->y;
                    if(self->show_vscroll){
                        if(motion->y < 15 && self->vscroll_offset > 0){
                            self->vscroll_offset -= (15-int(motion->y)+1)/2;
                            if(self->vscroll_offset < 0)
                                self->vscroll_offset = 0;
                        }
                        if(motion->y > self->scroll_height - 15){
                            self->vscroll_offset += (int(motion->y) + 15 - self->scroll_height)/2;
                            if(self->vscroll_offset > self->vscroll_offset_max)
                                self->vscroll_offset = self->vscroll_offset_max;
                        }
                    }
                }
            }
        }
        gtk_widget_queue_draw(widget);
        return TRUE;
    }
static
    gboolean key_pressed(GtkWidget* widget, GdkEventKey* event, HsvMarks* self){
        g_assert (self->layout==widget);
        if(self->drag_dst && self->drag_to_y>-1){
            if(event->keyval == GDK_KEY_Return || event->keyval == GDK_KP_Enter || event->keyval == GDK_space){
                app.current_view->start_line = self->drag_dst->linenum;
                gtk_widget_queue_draw(app.current_view->get_top_widget());
            }else if(event->keyval == GDK_j || event->keyval == GDK_J || event->keyval == GDK_Down){
                map<long, vu_mark*>::iterator itr = self->marks.find(self->drag_dst->linenum); 
                if(itr==self->marks.end() || itr->second != self->drag_dst)
                    return TRUE;
                if(++itr != self->marks.end()){
                    self->drag_to_y += itr->second->y - self->drag_dst->y;
                }
            }else if(event->keyval == GDK_k || event->keyval == GDK_K || event->keyval == GDK_Up){
                map<long, vu_mark*>::iterator itr = self->marks.find(self->drag_dst->linenum); 
                if(itr==self->marks.end() || itr->second != self->drag_dst)
                    return TRUE;
                if(itr != self->marks.begin()){
                    --itr;
                    self->drag_to_y += itr->second->y - self->drag_dst->y;
                }
            }else if(event->keyval == GDK_h || event->keyval == GDK_H || event->keyval == GDK_Left){
                if(self->drag_dst->prev){
                    self->drag_to_y += self->drag_dst->prev->y - self->drag_dst->y;
                }else if(self->drag_dst->parent){
                    self->drag_to_y += self->drag_dst->parent->y - self->drag_dst->y;
                }else{
                    map<long, vu_mark*>::iterator itr = self->marks.find(self->drag_dst->linenum); 
                    if(itr==self->marks.end() || itr->second != self->drag_dst)
                        return TRUE;
                    while(itr!=self->marks.begin()){
                        --itr;
                        if(!itr->second->parent){
                            self->drag_to_y += itr->second->y - self->drag_dst->y;
                            break;
                        }
                    }
                }
            }else if(event->keyval == GDK_l || event->keyval == GDK_L || event->keyval == GDK_Right){
                if(self->drag_dst->first_child){
                    self->drag_to_y += self->drag_dst->first_child->y - self->drag_dst->y;
                }else if(self->drag_dst->next){
                    self->drag_to_y += self->drag_dst->next->y - self->drag_dst->y;
                }else{
                    vu_mark* parent = self->drag_dst->parent;
                    while(parent && !parent->next) parent=parent->parent;
                    if(parent){
                        g_assert(parent->next);
                        self->drag_to_y += parent->next->y - self->drag_dst->y;
                    }else{
                        map<long, vu_mark*>::iterator itr = self->marks.find(self->drag_dst->linenum); 
                        if(itr==self->marks.end() || itr->second != self->drag_dst)
                            return TRUE;
                        for(itr++; itr!=self->marks.end(); ++itr){ 
                            if(!itr->second->parent){
                                self->drag_to_y += itr->second->y - self->drag_dst->y;
                                break;
                            }
                        }
                    }
                }
            }
            gtk_widget_queue_draw(widget);
        }
        return TRUE;
    }

};

void add_mark_from_menu_item_activation(GtkWidget* item, long linenum){
    app.current_view->marks->add_mark(linenum);
    gtk_widget_queue_draw(app.current_view->marks->get_top_widget());
}

GtkWidget* VU_new_page(GtkWidget* page){

    GtkWidget* hpaned = gtk_hpaned_new();
    HsvMarks* marks = new HsvMarks();
    GtkWidget* mark_frame = gtk_frame_new("Marks");
    gtk_widget_set_size_request(mark_frame, -1, app.window_height/3);
    gtk_container_add(GTK_CONTAINER(mark_frame), marks->get_top_widget());
    if(0){
        gtk_paned_add1(GTK_PANED(hpaned), mark_frame);
        mark_frame = NULL;
    }
    Hsview* view = new Hsview();
    gtk_widget_set_size_request(view->top, app.window_width*2/3, app.window_height*2/3);
    gtk_paned_add2(GTK_PANED(hpaned), view->top);
    app.current_view = view;

    GtkWidget* hl_pane = gtk_vpaned_new(); 
    gtk_paned_add1(GTK_PANED(hl_pane), hpaned);
    HsvSearch* search = new HsvSearch();
    gtk_paned_add2(GTK_PANED(hl_pane), search->get_top_widget());

    view->searches = search;
    view->marks    = marks;
    search->set_hsview(view->top);

    GtkWidget* top_pane = gtk_hpaned_new();
    GtkWidget* console_pane = gtk_vpaned_new();
    gtk_paned_add1(GTK_PANED(top_pane), hl_pane);
    gtk_paned_add2(GTK_PANED(top_pane), console_pane);
#if 1
    GtkWidget* console = new_console();
#else
    GtkWidget* console = gtk_label_new("console...");
#endif
    if(mark_frame){
        gtk_paned_add1(GTK_PANED(console_pane), mark_frame);
    }
    gtk_paned_add2(GTK_PANED(console_pane), console);

    g_object_set(G_OBJECT(page), "user-data", view, NULL);

    return top_pane;
}

void new_blank_page(){
    GtkWidget* page = gtk_vbox_new(FALSE, 0);
    GtkWidget* img_add  = gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_MENU);
    gint idx = gtk_notebook_append_page(GTK_NOTEBOOK(app.notebook), app.new_page=page, img_add);
    app.new_page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(app.notebook), idx);
    gtk_widget_show_all(app.notebook);
}

gboolean label_clicked(GtkWidget* widget, GdkEvent* event, GtkWidget* label){
    if(event->type==GDK_2BUTTON_PRESS){
        GtkWidget* dialog = gtk_dialog_new_with_buttons("Rename page...", 
          GTK_WINDOW(app.top_window), GTK_DIALOG_MODAL,
          GTK_STOCK_APPLY, GTK_RESPONSE_APPLY, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
        GtkWidget* entry = gtk_entry_new();
        gtk_entry_set_text(GTK_ENTRY(entry), gtk_label_get_text(GTK_LABEL(label)));
        gtk_box_pack_start_defaults(GTK_BOX(GTK_DIALOG(dialog)->vbox), entry);
        gtk_widget_show_all(dialog);
        if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_APPLY){
            gtk_label_set_text(GTK_LABEL(label),
              gtk_entry_get_text(GTK_ENTRY(entry)));
        }
        gtk_widget_destroy(dialog);
    }
    return FALSE;
}

gboolean close_page_pressed(GtkWidget* widget, GdkEvent* event, GtkWidget* page){
    if(event->type==GDK_BUTTON_PRESS){
        gint        curr_idx = gtk_notebook_get_current_page(GTK_NOTEBOOK(app.notebook));
        GtkWidget*  curr_pag = gtk_notebook_get_nth_page(GTK_NOTEBOOK(app.notebook), curr_idx);
        gint        numb_pag = gtk_notebook_get_n_pages(GTK_NOTEBOOK(app.notebook));  
        if(curr_pag == page){
            if(cu_executing==app.current_view){
                GtkWidget* dialog = gtk_message_dialog_new(
                  GTK_WINDOW(app.top_window), GTK_DIALOG_MODAL,
                  GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
                  "This page is executing a CU program.");
                gtk_dialog_run(GTK_DIALOG(dialog));
                gtk_widget_destroy(dialog);
                return FALSE;
            }
            if(curr_idx==(numb_pag-2)){
                gtk_notebook_set_current_page(GTK_NOTEBOOK(app.notebook), curr_idx-1);
            }
            gtk_notebook_remove_page(GTK_NOTEBOOK(app.notebook), curr_idx);
        }
    }
    return FALSE;
}

void switch_page(GtkWidget* widget, GtkWidget* page, guint page_idx, gpointer data){
    page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(app.notebook), page_idx);

    if(page==app.new_page){
        if(!app.file){
            return;
        }
        static int count = 0;
        char buffer[32];
        sprintf(buffer, "Page%d", ++count);
        GtkWidget* label = gtk_label_new(buffer);
        if(1){
            GtkWidget* evtbox = gtk_event_box_new();
            gtk_event_box_set_above_child(GTK_EVENT_BOX(evtbox), TRUE);
            gtk_event_box_set_visible_window(GTK_EVENT_BOX(evtbox), FALSE);
            g_signal_connect(G_OBJECT(evtbox), "button_press_event", G_CALLBACK(label_clicked), label);
            gtk_container_add(GTK_CONTAINER(evtbox), label);
            gtk_widget_show_all(evtbox);
            label = evtbox;
        }
        if(1){
            GtkWidget* hbox = gtk_hbox_new(FALSE, 0);
            GtkWidget* evtbox = gtk_event_box_new();
            gtk_event_box_set_above_child(GTK_EVENT_BOX(evtbox), TRUE);
            gtk_event_box_set_visible_window(GTK_EVENT_BOX(evtbox), FALSE);
            g_signal_connect(G_OBJECT(evtbox), "button_press_event", G_CALLBACK(close_page_pressed), page);
            GtkWidget* close  = gtk_image_new_from_stock(GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU);
            gtk_container_add(GTK_CONTAINER(evtbox), close);
            gtk_box_pack_start_defaults(GTK_BOX(hbox), label);
            gtk_box_pack_start_defaults(GTK_BOX(hbox), evtbox);
            gtk_widget_show_all(hbox);
            label = hbox;
        }
        gtk_notebook_set_tab_label(GTK_NOTEBOOK(app.notebook), page, label);

        gtk_box_pack_start_defaults(GTK_BOX(page), 
          VU_new_page(page)
        );
        
        new_blank_page();
    }

    g_object_get(G_OBJECT(page), "user-data", &app.current_view, NULL);
}

void show_open_file_dialog(GtkButton* button, gpointer user_data){
    GtkEntry* entry = GTK_ENTRY(user_data);
    if(!gtk_widget_self_sensitive(GTK_WIDGET(entry))){
        return;
    }
    if(!gtk_editable_get_editable(GTK_EDITABLE(entry))){
        return;
    }
    static GtkWidget* dialog = NULL;
    if(!dialog) dialog = gtk_file_chooser_dialog_new("Open file", GTK_WINDOW(app.top_window),
                           GTK_FILE_CHOOSER_ACTION_OPEN,
                           GTK_STOCK_OPEN  , GTK_RESPONSE_OK,
                           GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                           NULL
                          );
    gint ret = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_hide(dialog);
    if(ret == GTK_RESPONSE_OK){
        gchar* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        if(filename){
            gtk_entry_set_text(entry, filename);
            g_free(filename);
            gtk_editable_set_position(GTK_EDITABLE(entry), -1);
        }
    }
}

gboolean show_open_file_dialog_from_img(GtkWidget* widget, GdkEventButton* event, gpointer user_data){
    if(event->type == GDK_BUTTON_PRESS){
        show_open_file_dialog(NULL, user_data);
        return TRUE;
    }
    return FALSE;
}

void file_path_entry_open_menu(GtkEntry* entry, GtkMenu* menu, gpointer user_data){
    GtkWidget* open_dialog = gtk_image_menu_item_new_with_mnemonic("_Open file chooser");
    GtkWidget* img = gtk_image_new_from_stock(GTK_STOCK_OPEN, GTK_ICON_SIZE_MENU);
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(open_dialog), img);
    g_signal_connect(G_OBJECT(open_dialog), "activate", G_CALLBACK(show_open_file_dialog), entry);
    gtk_menu_shell_insert(GTK_MENU_SHELL(menu), open_dialog, 0);
    if(!gtk_editable_get_editable(GTK_EDITABLE(entry))){
        gtk_widget_set_sensitive(GTK_WIDGET(open_dialog), FALSE); 
    }
    gtk_widget_show(open_dialog);
}

void do_open_file(GtkButton* button, gpointer user_data){
    GtkEntry* entry = GTK_ENTRY(user_data);

    char* path = g_strstrip(g_strdup(gtk_entry_get_text(entry)));
    if(!*path){
        GdkColor color = {0, 255<<8, 220<<8, 220<<8};
        gtk_widget_modify_base(GTK_WIDGET(entry), GTK_STATE_NORMAL, &color);

        GtkWidget* dialog = gtk_message_dialog_new(
          GTK_WINDOW(app.top_window), GTK_DIALOG_MODAL,
          GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
          "File not specified.");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        g_free(path);
        return;
    }
    int f = open(path, O_RDONLY|O_LARGEFILE);
    if(f<0){
        GtkWidget* dialog = gtk_message_dialog_new(
          GTK_WINDOW(app.top_window), GTK_DIALOG_MODAL,
          GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
          "Can not open file\n  %s", path);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        g_free(path);
        return;
    }
    long file_size = -1;
    struct stat stat_buf;
    if(fstat(f, &stat_buf)==0){
        file_size = stat_buf.st_size;
        if(!S_ISREG(stat_buf.st_mode)){
            GtkWidget* dialog = gtk_message_dialog_new(
              GTK_WINDOW(app.top_window), GTK_DIALOG_MODAL,
              GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
              "%s\nis not a file.", path);
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            g_free(path);
            return;
        }
        if(!file_size){
            GtkWidget* dialog = gtk_message_dialog_new(
              GTK_WINDOW(app.top_window), GTK_DIALOG_MODAL,
              GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
              "%s\nis empty.", path);
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            g_free(path);
            return;
        }
    }else{
        puts("fstat failed.");
        exit(1);
    }

    GtkStyle* style = gtk_widget_get_style(GTK_WIDGET(entry));
    GdkColor  color = style->base[GTK_STATE_INSENSITIVE];
    gtk_widget_modify_base(GTK_WIDGET(entry), GTK_STATE_NORMAL, &color);

    gtk_editable_set_editable(GTK_EDITABLE(entry), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(button), FALSE); 


    app.file = HsvFile::load_file(f, file_size);
    if(app.closing){
        g_free(path);
        return;
    }
    g_assert(app.file);

    new_blank_page();

    time_t t = time(NULL);
    char*  time_str = ctime(&t);
    for(int i=0; time_str[i]; ++i) if(time_str[i]=='\n') time_str[i] = 0;

    int found = 0;
    GtkTreeModel* model = GTK_TREE_MODEL(app.recent_files_store);
    GtkTreeIter iter;
    if(gtk_tree_model_get_iter_first(model, &iter)){
        do{
            char* filepath = NULL;
            gtk_tree_model_get(model, &iter, RF_FILEPATH, &filepath, -1);
            if(filepath){
                if(strcmp(filepath, path)==0){
                    gtk_list_store_set(GTK_LIST_STORE(model), &iter, RF_OPENDATE, time_str, -1);
                    gtk_list_store_set(GTK_LIST_STORE(model), &iter, RF_DATETIME,  (int) t, -1);
                    found = 1;
                    break;
                }
                g_free(filepath);
            }
        }while(gtk_tree_model_iter_next(model, &iter));
    }
    if(!found){
        GtkTreeIter iter;
        gtk_list_store_prepend(app.recent_files_store, &iter);
        gtk_list_store_set(app.recent_files_store, &iter, 
            RF_FILEPATH, path,
            RF_OPENDATE, time_str,
            RF_DATETIME, (int)t,
            RF_COMMENTS, "",
            RF_KEEPFILE, TRUE,
            RF_HIDENSEQ, 0,
          -1);
    }

    gtk_notebook_set_current_page(GTK_NOTEBOOK(app.notebook), 1);
    g_free(path);
}

void update_file_path_completion_model(GtkEntry* entry, GtkListStore* store){
    gtk_list_store_clear(store); 
    GString* pattern = g_string_new(gtk_entry_get_text(entry));
    g_string_append_c(pattern, '*');
    glob_t globbuf;
    if(glob(pattern->str, GLOB_MARK | (0&GLOB_TILDE), NULL, &globbuf)==0 && globbuf.gl_pathc>0){
        if(globbuf.gl_pathc==1){
            gtk_entry_set_text(entry, globbuf.gl_pathv[0]);
            gtk_editable_set_position(GTK_EDITABLE(entry), -1);
        }else{
            int i;
            for(i=0; i<globbuf.gl_pathc; ++i){
                GtkTreeIter iter;
                gtk_list_store_append(store, &iter);  
                gtk_list_store_set(store, &iter, 0, globbuf.gl_pathv[i], -1);
            }
            char* str1 = globbuf.gl_pathv[0];
            char* str2 = globbuf.gl_pathv[globbuf.gl_pathc-1];
            for(i=0; str1[i] && str2[i] && str1[i]==str2[i]; i++) ;
            if(strlen(gtk_entry_get_text(entry)) <= i){
                char c = str1[i];
                str1[i] = 0; 
                gtk_entry_set_text(entry, str1);
                gtk_editable_set_position(GTK_EDITABLE(entry), -1);
                str1[i] = c;
            }
        }
    }
    globfree(&globbuf);
    g_string_free(pattern, TRUE);
    g_signal_emit_by_name(G_OBJECT(entry), "changed", gtk_entry_get_completion(entry));
}

void file_path_entry_activate(GtkWidget* widget, gpointer user_data){
    GtkEntry* entry = GTK_ENTRY(widget);
    GtkListStore* store = GTK_LIST_STORE(user_data);
    update_file_path_completion_model(entry, store);
}

gboolean file_path_entry_focus(GtkWidget* widget, GtkDirectionType direction, gpointer user_data){
    GtkEntry* entry = GTK_ENTRY(widget);
    GtkListStore* store = GTK_LIST_STORE(user_data);
    if(direction==GTK_DIR_TAB_FORWARD && gtk_widget_is_focus(widget)){
        update_file_path_completion_model(entry, store);
        return TRUE;
    }
    return FALSE;
}

gboolean file_path_entry_keypress(GtkWidget* widget, GdkEventKey* event, gpointer user_data){
    if(event->keyval==GDK_Escape){
        gulong handler = (gulong) user_data;
        g_signal_handler_block(widget, handler);
        gboolean rtn;
        g_signal_emit_by_name(app.top_window, "focus", GTK_DIR_TAB_FORWARD, &rtn);
        g_signal_handler_unblock(widget, handler);
        return TRUE;
    }
    return FALSE;
}

void setup_file_path_auto_completion(GtkEntry* entry){
    GtkEntryCompletion* completion = gtk_entry_completion_new();
    gtk_entry_completion_set_minimum_key_length(completion, 0);
    gtk_entry_set_completion(entry, completion);

    GtkListStore* store = gtk_list_store_new(1, G_TYPE_STRING);
    gtk_entry_completion_set_model(completion, GTK_TREE_MODEL(store));
    gtk_entry_completion_set_text_column(completion, 0);

    gulong handler = g_signal_connect(G_OBJECT(entry), "focus", G_CALLBACK(file_path_entry_focus), store);
    g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(file_path_entry_activate), store);
    g_signal_connect(G_OBJECT(entry), "key-press-event", G_CALLBACK(file_path_entry_keypress), (gpointer) handler);
}

extern FILE* vuin;
int  vuparse();
void vuerror(const char* msg){
    puts(msg);
}

void add_recent_file(char* path, int date, char* comment){
    time_t t = (time_t) date;
    char*  time_str = ctime(&t);
    for(int i=0; time_str[i]; ++i) if(time_str[i]=='\n') time_str[i] = 0;
    if(comment==NULL) comment = g_strdup("");
    GtkTreeIter iter;
    gtk_list_store_append(app.recent_files_store, &iter);
    gtk_list_store_set(app.recent_files_store, &iter, RF_FILEPATH, path,
                                                      RF_OPENDATE, time_str,
                                                      RF_DATETIME, date,
                                                      RF_COMMENTS, comment,
                                                      RF_KEEPFILE, TRUE,
                                                      RF_HIDENSEQ, 0,
                                                      -1);
    g_free(path);
    g_free(comment); 
}

void load_app_configurations(){
    app.window_width = 1000;
    app.window_height = 700;

    app.home = getenv("HOME");
    if(!app.home){
        app.home = g_get_home_dir();
    }
    if(app.home){
        if(*app.home){
            app.home = g_strdup(app.home);
        }else{
            app.home = NULL;
        }
    }
    app.dotVU_history = NULL;
    if(app.home){
        app.dotVU_history = g_build_filename(app.home, ".VU.config", NULL);
    }
    app.recent_files_store = gtk_list_store_new(RF_NUM_COLS, 
                                                G_TYPE_STRING, 
                                                G_TYPE_STRING, 
                                                G_TYPE_INT,
                                                G_TYPE_STRING, 
                                                G_TYPE_BOOLEAN, 
                                                G_TYPE_INT);
    FILE* fq=NULL;
    if(app.dotVU_history){
        fq = fopen(app.dotVU_history, "r"); 
    }
    if(fq){
        vuin = fq;
        if(vuparse()){
            app.dotVU_history = NULL;
        }
        fclose(fq);
    }else{
        if(errno!=ENOENT){
            app.dotVU_history = NULL;
        }
    }
}

void save_app_configurations(){
    if(!app.dotVU_history) return;

    FILE* fp = fopen(app.dotVU_history, "w");
    if(!fp) return;

    fprintf(fp, "window_width %d\n", app.window_width);
    fprintf(fp, "window_height %d\n", app.window_height);

    GtkTreeIter iter;
    if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(app.recent_files_store), &iter)){
        do{
            char* path;
            int   date;
            char* comment;
            gboolean keep;
            gtk_tree_model_get(GTK_TREE_MODEL(app.recent_files_store), &iter, RF_FILEPATH, &path, RF_DATETIME, &date, RF_COMMENTS, &comment, RF_KEEPFILE, &keep, -1);   
            if(!keep) continue;

            typedef const guchar* data; 
            char* path_enc = g_base64_encode(data(path), strlen(path)+1);
            char* cmmt_enc = g_base64_encode(data(comment), strlen(comment)+1);
            fprintf(fp, "path =%s\n"   , path_enc);
            fprintf(fp, "date %d\n"    , date    );
            fprintf(fp, "comment =%s\n", cmmt_enc);
            g_free(path_enc);
            g_free(cmmt_enc);
            g_free(path);
            g_free(comment);
        }while(gtk_tree_model_iter_next(GTK_TREE_MODEL(app.recent_files_store), &iter));
    }
    fclose(fp);
}

void window_size_update(GtkWidget* widget, GdkRectangle* allocation, gpointer user_data){
    app.window_width  = allocation->width;
    app.window_height = allocation->height;
}

void enter_file_path_entry_from_recent_files(GtkTreeView* tree_view, GtkTreePath* path, GtkTreeViewColumn* column, gpointer user_data){
    char* filepath;
    GtkTreeIter iter;
    GtkEntry* entry = GTK_ENTRY(user_data);
    if(!gtk_editable_get_editable(GTK_EDITABLE(entry))) return;
    GtkTreeModel* model = gtk_tree_view_get_model(tree_view);
    if(!gtk_tree_model_get_iter(model, &iter, path)) return;
    gtk_tree_model_get(model, &iter, RF_FILEPATH, &filepath, -1);
    gtk_entry_set_text(entry, filepath);
}

void recent_file_comment_edited(GtkCellRendererText* renderer, gchar* path, gchar* new_text, gpointer user_data){
    GtkTreeModel* model = GTK_TREE_MODEL(user_data);
    GtkTreePath* tree_path = gtk_tree_path_new_from_string(path);
    GtkTreeIter  iter;
    if(gtk_tree_model_get_iter(model, &iter, tree_path)){
        GtkTreeIter source;
        gtk_tree_model_filter_convert_iter_to_child_iter(GTK_TREE_MODEL_FILTER(model), &source, &iter);
        gtk_list_store_set(app.recent_files_store, &source, RF_COMMENTS, new_text, -1);
    }
    gtk_tree_path_free(tree_path);
}

void remove_item_from_recent_files(GtkMenuItem* menuitem, gpointer user_data){
    GtkTreeIter* source = (GtkTreeIter*) user_data;
    gtk_list_store_set(app.recent_files_store, source, RF_KEEPFILE, FALSE, -1);
    gtk_list_store_set(app.recent_files_store, source, RF_HIDENSEQ, ++app.recent_files_removed, -1);
}

void undo_remove_item_from_recent_files(GtkMenuItem* menuitem, gpointer user_data){
    g_assert(app.recent_files_removed);

    GtkTreeIter iter;
    if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(app.recent_files_store), &iter)){
        do{
            int seqno = 0; 
            gtk_tree_model_get(GTK_TREE_MODEL(app.recent_files_store), &iter, RF_HIDENSEQ, &seqno, -1);   
            if(seqno==app.recent_files_removed){
                gtk_list_store_set(app.recent_files_store, &iter, RF_KEEPFILE, TRUE, -1);
                gtk_list_store_set(app.recent_files_store, &iter, RF_HIDENSEQ,    0, -1);
                app.recent_files_removed--;
                return;
            }
        }while(gtk_tree_model_iter_next(GTK_TREE_MODEL(app.recent_files_store), &iter));
    }
    g_assert(0);
}

gboolean recent_files_button_press(GtkWidget* widget, GdkEventButton* event, gpointer user_data){
    if(event->type==GDK_BUTTON_PRESS && event->button==3){
        GtkTreeView* tree_view = GTK_TREE_VIEW(widget);
        printf("%s\n", (event->window==gtk_tree_view_get_bin_window(tree_view))?"yes":"no");
        if(1){

            GtkWidget*   menu = gtk_menu_new(); 
            GtkWidget*   undo = gtk_image_menu_item_new_with_mnemonic("_Undo removal");
            GtkWidget* remove = gtk_image_menu_item_new_with_mnemonic("_Remove from list");
            gtk_widget_set_sensitive(  undo, FALSE);
            gtk_widget_set_sensitive(remove, FALSE);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu),   undo);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), remove);

            GtkTreePath* tree_path;
            if(gtk_tree_view_get_path_at_pos(tree_view, int(event->x), int(event->y), &tree_path, NULL, NULL, NULL)){
                GtkTreeIter iter;
                GtkTreeModel* model = gtk_tree_view_get_model(tree_view);
                if(gtk_tree_model_get_iter(model, &iter, tree_path)){
                    char* filepath = NULL;
                    gtk_tree_model_get(model, &iter, RF_FILEPATH, &filepath, -1);

                    static GtkTreeIter source;
                    gtk_tree_model_filter_convert_iter_to_child_iter(GTK_TREE_MODEL_FILTER(model), &source, &iter);
                    g_signal_connect(G_OBJECT(remove), "activate", G_CALLBACK(remove_item_from_recent_files), &source);
                    gtk_widget_set_sensitive(remove, TRUE);
                }
                gtk_tree_path_free(tree_path);
            }

            if(app.recent_files_removed){
                gtk_widget_set_sensitive(undo, TRUE);
                g_signal_connect(G_OBJECT(undo), "activate", G_CALLBACK(undo_remove_item_from_recent_files), NULL);
            }

            g_signal_connect(G_OBJECT(menu), "hide", G_CALLBACK(gtk_menu_detach), NULL);

            gtk_widget_show_all(menu);
            gtk_menu_attach_to_widget(GTK_MENU(menu), widget, NULL);
            gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, event->button, event->time);
        }
        return TRUE;
    }
    return FALSE;
}

GtkWidget* create_recent_files(GtkWidget* file_path_entry){
    GtkWidget* sw = gtk_scrolled_window_new(NULL, NULL);
    GtkWidget* tv = gtk_tree_view_new();
    gtk_container_add(GTK_CONTAINER(sw), tv);

    GtkCellRenderer*   renderer;

    GtkTreeViewColumn* filename_col = gtk_tree_view_column_new_with_attributes("Path",    renderer = gtk_cell_renderer_text_new(), "text", RF_FILEPATH, NULL);
    g_object_set(renderer, "ellipsize", PANGO_ELLIPSIZE_START, NULL);
    gtk_tree_view_column_set_min_width(filename_col, 300);

    GtkTreeViewColumn* accedate_col = gtk_tree_view_column_new_with_attributes("Date",    renderer = gtk_cell_renderer_text_new(), "text", RF_OPENDATE, NULL);

    GtkTreeViewColumn* ucomment_col = gtk_tree_view_column_new_with_attributes("Comment", renderer = gtk_cell_renderer_text_new(), "text", RF_COMMENTS, NULL);
    g_object_set(renderer, "editable", TRUE, "editable-set", TRUE, NULL);

    gtk_tree_view_column_set_resizable(filename_col, TRUE);
    gtk_tree_view_column_set_resizable(accedate_col, TRUE);
    gtk_tree_view_column_set_resizable(ucomment_col, TRUE);

    gtk_tree_view_append_column(GTK_TREE_VIEW(tv), filename_col);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tv), accedate_col);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tv), ucomment_col);

    GtkTreeModel* filtered = gtk_tree_model_filter_new(GTK_TREE_MODEL(app.recent_files_store), NULL);
    gtk_tree_model_filter_set_visible_column(GTK_TREE_MODEL_FILTER(filtered), RF_KEEPFILE);
    gtk_tree_view_set_model(GTK_TREE_VIEW(tv), filtered);
    gtk_tree_view_set_hover_selection(GTK_TREE_VIEW(tv), TRUE);
    gtk_tree_view_set_headers_clickable(GTK_TREE_VIEW(tv), TRUE);

    g_signal_connect(G_OBJECT(renderer), "edited", G_CALLBACK(recent_file_comment_edited), filtered);
    g_signal_connect(G_OBJECT(tv), "row-activated", G_CALLBACK(enter_file_path_entry_from_recent_files), file_path_entry);

    g_signal_connect(G_OBJECT(tv), "button-press-event", G_CALLBACK(recent_files_button_press), NULL);

    gtk_widget_set_size_request(sw, -1, 150);
    return sw;
}

gboolean config_sep_drag(GtkWidget* widget, GdkEvent* event, gpointer user_data){
    static int y_begin = 0;
    if(event->type==GDK_ENTER_NOTIFY){
        static int set_curosr = 0;
        if(!set_curosr){
            set_curosr = 1;
            gdk_window_set_cursor(widget->window, gdk_cursor_new(GDK_SB_V_DOUBLE_ARROW));
        }
    }else if(event->type==GDK_BUTTON_PRESS){
        GdkEventButton* button = (GdkEventButton*) event;
        if(button->button==1){
            y_begin = (int) button->y;
        }
    }else if(event->type==GDK_MOTION_NOTIFY){
        GdkEventMotion* motion = (GdkEventMotion*) event;
        if((motion->state & GDK_BUTTON1_MASK) && user_data){
            GtkWidget* recent_files = GTK_WIDGET(user_data);
            GtkRequisition requisition;
            gtk_widget_get_child_requisition(recent_files, &requisition);
            int height = requisition.height + int(motion->y)-y_begin;
            if( height < 40 ) height = 40;
            gtk_widget_set_size_request(recent_files, -1, height);
        }
    }
    return TRUE;
}

int main(int argc, char* argv[]){
    gtk_init(&argc, &argv);

    load_app_configurations();

    app.top_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app.top_window), "VUCU");
    gtk_window_set_icon (GTK_WINDOW(app.top_window),
      gtk_widget_render_icon(app.top_window, GTK_STOCK_EDIT, GTK_ICON_SIZE_DIALOG, NULL));
    gtk_window_set_default_size(GTK_WINDOW(app.top_window), 300, 200);
    gtk_window_set_position    (GTK_WINDOW(app.top_window), GTK_WIN_POS_CENTER);
    g_signal_connect(app.top_window, "destroy", G_CALLBACK(app_main_quit), NULL);
    g_signal_connect(app.top_window, "size-allocate", G_CALLBACK(window_size_update), NULL);

    app.info_page = gtk_vbox_new(FALSE, 0);
    {
        GtkTable*  table = GTK_TABLE(gtk_table_new(9,2,FALSE));
        int        row   = 0;
        GtkAttachOptions GTK_GROW = GtkAttachOptions(GTK_EXPAND|GTK_FILL);
        GtkAttachOptions GTK_FIXD = GTK_SHRINK;

        GtkWidget* align;
        GtkWidget* label;
        gtk_container_add(GTK_CONTAINER(align=gtk_alignment_new(0,0,0,0)), 
          label = gtk_label_new(NULL));
        gtk_box_pack_start(GTK_BOX(app.info_page), align, FALSE, FALSE, 0);
        gtk_label_set_markup(GTK_LABEL(label), 
          "<span size=\"x-large\" weight=\"normal\">VUCU " VUCU_VERSION_STRING "</span>\n"
        );

        gtk_box_pack_start(GTK_BOX(app.info_page), GTK_WIDGET(table), TRUE, TRUE, 0);
        gtk_table_attach(table, gtk_image_new_from_stock(GTK_STOCK_EDIT, GTK_ICON_SIZE_MENU), 0, 1, row, row+1, GTK_SHRINK, GTK_FIXD, 3, 0);
        gtk_table_attach(table, gtk_hseparator_new(), 1, 2, row, row+1, GTK_GROW, GTK_FIXD, 3, 0);
        row += 1;

        gtk_container_add(GTK_CONTAINER(align=gtk_alignment_new(0,0,0,0)), 
          label = gtk_label_new("Open a file at:"));
        gtk_table_attach(table, align, 1, 2, row, row+1, GTK_GROW, GTK_FIXD, 3, 0);
        row += 1;

        GtkWidget* hbox = gtk_hbox_new(FALSE, 0);
        gtk_table_attach(table, hbox, 1, 2, row, row+1, GTK_GROW, GTK_FIXD, 3, 0);
        row += 1;
        GtkWidget* open_img = gtk_image_new_from_stock(GTK_STOCK_OPEN, GTK_ICON_SIZE_MENU);
      #if 1
        GtkWidget* open_btn = gtk_event_box_new();
        gtk_event_box_set_above_child(GTK_EVENT_BOX(open_btn), TRUE);
        gtk_event_box_set_visible_window(GTK_EVENT_BOX(open_btn), FALSE);
        gtk_widget_set_sensitive(open_img, FALSE);
        g_signal_connect_swapped(G_OBJECT(open_btn), "enter-notify-event", G_CALLBACK(  set_widget_sensitive), open_img);
        g_signal_connect_swapped(G_OBJECT(open_btn), "leave-notify-event", G_CALLBACK(unset_widget_sensitive), open_img);
        gtk_container_add(GTK_CONTAINER(open_btn), open_img);
      #else
        GtkWidget* open_btn = (new HsvImageButton(open_img))->get_widget();
      #endif
        gtk_box_pack_start(GTK_BOX(hbox), open_btn, FALSE, FALSE, 3);
        gtk_widget_set_tooltip_text(open_btn, "Open file chooser dialog");
        GtkWidget* vbox = gtk_vbox_new(FALSE, 0);
        gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0); 
        hbox = gtk_hbox_new(FALSE, 0);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
        GtkWidget* file_path_entry = gtk_entry_new();
        gtk_box_pack_start(GTK_BOX(hbox), file_path_entry, TRUE, TRUE, 0);
        setup_file_path_auto_completion(GTK_ENTRY(file_path_entry));
        g_signal_connect(G_OBJECT(file_path_entry), "populate-popup", G_CALLBACK(file_path_entry_open_menu), NULL);
        GtkWidget* ok_img = gtk_image_new_from_stock(GTK_STOCK_YES, GTK_ICON_SIZE_MENU);
        GtkWidget* ok_btn = gtk_button_new();
        gtk_container_add(GTK_CONTAINER(ok_btn), ok_img);
        gtk_box_pack_start(GTK_BOX(hbox), ok_btn, FALSE, FALSE, 0);
        g_signal_connect(G_OBJECT(ok_btn), "clicked", G_CALLBACK(do_open_file), file_path_entry); 

        gtk_widget_set_tooltip_text(open_btn, "load file");
        g_signal_connect(G_OBJECT(open_btn), "button-press-event", G_CALLBACK(show_open_file_dialog_from_img), file_path_entry);

        GtkWidget* load_prog = app.load_progress = gtk_progress_bar_new();
        gtk_widget_set_size_request(load_prog, -1, 5);
        gtk_box_pack_start(GTK_BOX(vbox), load_prog, TRUE, TRUE, 0);

        GtkWidget* recent_files = NULL;
        gtk_table_attach(table, gtk_image_new_from_stock(GTK_STOCK_DND_MULTIPLE, GTK_ICON_SIZE_MENU), 0, 1, row, row+1, GTK_SHRINK, GTK_FIXD, 3, 5);
        gtk_container_add(GTK_CONTAINER(align=gtk_alignment_new(0,0,0,0)), 
          label = gtk_label_new("Recent files:"));
        gtk_table_attach(table, align, 1, 2, row, row+1, GTK_GROW, GTK_FIXD, 3, 0);
        row += 1;
        recent_files = create_recent_files(file_path_entry);
        gtk_table_attach(table, recent_files, 1, 2, row, row+1, GTK_GROW, GTK_FIXD, 3, 0);
        row += 1;
 
        GtkWidget* config_sep = gtk_hseparator_new();
        GtkWidget* config_sep_evt = gtk_event_box_new();
        gtk_container_add(GTK_CONTAINER(config_sep_evt), config_sep);
        config_sep = config_sep_evt;
        gtk_table_attach(table, gtk_image_new_from_stock(GTK_STOCK_PREFERENCES, GTK_ICON_SIZE_MENU), 0, 1, row, row+1, GTK_SHRINK, GTK_FIXD, 3, 5);
        gtk_table_attach(table, config_sep, 1, 2, row, row+1, GTK_GROW, GTK_FIXD, 3, 0);
        gtk_event_box_set_above_child(GTK_EVENT_BOX(config_sep_evt), TRUE);
        gtk_event_box_set_visible_window(GTK_EVENT_BOX(config_sep_evt), TRUE);
        gtk_widget_set_size_request(config_sep_evt, -1, 5);
        g_signal_connect(G_OBJECT(config_sep_evt), "motion-notify-event", G_CALLBACK(config_sep_drag), recent_files);
        g_signal_connect(G_OBJECT(config_sep_evt), "button-press-event", G_CALLBACK(config_sep_drag), NULL);
        g_signal_connect(G_OBJECT(config_sep_evt), "enter-notify-event", G_CALLBACK(config_sep_drag), NULL);
        row += 1;

        const char* default_fonts[] = {"Courier 12", NULL};
        int i;
        for(i=0; default_fonts[i]; ++i){
            PangoContext*             pango = gtk_widget_get_pango_context(label); 
            PangoFontDescription* font_desc = pango_font_description_from_string(default_fonts[i]);
            PangoFont*                 font = pango_context_load_font(pango, font_desc);
            pango_font_description_free(font_desc);
            if(font){
                app.code_font_button = GTK_FONT_BUTTON(gtk_font_button_new_with_font(default_fonts[i]));
                g_signal_connect(G_OBJECT(app.code_font_button), "destroy", G_CALLBACK(gtk_widget_destroyed), &app.code_font_button);
                g_object_unref(font);
                break;
            }
        }
        GtkWidget* config_box = gtk_hbox_new(FALSE, 0);
        gtk_table_attach(table, config_box, 1, 2, row, row+1, GTK_FILL, GTK_FIXD, 3, 0);
        row += 1;
        gtk_box_pack_start(GTK_BOX(config_box), gtk_label_new("Font:"), FALSE, FALSE, 0);
        gtk_container_add(GTK_CONTAINER(align=gtk_alignment_new(0,0,0,0)), 
          GTK_WIDGET(app.code_font_button));
        gtk_box_pack_start(GTK_BOX(config_box), align, TRUE, TRUE, 5);

        gtk_box_pack_start(GTK_BOX(config_box), gtk_label_new("Memory limit:"), FALSE, FALSE, 0);
        GtkWidget* hs = gtk_hscale_new_with_range(0, 1, 0.1);
        gtk_range_set_value(GTK_RANGE(hs), 0);
        gtk_widget_set_size_request(hs, 100, -1);
        gtk_scale_set_draw_value(GTK_SCALE(hs), FALSE);
        gtk_box_pack_start(GTK_BOX(config_box), hs, FALSE, TRUE, 5);
        GtkWidget* limit_label = gtk_label_new("???");
        gtk_container_add(GTK_CONTAINER(align=gtk_alignment_new(0,0.5,0,0)), limit_label);
        gtk_box_pack_start(GTK_BOX(config_box), align, TRUE, TRUE, 5);
        gtk_widget_set_size_request(align, 100, -1);
        g_signal_connect(G_OBJECT(hs), "value-changed", G_CALLBACK(memory_limit_changed), limit_label);
        g_signal_emit_by_name(G_OBJECT(hs), "value-changed"); 

        gtk_box_pack_end_defaults(GTK_BOX(app.info_page), label=gtk_label_new(
          "\nCopyright (C) 2020  Peng-Hsien Lai"
          "\nThis program comes with ABSOLUTELY NO WARRANTY"
        ));
        gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
    }
    gtk_widget_set_size_request(app.info_page, 700, -1);

    GtkWidget* info_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(info_window), center(app.info_page));
    gtk_scrolled_window_set_policy       (GTK_SCROLLED_WINDOW(info_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    app.notebook = gtk_notebook_new();
    gtk_notebook_append_page(GTK_NOTEBOOK(app.notebook), info_window, 
       gtk_image_new_from_stock(GTK_STOCK_DIALOG_INFO, GTK_ICON_SIZE_MENU));

    g_signal_connect(G_OBJECT(app.notebook), "switch-page", G_CALLBACK(switch_page), NULL);

    gtk_container_add(GTK_CONTAINER(app.top_window), app.notebook);
    gtk_window_set_default_size(GTK_WINDOW(app.top_window), app.window_width, app.window_height);
    gtk_widget_show_all(app.top_window);

    if(pipe(cu_pipe)!=0){
        puts("pipe failed.");
        exit(1);
    }
    if(!(cu_out=fdopen(cu_pipe[1], "w"))){
        puts("fdopen failed.");
        exit(1);
    }
    if(fcntl(cu_pipe[0], F_SETFL, O_NONBLOCK)!=0){
        puts("fcntl failed.");
        exit(1);
    }
    cu_push_scope(); 
    extern void (*app_goto)(int);
    app_goto = set_goto_request;

    g_timeout_add(100, GSourceFunc(console_refresh), NULL);

    gtk_main();
    if(app.file) delete app.file;
    HsvBlock::free_pool();
    save_app_configurations();

    cu_pop_scope();
    cu_delete_scopes();

    return 0;
}

