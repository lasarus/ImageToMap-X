/* This file is part of ImageToMapX.
   ImageToMapX is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   any later version.
 
   ImageToMapX is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   You should have received a copy of the GNU General Public License
   along with ImageToMapX. If not, see <http://www.gnu.org/licenses/>. */

#include "../version"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <errno.h>
#include <gtk/gtk.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include "data_structures.h"
#include "generate.h"
#include "nbtsave.h"
#include "map_render.h"

#ifdef OS_LINUX
#define MINECRAFT_PATH "/home/<user>/.minecraft/saves/<world name>/region"
#else
#define MINECRAFT_PATH "<path to .minecraft>/.minecraft/saves/<world name>/region"
#endif

#define BUFFER_COUNT 256

enum
  {
    ITEM_SIGNAL_OPEN,
    ITEM_SIGNAL_SAVE,
    ITEM_SIGNAL_SAVE_RM,
    ITEM_SIGNAL_EXPORT_IMAGE,
    ITEM_SIGNAL_WORLD_RENDER_ITEM,

    ITEM_SIGNAL_GENERATE_MANDELBROT,
    ITEM_SIGNAL_GENERATE_JULIA,
    ITEM_SIGNAL_GENERATE_PALETTE,
    ITEM_SIGNAL_GENERATE_RANDOM_NOISE,
    ITEM_SIGNAL_GENERATE_FROM_CLIPBOARD,

    ITEM_SIGNAL_QUIT,
  };

void set_image();
void remove_buffer(int id);
void update_sidepanel();
int get_buffer_count();
void add_buffer();

configvars_t * config = NULL;

static GtkWidget * window;

int current_buffer = -1;
static GdkPixbuf * dimage;
static GtkWidget * image;
static GtkWidget * list_vbox;

static GtkWidget * FSD_checkbox;

color_t * colors = NULL;
unsigned char * mdata[BUFFER_COUNT];
map_data_t mdata_info[BUFFER_COUNT];

GtkWidget * selected_buffer_frame = NULL;
GtkWidget * icons[BUFFER_COUNT];
GtkWidget * icon_event_boxes[BUFFER_COUNT];

configvars_t * config_new()
{
  configvars_t * config = malloc(sizeof(configvars_t));
  config->interp = GDK_INTERP_BILINEAR;
  config->zooms = 1.25;
  config->stdzoom = 4;
	
  config->maxzoom = 128 * 8;
  config->minzoom = 128 / 4;
  return config;
}

void config_free(configvars_t * config)
{
  free(config);
}

GtkResponseType message_dialog(GtkMessageType type, GtkWindow *parent, const gchar * title, const gchar * message)
{
  GtkWidget *dialog;
  GtkResponseType response;
  GtkButtonsType buttons;

  if(type == GTK_MESSAGE_QUESTION)
    buttons = GTK_BUTTONS_YES_NO;
  else buttons = GTK_BUTTONS_OK;
        
  dialog = gtk_message_dialog_new(parent, GTK_DIALOG_DESTROY_WITH_PARENT, type, buttons, "%s", message);
                        
  gtk_window_set_title(GTK_WINDOW(dialog), title);
        
  response = gtk_dialog_run(GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);   
        
  return response;
}

void information(const char * message)
{
  message_dialog(GTK_MESSAGE_INFO, GTK_WINDOW(window), "Information", message);
  printf("%s\n", message);
}

GdkPixbuf * image_from_data(unsigned char * data, int scale)
{
  GdkPixbuf * pixbuf = gdk_pixbuf_new_from_data(data, GDK_COLORSPACE_RGB,
						FALSE, 8, 128, 128,
						128 * 3, NULL, NULL);
	
  if(scale)
    {
      GdkPixbuf * rpixbuf = gdk_pixbuf_scale_simple(pixbuf, 128 * config->stdzoom, 128 * config->stdzoom, GDK_INTERP_NEAREST);
      g_object_unref(pixbuf);
      return rpixbuf;
    }
  else
    {
      GdkPixbuf * rpixbuf = gdk_pixbuf_scale_simple(pixbuf, 128, 128, GDK_INTERP_NEAREST); // Don't ask me why this is needed
      g_object_unref(pixbuf);
      return rpixbuf;
    }
}

GdkPixbuf * get_pixbuf_from_data(unsigned char * data, int scale)
{
  unsigned char * tmpdata = malloc(128 * 128 * 3);
  int i;
  for(i = 0; i < 128 * 128; i++)
    {
      if(data[i] > 3)
	{
	  tmpdata[i * 3] = colors[data[i]].r;
	  tmpdata[i * 3 + 1] = colors[data[i]].g;
	  tmpdata[i * 3 + 2] = colors[data[i]].b;
	}
      else
	{
	  int x = i % 128, y = i / 128;
	  x /= 4;
	  y /= 4;
	  tmpdata[i * 3] = ((x + (y % 2)) % 2) ? 0xFF : 0xAA;
	  tmpdata[i * 3 + 1] = ((x + (y % 2)) % 2) ? 0xFF : 0xAA;
	  tmpdata[i * 3 + 2] = ((x + (y % 2)) % 2) ? 0xFF : 0xAA;
	}
    }
	
  GdkPixbuf * fdata = image_from_data(tmpdata, scale);
  free(tmpdata);

  return fdata;
}

int drop_down_menu_id;
GtkWidget * drop_down_menu;

static void drop_down_menu_callback(gpointer data)
{
  if(strcmp("drop_down_menu.remove", (char *)data) == 0)
    {
      remove_buffer(drop_down_menu_id);
    }
  else if(strcmp("drop_down_menu.merge_down", (char *)data) == 0)
    {
      if(drop_down_menu_id < get_buffer_count() - 1)
	{
	  merge_buffers(mdata[drop_down_menu_id], mdata[drop_down_menu_id + 1]);
	  remove_buffer(drop_down_menu_id + 1);
	  set_image();
	}
    }
  else if(strcmp("drop_down_menu.move_up", (char *)data) == 0)
    {
      if(drop_down_menu_id > 0)
	{
	  unsigned char * tmpdata = mdata[drop_down_menu_id];
	  mdata[drop_down_menu_id] = mdata[drop_down_menu_id - 1];
	  mdata[drop_down_menu_id - 1] = tmpdata;

	  set_image();
	}
    }
  else if(strcmp("drop_down_menu.move_down", (char *)data) == 0)
    {
      if(drop_down_menu_id < get_buffer_count() - 1)
	{
	  unsigned char * tmpdata = mdata[drop_down_menu_id];
	  mdata[drop_down_menu_id] = mdata[drop_down_menu_id + 1];
	  mdata[drop_down_menu_id + 1] = tmpdata;

	  set_image();
	}
    }
  else if(strcmp("drop_down_menu.edit_map_data", (char *)data) == 0)
    {
      char buffer[256];
      GtkWidget * dialog = gtk_dialog_new_with_buttons("Edit Map Data",
						       GTK_WINDOW(window),
						       GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
						       GTK_STOCK_OK,
						       GTK_RESPONSE_ACCEPT,
						       GTK_STOCK_CANCEL,
						       GTK_RESPONSE_REJECT, NULL);


      GtkWidget * content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
      
      sprintf(buffer, "%i", mdata_info[drop_down_menu_id].scale);
      GtkWidget * scale_entry = gtk_entry_new();
      gtk_entry_set_text(GTK_ENTRY(scale_entry), buffer);
      gtk_container_add(GTK_CONTAINER(content_area), scale_entry);

      sprintf(buffer, "%i", mdata_info[drop_down_menu_id].xpos);
      GtkWidget * xpos_entry = gtk_entry_new();
      gtk_entry_set_text(GTK_ENTRY(xpos_entry), buffer);
      gtk_container_add(GTK_CONTAINER(content_area), xpos_entry);

      sprintf(buffer, "%i", mdata_info[drop_down_menu_id].zpos);
      GtkWidget * zpos_entry = gtk_entry_new();
      gtk_entry_set_text(GTK_ENTRY(zpos_entry), buffer);
      gtk_container_add(GTK_CONTAINER(content_area), zpos_entry);

      sprintf(buffer, "%i", mdata_info[drop_down_menu_id].dimension);
      GtkWidget * dimension_entry = gtk_entry_new();
      gtk_entry_set_text(GTK_ENTRY(dimension_entry), buffer);
      gtk_container_add(GTK_CONTAINER(content_area), dimension_entry);

      gtk_widget_show_all(dialog);

      if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
	  mdata_info[drop_down_menu_id].scale = atoi((char *)gtk_entry_get_text(GTK_ENTRY(scale_entry)));
	  mdata_info[drop_down_menu_id].xpos = atoi((char *)gtk_entry_get_text(GTK_ENTRY(xpos_entry)));
	  mdata_info[drop_down_menu_id].zpos = atoi((char *)gtk_entry_get_text(GTK_ENTRY(zpos_entry)));
	  mdata_info[drop_down_menu_id].dimension = atoi((char *)gtk_entry_get_text(GTK_ENTRY(dimension_entry)));
	}
      gtk_widget_destroy(dialog);
    }
}

void init_drop_down_menu()
{
  GtkWidget * item;
  drop_down_menu = gtk_menu_new();
  item = gtk_menu_item_new_with_label("Remove");
  gtk_menu_shell_append(GTK_MENU_SHELL(drop_down_menu), item);
  g_signal_connect_swapped(item, "activate",
			   G_CALLBACK(drop_down_menu_callback),
			   (gpointer)"drop_down_menu.remove");
  gtk_widget_show(item);

  item = gtk_menu_item_new_with_label("Merge Down");
  gtk_menu_shell_append(GTK_MENU_SHELL(drop_down_menu), item);
  g_signal_connect_swapped(item, "activate",
			   G_CALLBACK(drop_down_menu_callback),
			   (gpointer)"drop_down_menu.merge_down");
  gtk_widget_show(item);

  item = gtk_menu_item_new_with_label("Move Up");
  gtk_menu_shell_append(GTK_MENU_SHELL(drop_down_menu), item);
  g_signal_connect_swapped(item, "activate",
			   G_CALLBACK(drop_down_menu_callback),
			   (gpointer)"drop_down_menu.move_up");
  gtk_widget_show(item);

  item = gtk_menu_item_new_with_label("Move Down");
  gtk_menu_shell_append(GTK_MENU_SHELL(drop_down_menu), item);
  g_signal_connect_swapped(item, "activate",
			   G_CALLBACK(drop_down_menu_callback),
			   (gpointer)"drop_down_menu.move_down");

  item = gtk_menu_item_new_with_label("Edit Map Data");
  gtk_menu_shell_append(GTK_MENU_SHELL(drop_down_menu), item);
  g_signal_connect_swapped(item, "activate",
			   G_CALLBACK(drop_down_menu_callback),
			   (gpointer)"drop_down_menu.edit_map_data");
  gtk_widget_show(item);
}

static gboolean buffer_callback(GtkWidget * event_box, GdkEventButton * event, gpointer data)
{
  if(event->button == 1)
    {
      current_buffer = (size_t)data;
      set_image();
    }
  else if(event->button == 3)
    {
      GdkEventButton * bevent = (GdkEventButton *)event;

      gtk_menu_popup(GTK_MENU(drop_down_menu), NULL, NULL, NULL, NULL,
		     bevent->button, bevent->time);

      drop_down_menu_id = (size_t)data;
    }
  else
    return FALSE;
  return TRUE;
}

void update_sidepanel()
{
  int i;
  for(i = 0; i < BUFFER_COUNT; i++)
    {
      if(icons[i] != NULL)
	{
	  gtk_widget_hide(icons[i]);
	  gtk_widget_destroy(icons[i]);
	  gtk_widget_hide(icon_event_boxes[i]);
	  gtk_widget_destroy(icon_event_boxes[i]);
	  icons[i] = NULL;
	  icon_event_boxes[i] = NULL;
	}
    }

  for(i = 0; i < BUFFER_COUNT; i++)
    {
      if(mdata[i] != NULL)
	{
	  if(i == current_buffer)
	    {
	      GdkColor color;
	      
	      gdk_color_parse("red", &color);
	      
	      if(selected_buffer_frame != NULL)
		gtk_widget_destroy(selected_buffer_frame);
	      selected_buffer_frame = gtk_frame_new(NULL);
	      gtk_widget_modify_bg(selected_buffer_frame, GTK_STATE_NORMAL, &color);
	      gtk_box_pack_start(GTK_BOX(list_vbox), selected_buffer_frame, FALSE, TRUE, 1);
	      
	      icons[i] = gtk_image_new();
	      icon_event_boxes[i] = gtk_event_box_new();
	      
	      gtk_image_set_from_pixbuf(GTK_IMAGE(icons[i]), get_pixbuf_from_data(mdata[i], 0));
	      
	      gtk_container_add(GTK_CONTAINER(selected_buffer_frame), icon_event_boxes[i]);
	      gtk_container_add(GTK_CONTAINER(icon_event_boxes[i]), icons[i]);
	      
	      gtk_widget_show(icons[i]);
	      gtk_widget_show(icon_event_boxes[i]);
	      gtk_widget_show(selected_buffer_frame);
	      
	      g_signal_connect(G_OBJECT(icon_event_boxes[i]), "button_press_event", G_CALLBACK(buffer_callback), (gpointer *)(size_t)i);
	    }
	  else
	    {
	      icons[i] = gtk_image_new();
	      icon_event_boxes[i] = gtk_event_box_new();
	      
	      gtk_image_set_from_pixbuf(GTK_IMAGE(icons[i]), get_pixbuf_from_data(mdata[i], 0));
	      
	      gtk_box_pack_start(GTK_BOX(list_vbox), icon_event_boxes[i], FALSE, TRUE, 2);
	      gtk_container_add(GTK_CONTAINER(icon_event_boxes[i]), icons[i]);
	      
	      gtk_widget_show(icons[i]);
	      gtk_widget_show(icon_event_boxes[i]);
	      
	      g_signal_connect(G_OBJECT(icon_event_boxes[i]), "button_press_event", G_CALLBACK(buffer_callback), (gpointer *)(size_t)i);
	    }
	}
    }
}

void set_image()
{	
  GdkPixbuf * fdata = get_pixbuf_from_data(mdata[current_buffer], 1);
  g_object_unref(dimage);
  dimage = fdata;
  gtk_image_set_from_pixbuf(GTK_IMAGE(image), fdata);
  
  update_sidepanel();
}

static void clipboard_callback(GtkClipboard * clipboard, GdkPixbuf * pixbuf, gpointer data)
{
  if(pixbuf == NULL)
    return;
  
  add_buffer();
  if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(FSD_checkbox)))
    generate_image_dithered_pixbuf(mdata[current_buffer], pixbuf, colors);
  else
    generate_image_pixbuf(mdata[current_buffer], pixbuf, colors);
  set_image();
}

void image_load_map(char * path)
{
  nbt_load_map(path, mdata[current_buffer]);
  set_image();
}

void save_map(char * path)
{
  nbt_save_map(path, mdata_info[current_buffer].dimension, mdata_info[current_buffer].scale,
	       128, 128, mdata_info[current_buffer].xpos, mdata_info[current_buffer].zpos, mdata[current_buffer]);
}

static gboolean kill_window(GtkWidget * widget, GdkEvent * event, gpointer data)
{
  gtk_main_quit();
  return FALSE;
}

int srecmpend(char * end, char * str)
{
  int elen = strlen(end);
  int slen = strlen(str);
  
  if(slen - elen < 0)
    return -1;
  
  str += slen - elen;
  
  return strcmp(end, str);
}

int get_buffer_count()
{
  int i;
  for(i = 0; i < BUFFER_COUNT; i++)
    {
      if(mdata[i] == NULL)
	{
	  return i;
	}
    }
  return BUFFER_COUNT;
}

void add_buffer()
{
  current_buffer = get_buffer_count();
  mdata[current_buffer] = (unsigned char *)malloc(128 * 128);
  mdata_info[current_buffer].xpos = 13371337;
  mdata_info[current_buffer].zpos = -13371337;
  mdata_info[current_buffer].scale = 3;
  mdata_info[current_buffer].dimension = 0;
}


void remove_buffer(int id)
{
  if(get_buffer_count() == 1)
    return;
  free(mdata[id]);
  memmove(&(mdata[id]), &(mdata[id + 1]), (BUFFER_COUNT - id - 2) * sizeof(unsigned char *));
  mdata[BUFFER_COUNT - 1] = NULL;
  if(mdata[current_buffer] == NULL)
    current_buffer--;
  
  memmove(&(mdata_info[id]), &(mdata_info[id + 1]), (BUFFER_COUNT - id - 2) * sizeof(map_data_t));
  memset(&(mdata_info[BUFFER_COUNT - 1]), 0, sizeof(map_data_t));
  set_image();
}

static void button_click(gpointer data)
{
  if((size_t)data == ITEM_SIGNAL_OPEN)
    {
      GtkWidget * dialog;
      dialog = gtk_file_chooser_dialog_new("Open file",
					   GTK_WINDOW(window),
					   GTK_FILE_CHOOSER_ACTION_OPEN,
					   GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					   GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
					   NULL);
      
      if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
	  char * file = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
	  
	  if(srecmpend(".dat", file) == 0)
	    {
	      add_buffer();
	      image_load_map(file);
	    }
	  else if(srecmpend(".bmp", file) == 0 || srecmpend(".png", file) == 0 || srecmpend(".jpg", file) == 0 || srecmpend(".jpeg", file) == 0 || srecmpend(".gif", file) == 0)
	    {
	      GError * err = NULL;
	      add_buffer();
	      if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(FSD_checkbox)))
		generate_image_dithered(mdata[current_buffer], file, colors, &err);
	      else
		generate_image(mdata[current_buffer], file, colors, &err);
	      if(err != NULL)
		{
		  information("Error while loading image file!");
		  printf("%s\n", err->message);
		  g_error_free(err);
		}
	      set_image();
	    }
	  else if(srecmpend(".imtm", file) == 0)
	    {
	      add_buffer();
	      load_raw_map(file, mdata[current_buffer]);
	      set_image();
	    }
	  else
	    information("File format not supported!");
	}
      gtk_widget_destroy(dialog);
    }
  else if((size_t)data == ITEM_SIGNAL_SAVE)
    {
      if(mdata[current_buffer] == NULL)
	return;
      
      GtkWidget * dialog;
      
      dialog = gtk_file_chooser_dialog_new ("Save Map",
					    GTK_WINDOW(window),
					    GTK_FILE_CHOOSER_ACTION_SAVE,
					    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					    GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
					    NULL);
      
      gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);
      gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER (dialog), "map_0.dat");
      
      if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
	  char * file = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
	  printf("%s\n", file);
	  if(srecmpend(".dat", file) == 0)
	    save_map(file);
	}
      gtk_widget_destroy(dialog);
      printf("bracket cleared\n");
    }
  else if((size_t)data == ITEM_SIGNAL_EXPORT_IMAGE)
    {
      if(mdata[current_buffer] == NULL)
	return;
      
      GtkWidget * dialog;
      
      dialog = gtk_file_chooser_dialog_new ("Export Image of Map",
					    GTK_WINDOW(window),
					    GTK_FILE_CHOOSER_ACTION_SAVE,
					    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					    GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
					    NULL);
      
      gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);
      gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER (dialog), "map.png");
      
      if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
	  char * file = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
	  printf("%s\n", file);
	  
	  unsigned char * data = malloc(128 * 128 * 3);
	  int i;
	  for(i = 0; i < 128 * 128; i++)
	    {
	      if(mdata[current_buffer][i] > 3)
		{
		  data[i * 3] = colors[mdata[current_buffer][i]].r;
		  data[i * 3 + 1] = colors[mdata[current_buffer][i]].g;
		  data[i * 3 + 2] = colors[mdata[current_buffer][i]].b;
		}
	      else
		{
		  int x = i % 128, y = i / 128;
		  x /= 4;
		  y /= 4;
		  data[i * 3] = ((x + (y % 2)) % 2) ? 0xFF : 0xAA;
		  data[i * 3 + 1] = ((x + (y % 2)) % 2) ? 0xFF : 0xAA;
		  data[i * 3 + 2] = ((x + (y % 2)) % 2) ? 0xFF : 0xAA;
		}
	    }
	  
	  GdkPixbuf * spixbuf = image_from_data(data, 0);
	  free(data);
	  
	  GError * err = NULL;
	  
	  gdk_pixbuf_save(spixbuf, file, "png", &err, "compression", "9", NULL);
	  if (err != NULL)
	    {
	      /* Report error to user, and free error */
	      printf("Error while saving: %s\n", err->message);
	      g_error_free(err);
	    }
	  
	  g_object_unref(spixbuf);
	}
      gtk_widget_destroy(dialog);
    }
  else if((size_t)data == ITEM_SIGNAL_SAVE_RM)
    {
      if(mdata == NULL)
	return;
      
      GtkWidget * dialog;
      
      dialog = gtk_file_chooser_dialog_new("Save Map",
					   GTK_WINDOW(window),
					   GTK_FILE_CHOOSER_ACTION_SAVE,
					   GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					   GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
					   NULL);
      
      gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER (dialog), TRUE);
      gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER (dialog), "map.imtm");
      
      if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
	  char * file = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
	  printf("%s\n", file);
	  save_raw_map(file, mdata[current_buffer]);
	}
      gtk_widget_destroy(dialog);
    }
  else if((size_t)data == ITEM_SIGNAL_GENERATE_PALETTE)
    {
      add_buffer();
      generate_palette(mdata[current_buffer]);
      set_image();
    }
  else if((size_t)data == ITEM_SIGNAL_GENERATE_RANDOM_NOISE)
    {
      add_buffer();
      generate_random_noise(mdata[current_buffer]);
      set_image();
    }
  else if((size_t)data == ITEM_SIGNAL_GENERATE_MANDELBROT)
    {
      add_buffer();
      generate_mandelbrot(mdata[current_buffer]);
      set_image();
    }
  else if((size_t)data == ITEM_SIGNAL_GENERATE_JULIA)
    {
      add_buffer();
      generate_julia(mdata[current_buffer], 0.5, 0.5);
      set_image();
    }
  else if((size_t)data == ITEM_SIGNAL_GENERATE_FROM_CLIPBOARD)
    {
      GtkClipboard * clipboard;
      clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);

      gtk_clipboard_request_image(clipboard, clipboard_callback, NULL);
    }
  else if((size_t)data == ITEM_SIGNAL_WORLD_RENDER_ITEM)
    {
      GtkWidget * dialog = gtk_dialog_new_with_buttons("Render World Map",
						       GTK_WINDOW(window),
						       GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
						       GTK_STOCK_OK,
						       GTK_RESPONSE_ACCEPT,
						       GTK_STOCK_CANCEL,
						       GTK_RESPONSE_REJECT, NULL);

      GtkWidget * content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
      GtkWidget * scale_entry = gtk_entry_new();
      gtk_entry_set_text(GTK_ENTRY(scale_entry), "3");
      gtk_container_add(GTK_CONTAINER(content_area), scale_entry);

      GtkWidget * xpos_entry = gtk_entry_new();
      gtk_entry_set_text(GTK_ENTRY(xpos_entry), "0");
      gtk_container_add(GTK_CONTAINER(content_area), xpos_entry);

      GtkWidget * zpos_entry = gtk_entry_new();
      gtk_entry_set_text(GTK_ENTRY(zpos_entry), "0");
      gtk_container_add(GTK_CONTAINER(content_area), zpos_entry);

      GtkWidget * directory_entry = gtk_entry_new();
      gtk_entry_set_text(GTK_ENTRY(directory_entry), MINECRAFT_PATH);
      gtk_container_add(GTK_CONTAINER(content_area), directory_entry);

      gtk_widget_show_all(dialog);

      if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
	  int scale = atoi((char *)gtk_entry_get_text(GTK_ENTRY(scale_entry)));
	  char * path = (char *)gtk_entry_get_text(GTK_ENTRY(directory_entry));
	  int x = atoi((char *)gtk_entry_get_text(GTK_ENTRY(xpos_entry)));
	  int z = atoi((char *)gtk_entry_get_text(GTK_ENTRY(zpos_entry)));
	  int rs = pow(2, scale) * 128;
	  block_info_t * blocks = read_region_files(path, x - (rs / 2), z - (rs / 2),
						    rs, rs);
	  add_buffer();
	  render_map(blocks, mdata[current_buffer], scale);
	  free(blocks);

	  mdata_info[current_buffer].scale = scale;
	  mdata_info[current_buffer].xpos = x;
	  mdata_info[current_buffer].zpos = z;
	  mdata_info[current_buffer].dimension = 0;
	  set_image();
	}
      gtk_widget_destroy(dialog);
    }
  else if((size_t)data == ITEM_SIGNAL_QUIT)
    {
      kill_window(NULL, NULL, NULL);
    }
  else
    printf("Unhandeled button press: %i\n", (int)(size_t)data);
}

static void button_click2(GtkWidget * widget, gpointer data)
{
  if(strcmp("button.zoomp", (char *)data) == 0)
    {
      GdkPixbuf * pixbuf = gtk_image_get_pixbuf(GTK_IMAGE(image));
      double h = gdk_pixbuf_get_height(pixbuf), w = gdk_pixbuf_get_width(pixbuf);
      if(h < config->maxzoom || w < config->maxzoom)
	{
	  GdkPixbuf * rpixbuf = gdk_pixbuf_scale_simple(dimage, h * config->zooms, w * config->zooms, config->interp);
	  gtk_image_set_from_pixbuf(GTK_IMAGE(image), rpixbuf);
	  g_object_unref(rpixbuf);
	}
    }
  else if(strcmp("button.zoomm", (char *)data) == 0)
    {
      GdkPixbuf * pixbuf = gtk_image_get_pixbuf(GTK_IMAGE(image));
      double h = gdk_pixbuf_get_height(pixbuf), w = gdk_pixbuf_get_width(pixbuf);
      if(h > config->minzoom || w > config->minzoom)
	{
	  GdkPixbuf * rpixbuf = gdk_pixbuf_scale_simple(dimage, h / config->zooms, w / config->zooms, config->interp);
	  gtk_image_set_from_pixbuf(GTK_IMAGE(image), rpixbuf);
	  g_object_unref(rpixbuf);
	}
    }
  else if(strcmp("button.zoome", (char *)data) == 0)
    {
      GdkPixbuf * rpixbuf = gdk_pixbuf_scale_simple(dimage, 128 * config->stdzoom, 128 * config->stdzoom, config->interp);
      gtk_image_set_from_pixbuf(GTK_IMAGE(image), rpixbuf);
      g_object_unref(rpixbuf);
    }
}

GdkPixbuf * create_pixbuf(const gchar * filename)
{
   GdkPixbuf *pixbuf;
   GError *error = NULL;
   pixbuf = gdk_pixbuf_new_from_file(filename, &error);
   if(!pixbuf) {
      fprintf(stderr, "%s\n", error->message);
      g_error_free(error);
   }

   return pixbuf;
}

static void construct_tool_bar_add(GtkWidget * menu, const char * text, size_t signal)
{
  GtkWidget * temp_item;
  temp_item = gtk_menu_item_new_with_label(text);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), temp_item);
  gtk_widget_show(temp_item);
  g_signal_connect_swapped(temp_item, "activate",
			   G_CALLBACK(button_click),
			   (gpointer)signal);
}

int main(int argc, char ** argv)
{
  GtkWidget * vbox;
  GtkWidget * hpaned;
  GtkWidget * sc_win, * sc_buffer;
  GtkWidget * menu_bar;
  GtkWidget * file_menu, * file_item;
  GtkWidget * generate_menu, * generate_item;
  GtkWidget * settings_menu, * settings_item;
  
  GtkWidget * zoom_box, * zoom_button;
  
  //init general
  colors = (color_t *)malloc(56 * sizeof(color_t));
  memset(mdata, 0, BUFFER_COUNT * sizeof(unsigned char *));
  memset(icons, 0, BUFFER_COUNT * sizeof(GtkWidget *));
  memset(icon_event_boxes, 0, BUFFER_COUNT * sizeof(GtkWidget *));
  mdata[current_buffer] = (unsigned char *)malloc(128 * 128);
  
  char * templine = malloc(13);
  FILE * fcolors = fopen("colors", "r");
  
  int i, r, g, b;
  for(i = 0; fgets(templine, 13, fcolors) == templine; i++)
    {
      sscanf(templine, "%i,%i,%i", &r, &g, &b);
      color_t color = {r, g, b};
      colors[i] = color;
    }
	
  free(templine);
  fclose(fcolors);
  
  save_colors(colors, "colors.bin");
  //load_colors(colors, "colors.bin");
  
  srand(time(NULL));
  
  config = config_new();
  
  //init gtk
  gtk_init(&argc, &argv);
  
  //window
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), "ImageToMap X v" VERSION_NUMBER);
  g_signal_connect(window, "delete_event", G_CALLBACK(kill_window), NULL);
  
  //vbox
#ifdef GTK2
  vbox = gtk_vbox_new(FALSE, 0);
#else
  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#endif
  gtk_container_add(GTK_CONTAINER (window), vbox);
  gtk_widget_show(vbox);
  
  //////menu_bar
  menu_bar = gtk_menu_bar_new();
  gtk_box_pack_start(GTK_BOX(vbox), menu_bar, FALSE, TRUE, 0);
  gtk_widget_show(menu_bar);
  
  ////////file_menu
  file_menu = gtk_menu_new();
  
  //////////file_menu items
  construct_tool_bar_add(file_menu, "Open", ITEM_SIGNAL_OPEN);
  construct_tool_bar_add(file_menu, "Save", ITEM_SIGNAL_SAVE);
  construct_tool_bar_add(file_menu, "Save Raw Map", ITEM_SIGNAL_SAVE_RM);
  construct_tool_bar_add(file_menu, "Export Image", ITEM_SIGNAL_EXPORT_IMAGE);
  construct_tool_bar_add(file_menu, "Render World", ITEM_SIGNAL_WORLD_RENDER_ITEM);
  construct_tool_bar_add(file_menu, "Quit", ITEM_SIGNAL_QUIT);
	
  /////////file_item
  file_item = gtk_menu_item_new_with_label("File");
  gtk_widget_show(file_item);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(file_item), file_menu);
  gtk_menu_shell_append((GtkMenuShell *)menu_bar, file_item);
  
  ////////generate_menu
  generate_menu = gtk_menu_new();
	
  //////////generate_menu items
  construct_tool_bar_add(generate_menu, "Mandelbrot", ITEM_SIGNAL_GENERATE_MANDELBROT);
  construct_tool_bar_add(generate_menu, "Julia", ITEM_SIGNAL_GENERATE_JULIA);
  construct_tool_bar_add(generate_menu, "Palette", ITEM_SIGNAL_GENERATE_PALETTE);
  construct_tool_bar_add(generate_menu, "Random Noise", ITEM_SIGNAL_GENERATE_RANDOM_NOISE);
   construct_tool_bar_add(generate_menu, "From Clipboard", ITEM_SIGNAL_GENERATE_FROM_CLIPBOARD);
   
  /////////generate_item
  generate_item = gtk_menu_item_new_with_label("Generate");
  gtk_widget_show(generate_item);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(generate_item), generate_menu);
  gtk_menu_shell_append((GtkMenuShell *)menu_bar, generate_item);

  ////////settings_menu
  settings_menu = gtk_menu_new();
  
  ////////settings_item
  settings_item = gtk_menu_item_new_with_label("Settings");
  gtk_widget_show(settings_item);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(settings_item), settings_menu);
  gtk_menu_shell_append((GtkMenuShell *)menu_bar, settings_item);

  //////////FSD_checkbox
  FSD_checkbox = gtk_check_menu_item_new_with_label("Floyd–Steinberg dithering");
  gtk_menu_shell_append(GTK_MENU_SHELL(settings_menu), FSD_checkbox);
  gtk_widget_show(FSD_checkbox);
  
  //drop_down_menu
  init_drop_down_menu();

  //hpaned
#ifdef GTK2
  hpaned = gtk_hpaned_new();
#else
  hpaned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
#endif
  gtk_widget_set_size_request (hpaned, 220, -1);
  gtk_box_pack_start(GTK_BOX(vbox), hpaned, TRUE, TRUE, 0);
  gtk_widget_show(hpaned);

  ////sc_buffer
  sc_buffer = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sc_buffer), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
#ifdef GTK2
  gtk_widget_set_size_request(sc_buffer, 128 + 32, 512);
#else
  gtk_scrolled_window_set_min_content_width(GTK_SCROLLED_WINDOW(sc_buffer), 128 + 32);
  gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(sc_buffer), 512);
#endif
  gtk_paned_pack2(GTK_PANED(hpaned), sc_buffer, FALSE, FALSE);
  gtk_widget_show(sc_buffer);

  //////list_vbox
#ifdef GTK2
  list_vbox = gtk_vbox_new(FALSE, 0);
#else
  list_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#endif
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(sc_buffer), list_vbox);
  gtk_widget_show(list_vbox);

  ////sc_win
  sc_win = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sc_win), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
#ifdef GTK2
  gtk_widget_set_size_request(sc_win, 128 * 4, 128 * 4);
  gtk_window_resize(GTK_WINDOW(window), 128 * 4 + 200, 128 * 4 + 70);
#else
  gtk_scrolled_window_set_min_content_width(GTK_SCROLLED_WINDOW(sc_win), 128 * 4);
  gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(sc_win), 128 * 4);
#endif
  gtk_paned_pack1(GTK_PANED(hpaned), sc_win, TRUE, FALSE);
  gtk_widget_show(sc_win);
	
  //////image
  dimage = gdk_pixbuf_new_from_file("start.png", NULL);
  image = gtk_image_new();
  gtk_image_set_from_pixbuf(GTK_IMAGE(image), dimage);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(sc_win), image);
  gtk_widget_show(image);
	
  ////zoom_box
#ifdef GTK2
  zoom_box = gtk_hbox_new(FALSE, 0);
#else
  zoom_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#endif
  gtk_box_pack_start(GTK_BOX(vbox), zoom_box, FALSE, FALSE, 0);
  gtk_widget_show(zoom_box);
	
  //////zoom_button (+)
  zoom_button = gtk_button_new_with_label("+");
	
  gtk_box_pack_start(GTK_BOX(zoom_box), zoom_button, TRUE, TRUE, 2);
  g_signal_connect(zoom_button, "clicked", G_CALLBACK(button_click2), "button.zoomp");
  gtk_widget_show(zoom_button);
	
  //////zoom_button (|)
  zoom_button = gtk_button_new_with_label("|");
	
  gtk_box_pack_start(GTK_BOX(zoom_box), zoom_button, TRUE, TRUE, 2);
  g_signal_connect(zoom_button, "clicked", G_CALLBACK(button_click2), "button.zoome");
  gtk_widget_show(zoom_button);
	
  //////zoom_button (-)
  zoom_button = gtk_button_new_with_label("-");
	
  gtk_box_pack_start(GTK_BOX(zoom_box), zoom_button, TRUE, TRUE, 2);
  g_signal_connect(zoom_button, "clicked", G_CALLBACK(button_click2), "button.zoomm");
  gtk_widget_show(zoom_button);
	
  //icon
  gtk_window_set_icon(GTK_WINDOW(window), create_pixbuf("imagetomap.ico"));
  
  //display window
  gtk_widget_show (window);
  gtk_main();
	
  //clean up
  free(colors);
  for(i = 0; i < BUFFER_COUNT; i++)
    free(mdata[i]);
  config_free(config);
	
  return 0;
}
