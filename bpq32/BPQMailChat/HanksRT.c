#include "stdafx.h"

char OurNode[10];
char OurAlias[10];

char RtKnown[MAX_PATH];
char RtUsr[MAX_PATH] = "STUsers.txt";
char RtUsrTemp[MAX_PATH] = "STUsers.tmp";

int AXIPPort =0;

CIRCUIT *circuit_hd = NULL;			// This is a chain of RT circuits. There may be others
NODE *node_hd = NULL;
LINK *link_hd = NULL;
TOPIC *topic_hd = NULL;
USER *user_hd = NULL;
KNOWNNODE * known_hd = NULL;

int ChatTmr = 0;

BOOL NeedStatus = FALSE;

char Verstring[80];


static void node_dec(NODE *node);
static KNOWNNODE *knownnode_add(char *call);
VOID SendChatLinkStatus();


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

VOID * _zalloc_dbg_trace(int len, int type, char * file, int line)
{
	// ?? malloc and clear

	void * ptr;

	ptr=_malloc_dbg(len, type, file, line);
	memset(ptr, 0, len);

	Debugprintf("Malloc Trace %s %d %x %d", file, line, ptr, len);

	return ptr;
}

VOID * _malloc_dbg_trace(int len, int type, char * file, int line)
{
	// ?? malloc and clear

	void * ptr;

	ptr=_malloc_dbg(len, type, file, line);

	Debugprintf("Malloc Trace %s %d %x %d", file, line, ptr, len);

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

VOID nputs(CIRCUIT * conn, char * buf)
{
	// Seems to send buf to socket

	WriteLogLine(conn, '>',buf,  strlen(buf), LOG_CHAT);
	QueueMsg(conn, buf, strlen(buf));
}

VOID nputc(CIRCUIT * conn, char buf)
{
	// Seems to send buf to socket

	WriteLogLine(conn, '>',&buf,  1, LOG_CHAT);
	QueueMsg(conn, &buf, 1);
}


VOID __cdecl nprintf(CIRCUIT * conn, const char * format, ...)
{
	// seems to be printf to a socket

	char buff[600];
	va_list(arglist);
	
	va_start(arglist, format);
	vsprintf_s(buff, 600, format, arglist);

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


VOID ProcessChatLine(CIRCUIT * conn, struct UserInfo * user, char* Buffer, int len)
{
	ConnectionInfo *c;

	if (conn->Paging && (conn->LinesSent >= conn->PageLen))
	{
		// Waiting for paging prompt

		if (len > 1)
		{
			if (_memicmp(Buffer, "Abort", 1) == 0)
			{
				ClearQueue(conn);
				conn->LinesSent = 0;

				QueueMsg(conn, AbortedMsg, strlen(AbortedMsg));
				SendPrompt(conn, user);
				return;
			}
		}

		conn->LinesSent = 0;
		return;
	}


	WriteLogLine(conn, '<',Buffer, len, LOG_CHAT);

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

	if (conn->rtcflags == p_linkwait)
	{
		//waiting for *RTL

		if (memcmp(Buffer, "*RTL", 4) == 0)
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
		__try 
		{
			chkctl(conn, Buffer, len);
		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
			Debugprintf("MAILCHAT *** Program Error procesing Chat Node Message %s", Buffer);
			Disconnect(conn->BPQStream);
			CheckProgramErrors();
		}
		return;
	}

	if(conn->u.user == NULL)
		return;									// A node link, but not activated yet

	if ((len <6) && (memcmp(Buffer, "*RTL", 4) == 0))
	{
		// Other end thinks this is a node-node link

		Logprintf(LOG_CHAT, conn, '!', "Station %s trying to start Node Protocol, but not defined as a Node",
			conn->Callsign);

		knownnode_add(conn->Callsign);			// So it won't happen again

		Disconnect(conn->BPQStream);
		return;
	}

	if (Buffer[0] == '/')
	{
		// Process Command

		if (_memicmp(&Buffer[1], "Bye", 1) == 0)
		{
			SendUnbuffered(conn->BPQStream, SignoffMsg, strlen(SignoffMsg));
			
			if (conn->BPQStream < 0)
			{
				logout(conn);
				conn->Flags = 0;
				if (conn->BPQStream == -2)
					CloseConsole(conn->BPQStream);
			}
			else
				ReturntoNode(conn->BPQStream);
								
			return;
		}

		if (_memicmp(&Buffer[1], "Quit", 4) == 0)
		{
			SendUnbuffered(conn->BPQStream, SignoffMsg, strlen(SignoffMsg));

			if (conn->BPQStream < 0)
			{
				logout(conn);
				conn->Flags = 0;
				if (conn->BPQStream == -2)
					CloseConsole(conn->BPQStream);
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
		
	text_tellu(conn->u.user, Buffer, NULL, o_topic); // To local users.

	conn->u.user->lastmsgtime = time(NULL);
		
	// Send to Linked nodes

	for (c = circuit_hd; c; c = c->next)
	{
		if ((c->rtcflags & p_linked) && c->refcnt && ct_find(c, conn->u.user->topic))
			nprintf(c, "%c%c%s %s %s\r", FORMAT, id_data, OurNode, conn->u.user->call, Buffer);
	}
}

static void upduser(USER *user)
{
	FILE *in, *out;
	char *c;
	char Buffer[2048];
	char *buf = Buffer;

	in = fopen(RtUsr, "r");

	if (!(in))
	{
		in = fopen(RtUsr, "w");
		fclose(in);
		in = fopen(RtUsr, "r");
	}

	out = fopen(RtUsrTemp, "w");

	if (!(in) || !(out)) return;

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

	fprintf(out, "%s %d %s %s¬%x\n", user->call, user->rtflags, user->name, user->qth, user->Colour);
	fclose(in);
	fclose(out);

	remove(RtUsr);
	rename(RtUsrTemp, RtUsr);
}

static void rduser(USER *user)
{
	FILE *in;
	char *name, *flags, *qth;
	char Buffer[2048];
	char *buf = Buffer;
	char * ptr;

	user->name = _strdup("?_name");
	user->qth  = _strdup("?_qth");

	in = fopen(RtUsr, "r");

	if (in)
	{
	  while(fgets(buf, 128, in))
	  {
		strlop(buf, '\n');

	    flags = strlop(buf, ' ');
			if (!matchi(buf, user->call)) continue;
			if (!flags) break;

			name = strlop(flags, ' ');
			user->rtflags = atoi(flags);

			qth = strlop(name, ' ');
			strnew(&user->name, name);

			if (!qth) break;
			
			ptr = strchr(qth, '¬');
			if (ptr)
			{
				*ptr++ = 0;
				sscanf(ptr, "%x", &user->Colour);
			}
			strnew(&user->qth,  qth);
			break;
		}
		fclose(in);
	}
}


void ReportBadJoin(ncall, ucall)
{
	Logprintf(LOG_CHAT, NULL, '!', "User %s Join from Node %s but already connected", ucall, ncall);
}

void ReportBadLeave(ncall, ucall)
{
	Logprintf(LOG_CHAT, NULL, '!', "Node %s reporting Node %s as a leaving user", ncall, ucall);
}


void chkctl(CIRCUIT *ckt_from, char * Buffer, int Len)
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
			user = user_find(ucall, ncall);
			user->lastmsgtime = time(NULL);
			if (!user) break;
			text_tellu(user, f1, NULL, o_topic);

			for (ckt_to = circuit_hd; ckt_to; ckt_to = ckt_to->next)
			{
				if ((ckt_to->rtcflags & p_linked) && ckt_to->refcnt &&
					!cn_find(ckt_to, node) && ct_find(ckt_to, user->topic))
				   nprintf(ckt_to, "%s\r", Buffer);
			}
			break;

// User ucall at node ncall changed their Name/QTH info.

		case id_user :

			user = user_find(ucall, ncall);
			if (!user) break;
			f2 = strlop(f1, ' ');
			if (!f2) break;

			if ((strcmp(user->name, f1) == 0) && (strcmp(user->qth, f2) == 0))	// No Change?
				break;

			echo(ckt_from, node, Buffer);  // Relay to other nodes.
			strnew(&user->name, f1);
			strnew(&user->qth,  f2);
			upduser(user);
			break;

// User ucall logged into node ncall.

		case id_join :

			user = user_find(ucall, ncall);

			if (user)
			{
				// Already Here

				ReportBadJoin(ncall, ucall);

				//if (strcmp(user->node->call, OurNode) == 0)
				//{
					// Locally connected, and at another node
				//}
		
				break;				// We have this user as an active Node
			}


			echo(ckt_from, node, Buffer);  // Relay to other nodes.
			f2 = strlop(f1, ' ');
			if (!f2) break;
			user = user_join(ckt_from, ucall, ncall, NULL, FALSE);
			if (!user) break;
			ckt_from->refcnt++;
			text_tellu_Joined(user);
			strnew(&user->name, f1);
			strnew(&user->qth,  f2);
			upduser(user);
//			makelinks();					// Bring up our links if not already up

			break;

// User ucall logged out of node ncall.

		case id_leave :

			user = user_find(ucall, ncall);
			if (!user)
			{
				Debugprintf("MAILCHAT: Leave for %s from %s when not on list", ucall, ncall);
				break;
			}

			echo(ckt_from, node, Buffer);  // Relay to other nodes.

			f2 = strlop(f1, ' ');
			if (!f2) break;

			text_tellu(user, rtleave, NULL, o_all);
			ckt_from->refcnt--;
			cn_dec(ckt_from, node);
			node_dec(node);
			strnew(&user->name, f1);
			strnew(&user->qth,  f2);
			upduser(user);
			user_leave(user);
			break;

// Node ncall lost its link to node ucall, alias f1.

		case id_unlink :

			// Only relay to other nodes if we had node. Could get loop otherwise.
			// ?? This could possibly cause stuck nodes

			ln = node_find(ucall);
			if (ln)
			{
				// is it on this circuit?

				if (cn_find(ckt_from, ln))
				{
					cn_dec(ckt_from, ln);
					node_dec(ln);
					echo(ckt_from, node, Buffer);  // Relay to other nodes if we had node. COuld get loop if
				}
				else
				{
					Debugprintf("MAILCHAT: node %s unlink for %s when not on this link", ncall, ucall);
				}
			}
			else
			{
				Debugprintf("MAILCHAT: node %s unlink for %s when not on list", ncall, ucall);
			}

			break;

// Node ncall acquired a link to node ucall, alias f1.
// If we are not linked, is no problem, don't link.
// If we are linked, is a loop, do what? (Try ignore!)

		case id_link :

			ln = node_find(ucall);
			if (!ln && !matchi(ncall, OurNode))
			{
				f2 = strlop(f1, ' ');
				cn_inc(ckt_from, ucall, f1, f2);
				echo(ckt_from, node, Buffer);  // Relay to other nodes.
			}
			else
			{
				Debugprintf("MAILCHAT: node %s link for %s when already on list", ncall, ucall);
				break;
			}

			break;

// User ucall at node ncall sent f2 to user f1.

		case id_send :
			user = user_find(ucall, ncall);
			if (!user) break;
			f2 = strlop(f1, ' ');
			if (!f2) break;
			su = user_find(f1, NULL);
			if (!su) break;

			if (su->circuit->rtcflags & p_user)
				text_tellu(user, f2, f1, o_one);
			else
				echo(ckt_from, node, Buffer);  // Relay to other nodes.
			break;

// User ucall at node ncall changed topic.

		case id_topic :
			user = user_find(ucall, ncall);
			if (user)
			{
				if (_stricmp(user->topic->name, f1) != 0)
				{
					echo(ckt_from, node, Buffer);  //  Relay to other nodes.
					topic_chg(user, f1);
				}
			}
			break;

					
		case id_keepalive :

			ln = node_find(ncall);
			if (ln)
			{
				if (ln->Version == NULL)
					if (f1)
						ln->Version = _strdup(f1);
			}
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

void state_tell(CIRCUIT *circuit, char * Version)
{
	NODE *node;
	USER *user;

	node = cn_inc(circuit, circuit->u.link->call, circuit->u.link->alias, Version);
	node_tell(node, id_link); // Tell other nodes about this new link

// Tell the node that just linked here about nodes known on other links.

	for (node = node_hd; node; node = node->next)
	{
	  if (!matchi(node->call, OurNode))
		  node_xmit(node, id_link, circuit);
	}

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
	NODE    *nn;
	TOPIC   *tn;

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
}


// Find a node in the node list.

NODE *node_find(char *call)
{
	NODE *node;

	for (node = node_hd; node; node = node->next)
	{
		//if (node->refcnt && matchi(node->call, call))   I don't think this is right!!!
		if (matchi(node->call, call))
			break;
	}

	return node;
}

// Add a reference to a node.

static NODE *node_inc(char *call, char *alias, char * Version)
{
	NODE *node;

	node = node_find(call);

	if (!node)
	{
		knownnode_add(call);

		node = zalloc(sizeof(NODE));
		sl_ins_hd(node, node_hd);
		node->call  = _strdup(call);
		node->alias = _strdup(alias);
		if (Version)
			node->Version = _strdup(Version);

//		Debugprintf("New Node Rec Created at %x for %s %s", node, node->call, node->alias);
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

//	Debugprintf("MAILCHAT: Removing %s From Node List", node->call);

	for (t = node_hd; t; tp = t, t = t->next)
	{
		if (t == node)
		{
			if (tp) tp->next = t->next; else node_hd = t->next;
			free(t->alias);
			t->alias = NULL;
			free(t->call);
			t->call = NULL;
			free(t);
			break;
		}
	}
//	Debugprintf("MAILCHAT: Remove Complete");

}

// User joins a topic.

static TOPIC *topic_join(CIRCUIT *circuit, char *s)
{
	CT    *ct;
	TOPIC *topic;

// Look for an existing topic.

	for (topic = topic_hd; topic; topic = topic->next)
	{
		if (matchi(topic->name, s))
			break;
	}

// Create a new topic, if needed.

	if (!topic)
	{
		topic = zalloc(sizeof(TOPIC));
		sl_ins_hd(topic, topic_hd);
		topic->name = _strdup(s);
	}

	topic->refcnt++;  // One more user in this topic.

	Logprintf(LOG_CHAT, circuit, '?', "topic_join complete user %s topic %s addr %x ref %d",
		circuit->u.user->call, topic->name, topic, topic->refcnt);


// Add the circuit / topic association.

	for (ct = circuit->topic; ct; ct = ct->next)
	{
		if (ct->topic == topic)
		{
			ct->refcnt++;
			return topic;
		}
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

	Logprintf(LOG_CHAT, circuit, '?', "topic_leave user %s topic %s addr %x ref %d",
		circuit->u.user->call, topic->name, topic, topic->refcnt);

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

	for (t = topic_hd; t; tp = t, t = t->next)
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
	{
		if (ct->topic == topic)
			return ct->refcnt;
	}
	return 0;
}

// Nodes reached from each circuit. Used only if the circuit is a link.

// Remove a circuit/node association.

static void cn_dec(CIRCUIT *circuit, NODE *node)
{
	CN *c, *cp;

//	Debugprintf("MAILCHAT: Remove c/n %s ", node->call);

	cp = NULL;

	for (c = circuit->hnode; c; cp = c, c = c->next)
	{
		if (c->node == node)
		{
//			CN * cn;
//			int len;
//			char line[1000]="";
			
			if (--c->refcnt) 
			{
//				Debugprintf("MAILCHAT: Remove c/n Node %s still in use refcount %d", node->call, c->refcnt);
				return;			// Still in use
			}

			/*
//			Debugprintf("MAILCHAT: Refcount 0 - Removing %s. List Before is:", node->call);

			__try{
			for (cn = circuit->hnode; cn; cn = cn->next)
			{
				if (cn->node && cn->node->alias)
				{
					__try
					{
						len = wsprintf(line, "%s %s", line, cn->node->alias);
						if (len > 80)
						{
//							Debugprintf("%s", line);
							len = wsprintf(line, "            ");
						}
					}
					__except(EXCEPTION_EXECUTE_HANDLER)
					{
						len = wsprintf(line, "%s *PE* Corrupt Rec %x %x ", line, cn, cn->node);
					}
				}
				else
				{
					len = wsprintf(line, "%s Corrupt Rec %x %x ", line, cn, cn->node);
				}
			}
			}
			__except(EXCEPTION_EXECUTE_HANDLER)
			{len = wsprintf(line, "%s *PE* Corrupt Rec %x %x ", line, cn, cn->node);}


//			Debugprintf("%s", line);
*/

			// CN record no longer needed

			if (cp)
				cp->next = c->next;
			else
				circuit->hnode = c->next;

			free(c);

			/*
			Debugprintf("MAILCHAT: Remove c/n Trace After");

			line[0] = 0;

			for (cn = circuit->hnode; cn; cn = cn->next)
			{
				if (cn->node && cn->node->alias)
				{
					__try
					{
						len = wsprintf(line, "%s %s", line, cn->node->alias);
						if (len > 80)
						{
							Debugprintf("%s", line);
							len = wsprintf(line, "            ");
						}
					}
					__except(EXCEPTION_EXECUTE_HANDLER)
					{len = wsprintf("%s *PE* Corrupt Rec %x %x ", line, cn, cn->node);}
				}
				else
				{
					len = wsprintf("%s Corrupt Rec %x %x ", line, cn, cn->node);
				}
			}
			Debugprintf("%s", line);
			*/
			break;
		}
	}

	if (c == NULL)
	{
		CN * cn;
		int len;
		char line[1000]="";
	
		// not found??
	
		Debugprintf("MAILCHAT: !! Remove c/n Node %s addr %x not found cn chain follows", node->call, node);

		line[0] = 0;

		for (cn = circuit->hnode; cn; cn = cn->next)
		{
				if (cn->node && cn->node->call)
				{
					__try
					{
						len = wsprintf(line, "%s %x %s", line, cn->node, cn->node->alias);
						if (len > 80)
						{
							Debugprintf("%s", line);
							len = wsprintf(line, "            ");
						}
					}
					__except(EXCEPTION_EXECUTE_HANDLER)
					{len = wsprintf("%s *PE* Corrupt Rec %x %x ", line, cn, cn->node);}
				}
				else
				{
					len = wsprintf("%s Corrupt Rec %x %x ", line, cn, cn->node);
				}
		}
		Debugprintf("%s", line);

	}


}

// Add a circuit/node association.

static NODE *cn_inc(CIRCUIT *circuit, char *call, char *alias, char * Version)
{
	NODE *node;
	CN *cn;

	node = node_inc(call, alias, Version);

	for (cn = circuit->hnode; cn; cn = cn->next)
	{
		if (cn->node == node)
		{
			cn->refcnt++;
//			Debugprintf("cn_inc cn Refcount for %s->%s  incremented to %d - adding Call %s",
//				circuit->Callsign, node->call, cn->refcnt, call);

			return node;
		}
	}

	cn = zalloc(sizeof(CN));
	sl_ins_hd(cn, circuit->hnode);
	cn->node   = node;
	cn->refcnt = 1;

//	Debugprintf("cn_inc New cn for %s->%s - adding Call %s",
//				circuit->Callsign, node->call, call);

	return node;
}

// Find a circuit/node association.

static int cn_find(CIRCUIT *circuit, NODE *node)
{
	CN *cn;

	for (cn = circuit->hnode; cn; cn = cn->next)
	{
		if (cn->node == node)
			return cn->refcnt;
	}
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
	char Buffer[2048];
	char *buf = Buffer;

	sprintf(buf, "%-6.6s %c %s\r", user->call, (who == o_one) ? '>' : ':', text);

// Send it to all connected users in the same topic.
// Echo to originator if requested.

	for (circuit = circuit_hd; circuit; circuit = circuit->next)
	{
		if (!(circuit->rtcflags & p_user)) continue;  // Circuit is a link.
		if ((circuit->u.user == user) && !(user->rtflags & u_echo)) continue;

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
}

void text_tellu_Joined(USER * user)
{
	CIRCUIT *circuit;
	char buf[256];

	sprintf(buf, "%-6.6s : *** Joined Chat, Topic %s", user->call, user->topic->name);

	if (ConsHeader[1]->FlashOnConnect)
		FlashWindow(hWnd, TRUE);

// Send it to all connected users in the same topic.
// Echo to originator if requested.

	for (circuit = circuit_hd; circuit; circuit = circuit->next)
	{
		if (!(circuit->rtcflags & p_user)) continue;  // Circuit is a link.
		if ((circuit->u.user == user) && !(user->rtflags & u_echo)) continue;

		nputs(circuit, buf);

		if (circuit->u.user->rtflags & u_bells)
			if (circuit->BPQStream < 0) // Console
			{
				if (ConsHeader[1]->FlashOnConnect) FlashWindow(ConsHeader[1]->hConsole, TRUE);
				nputc(circuit, 7);
//				PlaySound ("BPQCHAT_USER_LOGIN", NULL, SND_ALIAS | SND_APPLICATION | SND_ASYNC);
			}
			else
				nputc(circuit, 7);

		nputc(circuit, 13);
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
	__try{
		if (!cn_find(circuit, node))
			if (node->Version && (kind == id_link))
				nprintf(circuit, "%c%c%s %s %s %s\r", FORMAT, kind, OurNode, node->call, node->alias, node->Version);
			else
				nprintf(circuit, "%c%c%s %s %s\r", FORMAT, kind, OurNode, node->call, node->alias);

	}
	__except(EXCEPTION_EXECUTE_HANDLER)
		{Debugprintf("*PE*node_xmit Corrupt Rec %x %x %x", node, node->call, node->alias);}

}

// Tell all other nodes about one node known by this node.

static void node_tell(NODE *node, char kind)
{
	CIRCUIT *circuit;

	for (circuit = circuit_hd; circuit; circuit = circuit->next)
	{
		if (circuit->rtcflags & p_linked)
			node_xmit(node, kind, circuit);
	}
}

// Tell another node about a user login/logout at this node.

static void user_xmit(USER *user, char kind, CIRCUIT *circuit)
{
	NODE *node;

	node = user->node;

	if (!cn_find(circuit, node))
		nprintf(circuit, "%c%c%s %s %s %s\r", FORMAT, kind, node->call, user->call, user->name, user->qth);
}

// Tell all other nodes about a user login/logout at this node.

static void user_tell(USER *user, char kind)
{
	CIRCUIT *circuit;

	for (circuit = circuit_hd; circuit; circuit = circuit->next)
	{
		if (circuit->rtcflags & p_linked)
			user_xmit(user, kind, circuit);
	}
}

// Find the user record for call@node. Node can be NULL, meaning any node

USER *user_find(char *call, char * node)
{
	USER *user;

	for (user = user_hd; user; user = user->next)
	{
		if (node)
		{
			if (matchi(user->call, call) && matchi(user->node->call, node))
				break;
		}
		else
		{
			if (matchi(user->call, call))
			break;
		}
	}

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

static BOOL topic_chg(USER *user, char *s)
{
	char buf[128];

	if (_stricmp(user->topic->name, s) == 0) return FALSE;			// Not Changed

	sprintf(buf, "*** Left Topic: %s", user->topic->name);
	text_tellu(user, buf, NULL, o_topic); // Tell everyone in the old topic.
	topic_leave(user->circuit, user->topic);
	user->topic = topic_join(user->circuit, s);
	sprintf(buf, "*** Joined Topic: %s", user->topic->name);
	text_tellu(user, buf, NULL, o_topic); // Tell everyone in the new topic.

	return TRUE;
}

// Create a user record for this user.

static USER *user_join(CIRCUIT *circuit, char *ucall, char *ncall, char *nalias, BOOL Local)
{
	NODE *node;
	USER *user;

	if (Local)
	{
		node = cn_inc(circuit, ncall, nalias, Verstring);
	}
	else
		node = cn_inc(circuit, ncall, nalias, NULL);

// Is this user already logged in at this node?

	for (user = user_hd; user; user = user->next)
	{
		if (matchi(user->call, ucall) && (user->node == node))
			return user;
	}

// User is not logged in, create a user record for them.

	user = zalloc(sizeof(USER));
	sl_ins_hd(user, user_hd);
	user->circuit = circuit;
	user->call = _strdup(ucall);
	_strupr(user->call);
	user->node = node;
	rduser(user);

	if (circuit->rtcflags & p_user)
		circuit->u.user = user;

	user->lastmsgtime = time(NULL);

	user->topic   = topic_join(circuit, deftopic);
	return user;
}

// Link went away. We dropped it, or the other node dropped it.
// Drop nodes and users connected from this link.
// Tell other (still connected) links what was dropped.

void link_drop(CIRCUIT *circuit)
{
	USER *user, *usernext;
	CN   *cn;
	struct _EXCEPTION_POINTERS exinfo;

// So we don't try and send anything on this circuit.

	if (circuit->u.link)
		circuit->u.link->flags = p_nil;
	
	circuit->rtcflags = p_nil;

// Users connected on the dropped link are no longer connected.

	__try {

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

	} My__except_Routine("link_drop clear users");

// Any node known from the dropped link is no longer known.

	__try{

	for (cn = circuit->hnode; cn; cn = cn->next)
	{
		__try{
			node_tell(cn->node, id_unlink);
		} My__except_Routine("link_drop clear nodes node tell");

		__try{
			node_dec(cn->node);
		} My__except_Routine("link_drop clear nodes node dec");
	}
	} My__except_Routine("link_drop clear nodes");


// The circuit is no longer used.

	__try{


	circuit_free(circuit);

    } My__except_Routine("link_drop clear circuit");

	NeedStatus = TRUE;
}

// Handle an incoming control frame from a linked RT system.

static void echo(CIRCUIT *fc, NODE *node, char * Buffer)
{
	CIRCUIT *tc;

	for (tc = circuit_hd; tc; tc = tc->next)
	{
		if ((tc != fc) && (tc->rtcflags & p_linked) && !cn_find(tc, node))
			nprintf(tc, "%s\r", Buffer);
	}
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
VOID removeknown()
{
	// Save Known Nodes list and free struct
	
	KNOWNNODE *node, *nextnode;
	FILE *out;

	out = fopen(RtKnown, "w");

	for (node = known_hd; node; node = nextnode)
	{
		fprintf(out, "%s %d\n", node->call, node->LastHeard);

		nextnode = node->next;
		free(node->call);
		free(node);
	}
	known_hd = NULL;

	fclose(out);
}

VOID LoadKnown()
{
	// Reload Known Nodes list 
	
	FILE *in;
	char buf[128];
	char * ptr;

	in = fopen(RtKnown, "r");

	if (in == NULL)
		return;

	while(fgets(buf, 128, in))
	{
		ptr = strchr(buf, ' ');
		if (ptr)
		{
			*(ptr) = 0;
			knownnode_add(buf);
		}
	}

	fclose(in);
}

// We don't allocate memory for circuit, but we do chain it

CIRCUIT *circuit_new(CIRCUIT *circuit, int flags)
{
	// Make sure circuit isn't already on list
	
	CIRCUIT *c;

	circuit->rtcflags = flags;
	circuit->next = NULL;

	for (c = circuit_hd; c; c = c->next)
	{
		if (c == circuit)
		{
			Debugprintf("MAILCHAT: Attempting to add Circuit when already on list");
			return circuit;
		}
	}
	
	sl_ins_hd(circuit, circuit_hd);

	return circuit;
}

// Handle an incoming link. We should only get here if we think the station is a node.

int rtloginl (CIRCUIT *conn, char * call)
{
	LINK    *link;

	if (node_find(call))
	{
		Logprintf(LOG_CHAT, conn, '|', "Refusing link with %s to prevent a loop", conn->Callsign);
		return FALSE; // Already linked.
	}

	for (link = link_hd; link; link = link->next)
	{
		if (matchi(call, link->call))
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
				len=sprintf_s(Msg, sizeof(Msg), "Chat Node %s Connect when Connected - Old Connection Closed", call);
				WriteLogLine(conn, '|',Msg, len, LOG_CHAT);

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
	link->delay = 0;			// Dont delay first restart
	state_tell(conn, NULL);
	nprintf(conn, "%c%c%s %s %s\r", FORMAT, id_keepalive, OurNode, conn->u.link->call, Verstring);

	NeedStatus = TRUE;

	return TRUE;
}

// User connected to chat, or did chat command from BBS

int rtloginu (CIRCUIT *circuit, BOOL Local)
{
	USER    *user;

// Is this user already logged in to RT somewhere else?

	user = user_find(circuit->UserPointer->Call, NULL);
	
	if (user)
	{
		nputs(circuit, "*** Already connected at another node.\r");
		return FALSE;
	}

//	if (log_rt) tlogp("RT Login");

// Create the user entry.

	circuit_new(circuit, p_user);

	user = user_join(circuit, circuit->UserPointer->Call, OurNode, OurAlias, Local);
	circuit->u.user = user;

	if (strcmp(user->name, "?_name") == 0)
	{
		user->name = _strdup(circuit->UserPointer->Name);
	}
	upduser(user);

	ExpandAndSendMessage(circuit, ChatWelcomeMsg, LOG_CHAT);
	text_tellu_Joined(user);
	user_tell(user, id_join);
	show_users(circuit);
//	makelinks();

	return TRUE;
}

void logout(CIRCUIT *circuit)
{
	USER *user;

	circuit->rtcflags = p_nil;
	user = circuit->u.user;
//	if (log_rt) tlogp("RT Logout");
	if (user)			// May not have logged in if already conencted
	{
		user_tell(user, id_leave);
		text_tellu(user, rtleave, NULL, o_all);
		cn_dec(circuit, user->node);
		node_dec(user->node);

		user_leave(user);
	}

	circuit_free(circuit);
}

void show_users(CIRCUIT *circuit)
{
	USER *user;
	char * Alias;
	char * Topic;

	nputs(circuit, "Stations connected:\r");

	for (user = user_hd; user; user = user->next)
	{
		if ((user->node == 0) || (user->node->alias == 0))
			Alias = "(Corrupt Alias)";
		else
			Alias = user->node->alias;

		if ((user->topic == 0) || (user->topic->name == 0))
			Topic = "(Corrupt Topic)";
		else
			Topic = user->topic->name;

		__try 
		{		
			nprintf(circuit, "%-6.6s at %-9.9s %s, %s [%s] Idle for %d seconds\r",
				user->call, Alias, user->name, user->qth, Topic, time(NULL) - user->lastmsgtime);
		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
			Debugprintf("MAILCHAT *** Program Error in show_users");
			CheckProgramErrors();
		}
	}
}


static void show_nodes(CIRCUIT *circuit)
{
	NODE *node;

	nputs(circuit, "Known Nodes:\r");

	for (node = node_hd; node; node = node->next)
	{
		if (node->refcnt)
			if (node->Version)
				nprintf(circuit, "%s:%s %s %u\r", node->alias, node->call, node->Version, node->refcnt);
			else
				nprintf(circuit, "%s:%s %s %u\r", node->alias, node->call, "Not Known", node->refcnt);
	}
}

// /P Command: List circuits and remote RT on them.

#define xxx "\r        "

static void show_circuits(CIRCUIT *conn)
{
	CIRCUIT *circuit;
	NODE    *node;
	LINK *link;
	char line[1000];

	int     len;
	CN	*cn;

/*	nprintf(conn, "Here %-6.6s <- ", OurAlias);
	len = 0;

	for (node = node_hd; node; node = node->next) if (node->refcnt)
	{
		len += strlen(node->alias) + 1;
		if (len > 60) { len = strlen(node->alias) + 1; nputs(conn, xxx); }
		nputs(conn, node->alias);
		nputc(conn, ' ');
	}
*/
	wsprintf(line, "Here %-6.6s <-", OurAlias);

	for (node = node_hd; node; node = node->next) if (node->refcnt)
	{
		len = wsprintf(line, "%s %s", line, node->alias);
		if (len > 80)
		{
			nprintf(conn, "%s\r", line);
			len = wsprintf(line, "              ");
		}
	}

	nprintf(conn, "%s\r", line);

	for (circuit = circuit_hd; circuit; circuit = circuit->next)
	{
		if (circuit->rtcflags & p_linked)
		{
			len = wsprintf(line, "Nodes via %-6.6s(%d) -", circuit->u.link->alias, circuit->refcnt);		
			__try{
				for (cn = circuit->hnode; cn; cn = cn->next)
				{
					if (cn->node && cn->node->alias)
					{
						__try
						{
							len = wsprintf(line, "%s %s", line, cn->node->alias);
							if (len > 80)
							{
								nprintf(conn, "%s\r", line);
								len = wsprintf(line, "            ");
							}
						}
						__except(EXCEPTION_EXECUTE_HANDLER)
							{len = wsprintf(line, "%s *PE* Corrupt Rec %x %x", line, cn, cn->node);}
					}
					else
						len = wsprintf(line, "%s Corrupt Rec %x %x ", line, cn, cn->node);
				}
			}
			__except(EXCEPTION_EXECUTE_HANDLER)
			{len = wsprintf(line, "%s *PE* Corrupt Rec %x %x ", line, cn, cn->node);}

			nprintf(conn, "%s\r", line);

		}
		else if (circuit->rtcflags & p_user)
			nprintf(conn, "User %-6.6s\r", circuit->u.user->call);
		else if (circuit->rtcflags & p_linkini)
			nprintf(conn, "Link %-6.6s (setup)\r", circuit->u.link->alias);
	}

	nprintf(conn, "Links Defined:\r");

	for (link = link_hd; link; link = link->next)
	{
		if (link->flags & p_linked )
			nprintf(conn, "  %-10.10s Open\r", link->call);
		else if (link->flags & (p_linked | p_linkini))
			nprintf(conn, "  %-10.10s Connecting\r", link->call);
		else
			nprintf(conn, "  %-10.10s Idle\r", link->call);
	}
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
			{
				if (user->topic == topic)
					nprintf(conn, " %s", user->call);
			}
			nputc(conn, '\r');
		}
	}
}

static void show_users_in_topic(CIRCUIT *conn)
{
	TOPIC *topic;
	USER  *user;

	nputs(conn, "Users in Topic:\r");

	topic = conn->u.user->topic;
	{
		if (topic->refcnt)
		{
			for (user = user_hd; user; user = user->next)
			{
				if (user->topic == topic)
					nprintf(conn, "%s ", user->call);
			}
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
			user->rtflags ^= u_bells;
			upduser(user);
			nprintf(circuit, "Alert %s\r",  (user->rtflags & u_bells) ? "Enabled" : "Disabled");
			return TRUE;

		case 'b' : return FALSE;
		case 'e' : 
			user->rtflags ^= u_echo;
			upduser(user);
			nprintf(circuit, "Echo %s\r",  (user->rtflags & u_echo) ? "Enabled" : "Disabled");
			return TRUE;
		
		case 'f' : makelinks(); return TRUE;

		case 'h' :
		case '?' :
			nputs(circuit, "Commands can be in upper or lower case.\r");
			nputs(circuit, "/U - Show Users.\r/N - Enter your Name.\r/Q - Enter your QTH.\r/T - Show Topics.\r");
			nputs(circuit, "/T Name - Join Topic or Create new Topic. Topic Names are not case sensitive\r/P - Show Ports and Links.\r");
			nputs(circuit, "/A - Toggle Alert on user join.\r");
			nputs(circuit, "/E - Toggle Echo.\r/S CALL Text - Send Text to that station only.\r");
			nputs(circuit, "/F - Force all links to be made.\r/K - Show Known nodes.\r");
			nputs(circuit, "/B - Leave Chat and return to node.\r/QUIT - Leave Chat and disconnect from node.\r");
			return TRUE;
		
		case 'k' : show_nodes(circuit);                 return TRUE;

		case 'n' :

			f1 = &Buffer[2];

			while ((*f1 != 0) && (*f1 == ' '))
				f1++;

			if (*f1 == 0)
			{
				nprintf(circuit, "Name is %s\r", user->name);
				return TRUE;
			}

			strnew(&user->name, f1);
			nprintf(circuit, "Name set to %s\r", user->name);
			upduser(user);
			user_tell(user, id_user);
			return TRUE;

		case 'p' : show_circuits(circuit); return TRUE;

		case 'q' :

			f1 = &Buffer[2];

			while ((*f1 != 0) && (*f1 == ' '))
				f1++;

			if (*f1 == 0)
			{
				nprintf(circuit, "QTH is %s\r", user->qth);
				return TRUE;
			}

			strnew(&user->qth, f1);
			
			nprintf(circuit, "QTH set to %s\r", user->qth);
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
			su = user_find(f1, NULL);

			if (!su)
			{
				nputs(circuit, "*** That user is not logged in.\r");
				return TRUE;
			}

			// Send to the desired user only.

			if (su->circuit->rtcflags & p_user)
				text_tellu(user, f2, f1, o_one);
			else
				text_xmit(user, su, f2);

			return TRUE;

		case 't' :
			f1 = strlop(Buffer, ' ');
			if (f1)
			{
				if (topic_chg(user, f1))
				{
					nprintf(circuit, "Switched to Topic %s\r", user->topic->name);
					show_users_in_topic(circuit);

					// Tell all link circuits about the change of topic.

					for (c = circuit_hd; c; c = c->next)
					{
						if (c->rtcflags & p_linked)
							topic_xmit(user, c);
					}
				}
				else
				{
					// Already in topic

					nprintf(circuit, "You were already in Topic %s\r", user->topic->name);
				}
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

		if (link->delay == 0)
		{
			link->flags = p_linkini;
			link->delay = 12;			// 2 mins
			chat_link_out(link);
			return;						// One at a time
		}
		else
			link->delay--;
		
	}
}

VOID node_close()
{
	// Close all Node-Node Links

	CIRCUIT *circuit;

	for (circuit = circuit_hd; circuit; circuit = circuit->next)
	{
		if (circuit->rtcflags & (p_linked | p_linkini | p_linkwait))
			Disconnect(circuit->BPQStream);
	}
}

// Send Keepalives to all connected nodes

static void node_keepalive()
{
	CIRCUIT *circuit;
	
	NeedStatus = TRUE;					// Send Report to Monitor

	if (user_hd)						// Any Users?
	{
		for (circuit = circuit_hd; circuit; circuit = circuit->next)
		{
			if (circuit->rtcflags & p_linked)
				nprintf(circuit, "%c%c%s %s %s\r", FORMAT, id_keepalive, OurNode, circuit->u.link->call, Verstring);
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
	TOPIC *topic;
	CIRCUIT *c;
	char Msg[1000];
	int len;

	ClearDebugWindow();

	if (NeedStatus)
	{
		NeedStatus = FALSE;
		SendChatLinkStatus();
	}

	WritetoDebugWindow("Chat Users\r\n", 12);

	i = 0;
	for (user = user_hd; user; user = user->next)
	{
		len = sprintf_s(Msg, sizeof(Msg), "%s Topic %s\r\n", user->call,
			(user->topic) ? user->topic->name : "** Missing Topic **"); 
		WritetoDebugWindow(Msg, len);
		i++;
	
		if (user->circuit->rtcflags & p_user)	// Local User
		{
			if ((time(NULL) - user->lastmsgtime) > 7200)
			{
				nprintf(user->circuit, "*** Disconnected - Idle time exceeded");
				Sleep(1000);

				if (user->circuit->BPQStream < 0)
				{
					CloseConsole(user->circuit->BPQStream);	
					break;
				}
				else
				{
					Disconnect(user->circuit->BPQStream);
					break;
				}
			}
		}

	}


	SetDlgItemInt(hWnd, IDC_USERS, i, FALSE);

	WritetoDebugWindow("Chat Nodes\r\n", 12);

	i = 0;
	for (node = node_hd; node; node = node->next)
	{
		len = sprintf_s(Msg, sizeof(Msg), "%s Version %s Count %d\r\n",
			node->call, node->Version, node->refcnt);
		WritetoDebugWindow(Msg, len);

		i++;
	}

	SetDlgItemInt(hWnd, IDC_NODES, i, FALSE);

	WritetoDebugWindow("Chat Links\r\n", 12);

	i = 0;
	for (c = circuit_hd; c; c = c->next)
	{
		if (c->rtcflags & p_linked) 
		{
			char buff[1000];
			int ptr;
			CT * ct;
			ptr = sprintf_s(buff, sizeof(buff), "%s Topics: ", c->u.user->call);
	
			for (ct = c->topic; ct; ct = ct->next)
			{
				ptr+= sprintf_s(&buff[ptr], sizeof(buff) - ptr, "%s ", ct->topic->name);
			}

			WritetoDebugWindow(buff, ptr);
			WritetoDebugWindow("\r\n", 2);

			i++;
		}
	}

	SetDlgItemInt(hWnd, IDC_LINKS,  i, FALSE);

	WritetoDebugWindow("Chat Topics\r\n", 12);

	i = 0;
	for (topic = topic_hd; topic; topic = topic->next)
	{
		len = sprintf_s(Msg, sizeof(Msg), "%s %d\r\n", topic->name, topic->refcnt); 
		WritetoDebugWindow(Msg, len);
		i++;
	}

	ChatTmr++;

	if (user_hd)				// Any Users?
		makelinks();

	if (ChatTmr > 60) // 10 Mins
	{
		ChatTmr = 1;
		node_keepalive();
		ProgramErrors = 0;
	}
}

VOID FreeChatMemory()
{
	struct _EXCEPTION_POINTERS exinfo;

	__try {

		removelinks();
		removeknown();
	}
	My__except_Routine("FreeChatMemory");

}

// Find a call in the known node list.

KNOWNNODE *knownnode_find(char *call)
{
	KNOWNNODE *node;

	for (node = known_hd; node; node = node->next)
	{
		if (matchi(node->call, call))
			break;
	}

	return node;
}

// Add a known node.

static KNOWNNODE *knownnode_add(char *call)
{
	KNOWNNODE *node;

	node = knownnode_find(call);

	if (!node)
	{
		node = zalloc(sizeof(KNOWNNODE));
		sl_ins_hd(node, known_hd);
		node->call  = _strdup(call);
	}

	node->LastHeard = time(NULL);
	return node;
}

static char UIDEST[10] = "DUMMY";
static char AXDEST[7];
static char MYCALL[7];

#pragma pack(1)


typedef struct _MESSAGEX
{
//	BASIC LINK LEVEL MESSAGE BUFFER LAYOUT

	struct _MESSAGE * CHAIN;

	UCHAR	PORT;
	USHORT	LENGTH;

	UCHAR	DEST[7];
	UCHAR	ORIGIN[7];

//	 MAY BE UP TO 56 BYTES OF DIGIS

	UCHAR	CTL;
	UCHAR	PID;
	UCHAR	DATA[256];

}MESSAGEX, *PMESSAGEX;

#pragma pack()

VOID SetupChat()
{
	ConvToAX25(OurNode, MYCALL);
	ConvToAX25(UIDEST, AXDEST);

	wsprintf(Verstring, "%d.%d.%d.%d",  Ver[0], Ver[1], Ver[2], Ver[3]);

	LoadKnown();
}


VOID Send_MON_Datagram(UCHAR * Msg, DWORD Len)
{
	MESSAGEX AXMSG;

	PMESSAGEX AXPTR = &AXMSG;

	// Block includes the Msg Header (7 bytes), Len Does not!

	memcpy(AXPTR->DEST, AXDEST, 7);
	memcpy(AXPTR->ORIGIN, MYCALL, 7);
	AXPTR->DEST[6] &= 0x7e;			// Clear End of Call
	AXPTR->DEST[6] |= 0x80;			// set Command Bit

	AXPTR->ORIGIN[6] |= 1;			// Set End of Call
	AXPTR->CTL = 3;		//UI
	AXPTR->PID = 0xf0;
	memcpy(AXPTR->DATA, Msg, Len);

	SendRaw(AXIPPort, (char *)&AXMSG.DEST, Len + 16);

	return;

}

VOID SendChatLinkStatus()
{
	char Msg[256] = {0};
	LINK * link;
	int len = 0;

	if (ChatApplNum == 0)
		return;

	if (AXIPPort == 0)
		return;

	if (MYCALL[0] == 0)
		return;

	for (link = link_hd; link; link = link->next)
	{
		len = wsprintf(Msg, "%s%s %c ", Msg, link->call, '0' + link->flags);

		if (len > 240)
			break;
	}
	Msg[len++] = '\r';

	Send_MON_Datagram(Msg, len);
}

VOID ClearChatLinkStatus()
{
	LINK * link;

	for (link = link_hd; link; link = link->next)
	{
		link->flags = 0;
	}
}

/*
Hi Alll.  Things are quite up here.  We had a nice day.  Overcast cool about +5 deg C.  cooling off a bit tonight to -2.  I had a nice chat with John G8BPQ this morning about the software etc.  Things lookign good, and he opes and thinks he's fixed the bugs in the CHAT so I guess we'll see.  bk to net kk
*/