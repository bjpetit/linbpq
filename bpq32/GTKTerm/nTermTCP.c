

#define GTK_ENABLE_BROKEN
#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>

#ifdef WIN32
#include "winsock2.h"
#include "WS2tcpip.h"

#else

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>

#define SOCKET int
#define closesocket close
typedef gint32   COLORREF;
#define RGB(r,g,b)          ((COLORREF)(((guint8)(r)|((guint16)((guint8)(g))<<8))|(((guint32)(guint8)(b))<<16)))

#endif

char Host[5][100];
char Port[5][10];
char UserName[5][80];
char Password[5][80];

char HN[5][6] = {"Host1", "Host2", "Host3", "Host4"};
char PN[5][6] = {"Port1", "Port2", "Port3", "Port4"};
char PASSN[5][6] = {"Pass1", "Pass2", "Pass3", "Pass4"};
char UN[5][6] = {"User1", "User2", "User3", "User4"};

int CurrentHost = 0;
char VersionString[80] = "0.0.0.2";

char DisMsg[] = "*** Disconnected\r";

int PortMask=65535;
int mtxparam=1;
int MCOM=1;
int Split;

int Bells = FALSE;
int StripLF = FALSE;
int LogMonitor = FALSE;
int LogOutput = FALSE;
int SendDisconnected = TRUE;
int MonNODES = FALSE;
int MONColour = TRUE;
int ChatMode = FALSE;
int MonPorts = 1;

int left = 100, right = 600, top = 100, bottom = 600;

int Connecting = FALSE;
int Disconnecting = FALSE;
int Connected = FALSE;
int SocketActive = FALSE;

char Title[80];

void ReadConfig();

int TCPConnect(char * Host, char * Port);
void WritetoOutputWindow(const char * Text, int Len);
void WritetoMonWindow(char * Text, int Len);
int Telnet_Connected(SOCKET sock, int Error);
int SendMsg(const char * msg, int len);
void SendTraceOptions();

#ifdef WIN32

void __cdecl Debugprintf(const char * format, ...)
{
	char Mess[1000];
	va_list(arglist);

	va_start(arglist, format);
	vsprintf(Mess, format, arglist);
	strcat(Mess, "\r\n");
	OutputDebugString(Mess);

	return;
}
#endif

COLORREF Colours[256] = {0,
		RGB(0,0,0), RGB(0,0,128), RGB(0,0,192), RGB(0,0,255),				// 1 - 4
		RGB(0,64,0), RGB(0,64,128), RGB(0,64,192), RGB(0,64,255),			// 5 - 8
		RGB(0,128,0), RGB(0,128,128), RGB(0,128,192), RGB(0,128,255),		// 9 - 12
		RGB(0,192,0), RGB(0,192,128), RGB(0,192,192), RGB(0,192,255),		// 13 - 16
		RGB(0,255,0), RGB(0,255,128), RGB(0,255,192), RGB(0,255,255),		// 17 - 20

		RGB(64,0,0), RGB(64,0,128), RGB(64,0,192), RGB(0,0,255),				// 21
		RGB(64,64,0), RGB(64,64,128), RGB(64,64,192), RGB(64,64,255),
		RGB(64,128,0), RGB(64,128,128), RGB(64,128,192), RGB(64,128,255),
		RGB(64,192,0), RGB(64,192,128), RGB(64,192,192), RGB(64,192,255),
		RGB(64,255,0), RGB(64,255,128), RGB(64,255,192), RGB(64,255,255),

		RGB(128,0,0), RGB(128,0,128), RGB(128,0,192), RGB(128,0,255),				// 41
		RGB(128,64,0), RGB(128,64,128), RGB(128,64,192), RGB(128,64,255),
		RGB(128,128,0), RGB(128,128,128), RGB(128,128,192), RGB(128,128,255),
		RGB(128,192,0), RGB(128,192,128), RGB(128,192,192), RGB(128,192,255),
		RGB(128,255,0), RGB(128,255,128), RGB(128,255,192), RGB(128,255,255),

		RGB(192,0,0), RGB(192,0,128), RGB(192,0,192), RGB(192,0,255),				// 61
		RGB(192,64,0), RGB(192,64,128), RGB(192,64,192), RGB(192,64,255),
		RGB(192,128,0), RGB(192,128,128), RGB(192,128,192), RGB(192,128,255),
		RGB(192,192,0), RGB(192,192,128), RGB(192,192,192), RGB(192,192,255),
		RGB(192,255,0), RGB(192,255,128), RGB(192,255,192), RGB(192,2552,255),

		RGB(255,0,0), RGB(255,0,128), RGB(255,0,192), RGB(255,0,255),				// 81
		RGB(255,64,0), RGB(255,64,128), RGB(255,64,192), RGB(255,64,255),
		RGB(255,128,0), RGB(255,128,128), RGB(255,128,192), RGB(255,128,255),
		RGB(255,192,0), RGB(255,192,128), RGB(255,192,192), RGB(255,192,255),
		RGB(255,255,0), RGB(255,255,128), RGB(255,255,192), RGB(255,2552,255)
};


SOCKET RecvSocket;
SOCKET sock;

GtkWidget *dialog;
GtkWidget *window;
GtkWidget *box1;
GtkWidget *box2;
GtkWidget *hbox;
GtkWidget *button;
GtkWidget *check;
GtkWidget *separator;
GtkWidget *table;
GtkWidget *vscrollbar;
GtkWidget *vscrollbar2;
GtkTextBuffer *text;
GtkTextBuffer *text2;
GtkWidget *entry;

GIOChannel *RecvChannel;
GtkWidget *menubar;
GtkWidget *view;
GtkWidget* scrolledwin;
GtkWidget *view2;
GtkWidget* scrolledwin2;

GtkTextTag *rtag, *btag, *tag[256], *tagm[256];

void EnableDisconnectMenu();
void DisableConnectMenu();
void EnableConnectMenu();
void DisableDisconnectMenu(V);

void close_application( GtkWidget *widget, gpointer   data )
{
       gtk_main_quit ();
}

void enter_callback( GtkWidget *widget,
                     GtkWidget *entry )
{
	const gchar *entry_text;
	entry_text = gtk_entry_get_text (GTK_ENTRY (entry));

	if (!Connected && ! Connecting)
	{
		TCPConnect(Host[CurrentHost], Port[CurrentHost]);
		gtk_entry_set_text (GTK_ENTRY (entry), "");
		return ;
	}

	SendMsg(entry_text, strlen(entry_text));
	SendMsg("\r", 1);
	WritetoOutputWindow(entry_text, strlen(entry_text));
	WritetoOutputWindow("\r ", 1);
	gtk_entry_set_text (GTK_ENTRY (entry), "");
}


static void Disconnect(GtkWidget *w, gpointer data);

static void Configure( GtkWidget *w, gpointer data )
{
	GtkWidget * dialog = gtk_dialog_new_with_buttons( "Configuration",
                                              GTK_WINDOW(window),
                                              GTK_DIALOG_MODAL,
                                              GTK_STOCK_OK, 1,
                                              GTK_STOCK_CANCEL,  2,
                                              NULL );

	GtkWidget *entry1;
	GtkWidget *entry2;
	GtkWidget *entry3;
	GtkWidget *entry4;

	GtkWidget *label, *content_area;
	GtkWidget *label2, *label3, *label4;

	int HostNum = (int)data;
	const gchar *entry_text;
	gint result;
	char Key[10];

	content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

	label = gtk_label_new ("Host");
	label2 = gtk_label_new ("Port");
	label3 = gtk_label_new ("Username");
	label4 = gtk_label_new ("Password");

	/* Add the label, and show everything we've added to the dialog. */

	gtk_container_add (GTK_CONTAINER (content_area), label);

	entry1 = gtk_entry_new();
	gtk_entry_set_max_length (GTK_ENTRY (entry1), 100);
	gtk_entry_set_text (GTK_ENTRY (entry1), &Host[HostNum][0]);

	gtk_container_add (GTK_CONTAINER (content_area), entry1);

	gtk_container_add (GTK_CONTAINER (content_area), label2);
	entry2 = gtk_entry_new();
	gtk_entry_set_max_length (GTK_ENTRY (entry2), 10);
	gtk_entry_set_text (GTK_ENTRY (entry2), &Port[HostNum][0]);

	gtk_container_add (GTK_CONTAINER (content_area), entry2);

	gtk_container_add (GTK_CONTAINER (content_area), label3);

   	entry3 = gtk_entry_new();
	gtk_entry_set_max_length (GTK_ENTRY (entry3), 100);
	gtk_entry_set_text (GTK_ENTRY (entry3), &UserName[HostNum][0]);
	gtk_container_add (GTK_CONTAINER (content_area), entry3);

	gtk_container_add (GTK_CONTAINER (content_area), label4);

	entry4 = gtk_entry_new();
	gtk_entry_set_max_length (GTK_ENTRY (entry4), 100);
	gtk_entry_set_text (GTK_ENTRY (entry4), &Password[HostNum][0]);
	gtk_container_add (GTK_CONTAINER (content_area), entry4);

	gtk_widget_show_all (dialog);

	result = gtk_dialog_run (GTK_DIALOG (dialog));

	if (result == 1)
	{
		GKeyFile * KF;
		gchar * Value;
		GError *error = NULL;
		gsize length;
		FILE *outfile;

		entry_text = gtk_entry_get_text (GTK_ENTRY (entry1));
		strcpy(&Host[HostNum][0], entry_text);

		entry_text = gtk_entry_get_text (GTK_ENTRY (entry2));
		strcpy(&Port[HostNum][0], entry_text);

		entry_text = gtk_entry_get_text (GTK_ENTRY (entry3));
		strcpy(&UserName[HostNum][0], entry_text);

		entry_text = gtk_entry_get_text (GTK_ENTRY (entry4));
		strcpy(&Password[HostNum][0], entry_text);

		KF = g_key_file_new();
		g_key_file_load_from_file(KF, "BPQTermTCP.ini", 0, NULL);

		sprintf(Key, "Host%d", HostNum + 1);
		g_key_file_set_string(KF, "Session 1", Key, &Host[HostNum][0]);

		sprintf(Key, "Port%d", HostNum + 1);
		g_key_file_set_string(KF, "Session 1", Key, &Port[HostNum][0]);

		sprintf(Key, "UserName%d", HostNum + 1);
		g_key_file_set_string(KF, "Session 1", Key, &UserName[HostNum][0]);

		sprintf(Key, "Password%d", HostNum + 1);
		g_key_file_set_string(KF, "Session 1", Key, &Password[HostNum][0]);


		Value = g_key_file_to_data(KF, &length, &error);

		outfile = fopen ("BPQTermTCP.ini", "w");
		fputs(Value, outfile);
		fclose(outfile);

		g_free(Value);

		g_key_file_free(KF);
    }

	gtk_widget_destroy (dialog);

}
static void Connect( GtkWidget *w, gpointer  data )
{
	CurrentHost = (int)data;
	TCPConnect(Host[CurrentHost], Port[CurrentHost]);
}

static void Toggled( GtkWidget *w, int * data )
{
	int NewVal = gtk_check_menu_item_get_active((GtkCheckMenuItem *)w);
	*(data) = NewVal;

	SendTraceOptions();

	return;
}


GtkWidget *montx;
GtkWidget *monsup;
GtkWidget *monnode;
GtkWidget *encol;
GtkWidget *addpor;
GtkWidget *menubar;
GtkWidget *conmenu, *conn_item, *Conn[4], *Conn2, *Conn3, *Conn4;
GtkWidget *dismenu, *dis_item;
GtkWidget *cfgmenu, *cfg_item, *Cfg[4], *Cfg2, *Cfg3, *Cfg4;
GtkWidget *monmenu, *mon_item, *Mon[10];

GtkWidget *get_menubar_menu(GtkWidget  *window)
{
	int i;

	menubar = gtk_menu_bar_new();

	conmenu = gtk_menu_new();
	cfgmenu = gtk_menu_new();
    dismenu = gtk_menu_new();
    monmenu = gtk_menu_new();
    montx = gtk_menu_new();
    monsup = gtk_menu_new();
    monnode = gtk_menu_new();
    encol = gtk_menu_new();
    addpor = gtk_menu_new();

    /* Create the menu items */

	for (i = 0; i < 4; i++)
	{
		Conn[i] = gtk_menu_item_new_with_label (Host[i]);
		gtk_menu_shell_append (GTK_MENU_SHELL (conmenu), Conn[i]);
		Cfg[i] = gtk_menu_item_new_with_label (Host[i]);
	    gtk_menu_shell_append (GTK_MENU_SHELL (cfgmenu), Cfg[i]);


		/* Attach the callback functions to the activate signal */

		g_signal_connect (Conn[i], "activate", G_CALLBACK (Connect), (void *) i);
		g_signal_connect (Cfg[i], "activate", G_CALLBACK (Configure), (void *) i);


	}
	
	montx = gtk_check_menu_item_new_with_label ("Monitor TX");
	gtk_check_menu_item_set_active((GtkCheckMenuItem *)montx, mtxparam);
	g_signal_connect (montx, "toggled", G_CALLBACK (Toggled), (void *) &mtxparam);

	monsup = gtk_check_menu_item_new_with_label ("Monitor Supervisor");
	gtk_check_menu_item_set_active((GtkCheckMenuItem *)monsup, MCOM);
	g_signal_connect (monsup, "toggled", G_CALLBACK (Toggled), (void *) &MCOM);

    monnode = gtk_check_menu_item_new_with_label ("Monitor Nodes");
	gtk_check_menu_item_set_active((GtkCheckMenuItem *)monnode, MonNODES);
	g_signal_connect (monnode, "toggled", G_CALLBACK (Toggled), (void *) &MonNODES);



    encol = gtk_check_menu_item_new_with_label ("Enable Colour");
    addpor = gtk_menu_item_new_with_label ("Add Port");

    conn_item = gtk_menu_item_new_with_label ("Connect");
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (conn_item), conmenu);

    dis_item = gtk_menu_item_new_with_label ("Disconnect");
	g_signal_connect (dis_item, "activate", G_CALLBACK (Disconnect), 0);
	gtk_widget_set_sensitive(dis_item, 0);
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (dis_item), dismenu);

    cfg_item = gtk_menu_item_new_with_label ("Setup");
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (cfg_item), cfgmenu);

	mon_item = gtk_menu_item_new_with_label ("Monitor");
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (mon_item), monmenu);


    gtk_menu_shell_append ((GtkMenuShell *)menubar, conn_item);
	gtk_menu_shell_append ((GtkMenuShell *)menubar, dis_item);
	gtk_menu_shell_append ((GtkMenuShell *)menubar, cfg_item);
	gtk_menu_shell_append ((GtkMenuShell *)menubar, mon_item);
	gtk_menu_shell_append ((GtkMenuShell *)monmenu, montx);
	gtk_menu_shell_append ((GtkMenuShell *)monmenu, monsup);
	gtk_menu_shell_append ((GtkMenuShell *)monmenu, monnode);
	gtk_menu_shell_append ((GtkMenuShell *)monmenu, encol);
	gtk_menu_shell_append ((GtkMenuShell *)monmenu, addpor);


	gtk_widget_show_all (menubar);

	return menubar;
}


gint ScrollTimer(gpointer data)
{
	GtkTextIter iter;

	gtk_text_buffer_get_end_iter(text, &iter);
	gtk_text_view_scroll_to_iter ((GtkTextView *)view, &iter, 0.0, FALSE, 0.0, 0.0);

	gtk_text_buffer_get_end_iter(text2, &iter);
	gtk_text_view_scroll_to_iter ((GtkTextView *)view2, &iter, 0.0, FALSE, 0.0, 0.0);

	return FALSE;
}

static fd_set readfs;
static fd_set writefs;
static fd_set errorfs;
static struct timeval timeout;

int MonData = FALSE;


//int Connecting = FALSE;
//int Disconnecting = FALSE;
//int Connected = FALSE;
//int SocketActive = FALSE;

int ProcessReceivedData();

gint PollTimer(gpointer data)
{
	FD_ZERO(&readfs);

	if (Connecting ||Connected)
		FD_SET(sock,&errorfs);
	else
		return TRUE;

	if (Connected) FD_SET(sock,&readfs);

	FD_ZERO(&writefs);

	if (Connecting) FD_SET(sock,&writefs);	// Need notification of Connect

	FD_ZERO(&errorfs);

	if (Connecting ||Connected) FD_SET(sock,&errorfs);

	if (select(sock + 1, &readfs, &writefs, &errorfs, &timeout) > 0)
	{
		//	See what happened

		if (FD_ISSET(sock, &readfs))
		{
			// data available

			ProcessReceivedData();

		}

		if (FD_ISSET(sock, &writefs))
		{
			//	Connect success

			Connecting = FALSE;
			Connected = TRUE;

		}

		if (FD_ISSET(sock, &errorfs))
		{
			//	if connecting, then failed, if connected then has just disconnected

			if (Connecting)
			{
				// Falied

				Connecting = FALSE;
				Connected = FALSE;
			}
			else
			{
				if (SocketActive)
					closesocket(sock);
				else
					return TRUE;

				sprintf(Title,"BPQTermTCP Version %s - Disconnected", VersionString);
				gtk_window_set_title (GTK_WINDOW (window), Title);
				DisableDisconnectMenu();
				EnableConnectMenu();

				WritetoOutputWindow(DisMsg, strlen(DisMsg));
				SocketActive = FALSE;
				Connected = FALSE;
				Disconnecting = FALSE;
				MonData = FALSE;
				return TRUE;

			}
		}
	}

	return TRUE;
}


int main(int argc, char *argv[])
{
	PangoFontDescription *font_desc;
	int i;

 #ifdef WIN32
	WSADATA WsaData;            // receives data from WSAStartup
	WSAStartup(MAKEWORD(2, 0), &WsaData);
#endif
	gtk_init (&argc, &argv);

	ReadConfig();

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW (window), right - left, bottom - top);
	gtk_widget_set_uposition(window, left, top);
//	gtk_window_set_resizable  (GTK_WINDOW (window), TRUE);
	g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (close_application), NULL);
	gtk_window_set_title (GTK_WINDOW (window), "BPQTermTCP");
	gtk_container_set_border_width (GTK_CONTAINER (window), 0);

	// Create a box for the menu

	box1 = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (window), box1);

	menubar = get_menubar_menu (window);
    gtk_box_pack_start (GTK_BOX (box1), menubar, FALSE, TRUE, 1);

	// Create another box for the text windows

	box2 = gtk_vbox_new (FALSE, 10);
	gtk_container_set_border_width (GTK_CONTAINER (box2), 10);
	gtk_box_pack_start (GTK_BOX (box1), box2, TRUE, TRUE, 0);

	// Create a table for the 2 text windows and the input line

	table = gtk_table_new (3, 1, FALSE);
	gtk_table_set_row_spacing (GTK_TABLE (table), 0, 2);
	gtk_table_set_col_spacing (GTK_TABLE (table), 0, 2);
	gtk_box_pack_start (GTK_BOX (box2), table, TRUE, TRUE, 0);

	view = gtk_text_view_new ();
	text = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
	gtk_widget_set_size_request (view, 600, 100);

	scrolledwin = gtk_scrolled_window_new(NULL,NULL);
	gtk_container_set_border_width(GTK_CONTAINER(scrolledwin), 1);
	gtk_widget_set_size_request(scrolledwin, 600, 100);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwin),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolledwin), GTK_SHADOW_IN);
	gtk_widget_show(scrolledwin);
	gtk_container_add(GTK_CONTAINER(scrolledwin), view);

     gtk_table_attach (GTK_TABLE (table), scrolledwin,0, 1, 0, 1,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);

	// Create output view

	view2 = gtk_text_view_new ();
	text2 = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view2));

	scrolledwin2 = gtk_scrolled_window_new(NULL,NULL);
	gtk_container_set_border_width(GTK_CONTAINER(scrolledwin2), 2);
	gtk_widget_set_size_request(scrolledwin2, 600, 100);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwin2),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolledwin2), GTK_SHADOW_IN);
	gtk_container_add(GTK_CONTAINER(scrolledwin2), view2);

	gtk_table_attach (GTK_TABLE (table), scrolledwin2, 0, 1, 1, 2,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);

	separator = gtk_hseparator_new ();
	gtk_box_pack_start (GTK_BOX (box1), separator, FALSE, TRUE, 0);

	box2 = gtk_vbox_new (FALSE, 10);
	gtk_container_set_border_width (GTK_CONTAINER (box2), 10);
	gtk_box_pack_start (GTK_BOX (box1), box2, FALSE, FALSE, 0);

	// set up the text entry line

	entry = gtk_entry_new();
	gtk_entry_set_max_length (GTK_ENTRY (entry), 100);
	g_signal_connect (G_OBJECT (entry), "activate", G_CALLBACK (enter_callback), (gpointer) entry);

	gtk_box_pack_start (GTK_BOX (box2), entry, FALSE, FALSE, 0);

	/* Change default font throughout the widget */

	font_desc = pango_font_description_from_string ("monospace 10");
	gtk_widget_modify_font (entry, font_desc);
	gtk_widget_modify_font (view, font_desc);
	gtk_widget_modify_font (view2, font_desc);
	pango_font_description_free (font_desc);



	gtk_widget_show_all (window);

	rtag = gtk_text_buffer_create_tag (text, NULL, "foreground", "red", NULL);
	btag = gtk_text_buffer_create_tag (text, NULL, "foreground", "blue", NULL);

	for (i = 0; i < 100; i++)
	{
		tag[i] = gtk_text_buffer_create_tag (text2, NULL, "foreground", "red", NULL);
		tag[i]->values->appearance.fg_color.red = (Colours[i] & 0xff) << 8;
		tag[i]->values->appearance.fg_color.green = (Colours[i] & 0xff00);
		tag[i]->values->appearance.fg_color.blue = (Colours[i] & 0xff0000) >> 8;
	}

	for (i = 0; i < 100; i++)
	{
		tagm[i] = gtk_text_buffer_create_tag (text, NULL, "foreground", "red", NULL);
		tagm[i]->values->appearance.fg_color.red = (Colours[i] & 0xff) << 8;
		tagm[i]->values->appearance.fg_color.green = (Colours[i] & 0xff00);
		tagm[i]->values->appearance.fg_color.blue = (Colours[i] & 0xff0000) >> 8;
	}

	g_timeout_add (200, PollTimer, 0);

	gtk_main ();

	return 0;
}

void SendTraceOptions()
{
	char Buffer[80];

	int Len = sprintf(Buffer,"\\\\\\\\%x %x %x %x %x\r", PortMask, mtxparam, MCOM, MonNODES, MONColour);

	send(sock, Buffer, Len, 0);

}

char Save[1000];
int SaveLen;

void WritetoOutputWindow(const char * Msg, int len)
{
	const char * ptr1 = Msg;
	char * ptr2;
	GtkTextIter iter;
	GtkTextIter enditer;
	int start, end;
	GtkTextTag *mtag;

	if (SaveLen)
	{
		// Have part line - append to it
		memcpy(&Save[SaveLen], Msg, len);
		SaveLen += len;
		ptr1 = Save;
		len = SaveLen;
		SaveLen = 0;
	}

lineloop:

	if (len <=  0)
	{
		g_timeout_add (100, ScrollTimer, 0);
		return;
	}

	//	copy text to control a line at a time

	ptr2 = memchr(ptr1, 13, len);

	if (ptr2 == 0)	// No CR
	{
		memmove(Save, ptr1, len);
		SaveLen = len;
		return;
	}

//	*(ptr2++) = 0;

	if (ptr1[0] == 0x1b)
	{
		mtag = tag[ptr1[1] - 10];

		gtk_text_buffer_get_end_iter(text2, &iter);
		start = gtk_text_buffer_get_char_count(text2);
		gtk_text_buffer_insert(text2, &iter, ptr1 + 2, (ptr2 - ptr1) - 2);
		end = gtk_text_buffer_get_char_count(text2);

		gtk_text_buffer_get_iter_at_offset (text2, &iter, start);
		gtk_text_buffer_get_iter_at_offset (text2, &enditer, end);
		gtk_text_buffer_apply_tag (text2, mtag, &iter, &enditer);

	}
	else
	{
//		gtk_text_insert (GTK_TEXT (text), fixed_font, &text->style->black, NULL, ptr1, -1);
		gtk_text_buffer_get_end_iter(text2, &iter);
		gtk_text_buffer_insert(text2, &iter, ptr1, ptr2 - ptr1);

	}

//	gtk_text_insert (GTK_TEXT (text), fixed_font, &text->style->black, NULL, "\n", -1);
	gtk_text_buffer_get_end_iter(text2, &iter);
	gtk_text_buffer_insert(text2, &iter,  "\n", -1);
//	gtk_text_view_place_cursor_onscreen (view2);
	gtk_text_buffer_get_end_iter(text2, &iter);
	gtk_text_view_scroll_to_iter ((GtkTextView *)view2, &iter, 0.0, FALSE, 0.0, 0.0);


//	if (LogMonitor) WriteMonitorLine(ptr1, ptr2 - ptr1);

	len -= (++ptr2 - ptr1);
	ptr1 = ptr2;

	goto lineloop;

}

char MonSave[1000];
int MonSaveLen;

void WritetoMonWindow(char * Msg, int len)
{
	char * ptr1 = Msg, * ptr2;
	GtkTextIter iter;
	GtkTextIter enditer;
	int start, end;
	GtkTextTag *mtag;

	if (MonSaveLen)
	{
		// Have part line - append to it
		memcpy(&MonSave[MonSaveLen], Msg, len);
		MonSaveLen += len;
		ptr1 = MonSave;
		len = MonSaveLen;
		MonSaveLen = 0;
	}

lineloop:

	if (len <=  0)
	{
		g_timeout_add (100, ScrollTimer, 0);
		return;
	}
	//	copy text to control a line at a time

	ptr2 = memchr(ptr1, 13, len);

	if (ptr2 == 0)	// No CR
	{
		memmove(MonSave, ptr1, len);
		MonSaveLen = len;
		return;
	}

	if (ptr1[0] == 0x1b)
	{
		mtag = tagm[ptr1[1] - 10];

		gtk_text_buffer_get_end_iter(text, &iter);
		start = gtk_text_buffer_get_char_count(text);
		gtk_text_buffer_insert(text, &iter, ptr1 + 2, (ptr2 - ptr1) - 2);
		end = gtk_text_buffer_get_char_count(text);

		gtk_text_buffer_get_iter_at_offset (text, &iter, start);
		gtk_text_buffer_get_iter_at_offset (text, &enditer, end);
		gtk_text_buffer_apply_tag (text, mtag, &iter, &enditer);
	}
	else
	{
		gtk_text_buffer_get_end_iter(text, &iter);
		gtk_text_buffer_insert(text, &iter, ptr1, (ptr2 - ptr1));

	}

	gtk_text_buffer_get_end_iter(text, &iter);
	gtk_text_buffer_insert(text, &iter,  "\n", 1);

	 gtk_text_view_scroll_to_iter ((GtkTextView *)view, &iter, 0.0, FALSE, 0.0, 0.0);

//	if (LogMonitor) WriteMonitorLine(ptr1, ptr2 - ptr1);

	len -= (++ptr2 - ptr1);
	ptr1 = ptr2;

	goto lineloop;
}


int SendMsg(const char * msg, int len)
{
	send(sock, msg, len, 0);
	return 0;
}

int TCPConnect(char * Host, char * Port)
{

	int err = 0;
	u_long param=1;
	int bcopt=TRUE;
	struct sockaddr_in sinx;
	int addrlen=sizeof(sinx);
	char Title[80];

	struct addrinfo hints, *res = NULL;

	Disconnecting = FALSE;

	// get host info, make socket, and connect it

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;  // use IPv4 or IPv6, whichever
	hints.ai_socktype = SOCK_STREAM;
	getaddrinfo(Host, Port, &hints, &res);

	if (!res)
	{
		dialog = gtk_message_dialog_new ((GtkWindow *)window,
                                  GTK_DIALOG_DESTROY_WITH_PARENT,
                                  GTK_MESSAGE_WARNING,
                                  GTK_BUTTONS_OK,
                                  "Resolve HostName Failed");

		gtk_window_set_title (GTK_WINDOW (dialog), "BPQTermTCP");

		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);


		sprintf(Title,"BPQTermTCP Version %s - Disconnected", VersionString);

		gtk_window_set_title (GTK_WINDOW (window), Title);

		return FALSE;			// Resolve failed

	}

	sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

	if (connect(sock, res->ai_addr, res->ai_addrlen) == 0)
	{
		//
		//	Connected successful
		//

		Telnet_Connected(sock, 0);

		return TRUE;
	}
	else
	{
		err=errno;

		if (err == 10035)
		{
			//	Connect in Progress

			sprintf(Title,"BPQTermTCP Version %s - Connecting to %s", VersionString, Host);
			gtk_window_set_title (GTK_WINDOW (window), Title);

			EnableDisconnectMenu();
			DisableConnectMenu();

			return TRUE;
		}
		else
		{
			//	Connect failed

			closesocket(sock);
			dialog = gtk_message_dialog_new ((GtkWindow *)window,
                GTK_DIALOG_DESTROY_WITH_PARENT,
                GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, "Connect Failed");

			gtk_window_set_title (GTK_WINDOW (dialog), "BPQTermTCP");

			gtk_dialog_run (GTK_DIALOG (dialog));
			gtk_widget_destroy (dialog);

			return FALSE;
		}
	}

	return FALSE;
}

#define MAX_MSG_LEN 512


int ProcessReceivedData()
{
	char message[MAX_MSG_LEN + 10];
	gchar * ptr;
	char * Buffptr;
	char * FEptr = 0;
	int len = 0, MonLen;

	len = recv(sock, message, MAX_MSG_LEN, 0);

	if (len <= 0)
	{
		if (Disconnecting == FALSE)
		{
			shutdown(sock, 2);		// SD_BOTH
			Disconnecting = TRUE;
		}
		else
			if (SocketActive)
				closesocket(sock);

		sprintf(Title,"BPQTermTCP Version %s - Disconnected", VersionString);
		gtk_window_set_title (GTK_WINDOW (window), Title);
		DisableDisconnectMenu();
		EnableConnectMenu();

		WritetoOutputWindow(DisMsg, strlen(DisMsg));
		SocketActive = FALSE;
		Connected = FALSE;
		MonData = FALSE;
		return TRUE;
	}

	if (len == 0)
	{
		printf("recv - len = 0\r\n");
		if (Disconnecting == FALSE)
		{
			shutdown(sock, 2);		// SD_BOTH
			Disconnecting = TRUE;
		}
		else
			closesocket(sock);

		return TRUE;
	}

	message[len] = 0;

	// Look for MON delimiters (FF/FE)

	Buffptr = message;

	if (MonData)
	{
		// Already in MON State

		FEptr = memchr(Buffptr, 0xfe, len);

		if (!FEptr)
		{
			// no FE - so send all to monitor

			WritetoMonWindow(Buffptr, len);
			return TRUE;
		}

		MonData = FALSE;

		MonLen = FEptr - Buffptr;		// Mon Data, Excluding the FE

		WritetoMonWindow(Buffptr, MonLen);

		Buffptr = ++FEptr;				// Char following FE

		if (++MonLen < len)
		{
			len -= MonLen;
			goto MonLoop;				// See if next in MON or Data
		}

		// Nothing Left

		return TRUE;
	}

MonLoop:

	ptr = memchr(Buffptr, 0xff, len);

	if (ptr)
	{
		// Buffer contains Mon Data

		if (ptr > Buffptr)
		{
			// Some Normal Data before the FF

			int NormLen = ptr - Buffptr;				// Before the FF
			WritetoOutputWindow(Buffptr, NormLen);

			len -= NormLen;
			Buffptr = ptr;
			goto MonLoop;
		}

		MonData = TRUE;

		FEptr = memchr(Buffptr, 0xfe, len);

		if (FEptr)
		{
			MonData = FALSE;

			MonLen = FEptr + 1 - Buffptr;				// MonLen includes FF and FE
			WritetoMonWindow(Buffptr+1, MonLen - 2);

			len -= MonLen;
			Buffptr += MonLen;							// Char Following FE

			if (len <= 0)
			{
				return TRUE;
			}
			goto MonLoop;
		}
		else
		{
			// No FE, so rest of buffer is MON Data

			WritetoMonWindow(Buffptr+1, len -1);		// Exclude FF
//			DoRefresh();
			return TRUE;
		}
	}

	// No FF, so must be session data

	WritetoOutputWindow(Buffptr, len);
//	SlowTimer = 0;

	return TRUE;
}
/*
gboolean GtkMsg_ShowMessage(GIOChannel *channel, GIOCondition  condition, gpointer data)
{
	gchar message[MAX_MSG_LEN + 10];
//    GtkTextMark* MarkEnd;
	gchar * ptr;
	char * Buffptr;
	char * FEptr = 0;
	int len = 0, MonLen;


//    GtkWidget *widgetMsgList = lookup_widget(MainWindow, "textview1");
//    GtkTextBuffer *textMsgList = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widgetMsgList));

	if (condition & G_IO_HUP)
	{
		if (SocketActive)
			closesocket(sock);
		else
			return 0;

		sprintf(Title,"BPQTermTCP Version %s - Disconnected", VersionString);
		gtk_window_set_title (GTK_WINDOW (window), Title);
		DisableDisconnectMenu();
		EnableConnectMenu();

		WritetoOutputWindow(DisMsg, strlen(DisMsg));
		SocketActive = FALSE;
		Connected = FALSE;
		Disconnecting = FALSE;
		MonData = FALSE;
		return TRUE;
	}

	if ((condition & G_IO_IN) == 0)
		return TRUE;

    g_io_channel_read_chars (channel, message, MAX_MSG_LEN, &len, NULL);

	if (len == 0)
	{
		if (Disconnecting == FALSE)
		{
			shutdown(sock, 2);		// SD_BOTH
			Disconnecting = TRUE;
		}
		return TRUE;
	}

	message[len] = 0;

	// Look for MON delimiters (FF/FE)

	Buffptr = message;

	if (MonData)
	{
		// Already in MON State

		FEptr = memchr(Buffptr, 0xfe, len);

		if (!FEptr)
		{
			// no FE - so send all to monitor

			WritetoMonWindow(Buffptr, len);
			return TRUE;
		}

		MonData = FALSE;

		MonLen = FEptr - Buffptr;		// Mon Data, Excluding the FE

		WritetoMonWindow(Buffptr, MonLen);

		Buffptr = ++FEptr;				// Char following FE

		if (++MonLen < len)
		{
			len -= MonLen;
			goto MonLoop;				// See if next in MON or Data
		}

		// Nothing Left

		return TRUE;
	}

MonLoop:

	ptr = memchr(Buffptr, 0xff, len);

	if (ptr)
	{
		// Buffer contains Mon Data

		if (ptr > Buffptr)
		{
			// Some Normal Data before the FF

			int NormLen = ptr - Buffptr;				// Before the FF
			WritetoOutputWindow(Buffptr, NormLen);

			len -= NormLen;
			Buffptr = ptr;
			goto MonLoop;
		}

		MonData = TRUE;

		FEptr = memchr(Buffptr, 0xfe, len);

		if (FEptr)
		{
			MonData = FALSE;

			MonLen = FEptr + 1 - Buffptr;				// MonLen includes FF and FE
			WritetoMonWindow(Buffptr+1, MonLen - 2);

			len -= MonLen;
			Buffptr += MonLen;							// Char Following FE

			if (len <= 0)
			{
				return TRUE;
			}
			goto MonLoop;
		}
		else
		{
			// No FE, so rest of buffer is MON Data

			WritetoMonWindow(Buffptr+1, len -1);		// Exclude FF
//			DoRefresh();
			return TRUE;
		}
	}

	// No FF, so must be session data

	WritetoOutputWindow(Buffptr, len);
//	SlowTimer = 0;

	return TRUE;
}

*/

int Telnet_Connected(SOCKET sock, int Error)
{
	char Msg[80];
	int Len;

	// Connect Complete

	if (Error)
	{
		dialog = gtk_message_dialog_new ((GtkWindow *)window,
               GTK_DIALOG_DESTROY_WITH_PARENT,
               GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, "Connect Failed");

		gtk_window_set_title (GTK_WINDOW (dialog), "BPQTermTCP");
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);

		closesocket(sock);
		Connecting = FALSE;
		SocketActive = FALSE;

		sprintf(Title,"BPQTermTCP Version %s - Disconnected", VersionString);
		gtk_window_set_title (GTK_WINDOW (window), Title);
		DisableDisconnectMenu();
		EnableConnectMenu();

		return 0;

	}

//	RecvChannel = g_io_channel_unix_new((gint)sock);
//	RecvChannel = g_io_channel_win32_new_socket((gint)sock);
//	g_io_channel_set_encoding (RecvChannel, NULL, NULL);
//	g_io_channel_set_flags(RecvChannel, G_IO_FLAG_APPEND| G_IO_FLAG_NONBLOCK, NULL);
//	g_io_add_watch(RecvChannel, G_IO_IN | G_IO_HUP, GtkMsg_ShowMessage, 0);

	SocketActive = TRUE;
	Connecting = FALSE;
	Connected = TRUE;

	Len = sprintf(Msg, "%s\r%s\rBPQTermTCP\r", UserName[CurrentHost], Password[CurrentHost]);

	SendMsg(Msg, Len);

	SendTraceOptions();

//	SlowTimer = 0;

	sprintf(Title,"BPQTermTCP Version %s - Connected to %s", VersionString, Host[CurrentHost]);
	gtk_window_set_title (GTK_WINDOW (window), Title);
	DisableConnectMenu();
	EnableDisconnectMenu();

	return 0;
}

static void Disconnect(GtkWidget *w, gpointer   data)
{
	if (Disconnecting)
	{
		// Force close

			if (SocketActive)
				closesocket(sock);

			sprintf(Title,"BPQTermTCP Version %s - Disconnected", VersionString);
			gtk_window_set_title (GTK_WINDOW (window), Title);

			DisableDisconnectMenu();
			EnableConnectMenu();


			WritetoOutputWindow(DisMsg, strlen(DisMsg));
			SocketActive = FALSE;
			Connected = FALSE;
			Disconnecting = FALSE;
			return;
	}

	shutdown(sock, 2);		// SD_BOTH
	Disconnecting = TRUE;
}

void EnableDisconnectMenu()
{
	gtk_widget_set_sensitive(dis_item, 1);
}
void DisableConnectMenu()
{
	gtk_widget_set_sensitive(conn_item, 0);
}
void EnableConnectMenu()
{
	gtk_widget_set_sensitive(conn_item, 1);
}
void DisableDisconnectMenu()
{
	gtk_widget_set_sensitive(dis_item, 0);

}
void ReadConfig()
{
	FILE *infile;
	GKeyFile * KF;
	gchar * Value;
	GError *error = NULL;

	KF = g_key_file_new();
	g_key_file_load_from_file(KF, "BPQTermTCP.ini", 0, NULL);

	Value = g_key_file_get_string (KF, "Session 1", "Size", &error);
	if (Value)
		sscanf(Value,"%d,%d,%d,%d",&left,&right,&top,&bottom);

	PortMask = g_key_file_get_integer(KF, "Session 1", "PortMask", &error);
	Bells = g_key_file_get_integer(KF, "Session 1", "Bells", &error);
	StripLF = g_key_file_get_integer(KF, "Session 1", "StripLF", &error);
	MCOM = g_key_file_get_integer(KF, "Session 1", "MCOM", &error);
	MONColour = g_key_file_get_integer(KF, "Session 1", "MONColour", &error);
	MonNODES= g_key_file_get_integer(KF, "Session 1", "MonNODES", &error);
	MonPorts= g_key_file_get_integer(KF, "Session 1", "MonPorts", &error);
	ChatMode= g_key_file_get_integer(KF, "Session 1", "ChatMode", &error);
	CurrentHost = g_key_file_get_integer (KF, "Session 1", "CurrentHost", &error);

	g_key_file_free(KF);

	infile = fopen ("BPQTermTCP.ini", "r");

	if (infile)
	{
		char buffer[1024];
		char * ret;
		char * ptr;

		while (1)
		{
			ret = fgets(buffer, 1024, infile);

			if (ret == 0)
			{
				fclose (infile);
				return;
			}

			ptr = strchr(buffer, 10);

			if (ptr)
				*ptr = 0;

			if (memcmp(buffer, "Host", 4) == 0)
			{
				int port = atoi(&buffer[4]) - 1;
				strcpy(&Host[port][0], &buffer[6]);
				continue;
			}
			if (memcmp(buffer, "Port", 4) == 0)
			{
				int port = atoi(&buffer[4]) - 1;
				strcpy(&Port[port][0], &buffer[6]);
				continue;
			}
			if (memcmp(buffer, "User", 4) == 0)
			{
				int port = atoi(&buffer[4]) - 1;
				strcpy(&UserName[port][0], &buffer[6]);
				continue;
			}
			if (memcmp(buffer, "Pass", 4) == 0)
			{
				int port = atoi(&buffer[4]) - 1;
				strcpy(&Password[port][0], &buffer[6]);
				continue;
			}
		}
	}
}
