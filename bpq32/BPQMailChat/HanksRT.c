#include "stdafx.h"

char OurNode[10];
char OurAlias[10];

char RtUsr[MAX_PATH] = "STUsers.txt";
char RtUsrTemp[MAX_PATH] = "STUsers.tmp";

CIRCUIT *circuit_hd = NULL;			// This is a chain of RT circuits. There may be others
NODE *node_hd = NULL;
LINK *link_hd = NULL;
TOPIC *topic_hd = NULL;
USER *user_hd = NULL;

int ChatTmr = 0;


char * strlop(char * buf, char delim)
{
	// Seems to Terminate buf at delim, and return rest of string

	char * ptr;
	
	ptr=strchr(buf, delim);

	if (ptr == NULL)
		return NULL;

	*(ptr)++=0;

	return ptr;
}

VOID * _zalloc_dbg(int len, int type, char * file, int line)
{
	// ?? malloc and clear

	void * ptr;

	ptr=_malloc_dbg(len, type, file, line);
	memset(ptr, 0, len);

	return ptr;
}

VOID * _zalloc(int len)
{
	// ?? malloc and clear

	void * ptr;

	ptr=malloc(len);
	memset(ptr, 0, len);

	return ptr;
}

VOID * mallocw(int len)
{
	// ?? malloc and warn if fails??

	return malloc(len);
}

VOID nputs(CIRCUIT * conn, char * buf)
{
	// Seems to send buf to socket

	WriteLogLine('>',buf,  strlen(buf), LOG_CHAT);
	QueueMsg(conn, buf, strlen(buf));
}

VOID nputc(CIRCUIT * conn, char buf)
{
	// Seems to send buf to socket

	WriteLogLine('>',&buf,  1, LOG_CHAT);
	QueueMsg(conn, &buf, 1);
}


VOID __cdecl nprintf(CIRCUIT * conn, const char * format, ...)
{
	// seems to be printf to a socket

	char buff[300];
	va_list(arglist);
	
	va_start(arglist, format);
	vsprintf(buff, format, arglist);

	nputs(conn, buff);
}

VOID saywhat(CIRCUIT *circuit)
{
	nputs(circuit, "Invalid Command\r");
}

VOID saydone(CIRCUIT *circuit)
{
	nputs(circuit, "Ok\r");
}

VOID strnew(char ** new, char *f1)
{
	// seems to allocate a new string, and copy the old one to it
	// how is this different to strdup??

	*new = _strdup(f1);
}

#define sl_ins_hd(link, hd) \
	if (hd == NULL)\
		hd=link;\
	else\
	{\
		link->next=hd->next;\
		hd->next=link;\
	}

BOOL matchi(char * p1, char * p2)
{
	// Return TRUE is strings match
	
	if (_stricmp(p1, p2)) 
		return FALSE;
	else
		return TRUE;
}

VOID ProcessChatLine(ConnectionInfo * conn, struct UserInfo * user, char* Buffer, int len)
{
	ConnectionInfo *c;

	WriteLogLine('<',Buffer, len, LOG_CHAT);

	if (conn->Flags & GETTINGUSER)
	{
		memcpy(user->Name, Buffer, len-1);
		conn->Flags &=  ~GETTINGUSER;
		SendWelcomeMsg(conn->BPQStream, conn, user);
		SaveUserDatabase();
		return;
	}

	Buffer[len] = 0;

	strlop(Buffer, '\r');

	if (conn->flags == p_linkwait)
	{
		//waiting for *RTL

		if ((len <6) && (memcmp(Buffer, "*RTL", 4) == 0))
		{
			// Node - Node Connect

			if (rtloginl (conn, conn->Callsign))
			{
				// Accepted
		
				conn->Flags |= CHATLINK;
				return;
			}
			else
			{
				// Connection refused
			
				Disconnect(conn->BPQStream);
				return;
			}
		}

		if (Buffer[0] == '[' && Buffer[len-2] == ']')		// SID
			return;

		nprintf(conn, "Unexpected Message on Chat Node-Node Link - Disconnecting\r");
		Flush(conn);
		Sleep(500);
		Disconnect(conn->BPQStream);
		return;
	}

	if (conn->Flags & CHATLINK)
	{
		chkctl(conn, Buffer);
		return;
	}

	if(conn->u.user == NULL)
		return;									// A node link, but not activated yet


	if ((len <6) && (memcmp(Buffer, "*RTL", 4) == 0))
	{
		// Other end thinks this is a node-node link

		Disconnect(conn->BPQStream);
		return;
	}

	if (Buffer[0] == '/')
	{

		// Process Command

		if (_memicmp(&Buffer[1], "Bye", 1) == 0)
		{
			SendUnbuffered(conn->BPQStream, SignoffMsg, strlen(SignoffMsg));
			
			if (conn->BPQStream == -1)
			{
				logout(conn);
				conn->Flags = 0;
			}

			else
				ReturntoNode(conn->BPQStream);
								
			return;
		}

		if (_memicmp(&Buffer[1], "Quit", 4) == 0)
		{
			SendUnbuffered(conn->BPQStream, SignoffMsg, strlen(SignoffMsg));

			if (conn->BPQStream == -1)
			{
				logout(conn);
				conn->Flags = 0;
			}

			else
			{
				Sleep(1000);
				Disconnect(conn->BPQStream);
			}
			
			return;
		}

		rt_cmd(conn, Buffer);

		return;

	}

	// Send message to all other connected users on same channel

	if (len > 200)
	{
		Buffer[200] = '\r';
		Buffer[201] = 0;
		len = 200;
	}
		
	text_tellu(conn->u.user, Buffer, NULL, o_topic); // To local users.
		
	// Send to Linked nodes

	for (c = circuit_hd; c; c = c->next)
		if ((c->flags & p_linked) && c->refcnt && ct_find(c, conn->u.user->topic))
			nprintf(c, "%c%c%s %s %s\r",
				FORMAT, id_data, OurNode, conn->u.user->call, Buffer);


}

static void upduser(USER *user)
{
	FILE *in, *out;
	char *c, *buf;

	in = fopen(RtUsr, "r");

	if (!(in))
	{
		in = fopen(RtUsr, "w");
		fclose(in);
		in = fopen(RtUsr, "r");
	}

	out = fopen(RtUsrTemp, "w");

	if (!(in) || !(out)) return;

	buf = mallocw(128);

	while(fgets(buf, 128, in))
	{
 	  c = strchr(buf, ' ');
 	  if (c) *c = '\0';
		if (!matchi(buf, user->call))
		{
			if (c) *c = ' ';
			fputs(buf, out);
		}
	}

	free(buf);
	fprintf(out, "%s %d %s %s\n", user->call, user->flags, user->name, user->qth);
	fclose(in);
	fclose(out);

	remove(RtUsr);
	rename(RtUsrTemp, RtUsr);


}

static void rduser(USER *user)
{
	FILE *in;
	char *buf, *name, *flags, *qth;

	user->name = _strdup("?_name");
	user->qth  = _strdup("?_qth");

	in = fopen(RtUsr, "r");

	if (in)
	{
		buf = mallocw(128);

	  while(fgets(buf, 128, in))
	  {
		strlop(buf, '\n');

	    flags = strlop(buf, ' ');
			if (!matchi(buf, user->call)) continue;
			if (!flags) break;

			name = strlop(flags, ' ');
			user->flags = atoi(flags);

			qth = strlop(name, ' ');
			strnew(&user->name, name);


			if (!qth) break;
			strnew(&user->qth,  qth);
			break;
		}

		free(buf);
		fclose(in);
	}

}

void chkctl(CIRCUIT *ckt_from, char * Buffer)
{
	NODE    *node, *ln;
	CIRCUIT *ckt_to;
	USER    *user, *su;
	char    *ncall, *ucall, *f1, *f2, *buf;

	if (Buffer[FORMAT_O] != FORMAT) return; // Not a control message.
	buf = _strdup(Buffer + DATA_O);

// FORMAT and TYPE bytes are followed by node and user callsigns.

	ncall = buf;
	ucall = strlop(buf, ' ');
	if (!ucall) { free(buf); return; } // Not a control message.

// There may be at least one field after the node and user callsigns.
// Node leave (id_unlink) has no F1.

	f1 = strlop(ucall, ' ');

// If the frame came from an unknown node ignore it.
// If the frame came from us ignore it (loop breaking).

	node = node_find(ncall);
	if (!node || matchi(ncall, OurNode)) { free(buf); return; }

	switch(Buffer[TYPE_O])
	{
// Data from user ucall at node ncall.

		case id_data :
			user = user_find(ucall);
			if (!user) break;
			text_tellu(user, f1, NULL, o_topic);

			for (ckt_to = circuit_hd; ckt_to; ckt_to = ckt_to->next)
				if ((ckt_to->flags & p_linked) &&
					   ckt_to->refcnt &&
					   !cn_find(ckt_to, node) &&
					   ct_find(ckt_to, user->topic)) nprintf(ckt_to, "%s\r", Buffer);
			break;

// User ucall at node ncall changed their Name/QTH info.

		case id_user :
			echo(ckt_from, node, Buffer);  // Relay to other nodes.
			user = user_find(ucall);
			if (!user) break;
			f2 = strlop(f1, ' ');
			if (!f2) break;
			strnew(&user->name, f1);
			strnew(&user->qth,  f2);
			upduser(user);
			break;

// User ucall logged into node ncall.

		case id_join :
			echo(ckt_from, node, Buffer);  // Relay to other nodes.
			f2 = strlop(f1, ' ');
			if (!f2) break;
			user = user_join(ckt_from, ucall, ncall, NULL);
			if (!user) break;
			ckt_from->refcnt++;
			text_tellu_Joined(user);
			strnew(&user->name, f1);
			strnew(&user->qth,  f2);
			upduser(user);
			makelinks();					// Bring up our links if not already up

			break;

// User ucall logged out of node ncall.

		case id_leave :
			echo(ckt_from, node, Buffer);  // Relay to other nodes.
			user = user_find(ucall);
			if (!user) break;
			f2 = strlop(f1, ' ');
			if (!f2) break;
			text_tellu(user, rtleave, NULL, o_all);
			ckt_from->refcnt--;
			cn_dec(ckt_from, node);
			strnew(&user->name, f1);
			strnew(&user->qth,  f2);
			upduser(user);
			user_leave(user);
			break;

// Node ncall lost its link to node ucall, alias f1.

		case id_unlink :
			echo(ckt_from, node, Buffer);  // Relay to other nodes.
			ln = node_find(ucall);
			if (ln)	cn_dec(ckt_from, ln);
			break;

// Node ncall acquired a link to node ucall, alias f1.
// If we are not linked, is no problem, don't link.
// If we are linked, is a loop, do what?

		case id_link :
			echo(ckt_from, node, Buffer);  // Relay to other nodes.
			ln = node_find(ucall);
			if (!ln && !matchi(ncall, OurNode)) cn_inc(ckt_from, ucall, f1);
			break;

// User ucall at node ncall sent f2 to user f1.

		case id_send :
			user = user_find(ucall);
			if (!user) break;
			f2 = strlop(f1, ' ');
			if (!f2) break;
			su = user_find(f1);
			if (!su) break;

			if (su->circuit->flags & p_user)
				text_tellu(user, f2, f1, o_one);
			else
				echo(ckt_from, node, Buffer);  // Relay to other nodes.
			break;

// User ucall at node ncall changed topic.

		case id_topic :
			echo(ckt_from, node, Buffer);  //  Relay to other nodes.
			user = user_find(ucall);
			if (user) topic_chg(user, f1);
			break;

		default :  break;
	}

	free(buf);
}

// Tell another node about nodes known by this node.
// Do not tell it about this node, the other node knows who it
// linked to (or who linked to it).
// Tell another node about users known by this node.
// Done at incoming or outgoing link establishment.

void state_tell(CIRCUIT *circuit)
{
	NODE *node;
	USER *user;

	node = cn_inc(circuit, circuit->u.link->call, circuit->u.link->alias);
	node_tell(node, id_link); // Tell other nodes about this new link

// Tell the node that just linked here about nodes known on other links.

	for (node = node_hd; node; node = node->next)
	  if (!matchi(node->call, OurNode)) node_xmit(node, id_link, circuit);

// Tell the node that just linked here about known users, and their topics.

	for (user = user_hd; user; user = user->next)
	{
		user_xmit(user, id_join, circuit);
		topic_xmit(user, circuit);
	}
}
static void circuit_free(CIRCUIT *circuit)
{
	CIRCUIT *c, *cp;
	CN      *ncn;
//	NODE    *nn;
//	TOPIC   *tn;

	cp = NULL;

	for (c = circuit_hd; c; cp = c, c = c->next)
	{
		if (c == circuit)
		{
			if (cp) cp->next = c->next; else circuit_hd = c->next;

			while (c->hnode)
			{
				ncn = c->hnode->next;
				free(c->hnode);
				c->hnode = ncn;
			}	
			
			break;
		}
	}

	if (circuit_hd) return;

// RT has gone inactive. Clean up.
/*
	while (node_hd)
	{
		nn = node_hd->next;
		free(node_hd->alias);
		free(node_hd->call);
		free(node_hd);
		node_hd = nn;
	}

	while (topic_hd)
	{
		tn = topic_hd->next;
		free(topic_hd->name);
		free(topic_hd);
		topic_hd = tn;
	}
*/
}


// Find a node in the node list.

NODE *node_find(char *call)
{
	NODE *node;

	for (node = node_hd; node; node = node->next)
		if (node->refcnt && matchi(node->call, call)) break;

	return node;
}

// Add a reference to a node.

static NODE *node_inc(char *call, char *alias)
{
	NODE *node;

	node = node_find(call);

	if (!node)
	{
		node = zalloc(sizeof(NODE));
		sl_ins_hd(node, node_hd);
		node->call  = _strdup(call);
		node->alias = _strdup(alias);
	}

	node->refcnt++;
	return node;
}

// Remove a reference to a node.

static void node_dec(NODE *node)
{
	NODE *t, *tp;

	if (--node->refcnt) return; // Other references.

// Remove the node from the node list.

	tp = NULL;

	for (t = node_hd; t; tp = t, t = t->next) if (t == node)
	{
		if (tp) tp->next = t->next; else node_hd = t->next;
		free(t->alias);
		free(t->call);
		free(t);
		break;
	}
}

// User joins a topic.

static TOPIC *topic_join(CIRCUIT *circuit, char *s)
{
	CT    *ct;
	TOPIC *topic;

// Look for an existing topic.

	for (topic = topic_hd; topic; topic = topic->next)
		if (matchi(topic->name, s)) break;

// Create a new topic, if needed.

	if (!topic)
	{
		topic = zalloc(sizeof(TOPIC));
		sl_ins_hd(topic, topic_hd);
		topic->name = _strdup(s);
	}

	topic->refcnt++;  // One more user in this topic.

// Add the circuit / topic association.

	for (ct = circuit->topic; ct; ct = ct->next) if (ct->topic == topic)
	{
	  ct->refcnt++;
	  return topic;
	}

	ct = zalloc(sizeof(CT));
	sl_ins_hd(ct, circuit->topic);
	ct->topic  = topic;
	ct->refcnt = 1;
	return topic;
}

// User leaves a topic.

static void topic_leave(CIRCUIT *circuit, TOPIC *topic)
{
	CT    *ct, *ctp;
	TOPIC *t,  *tp;

	topic->refcnt--;

	ctp = NULL;

	for (ct = circuit->topic; ct; ctp = ct, ct = ct->next)
	{
		if (ct->topic == topic)
		{
			if (!--ct->refcnt)
			{
	  			if (ctp) ctp->next = ct->next; else circuit->topic = ct->next;
				free(ct);
				break;
			}
		}
	}

	tp = NULL;

	for (t = topic_hd; t; t = t->next)
	{
		if (!t->refcnt && (t == topic))
		{
			if (tp) tp->next = t->next; else topic_hd = t->next;
			free(t->name);
			free(t);
			break;
		}
	}
}

// Find a circuit/topic association.

int ct_find(CIRCUIT *circuit, TOPIC *topic)
{
	CT *ct;

	for (ct = circuit->topic; ct; ct = ct->next)
		if (ct->topic == topic) return ct->refcnt;

	return 0;
}

// Nodes reached from each circuit. Used only if the circuit is a link.

// Remove a circuit/node association.

static void cn_dec(CIRCUIT *circuit, NODE *node)
{
	CN *c, *cp;

	node_dec(node);

	cp = NULL;

	for (c = circuit->hnode; c; cp = c, c = c->next)
	{
		if (c->node == node)
		{
			if (--c->refcnt) return;			// Still in use

			// CN record no longer needed

			if (cp) cp->next = c->next; else circuit->hnode = c->next;

			free(c);

			break;
		}
	}
}

// Add a circuit/node association.

static NODE *cn_inc(CIRCUIT *circuit, char *call, char *alias)
{
	NODE *node;
	CN *cn;

	node = node_inc(call, alias);

	for (cn = circuit->hnode; cn; cn = cn->next) if (cn->node == node)
	{
	  cn->refcnt++;
	  return node;
	}

	cn = zalloc(sizeof(CN));
	sl_ins_hd(cn, circuit->hnode);
	cn->node   = node;
	cn->refcnt = 1;
	return node;
}

// Find a circuit/node association.

static int cn_find(CIRCUIT *circuit, NODE *node)
{
	CN *cn;

	for (cn = circuit->hnode; cn; cn = cn->next) if (cn->node == node)
		return cn->refcnt;

	return 0;
}

// From a local user to a specific user at another node.

static void text_xmit(USER *user, USER *to, char *text)
{
	nprintf(to->circuit, "%c%c%s %s %s %s\r",
		FORMAT, id_send, OurNode, user->call, to->call, text);
}

void text_tellu(USER *user, char *text, char *to, int who)
{
	CIRCUIT *circuit;
	char    *buf;

	buf = mallocw(strlen(text) + 11);
	sprintf(buf, "%-6.6s %c %s\r", user->call, (who == o_one) ? '>' : ':', text);

// Send it to all connected users in the same topic.
// Echo to originator if requested.

	for (circuit = circuit_hd; circuit; circuit = circuit->next)
	{
		if (!(circuit->flags & p_user)) continue;  // Circuit is a link.
		if ((circuit->u.user == user) && !(user->flags & u_echo)) continue;

		switch(who)
		{
			case o_topic :
				if (circuit->u.user->topic == user->topic) nputs(circuit, buf);
				break;
			case o_all :
				nputs(circuit, buf);
				break;
			case o_one :
				if (matchi(circuit->u.user->call, to)) nputs(circuit, buf);
				break;
		}
	}

	free(buf);
}

void text_tellu_Joined(USER * user)
{
	CIRCUIT *circuit;
	char buf[256];

	sprintf(buf, "%-6.6s *** Joined Chat, Topic %s\r", user->call, user->topic->name);

	if (FlashOnConnect)
		FlashWindow(hWnd, TRUE);


// Send it to all connected users in the same topic.
// Echo to originator if requested.

	for (circuit = circuit_hd; circuit; circuit = circuit->next)
	{
		if (!(circuit->flags & p_user)) continue;  // Circuit is a link.
		if ((circuit->u.user == user) && !(user->flags & u_echo)) continue;

		nputs(circuit, buf);

		if (circuit->u.user->flags & u_bells)
			if (circuit->BPQStream < 0) // Console
			{
				if (FlashOnConnect) FlashWindow(hConsole, TRUE);
				nputc(circuit, 7);
			}
			else
				nputc(circuit, 7);
	}
}
// Tell one link circuit about a local user change of topic.

static void topic_xmit(USER *user, CIRCUIT *circuit)
{
	nprintf(circuit, "%c%c%s %s %s\r",
		FORMAT, id_topic, OurNode, user->call, user->topic->name);
}

// Tell another node about one known node on a link add or drop
// if that node is from some other link.

static void node_xmit(NODE *node, char kind, CIRCUIT *circuit)
{
	if (!cn_find(circuit, node)) nprintf(circuit, "%c%c%s %s %s\r",
		FORMAT, kind, OurNode, node->call, node->alias);
}

// Tell all other nodes about one node known by this node.

static void node_tell(NODE *node, char kind)
{
	CIRCUIT *circuit;

	for (circuit = circuit_hd; circuit; circuit = circuit->next)
		if (circuit->flags & p_linked) node_xmit(node, kind, circuit);
}

// Tell another node about a user login/logout at this node.

static void user_xmit(USER *user, char kind, CIRCUIT *circuit)
{
	NODE *node;

	node = user->node;
	if (!cn_find(circuit, node)) nprintf(circuit, "%c%c%s %s %s %s\r",
		FORMAT, kind, node->call, user->call, user->name, user->qth);
}

// Tell all other nodes about a user login/logout at this node.

static void user_tell(USER *user, char kind)
{
	CIRCUIT *circuit;

	for (circuit = circuit_hd; circuit; circuit = circuit->next)
		if (circuit->flags & p_linked)
	  user_xmit(user, kind, circuit);
}

// Find the user record for call.

static USER *user_find(char *call)
{
	USER *user;

	for (user = user_hd; user; user = user->next)
		if (matchi(user->call, call)) break;

	return user;
}

static void user_leave(USER *user)
{
	USER *t, *tp;

	topic_leave(user->circuit, user->topic);

	tp = NULL;

	for (t = user_hd; t; tp = t, t = t->next)
	{
		if (t == user)
		{
			if (tp) tp->next = t->next; else user_hd = t->next;
		
			free(t->name);
			free(t->call);
			free(t->qth);
			free(t);
			break;
		}
	}

	if (user_hd == NULL)
		ChatTmr = 59;					// If no users, disconnect links after 10-20 secs
}

// User changed to a different topic.

static void topic_chg(USER *user, char *s)
{
	char buf[128];

	if (_stricmp(user->topic->name, s) == 0) return;			// Not Changed

	sprintf(buf, "*** Left Topic: %s", user->topic->name);
	text_tellu(user, buf, NULL, o_topic); // Tell everyone in the old topic.
	topic_leave(user->circuit, user->topic);
	user->topic = topic_join(user->circuit, s);
	sprintf(buf, "*** Joined Topic: %s", user->topic->name);
	text_tellu(user, buf, NULL, o_topic); // Tell everyone in the new topic.

}

// Create a user record for this user.

static USER *user_join(CIRCUIT *circuit, char *ucall, char *ncall, char *nalias)
{
	NODE *node;
	USER *user;

	node = cn_inc(circuit, ncall, nalias);

// Is this user already logged in at this node?

	for (user = user_hd; user; user = user->next)
		if (matchi(user->call, ucall) && (user->node == node)) return user;

// User is not logged in, create a user record for them.

	user = zalloc(sizeof(USER));
	sl_ins_hd(user, user_hd);
	user->circuit = circuit;
	user->topic   = topic_join(circuit, deftopic);
	user->call    = _strdup(ucall);
	_strupr(user->call);
	user->node    = node;
	rduser(user);
	if (circuit->flags & p_user) circuit->u.user = user;
	return user;
}

// Link went away. We dropped it, or the other node dropped it.
// Drop nodes and users connected from this link.
// Tell other (still connected) links what was dropped.

void link_drop(CIRCUIT *circuit)
{
	USER *user, *usernext;
	CN   *cn;

// So we don't try and send anything on this circuit.

	circuit->u.link->flags = p_nil;
	circuit->flags = p_nil;

// Users connected on the dropped link are no longer connected.

	for (user = user_hd; user; user = usernext)
	{
		usernext = user->next;				// Save next pointer in case entry is free'd

		if (user->circuit == circuit)
		{
			circuit->refcnt--;
			node_dec(user->node);
			text_tellu(user, rtleave, NULL, o_all);
			user_tell(user, id_leave);
			user_leave(user);
		}
	}
// Any node known from the dropped link is no longer known.

	for (cn = circuit->hnode; cn; cn = cn->next)
	{
		node_tell(cn->node, id_unlink);
		node_dec(cn->node);
	}

// The circuit is no longer used.

	circuit_free(circuit);
}

// Handle an incoming control frame from a linked RT system.

static void echo(CIRCUIT *fc, NODE *node, char * Buffer)
{
	CIRCUIT *tc;

	for (tc = circuit_hd; tc; tc = tc->next)
	if ((tc != fc) && (tc->flags & p_linked) && !cn_find(tc, node))
		nprintf(tc, "%s\r", Buffer);
}


// Add an entry to list of link partners

int rtlink (char * Call)
{
	LINK *link;
	char *c;

	_strupr(Call);
	c = strlop(Call, ':');
	if (!c) return FALSE;

	link = zalloc(sizeof(LINK));

	sl_ins_hd(link, link_hd);

	link->alias = _strdup(Call);
	link->call  = _strdup(c);

	free(Call);

	return TRUE;
}

VOID removelinks()
{
	LINK *link, *nextlink;

	for (link = link_hd; link; link = nextlink)
	{
		nextlink = link->next;
		
		free(link->alias);
		free(link->call);
		free(link);
	}
	link_hd = NULL;
}

// We don't allocate memory for circuit, but we do chain it

CIRCUIT *circuit_new(CIRCUIT *circuit, int flags)
{
	// Make sure circuit isn't already on list
	
	CIRCUIT *c;

	circuit->flags = flags;
	circuit->next = NULL;

	for (c = circuit_hd; c; c = c->next)
	{
		if (c == circuit)
		{
			MessageBox(MainWnd, "Corruption in Chat Circult List Detected - Trying to recover", "BPQMailChat", MB_OK);
			return circuit;
		}
	}
	
	sl_ins_hd(circuit, circuit_hd);

	return circuit;
}

// Handle an incoming link.

int rtloginl (CIRCUIT *conn, char * call)
{
	LINK    *link;

	if (node_find(call)) return FALSE; // Already linked.

	for (link = link_hd; link; link = link->next)
	{	if (matchi(call, link->call))
			break;
	}

	if (!link) return FALSE;           // We don't link with this system.

	if (link->flags & (p_linked | p_linkini))
	{
		// Already Linked. Used to Disconnect, but that can cause sync errors
		// Try closing old link and keeping new

		CIRCUIT *c;
		int len;
		char Msg[80];

		for (c = circuit_hd; c; c = c->next)
		{
			if (c->u.link == link)
			{
				len=wsprintf(Msg, "Chat Node %s Connect when Connected - Old Connection Closed", call);
				WriteLogLine('|',Msg, len, LOG_CHAT);

				c->Active = FALSE;			// So we don't try to clear circuit again
				Disconnect(c->BPQStream);
				link_drop(c);
				RefreshMainWindow();
				break;
			}
		}
	}

// Accept the link request.

	circuit_new(conn, p_linked);

	nputs(conn, "OK\r");
	conn->u.link = link;
	link->flags = p_linked;
	state_tell(conn);

	return TRUE;

}

// User connected to chat, or did chat command from BBS

int rtloginu (CIRCUIT *circuit)
{
	USER    *user;

// Is this user already logged in to RT somewhere else?

	if (user_find(circuit->UserPointer->Call))
	{
		nputs(circuit, "*** Already connected at another node.\r");
		return FALSE;
	}

//	if (log_rt) tlogp("RT Login");

// Create the user entry.

	circuit_new(circuit, p_user);

	user = user_join(circuit, circuit->UserPointer->Call, OurNode, OurAlias);
	circuit->u.user = user;

	if (strcmp(user->name, "?_name") == 0)
	{
		user->name = _strdup(circuit->UserPointer->Name);
	}
	upduser(user);
	nputs(circuit, "G8BPQ Chat Server.\rType /h for command summary.\rBringing up links to other nodes.\r");
	nputs(circuit, "This may take a minute or two.\rThe /p command shows what nodes are linked.\r");
	text_tellu_Joined(user);
	user_tell(user, id_join);
	show_users(circuit);
	makelinks();

	return TRUE;
}

void logout(CIRCUIT *circuit)
{
	USER *user;

	circuit->flags = p_nil;
	user = circuit->u.user;
//	if (log_rt) tlogp("RT Logout");
	if (user)			// May not have logged in if alrready conencted
	{
		user_tell(user, id_leave);
		text_tellu(user, rtleave, NULL, o_all);
		cn_dec(circuit, user->node);
		user_leave(user);
	}

	circuit_free(circuit);

}


void show_users(CIRCUIT *circuit)
{
	USER *user;

	nputs(circuit, "Stations connected:\r");

	for (user = user_hd; user; user = user->next)
		nprintf(circuit, "%-6.6s at %-9.9s %s, %s [ %s ]\r",
		user->call, user->node->alias, user->name, user->qth, user->topic->name);
}

static void show_nodes(CIRCUIT *circuit)
{
	NODE *node;

	nputs(circuit, "Known Nodes:\r");

	for (node = node_hd; node; node = node->next) if (node->refcnt)
		nprintf(circuit, "%s:%s %u\r", node->alias, node->call, node->refcnt);
}

// /P Command: List circuits and remote RT on them.

#define xxx "\r        "

static void show_circuits(CIRCUIT *conn)
{
	CIRCUIT *circuit;
	NODE    *node;
	int     len;

	nprintf(conn, "Here %-6.6s <- ", OurAlias);
	len = 0;

	for (node = node_hd; node; node = node->next) if (node->refcnt)
	{
		len += strlen(node->alias) + 1;
		if (len > 60) { len = strlen(node->alias) + 1; nputs(conn, xxx); }
		nputs(conn, node->alias);
		nputc(conn, ' ');
	}

	nputc(conn, '\r');

	for (circuit = circuit_hd; circuit; circuit = circuit->next)
	if (circuit->flags & p_linked)
	{
		nprintf(conn, "Link %-6.6s <- ", circuit->u.link->alias);
		len = 0;

		for (node = node_hd; node; node = node->next)
		if (node->refcnt && !cn_find(circuit, node))
		{
			len += strlen(node->alias) + 1;
			if (len > 60) { len = strlen(node->alias) + 1; nputs(circuit, xxx); }
			nputs(conn, node->alias);
			nputc(conn, ' ');
		}

		nputc(conn, '\r');
	}
	else if (circuit->flags & p_user)
		nprintf(conn, "User %-6.6s\r", circuit->u.user->call);
	else if (circuit->flags & p_linkini)
		nprintf(conn, "Link %-6.6s (setup)\r", circuit->u.link->alias);
}

// /T Command: List topics and users in them.

static void show_topics(CIRCUIT *conn)
{
	TOPIC *topic;
	USER  *user;

	nputs(conn, "Active Topics are:\r");

	for (topic = topic_hd; topic; topic = topic->next)
	{
		nprintf(conn, "%s\r", topic->name);

		if (topic->refcnt)
		{
			nputs(conn, "  ");
			for (user = user_hd; user; user = user->next)
				if (user->topic == topic) nprintf(conn, " %s", user->call);
			nputc(conn, '\r');
		}
	}
}



// Do a user command.

int rt_cmd(CIRCUIT *circuit, char * Buffer)
{
	CIRCUIT *c;
	USER    *user, *su;
	char    *f1, *f2;

	user = circuit->u.user;

	switch(tolower(Buffer[1]))
	{
		case 'a' :
			user->flags ^= u_bells;
			upduser(user);
			nprintf(circuit, "Alert %s\r",  (user->flags & u_bells) ? "Enabled" : "Disabled");
			return TRUE;

		case 'b' : return FALSE;
		case 'e' : 
			user->flags ^= u_echo;
			upduser(user);
			nprintf(circuit, "Echo %s\r",  (user->flags & u_echo) ? "Enabled" : "Disabled");
			return TRUE;
		
		case 'f' : makelinks(); return TRUE;

		case 'h' :
			nputs(circuit, "/U - Show Users.\r/N - Enter your Name.\r/Q - Enter your QTH.\r/T - Show Topics.\r");
			nputs(circuit, "/T Name - Join Topic or Create new Topic.\r/P - Show Ports and Links.\r");
			nputs(circuit, "/A - Toggle Alert on user join.\r");
			nputs(circuit, "/E - Toggle Echo.\r/S CALL Text - Send Text to that station only.\r");
			nputs(circuit, "/F - Force all links to be made.\r/K - Show Known nodes.\r");
			nputs(circuit, "/B - Leave Chat and return to node.\r");
			return TRUE;
		
		case 'k' : show_nodes(circuit);                 return TRUE;

		case 'n' :
			strnew(&user->name, Buffer + 3);
			saydone(circuit);
			upduser(user);
			user_tell(user, id_user);
			return TRUE;

		case 'p' : show_circuits(circuit); return TRUE;

		case 'q' :
			strnew(&user->qth, Buffer + 3);
			saydone(circuit);
			upduser(user);
			user_tell(user, id_user);
			return TRUE;

		case 's' :
			strcat(Buffer, "\r");
			f1 = strlop(Buffer, ' ');  // To.
			if (!f1) break;
			f2 = strlop(f1, ' ');            // Text to send.
			if (!f2) break;
			_strupr(f1);
			su = user_find(f1);

			if (!su)
			{
				nputs(circuit, "*** That user is not logged in.\r");
				return TRUE;
			}

// Send to the desired user only.

			if (su->circuit->flags & p_user)
				text_tellu(user, f2, f1, o_one);
			else
				text_xmit(user, su, f2);

			return TRUE;

		case 't' :
			f1 = strlop(Buffer, ' ');
			if (f1)
			{
				topic_chg(user, f1);

// Tell all link circuits about the change of topic.

				for (c = circuit_hd; c; c = c->next)
					if (c->flags & p_linked) topic_xmit(user, c);
			}
			else
			  show_topics(circuit);
			return TRUE;

		case 'u' : show_users(circuit); return TRUE;

		default  : break;
	}

	saywhat(circuit);
	return TRUE;
}



void makelinks(void)
{
	LINK *link;

// Make the links.

	for (link = link_hd; link; link = link->next)
	{
// Is this link already established?
		if (link->flags & (p_linked | p_linkini)) continue;
// Already linked through some other node?
// If so, making this link would create a loop.
		if (node_find(link->call)) continue;
// Fire up the process to handle this link.
		link->flags = p_linkini;
		chat_link_out(link);
	}
}

VOID node_close()
{
	// Close all Node-Node Links

	CIRCUIT *circuit;

	for (circuit = circuit_hd; circuit; circuit = circuit->next)
	{
		if (circuit->flags & (p_linked | p_linkini | p_linkwait))
			Disconnect(circuit->BPQStream);
	}
}

// Send Keepalives to all connected nodes

static void node_keepalive()
{
	CIRCUIT *circuit;

	if (user_hd)			// Any Users?
	{
		for (circuit = circuit_hd; circuit; circuit = circuit->next)
		{
			if (circuit->flags & p_linked)
				nprintf(circuit, "%c%c%s %s\r", FORMAT, id_keepalive, OurNode, circuit->u.link->call);
		}
	}
	else
	{
		// No users. Close links

		node_close();
	}
}

VOID ChatTimer()
{
	// Entered every 10 seconds

	int i;
	NODE *node;
	USER *user;
	CIRCUIT *c;

	ClearDebugWindow();

	WritetoDebugWindow("Chat Users\r\n", 12);


	i = 0;
	for (user = user_hd; user; user = user->next)
	{
		WritetoDebugWindow(user->call, strlen(user->call));
		WritetoDebugWindow("\r\n", 2);
		i++;
	}
	SetDlgItemInt(hWnd, IDC_USERS, i, FALSE);

	WritetoDebugWindow("Chat Nodes\r\n", 12);


	i = 0;
	for (node = node_hd; node; node = node->next)
	{
		WritetoDebugWindow(node->call, strlen(node->call));
		WritetoDebugWindow("\r\n", 2);

		i++;
	}
	SetDlgItemInt(hWnd, IDC_NODES, i, FALSE);

	WritetoDebugWindow("Chat Links\r\n", 12);

	i = 0;
	for (c = circuit_hd; c; c = c->next)
	{
		if (c->flags & p_linked) 
		{
			WritetoDebugWindow(c->u.user->call, strlen(c->u.user->call));
			WritetoDebugWindow("\r\n", 2);

			i++;
		}
	}
	SetDlgItemInt(hWnd, IDC_LINKS,  i, FALSE);

	ChatTmr++;

	if (ChatTmr > 60) // 10 Mins
	{
		ChatTmr = 0;
		node_keepalive();
	}
}





VOID FreeChatMemory()
{
	removelinks();
}